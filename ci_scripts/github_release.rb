#!/usr/bin/env ruby
# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Thanks to Valerio Mazzeo (@valeriomazzeo) for inspiring this.

gem 'octokit', '~> 4.15.0'
require 'optparse'
require 'ostruct'
require 'octokit'
require 'mime/types'

options = OpenStruct.new
options.draft = false
options.overwrite = false
options.prerelease = false
OptionParser.new do |opt|
  opt.on('-s', '--secret SECRET', 'GitHub access token') { |o| options[:secret] = o }
  opt.on('-r', '--repo-slug REPO_SLUG', 'Repo slug. i.e.: apple/swift') { |o| options[:repo_slug] = o }
  opt.on('-n', '--relnotes RELNOTES_STRING', 'Release notes') { |o| options[:relnotes] = o }
  opt.on('-c', '--relnotes-file RELNOTES_FILE', 'Release notes path') { |o| options[:relnotes_file] = o }
  opt.on('-t', '--tag TAG', 'Tag name') { |o| options[:tag_name] = o }
  opt.on('-d', '--draft BOOLEAN', TrueClass, 'true if draft release') { |o| options[:draft] = o }
  opt.on('-d', '--overwrite BOOLEAN', TrueClass, 'true to overwrite existing assets') { |o| options[:overwrite] = o }
  opt.on('-p', '--prerelease BOOLEAN', TrueClass, 'true if prerelease') { |o| options[:prerelease] = o }
end.parse!

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

if not options[:relnotes].nil?
  body = options[:relnotes]
else
  body = File.open(options[:relnotes_file], "rb").read
end

tag_matched = false
our_release = nil
releases = client.releases(options[:repo_slug])
releases.each do |release|
  puts "Release tag_name = #{release.tag_name}"
  if release.tag_name == options[:tag_name]
    tag_matched = true
    our_release = release
    break
  end
end
puts "tag #{options[:tag_name]} matched: #{tag_matched}."
if our_release
  puts "our_release.url: #{our_release.url}."
end

# if tag has been pushed directly to git, create a github release
if not our_release
  our_release = client.create_release(
    options[:repo_slug],
    options[:tag_name],
    { :name => options[:tag_name],
      :draft => options[:draft],
      :prerelease => options[:prerelease],
      :body => body 
    })
else
  our_release = client.update_release(our_release.url,
    {
      :body => body,
      :tag_name => our_release.tag_name,
      :name => our_release.name,
      :draft => our_release.draft,
      :prerelease => our_release.prerelease
    })
end

def asset(client, name, url)
  client.release_assets(url).detect { |asset| asset.name == name }
end

def content_type(file)
  type = MIME::Types.type_for(file).first
  type ||= 'application/octet-stream'
  type.to_s
end

def delete(client, asset, file)
  #info :overwrite, file
  client.delete_release_asset(asset.url)
end

def normalize_filename(str)
  str = File.basename(str)
  str = str.split(' ').first
  # We likely don't need transliterate as our package names are English.
  # See https://github.com/travis-ci/dpl/blob/d104a6ec5fb0ba480875fe120d1e9e986824b9dc/lib/dpl/helper/transliterate.rb
  #str = transliterate(str) # 
  str.gsub(/[^\w@+\-_]/, '.')
end

def upload_file(path, overwrite, release, client)
  file = normalize_filename(path)
  asset = asset(client, file, release.url)
  #return info :skip_existing, file if asset && !overwrite
  return if asset && !overwrite
  puts "uploading asset #{path} to #{release.url}"
  delete(client, asset, file) if asset
  #info :upload_file, file
  client.upload_asset(release.url, path,
    {:name => file, :content_type => content_type(file)})
end

# This is not thoroughly tested. Asset uploads are normally
# done using the Travis CI "releases" provider.
ARGV.each { |file| upload_file(file, options[:overwrite], our_release, client) }
