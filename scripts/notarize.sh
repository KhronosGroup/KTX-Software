#!/bin/zsh
# Copyright 2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

# notarize.sh

# Checks pkg for a valid signature. If valid, uploads the pkg for
# notarization and waits for the notarization status

# Usage: notarize.sh <path/to/pkg> <appleid> <devteam> <password>
#    path/to/pkg - path a signed .pkg file to be notarized.
#    appleid - Apple developer login.
#    devteam - 10 character development team identifier. Use Khronos dev
#              team id when notarizing apps signed with Khronos cert.
#    password - app-specific password for to use when logging in to
#               <appleid>. The same app-specific password can be used
#               for both altool and notarytool. If the password is stored
#               in the keychain then this argument should be of the form
#               @keychain:<pw_label> where <pw_label> is the label of the
#               password in the keychain.
#
# Retrieving this password from the keychain during a Travis-CI build was
# not working for altool for reasons that are not clear. altool would hang.
# Likely macOS security is asking for permission for altool to access the
# keychain, even though this was set up. This has not been tried with
# notarytool.

# Thanks to Armin Briegel for the script that inspired this one.

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

logfile=notarization.json

# functions
notarizefile() { # $1: path to file to notarize
    # upload file
    echo "## Sending $1 for notarization."
    echo "## Will await result. This may take some time..."
    if nt_out=$(xcrun notarytool submit "$1" \
                           --wait \
                           --apple-id "$appleid" \
                           --password "$passwd" \
                           --team-id "$devteam")
    then
        # N.B. notarytool outputs 2 lines starting with "status:". The first
        # initially says "In Progress" and is later overwritten with the
        # final status. This is done by sending a CR followed by the new text.
        # The second "status:" line is output at the end. All interim and
        # final messages are recorded in $nt_out. To avoid matching any interim
        # messages - "In" is a particular problem - use entire status response
        # words in this RE.
        #
        # A big thank to Apples, NOT :-(, for failing to document in the
        # command help or even the TechNotes about notarization, the output
        # from notarytool during --wait or to give any example of use.
        if [[ "$nt_out" =~ "status: (Accepted|Invalid|Rejected)" ]]; then
            ntz_status=$match[1]
            # There are many "id:" lines in the output. Fortunately they
            # all have the same value.
            [[ $nt_out =~ "id: ([0-9a-f\-]+)" ]] && id=$match[1]
            if ! xcrun notarytool log $id $logfile \
                           --apple-id "$appleid" \
                           --password "$passwd" \
                           --team-id "$devteam"
            then
               echo "$0: Retrieval of notarization log for id $id failed."
               exit 1
            fi
            echo "## Notarization status: $ntz_status. Log:"
            cat $logfile
            echo
            rm $logfile
        else
            echo "$0: \"status:\" not found in notarytool output."
            exit 1
        fi
    else
        echo "$0: Error while attempting to notarize $1."
        echo $nt_out
        exit 1
    fi

    if [[ $ntz_status != "Accepted" ]]; then
        echo "## $1 not notarized. Exiting"
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
  echo "$0: Package $pkg does not have a valid signature."
  exit 2
fi

# Upload for notarization
notarizefile "$pkg"

# staple result
echo "## Stapling $pkg"
xcrun stapler staple "$pkg"

echo '## Done!'

exit 0

# vim:ai:ts=4:sts=2:sw=2:expandtab
