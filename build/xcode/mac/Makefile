##
# @copyright © 2019, Mark Callow. For license see LICENSE.md.
#
# Makefile to run xcodebuild to directly install apps on local machine.

# You need to set the environment variables DEVELOPMENT_TEAM and VULKAN_SDK
# to the same values you have these set in Xcode's Custom Paths preferences.
# xcodebuild apparently does not see preferences set in Xcode's GUI. We must
# pass these as settings on the xcodebuild command line. Read them from
# the shell environment to avoid having one user's settings hardwired in
# this file.
devteam=$(DEVELOPMENT_TEAM)
vulkan_sdk=$(VULKAN_SDK)

config=Release
dstroot=/

install.lib:
	xcodebuild -project libktx.xcodeproj -configuration $(config) -target install.lib DEVELOPMENT_TEAM=$(devteam) VULKAN_SDK=$(vulkan_sdk) DSTROOT=$(dstroot)

install.tools:
	xcodebuild -project ktxtools.xcodeproj -configuration $(config) -target install.tools DEVELOPMENT_TEAM=$(devteam) VULKAN_SDK=$(vulkan_sdk) DSTROOT=$(dstroot)

toktx-tests:
	xcodebuild -project ktxtools.xcodeproj -configuration $(config) -target toktx-tests DEVELOPMENT_TEAM=$(devteam) VULKAN_SDK=$(vulkan_sdk) DSTROOT=$(dstroot)

clean:
	xcodebuild -project ktxtools.xcodeproj -configuration $(config) clean DEVELOPMENT_TEAM=$(devteam) VULKAN_SDK=$(vulkan_sdk) DSTROOT=$(dstroot)

install.lib.debug install.tools.debug toktx-tests.debug clean.debug: config=Debug
install.lib.debug: install.lib
install.tools.debug: install.tools
toktx-tests.debug: toktx-tests
clean.debug: clean
