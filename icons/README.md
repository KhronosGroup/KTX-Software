Icon Notes
==========

The icons were designed by Manmohan Bishnoi,
[Renuilus Technologies](http://www.renuilus.com/).

The `ktx_document` icons here were exported from the SVG master and
the `ktx_app` icons from the PNG master both housed in the KTX
specification repo.

The iOS icon set, `ios/Icons.xcassets/ktx_app.appiconset`, was
created using the [MakeAppIcon](http://makeappicon.com) website
which can be used for Android icons as well. The icon set sits in
the subdirectory `Icons.xcassets` in order to be found by Xcode.
When a symbolic link from the apps' existing Asset Catalogs to
`ktx_app.appiconset` was tried, Xcode failed to read the icon.
Copying it to the existing catalogs works but having multiple copies
of the icon is unappealing. So the Xcode targets refer to this
additional Asset Catalog.

[Image2Icon](https://itunes.apple.com/us/app/image2icon-make-your-own-icons/id992115977?mt=12)
can also be used to create iOS icon sets or, on Windows,
[Axialis](http://www.axialis.com/iconworkshop/).

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


