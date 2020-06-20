#!/bin/sh

# Runs at before_script stage of a Travis-CI build.

# exit if any command fails
set -e

# Pull just the mac/ios files and images to save time. git clone
# was done before any code we control so before we could install
# git-lfs. Therefore we have to pull the files stored in git LFS.
git lfs pull --include=other_lib/mac,other_lib/ios,tests/testimages,tests/srcimages
sudo cp -r other_lib/mac/Release/SDL2.framework /Library/Frameworks

# Set up a keychain for signing certificates
security create-keychain -p mysecretpassword build.keychain
security default-keychain -s build.keychain
security unlock-keychain -p mysecretpassword build.keychain

# Import the macOS certificates
echo $MACOS_CERTIFICATES_P12 | base64 --decode > macOS_certificates.p12
security import macOS_certificates.p12 -k build.keychain -P $MACOS_CERTIFICATE_PASSWORD -T /usr/bin/codesign
rm macOS_certificates.p12

# Check it worked
security find-identity -v

