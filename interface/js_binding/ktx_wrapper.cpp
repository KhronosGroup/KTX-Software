/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=80: */

/*
 * Copyright 2019-2020 Khronos Group, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <emscripten/bind.h>
#include <ktx.h>
#include <iostream>

using namespace emscripten;

#define ktxTexture2(t) reinterpret_cast<ktxTexture2*>(m_ptr.get())

namespace ktx
{
    class texture
    {
    public:
        texture(texture&) = delete;
        texture(texture&& other) = default;

        static texture createFromMemory(const emscripten::val& data)
        {
            std::vector<uint8_t> bytes{};
            bytes.resize(data["byteLength"].as<size_t>());
            // Yes, this code IS copying the data. Sigh! According to Alon
            // Zakai:
            //     "There isn't a way to let compiled code access a new
            //     ArrayBuffer. The compiled code has hardcoded access to the
            //     wasm Memory it was instantiated with - all the pointers it
            //     can understand are indexes into that Memory. It can't refer
            //     to anything else, I'm afraid."
            //
            //     "In the future using different address spaces or techniques
            //     with reference types may open up some possibilities here."
            emscripten::val memory = emscripten::val::module_property("HEAP8")["buffer"];
            emscripten::val memoryView = data["constructor"].new_(memory, reinterpret_cast<uintptr_t>(bytes.data()), data["length"].as<uint32_t>());
            memoryView.call<void>("set", data);

            // Load the image data now, otherwise we'd have to  copy it from
            // JS into a buffer only for it to be copied from that buffer into
            // the texture later.
            ktxTexture* ptr = nullptr;
            KTX_error_code result = ktxTexture_CreateFromMemory(
                                        bytes.data(),
                                        bytes.size(),
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        &ptr);
            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to create from memory: " << ktxErrorString(result) << std::endl;
                return texture(nullptr);
            }

            return texture(ptr);
        }

        uint32_t baseWidth() const
        {
            return m_ptr->baseWidth;
        }

        uint32_t baseHeight() const
        {
            return m_ptr->baseHeight;
        }

        bool needsTranscoding() const
        {
            return ktxTexture_NeedsTranscoding(m_ptr.get());
        }

        bool isPremultiplied() const
        {
            return (m_ptr->classId == ktxTexture2_c
                    && ktxTexture2_GetPremultipliedAlpha(ktxTexture2(m_ptr.get())));
        }

        // @copydoc ktxTexture2::GetNumComponents
        uint32_t numComponents() const
        {
            if (m_ptr->classId != ktxTexture2_c)
            {
                std::cout << "ERROR: numComponents is only supported for KTX2" << std::endl;
                return 0;
            }

            return ktxTexture2_GetNumComponents(ktxTexture2(m_ptr.get()));
        }

        enum ktxSupercmpScheme supercompressionScheme() const
        {
            if (m_ptr->classId == ktxTexture1_c)
                return KTX_SS_NONE;
            else
                return ktxTexture2(m_ptr.get())->supercompressionScheme;
        }

        uint32_t vkFormat() const
        {
            if (m_ptr->classId != ktxTexture2_c)
            {
                std::cout << "ERROR: vkFormat is only supported for KTX2" << std::endl;
                return 0;
            }

            return ktxTexture2(m_ptr.get())->vkFormat;
        }

        struct ktxOrientation orientation() const
        {
            return m_ptr->orientation;
        }

        ktx_error_code_e transcodeBasis(const val& targetFormat, const val& decodeFlags)
        {
            if (m_ptr->classId != ktxTexture2_c)
            {
                std::cout << "ERROR: transcodeBasis is only supported for KTX2" << std::endl;
                return KTX_INVALID_VALUE;
            }

            KTX_error_code result = ktxTexture2_TranscodeBasis(
                ktxTexture2(m_ptr.get()),
                targetFormat.as<ktx_texture_transcode_fmt_e>(),
                decodeFlags.as<ktx_transcode_flags>());

            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to transcode: " << ktxErrorString(result) << std::endl;
            }
            return result;
        }

        // NOTE: WebGLTexture objects are completely opaque so the option of passing in the texture
        // to use is not viable. Unknown at present is how to find the WebGLTexture for the texture
        // created by ktxTexture_GLUpload via the Emscripten OpenGL ES emulation.
        emscripten::val glUpload()
        {
            GLuint texname = 0;
            GLenum target = 0;
            GLenum error = 0;
            KTX_error_code result = ktxTexture_GLUpload(m_ptr.get(), &texname, &target, &error);
            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to GL upload: " << ktxErrorString(result) << std::endl;
            }

            val ret = val::object();
            // Find the WebGLTexture for texture.
            val texture = val::module_property("GL")["textures"][texname];
            ret.set("texture", texture);
            ret.set("target", target);
            ret.set("error", error);
            return ret;
        }

    private:
        texture(ktxTexture* ptr)
            : m_ptr{ ptr, &destroy }
        {
        }

        static void destroy(ktxTexture* ptr)
        {
            ktxTexture_Destroy(ptr);
        }

        std::unique_ptr<ktxTexture, decltype(&destroy)> m_ptr;
    };
}

/** @mainpage

 Javascript bindings are provided to:

 @li @subpage libktx_js "libktx (in libktx.js)"
 @li @subpage msc_basis_transcoder "Basis Universal Transcoder (in msc_basis_transcoder.js)


 ---
 @par This page last modified $Date$
*/

