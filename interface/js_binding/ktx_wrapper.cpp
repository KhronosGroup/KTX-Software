/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2019 Khronos Group, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <emscripten/bind.h>
#include <ktx.h>
#include <iostream>

#include "basis_transcoder_config.h"

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
            // Yes, this code IS copying the data. Sigh! According to Alon Zakai:
            //     "There isn't a way to let compiled code access a new ArrayBuffer.
            //     The compiled code has hardcoded access to the wasm Memory it was
            //     instantiated with - all the pointers it can understand are indexes
            //     into that Memory. It can't refer to anything else, I'm afraid."
            //
            //     "In the future using different address spaces or techniques with
            //     reference types may open up some possibilities here."
            emscripten::val memory = emscripten::val::module_property("HEAP8")["buffer"];
            emscripten::val memoryView = data["constructor"].new_(memory, reinterpret_cast<uintptr_t>(bytes.data()), data["length"].as<uint32_t>());
            memoryView.call<void>("set", data);

            ktxTexture* ptr = nullptr;
            KTX_error_code result = ktxTexture_CreateFromMemory(bytes.data(), bytes.size(), KTX_TEXTURE_CREATE_NO_FLAGS, &ptr);
            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to create from memory: " << ktxErrorString(result) << std::endl;
                return texture(nullptr, {});
            }

            // TODO: Is this move copying all the data? If so can we avoid it?
            return texture(ptr, std::move(bytes));
        }

        // emscripten::val getData() const
        // {
        //     return emscripten::val(emscripten::typed_memory_view(ktxTexture_GetSize(m_ptr), ktxTexture_GetData(m_ptr)));
        // }

        uint32_t baseWidth() const
        {
            return m_ptr->baseWidth;
        }

        uint32_t baseHeight() const
        {
            return m_ptr->baseHeight;
        }

        bool isSupercompressed() const
        {
            return (m_ptr->classId == ktxTexture2_c
                    && ktxTexture2(m_ptr)->supercompressionScheme != KTX_SUPERCOMPRESSION_NONE);
        }

        bool isBasisSupercompressed() const
        {
            return (m_ptr->classId == ktxTexture2_c
                    && ktxTexture2(m_ptr)->supercompressionScheme == KTX_SUPERCOMPRESSION_BASIS);
        }

        // @copydoc ktxTexture2::GetNumComponents
        uint32_t numComponents() const
        {
            if (m_ptr->classId != ktxTexture2_c)
            {
                std::cout << "ERROR: numComponents is only supported for KTX2" << std::endl;
                return 0;
            }

            return ktxTexture2_GetNumComponents(ktxTexture2(m_ptr));
        }

        uint32_t vkFormat() const
        {
            if (m_ptr->classId != ktxTexture2_c)
            {
                std::cout << "ERROR: vkFormat is only supported for KTX2" << std::endl;
                return 0;
            }

            return ktxTexture2(m_ptr)->vkFormat;
        }

        void transcodeBasis(const val& targetFormat, const val& decodeFlags)
        {
            if (m_ptr->classId != ktxTexture2_c)
            {
                std::cout << "ERROR: transcodeBasis is only supported for KTX2" << std::endl;
                return;
            }

            KTX_error_code result = ktxTexture2_TranscodeBasis(
                reinterpret_cast<ktxTexture2*>(m_ptr.get()),
                targetFormat.as<ktx_texture_transcode_fmt_e>(),
                decodeFlags.as<ktx_transcode_flags>());

            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to transcode: " << ktxErrorString(result) << std::endl;
                return;
            }
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
            val newtexture = val::module_property("GL")["textures"][texname];
            ret.set("newtexture", newtexture);
            ret.set("target", target);
            ret.set("error", error);
            return std::move(ret);
        }

    private:
        texture(ktxTexture* ptr, std::vector<ktx_uint8_t> bytes)
            : m_ptr{ ptr, &destroy }
            , m_bytes{ std::move(bytes) }
        {
        }

        static void destroy(ktxTexture* ptr)
        {
            ktxTexture_Destroy(ptr);
        }

        std::unique_ptr<ktxTexture, decltype(&destroy)> m_ptr;
        std::vector<uint8_t> m_bytes;
    };
}

