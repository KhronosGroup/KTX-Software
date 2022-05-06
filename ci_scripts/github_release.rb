#!/usr/bin/env ruby
# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Thanks to Valerio Mazzeo (@valeriomazzeo) for the original of this.

require 'optparse'
require 'ostruct'
require 'octokit'
require 'mime/types'

options = OpenStruct.new
options.draft = false
options.prerelease = false
OptionParser.new do |opt|
  opt.on('-s', '--secret SECRET', 'GitHub access token') { |o| options[:secret] = o }
  opt.on('-r', '--repo-slug REPO_SLUG', 'Repo slug. i.e.: apple/swift') { |o| options[:repo_slug] = o }
  opt.on('-n', '--relnotes RELNOTES_STRING', 'Release notes') { |o| options[:relnotes] = o }
  opt.on('-c', '--relnotes-file RELNOTES_FILE', 'Release notes path') { |o| options[:relnotes_file] = o }
  opt.on('-t', '--tag TAG', 'Tag name') { |o| options[:tag_name] = o }
  opt.on('-d', '--draft BOOLEAN', TrueClass, 'true if draft release') { |o| options[:draft] = o }
  opt.on('-p', '--prerelease BOOLEAN', TrueClass, 'true if prerelease') { |o| options[:prerelease] = o }
end.parse!

puts ARGV.inspect

raise OptionParser::MissingArgument if options[:secret].nil?
raise OptionParser::MissingArgument if options[:repo_slug].nil?
raise OptionParser::MissingArgument if options[:relnotes_file].nil? and options[:relnotes].nil?
raise OptionParser::MissingArgument if options[:tag_name].nil?

client = Octokit::Client.new(:access_token => options[:secret])
user = client.user
user.login

unless client.scopes.include? 'public_repo' or client.scopes.include? 'repo'
  raise Error, "Insufficient permissions. Make sure your token contains the repo or public_repo scope."
end

puts "Logged in as #{user.name}"
puts "Deploying to repo: #{options[:repo_slug]}"

tag_matched = false
release_url = nil
releases = client.releases(options[:repo_slug])
if not options[:relnotes].nil?
  body = options[:relnotes]
else
  body = File.open(options[:relnotes_file], "rb").read
end

releases.each do |release|
  puts "Release tag_name = #{release.tag_name}"
  if release.tag_name == options[:tag_name]
    release_url = release.rels[:self].href
    tag_matched = true
  end
end
puts "tag #{options[:tag_name]} matched: #{tag_matched}."
puts "release_url: #{release_url}."

# if tag has been pushed directly to git, create a github release
if tag_matched == false
  release = client.create_release(
    options[:repo_slug],
    options[:tag_name],
    { :name => options[:tag_name],
      :draft => options[:draft],
      :prerelease => options[:prerelease],
      :body => body 
    })
    release_url = release.url
else
  release = client.update_release(release_url,
    { :name => options[:tag_name],
      :draft => options[:draft],
      :prerelease => options[:prerelease],
      :body => body 
    })
end
puts "release.assets_url: #{release.assets_url}"
ARGV.each do |file|
  puts "uploading asset #{file} to #{release_url}"
  types = MIME::Types.type_for(file)
  puts types.inspect
  puts "#{types[0]}"
  client.upload_asset(release_url, file, {:content_type => "#{types[0]}"})
end
