#!/bin/zsh

set -x

# notarize.sh

# ©2020 - Mark Callow

# Usage: notarize.sh <path/to/pkg> <appleid> <devteam> <password-label>
#    path/to/pkg - path a signed .pkg file to be notarized.
#    appleid - Apple developer login.
#    devtem - 10 character development team identifier.
#    password-label - the label of the altool app-specific password in your keychain.

# Checks pkg for a valid signature. If valid, uploads the pkg for
# notarization and monitors the notarization status

# Thanks to Armin Briegel for the script that inspired this one.

# Project information
bundleid="com.khronos.ktx"

# Code starts here

if [[ $# != 4 ]]; then
  echo "Usage: $0: <path/to/pkg> <appleid> <devteam> <password-label>"
  exit 1
else
  pkg=$1
  appleid=$2
  devteam=$3
  pw_label=$4
fi

# functions
requeststatus() { # $1: requestUUID
    req_status=$(xcrun altool --notarization-info "$requestUUID" \
                              --username "$appleid" \
                              --password "@keychain:$pw_label" 2>&1 \
                 | awk -F ': ' '/Status:/ { print $2; }' )
    echo "$req_status"
}

notarizefile() { # $1: path to file to notarize, $2: bundle id.
    # upload file
    echo "## uploading $1 for notarization"
    echo "altool is $(xcrun -find altool)"
    requestUUID=$(xcrun altool --notarize-app \
                               --primary-bundle-id "$2" \
                               --username "$appleid" \
                               --password "$pw_label" \
                               --asc-provider "$devteam" \
                               --file "$1" 2>&1 \
                  | awk '/RequestUUID/ { print $NF; }')

    echo "Notarization RequestUUID: $requestUUID"

    if [[ $requestUUID == "" ]]; then 
        echo "Could not upload for notarization"
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
                 --password "@keychain:$pw_label"
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

security unlock-keychain -p mysecretpassword build.keychain
security set-keychain-settings -t 3600 -u build.keychain

# Upload for notarization
notarizefile "$pkg" "$bundleid"

# staple result
echo "## Stapling $pkg"
xcrun stapler staple "$pkg"

echo '## Done!'

exit 0

# vim:ai:ts=4:sts=2:sw=2:expandtab
