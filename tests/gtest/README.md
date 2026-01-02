
Why is gtest source included here?
==================================

1. Compiled gtest libraries are incompatible with anything but the MSVS
   version that compiled them.

2. The repo includes googlemock and a lot of other code not needed here.

3. No 2 + the complexities of git submodule and git subtree make depot
   inclusion unappealing.

Note: This is bog standard gtest.
