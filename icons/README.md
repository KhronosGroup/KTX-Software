<!-- Copyright 2016-2021 Mark Callow -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

Icon Notes
==========

These are application and document icons for each platform. They were created from the [KTX logo](https://github.com/KhronosGroup/KTX-Specification/blob/main/images/ktx.svg) found in the [KTX specification repo](https://github.com/KhronosGroup/KTX-Specification).

The iOS icon images found in, `ios/Icons.xcassets/ktx_app.appiconset`, were
created using
[Image2Icon](https://itunes.apple.com/us/app/image2icon-make-your-own-icons/id992115977?mt=12).
On Windows, [Axialis](http://www.axialis.com/iconworkshop/) can be used.
_Image2Icon_ produces a directory of images at the many sizes needed by different iOS platforms and versions. Not all of these are relevant to KTX-Software. The actual icon set was created by manually assigning the images, copied from the KTX Specification repo, to the needed roles and deleting unneeded images. Rather painful. The result of this effort are
the Contents.json files within the .appiconset directories.

The iOS icon sets sit in the subdirectory `CommonIcons.xcassets` in order
to be found by Xcode. When a symbolic link from an app's existing Asset
Catalog to `ktx_app.appiconset` was tried, Xcode failed to read the icon.
Copying it to the existing catalogs works but having multiple 
copies of the icon is unappealing. So the Xcode targets refer
to this additional Asset Catalog.

The Mac icon sets,`mac/*.icns`, were produced by
[Image2Icon](https://itunes.apple.com/us/app/image2icon-make-your-own-icons/id992115977?mt=12).
[Axialis IconWorkshop](http://www.axialis.com/iconworkshop/) was
tried for creating thesse but the files produced had some strange
images at certain sizes. The cause has not been investigated.

The Windows (`win/*.ico`) files were produced by [Axialis
IconWorkshop](http://www.axialis.com/iconworkshop/). On macOS
[Image2Icon](https://itunes.apple.com/us/app/image2icon-make-your-own-icons/id992115977?mt=12)
can be used to create `.ico` files but an in-app purchase is required
to enable this function. Furthermore the size is limited to 256x256.
Windows 10 wants 768x768.