/** @page ktx_js_binding Using the JS binding to libktx

=== Making the LIBKTX (Module) instance

Add this to the .html file

    <script type="text/javascript">
      LIBKTX().then(module => {
        window.LIBKTX = module;
       // Make existing WebGL context current for Emscripten OpenGL.
       LIBKTX.preinitializedWebGLContext = gl;
       LIBKTX.GL.makeContextCurrent(LIBKTX.GL.createContext(null, { majorVersion: 2.0 }));
       texture = loadTexture('url');
    </script>

=== loadTexture function

    // Set up an XHR to fetch the url into an ArrayBuffer.
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url);
    xhr.responseType = "arrayBuffer";

    const { ktxTexture, TranscodeTarget } = LIBKTX;
    // this.response is a generic binary buffer which
    // we can interpret as Uint8Array.
    var ktxdata = new Uint8Array(this.response);
    ktexture = new ktxTexture(ktxdata);

    if (ktexture.isBasisSupercompressed) {
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
      ktexture.transcodeBasis(format, 0);
    }

    const {newtexture, target, error} = ktexture.glUpload();
    if (error != gl.NO_ERROR) {
      alert('WebGL error when uploading texture, code = ' + error.toString(16));
      return undefined;
    }
    if (target != gl.TEXTURE_2D) {
      alert('Loaded texture is not a TEXTURE2D.');
      return undefined;
    }

    gl.bindTexture(target, newtexture);
    // If using a placeholder texture during loading, delete
    // it now.
    //gl.deleteTexture(placeholder);
    texture = newtex;

    if (ktexture.numLevels > 1 || ktexture.generateMipmaps)
       // Enable bilinear mipmapping.
       gl.texParameteri(target,
                        gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_NEAREST);
    else
      gl.texParameteri(target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    ktexture.delete();
  };

  xhr.send();

It is not clear if glUpload can be used with, e.g. THREE.js. It may
be necessary to expose the ktxTexture_IterateLevelFaces or
ktxTexture_IterateLoadLevelFaces API to JS with those calling a
callback in JS to upload each image to WebGL.
*/

EMSCRIPTEN_BINDINGS(ktx)
{
    enum_<ktx_texture_transcode_fmt_e>("TranscodeTarget")
        .value("ETC1_RGB", KTX_TTF_ETC1_RGB)
#if BASISD_SUPPORT_DXT1
        .value("BC1_RGB", KTX_TTF_BC1_RGB)
#endif
#if BASISD_SUPPORT_DXT5A
        .value("BC4_R", KTX_TTF_BC4_R)
        .value("BC5_RG", KTX_TTF_BC5_RG)
#endif
#if BASISD_SUPPORT_DXT1 && BASISD_SUPPORT_DXT5A
        .value("BC3_RGBA", KTX_TTF_BC3_RGBA)
        .value("BC1_OR_3", KTX_TTF_BC1_OR_3)
#endif
#if BASISD_SUPPORT_PVRTC1
        .value("PVRTC1_4_RGB", KTX_TTF_PVRTC1_4_RGB)
        .value("PVRTC1_4_RGBA", KTX_TTF_PVRTC1_4_RGBA)
#endif
#if BASISD_SUPPORT_BC7_MODE6_OPAQUE_ONLY
        .value("BC7_M6_RGB", KTX_TTF_BC7_M6_RGB)
#endif
#if BASISD_SUPPORT_BC7_MODE5
        .value("BC7_M5_RGBA", KTX_TTF_BC7_M5_RGBA)
#endif
#if BASISD_SUPPORT_ETC2_EAC_A8
        .value("ETC2_RGBA", KTX_TTF_ETC2_RGBA)
#endif
#if BASISD_SUPPORT_ASTC
        .value("ASTC_4x4_RGBA", KTX_TTF_ASTC_4x4_RGBA)
#endif
        .value("RGBA32", KTX_TTF_RGBA32)
        .value("RGB565", KTX_TTF_RGB565)
        .value("BGR565", KTX_TTF_BGR565)
        .value("RGBA4444", KTX_TTF_RGBA4444)
#if BASISD_SUPPORT_PVRTC2
        .value("PVRTC2_4_RGB", KTX_TTF_PVRTC2_4_RGB)
        .value("PVRTC2_4_RGBA", KTX_TTF_PVRTC2_4_RGBA)
#endif
#if BASISD_SUPPORT_ETC2_EAC_RG11
        .value("ETC", KTX_TTF_ETC)
        .value("EAC_R11", KTX_TTF_ETC2_EAC_R11)
        .value("EAC_RG11", KTX_TTF_ETC2_EAC_RG11)
#endif
    ;
    enum_<ktx_transcode_flag_bits_e>("TranscodeFlagBits")
        .value("TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS",
               KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS)
    ;

    class_<ktx::texture>("ktxTexture")
        .constructor(&ktx::texture::createFromMemory)
        //.class_function("createFromMemory", &ktx::texture::createFromMemory)
        // .property("data", &ktx::texture::getData)
        .property("baseWidth", &ktx::texture::baseWidth)
        .property("baseHeight", &ktx::texture::baseHeight)
        .property("isBasisSupercompressed", &ktx::texture::isBasisSupercompressed)
        .property("isSupercompressed", &ktx::texture::isSupercompressed)
        .property("numComponents", &ktx::texture::numComponents)
        .property("vkFormat", &ktx::texture::vkFormat)
        .function("transcodeBasis", &ktx::texture::transcodeBasis)
        .function("glUpload", &ktx::texture::glUpload)
    ;
}
