#!/bin/zsh
# Copyright 2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

# notarize.sh

# Checks pkg for a valid signature. If valid, uploads the pkg for
# notarization and monitors the notarization status

# Usage: notarize.sh <path/to/pkg> <appleid> <devteam> <password-label>
#    path/to/pkg - path a signed .pkg file to be notarized.
#    appleid - Apple developer login.
#    devtem - 10 character development team identifier.
#    password - app-specific password for altool to use when logging in to <applieid>.
#               If the password is stored in the keychain then this argument should be
#               of the form @keychain:<pw_label> where <pw_label> is the label
#               of the password in the keychain.
#
# Retrieving this password from the keychain during a Travis-CI build is not working for
# reasons that are not clear. altool is hanging. Clearly macOS security is asking
# for permission for altool to access the keychain, even though this was set up.

# Thanks to Armin Briegel for the script that inspired this one.

# Project information
bundleid="com.khronos.ktx"

# Code starts here

if [[ $# != 4 ]]; then
  echo "Usage: $0: <path/to/pkg> <appleid> <devteam> <password>"
  exit 1
else
  pkg=$1
  appleid=$2
  devteam=$3
  passwd=$4
fi

# functions
requeststatus() { # $1: requestUUID
    req_status=$(xcrun altool --notarization-info "$requestUUID" \
                              --username "$appleid" \
                              --password "$passwd" 2>&1 \
                 | awk -F ': ' '/Status:/ { print $2; }' )
    echo "$req_status"
}

notarizefile() { # $1: path to file to notarize, $2: bundle id.
    # upload file
    echo "## uploading $1 for notarization"
    if al_out=$(xcrun altool --notarize-app \
                               --primary-bundle-id "$2" \
                               --username "$appleid" \
                               --password "$passwd" \
                               --asc-provider "$devteam" \
                               --file "$1" 2>&1)
    then
        requestUUID=$(echo -n $al_out | awk '/RequestUUID/ { print $NF; }')
        echo "Notarization RequestUUID: $requestUUID"
    else
        echo "Could not upload for notarization: $al_out"
        exit 1
    fi

    # Wait for status to be not "in progress" any more
    request_status="in progress"
    while [[ "$request_status" == "in progress" ]]; do
        echo -n "waiting... "
        sleep 10
        request_status=$(requeststatus "$requestUUID")
        echo "$request_status"
    done

    # Print status information
    xcrun altool --notarization-info "$requestUUID" \
                 --username "$appleid" \
                 --password "$passwd"
    echo

    if [[ $request_status != "success" ]]; then
        echo "Could not notarize $1"
        exit 1
    fi
}


# Check if pkg exists where we expect it
if [[ ! -f $pkg ]]; then
    echo "$0: Couldn't find pkg $pkg"
    exit 1
fi

# Check the package is signed
if ! pkgutil --check-signature $pkg > /dev/null; then
  echo "Package does not have a valid signature."
  exit 2
fi

# Upload for notarization
notarizefile "$pkg" "$bundleid"

# staple result
echo "## Stapling $pkg"
xcrun stapler staple "$pkg"

echo '## Done!'

exit 0

# vim:ai:ts=4:sts=2:sw=2:expandtab
