#!/bin/sh
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Runs at before_script stage of a Travis-CI build.

# exit if any command fails
set -e

# No certs so we're building either a PR or a fork.
if [ -z "$MACOS_CERTIFICATES_P12" ]; then
  exit 0
fi

KEY_CHAIN=build.keychain
KEY_PASS=mysecretpassword
MACOS_CERTS_TMPFILE=macOS_certificates.p12
# All other env vars used here are encrypted env vars set in the Travis
# settings.

# Set up a keychain for signing certificates
security create-keychain -p $KEY_PASS $KEY_CHAIN
security default-keychain -s $KEY_CHAIN
# Turn off timeout that re-locks the keychain to avoid risk of build
# taking longer than whatever timeout is set (default is 300s).
security set-keychain-settings -u $KEY_CHAIN
security unlock-keychain -p $KEY_PASS $KEY_CHAIN

# Import the macOS certificates
#
# $MACOS_CERTIFICATES_P12 holds a base64 encoded version of the .p12 file
# created by Keychain Access with the exported application and installer
# certificates.
#
# $MACOS_CERTIFICATES_PASSWORD is the password created for the .p12 file when
# it was exported.
#
echo $MACOS_CERTIFICATES_P12 | base64 --decode > $MACOS_CERTS_TMPFILE
# In CI (macOS 12.6) `security` prints a bunch of "attribute" info when
# importing. I have been unable to find out if it is a security risk.
# -q does not squelch it. macOS 14.6 `security` does not do this.
#
security import $MACOS_CERTS_TMPFILE -k $KEY_CHAIN -P $MACOS_CERTIFICATES_PASSWORD -T /usr/bin/codesign -T /usr/bin/productbuild
rm $MACOS_CERTS_TMPFILE

# Allow Apple tools access to signing certs in the keychain. Both this and
# an unlocked keychain are needed for access.
#
security set-key-partition-list -S apple-tool:,apple: -s -k $KEY_PASS $KEY_CHAIN

# Add altool-specific password for notarization
#
# This is the altool-specific password created in the Apple developer account
# to be used for notarization, the same account that was used to created the
# signing certificates imported above.
#
# $APPLE_ID is the id of the developer account.
#
# $ALTOOL_PW_LABEL is a label given to the password. This is used later by
# `altool` to find the password when submitting the notarization request.
#
# $ALTOOL_PW is the actual password. -w must NOT be the last option. If so
# it will incorrectly interpret $ALTOOL_PW as the keychain name and will
# prompt for a password!
#
security add-generic-password -a $APPLE_ID -T $(xcrun -find altool) -w $ALTOOL_PASSWORD -l $ALTOOL_PW_LABEL -s $ALTOOL_PW_LABEL

# Verify it is there
security find-generic-password -l $ALTOOL_PW_LABEL

# vim:ai:ts=4:sts=2:sw=2:expandtab