/** @page libktx_js libktx Binding

## WebIDL for the binding

@code{.unparsed}
interface ktxOrientation {
    readonly attribute OrientationX x;
    readonly attribute OrientationY y;
    readonly attribute OrientationZ z;
};

interface ktxUploadResult {
    readonly attribute WebGLTexture texture;
    readonly attribute GLenum target;
    readonly attribute GLenum error;
};

interface ktxTexture {
    void ktxTexture(ArrayBufferView fileData);
    UploadResult glUpload();
    ErrorCode transcodeBasis();

    readonly attribute long baseWidth;
    readonly attribute long baseHeight;
    readonly attribute bool isPremultiplied;
    readonly attribute bool needsTranscoding;
    readonly attribute long numComponents;
    readonly attribute long vkFormat;
    readonly attribute SupercmpScheme supercompressionScheme;
    readonly attribute ktxOrientation orientation;
};

enum ErrorCode = {
    "SUCCESS",
    "FILE_DATA_ERROR",
    "FILE_ISPIPE",
    "FILE_OPEN_FAILED",
    "FILE_OVERFLOW",
    "FILE_READ_ERROR",
    "FILE_SEEK_ERROR",
    "FILE_UNEXPECTED_ERROR",
    "FILE_WRITE_ERROR",
    "GL_ERROR",
    "INVALID_OPERATION",
    "INVALID_VALUE",
    "NOT_FOUND",
    "OUT_OF_MEMORY",
    "TRANSCODE_FAILED",
    "UNKNOWN_FILE_FORMAT",
    "UNSUPPORTED_TEXTURE_TYPE",
    "UNSUPPORTED_FEATURE",
    "LIBRARY_NOT_LINKED"
};

// Some targets may not be available depending on options used when compiling
// the web assembly. ktxTexture.transcodeBasis will report this.
enum TranscodeTarget = {
    "ETC1_RGB",
    "BC1_RGB",
    "BC4_R",
    "BC5_RG",
    "BC3_RGBA",
    "BC1_OR_3",
    "PVRTC1_4_RGB",
    "PVRTC1_4_RGBA",
    "BC7_RGBA",
    "BC7_M6_RGB",  // Deprecated. Use BC7_RGBA.
    "BC7_M5_RGBA", // Deprecated. Use BC7_RGBA.
    "ETC2_RGBA",
    "ASTC_4x4_RGBA",
    "RGBA32",
    "RGB565",
    "BGR565",
    "RGBA4444",
    "PVRTC2_4_RGB",
    "PVRTC2_4_RGBA",
    "ETC",
    "EAC_R11",
    "EAC_RG11"
};

enum TranscodeFlagBits {
   "TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS"
};

enum OrientationX {
    "LEFT",
    "RIGHT"
};
enum OrientationY {
    "UP",
    "DOWN"
};
enum OrientationZ {
    "IN",
    "OUT"
};

enum SupercmpScheme {
    "NONE",
    "BASIS_LZ",
    "ZSTD"
    "ZLIB"
};

@endcode

## How to use

Put libktx.js and libktx.wasm in a directory on your server. Create a script
tag with libktx.js as the @c src as shown below, changing the path as necessary
for the relative locations of your .html file and the script source. libktx.js
will automatically load msc_basis_transcoder.wasm.

### Create an instance of the LIBKTX module

Add this to the .html file to initialize the libktx module and make it available on the main window.
@code{.unparsed}
    &lt;script src="libktx.js">&lt;/script>
    &lt;script type="text/javascript">
      LIBKTX({preinitializedWebGLContext: gl}).then(module => {
        window.LIBKTX = module;
        // Make existing WebGL context current for Emscripten OpenGL.
        LIBKTX.GL.makeContextCurrent(LIBKTX.GL.createContext(null, { majorVersion: 2.0 }));
        texture = loadTexture(gl, 'ktx_app.ktx2');
      });
    &lt;/script>
@endcode

@e After the module is initialized, invoke code that will directly or indirectly cause
a function like the following to be executed.

### loadTexture function

@code{.unparsed}
    // Set up an XHR to fetch the url into an ArrayBuffer.
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url);
    xhr.responseType = "arrayBuffer";
    xhr.onload = function() {
      const { ktxTexture, TranscodeTarget, OrientationX, OrientationY } = LIBKTX;
      // this.response is a generic binary buffer which
      // we can interpret as Uint8Array.
      var ktxdata = new Uint8Array(this.response);
      ktexture = new ktxTexture(ktxdata);

      if (ktexture.needsTranscoding) {
        var formatString;
        var format;
        if (astcSupported) {
          formatString = 'ASTC';
          format = TranscodeTarget.ASTC_4x4_RGBA;
        } else if (dxtSupported) {
          formatString = 'BC1 or BC3';
          format = TranscodeTarget.BC1_OR_3;
        } else if (pvrtcSupported) {
          formatString = 'PVRTC1';
          format = TranscodeTarget.PVRTC1_4_RGBA;
        } else if (etcSupported) {
          formatString = 'ETC';
          format = TranscodeTarget.ETC;
        } else {
          formatString = 'RGBA4444';
          format = TranscodeTarget.RGBA4444;
        }
        if (ktexture.transcodeBasis(format, 0) != LIBKTX.ErrorCode.SUCCESS) {
          alert('Texture transcode failed. See console for details.');
          return undefined;
        }
      }

      // If there is no global variable "texture".
      //const {texture, target, error} = ktexture.glUpload();
      // If there is a globla variable "texture"
      const result = ktexture.glUpload();
      const {target, error} = result;
      texture = result.texture;

      if (error != gl.NO_ERROR) {
        alert('WebGL error when uploading texture, code = ' + error.toString(16));
        return undefined;
      }
      if (texture === undefined) {
        alert('Texture upload failed. See console for details.');
        return undefined;
      }
      if (target != gl.TEXTURE_2D) {
        alert('Loaded texture is not a TEXTURE2D.');
        return undefined;
      }

      gl.bindTexture(target, texture);
      // If using a placeholder texture during loading, delete
      // it now.
      //gl.deleteTexture(placeholder);

      if (ktexture.numLevels > 1 || ktexture.generateMipmaps)
         // Enable bilinear mipmapping.
         gl.texParameteri(target,
                          gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_NEAREST);
      else
        gl.texParameteri(target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
      gl.texParameteri(target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

      if (ktexture.orientation.x == OrientationX.LEFT) {
        // Adjust u coords, e.g. by setting up a uv transform
      }
      if (ktexture.orientation.y == OrientationY.DOWN) {
        // Adjust v coords, e.g. by setting up a uv transform
      }
      ktexture.delete();
    }

    xhr.send();
@endcode

@note It is not clear if glUpload can be used with, e.g. THREE.js. It may
be necessary to expose the ktxTexture_IterateLevelFaces or
ktxTexture_IterateLoadLevelFaces API to JS with those calling a
callback in JS to upload each image to WebGL.


*/

EMSCRIPTEN_BINDINGS(ktx)
{
    enum_<ktx_error_code_e>("ErrorCode")
        .value("SUCCESS", KTX_SUCCESS)
        .value("FILE_DATA_ERROR", KTX_FILE_DATA_ERROR)
        .value("FILE_ISPIPE", KTX_FILE_ISPIPE)
        .value("FILE_OPEN_FAILED", KTX_FILE_OPEN_FAILED)
        .value("FILE_OVERFLOW", KTX_FILE_OVERFLOW)
        .value("FILE_READ_ERROR", KTX_FILE_READ_ERROR)
        .value("FILE_SEEK_ERROR", KTX_FILE_SEEK_ERROR)
        .value("FILE_UNEXPECTED_ERROR", KTX_FILE_UNEXPECTED_EOF)
        .value("FILE_WRITE_ERROR", KTX_FILE_WRITE_ERROR)
        .value("GL_ERROR", KTX_GL_ERROR)
        .value("INVALID_OPERATION", KTX_INVALID_OPERATION)
        .value("INVALID_VALUE", KTX_INVALID_VALUE)
        .value("NOT_FOUND", KTX_NOT_FOUND)
        .value("OUT_OF_MEMORY", KTX_OUT_OF_MEMORY)
        .value("TRANSCODE_FAILED", KTX_TRANSCODE_FAILED)
        .value("UNKNOWN_FILE_FORMAT", KTX_UNKNOWN_FILE_FORMAT)
        .value("UNSUPPORTED_TEXTURE_TYPE", KTX_UNSUPPORTED_TEXTURE_TYPE)
        .value("UNSUPPORTED_FEATURE", KTX_UNSUPPORTED_FEATURE)
        .value("LIBRARY_NOT_LINKED", KTX_LIBRARY_NOT_LINKED)
        .value("DECOMPRESS_LENGTH_ERROR", KTX_DECOMPRESS_LENGTH_ERROR)
        .value("DECOMPRESS_CHECKSUM_ERROR", KTX_DECOMPRESS_CHECKSUM_ERROR)
        ;

    enum_<ktx_texture_transcode_fmt_e>("TranscodeTarget")
        .value("ETC1_RGB", KTX_TTF_ETC1_RGB)
        .value("BC1_RGB", KTX_TTF_BC1_RGB)
        .value("BC4_R", KTX_TTF_BC4_R)
        .value("BC5_RG", KTX_TTF_BC5_RG)
        .value("BC3_RGBA", KTX_TTF_BC3_RGBA)
        .value("BC1_OR_3", KTX_TTF_BC1_OR_3)
        .value("PVRTC1_4_RGB", KTX_TTF_PVRTC1_4_RGB)
        .value("PVRTC1_4_RGBA", KTX_TTF_PVRTC1_4_RGBA)
        .value("BC7_RGBA", KTX_TTF_BC7_RGBA)
        .value("BC7_M6_RGB", KTX_TTF_BC7_M6_RGB)
        .value("BC7_M5_RGBA", KTX_TTF_BC7_M5_RGBA)
        .value("ETC2_RGBA", KTX_TTF_ETC2_RGBA)
        .value("ASTC_4x4_RGBA", KTX_TTF_ASTC_4x4_RGBA)
        .value("RGBA32", KTX_TTF_RGBA32)
        .value("RGB565", KTX_TTF_RGB565)
        .value("BGR565", KTX_TTF_BGR565)
        .value("RGBA4444", KTX_TTF_RGBA4444)
        .value("PVRTC2_4_RGB", KTX_TTF_PVRTC2_4_RGB)
        .value("PVRTC2_4_RGBA", KTX_TTF_PVRTC2_4_RGBA)
        .value("ETC", KTX_TTF_ETC)
        .value("EAC_R11", KTX_TTF_ETC2_EAC_R11)
        .value("EAC_RG11", KTX_TTF_ETC2_EAC_RG11)
    ;
    enum_<ktx_transcode_flag_bits_e>("TranscodeFlagBits")
        .value("TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS",
               KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS)
    ;
    enum_<ktxOrientationX>("OrientationX")
        .value("LEFT", KTX_ORIENT_X_LEFT)
        .value("RIGHT", KTX_ORIENT_X_RIGHT)
    ;
    enum_<ktxOrientationY>("OrientationY")
        .value("UP", KTX_ORIENT_Y_UP)
        .value("DOWN", KTX_ORIENT_Y_DOWN)
    ;
    enum_<ktxOrientationZ>("OrientationZ")
        .value("IN", KTX_ORIENT_Z_IN)
        .value("OUT", KTX_ORIENT_Z_OUT)
    ;
    enum_<ktxSupercmpScheme>("SupercmpScheme")
        .value("NONE", KTX_SS_NONE)
        .value("BASIS_LZ", KTX_SS_BASIS_LZ)
        .value("ZSTD", KTX_SS_ZSTD)
        .value("ZLIB", KTX_SS_ZLIB)
    ;

    value_object<ktxOrientation>("Orientation")
        .field("x", &ktxOrientation::x)
        .field("y", &ktxOrientation::y)
        .field("z", &ktxOrientation::z)
        ;

    class_<ktx::texture>("ktxTexture")
        .constructor(&ktx::texture::createFromMemory)
        //.class_function("createFromMemory", &ktx::texture::createFromMemory)
        // .property("data", &ktx::texture::getData)
        .property("baseWidth", &ktx::texture::baseWidth)
        .property("baseHeight", &ktx::texture::baseHeight)
        .property("isPremultiplied", &ktx::texture::isPremultiplied)
        .property("needsTranscoding", &ktx::texture::needsTranscoding)
        .property("numComponents", &ktx::texture::numComponents)
        .property("orientation", &ktx::texture::orientation)
        .property("supercompressScheme", &ktx::texture::supercompressionScheme)
        .property("vkFormat", &ktx::texture::vkFormat)
        .function("transcodeBasis", &ktx::texture::transcodeBasis)
        .function("glUpload", &ktx::texture::glUpload)
    ;
}
