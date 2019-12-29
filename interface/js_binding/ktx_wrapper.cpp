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

using namespace emscripten;

namespace ktx_wrapper
{
    class texture
    {
    public:
        texture(texture&) = delete;
        texture(texture&& other) = default;

        static const uint32_t KTX_TTF_ETC1_RGB = ::KTX_TTF_ETC1_RGB;
        static const uint32_t KTX_TTF_ETC2_RGBA = ::KTX_TTF_ETC2_RGBA;
        static const uint32_t KTX_TTF_BC1_RGB = ::KTX_TTF_BC1_RGB;
        static const uint32_t KTX_TTF_BC3_RGBA = ::KTX_TTF_BC3_RGBA;
        static const uint32_t KTX_TTF_BC4_R = ::KTX_TTF_BC4_R;
        static const uint32_t KTX_TTF_BC5_RG = ::KTX_TTF_BC5_RG;
        static const uint32_t KTX_TTF_BC7_M6_RGB = ::KTX_TTF_BC7_M6_RGB;
        static const uint32_t KTX_TTF_BC7_M5_RGBA = ::KTX_TTF_BC7_M5_RGBA;
        static const uint32_t KTX_TTF_PVRTC1_4_RGB = ::KTX_TTF_PVRTC1_4_RGB;
        static const uint32_t KTX_TTF_PVRTC1_4_RGBA = ::KTX_TTF_PVRTC1_4_RGBA;
        static const uint32_t KTX_TTF_ASTC_4x4_RGBA = ::KTX_TTF_ASTC_4x4_RGBA;
        static const uint32_t KTX_TTF_PVRTC2_4_RGB = ::KTX_TTF_PVRTC2_4_RGB;
        static const uint32_t KTX_TTF_PVRTC2_4_RGBA = ::KTX_TTF_PVRTC2_4_RGBA;
        static const uint32_t KTX_TTF_ETC2_EAC_R11 = ::KTX_TTF_ETC2_EAC_R11;
        static const uint32_t KTX_TTF_ETC2_EAC_RG11 = ::KTX_TTF_ETC2_EAC_RG11;
        static const uint32_t KTX_TTF_RGBA32 = ::KTX_TTF_RGBA32;
        static const uint32_t KTX_TTF_RGB565 = ::KTX_TTF_RGB565;
        static const uint32_t KTX_TTF_BGR565 = ::KTX_TTF_BGR565;
        static const uint32_t KTX_TTF_RGBA4444 = ::KTX_TTF_RGBA4444;
        static const uint32_t KTX_TTF_ETC = ::KTX_TTF_ETC;
        static const uint32_t KTX_TTF_BC1_OR_3 = ::KTX_TTF_BC1_OR_3;

        static texture createFromMemory(const emscripten::val& data)
        {
            std::vector<uint8_t> bytes{};
            bytes.resize(data["byteLength"].as<size_t>());
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

        void transcodeBasis(uint32_t fmt, uint32_t decodeFlags)
        {
            if (m_ptr->classId != ktxTexture2_c)
            {
                std::cout << "ERROR: transcodeBasis is only supported for KTX2" << std::endl;
                return;
            }

            KTX_error_code result = ktxTexture2_TranscodeBasis(
                reinterpret_cast<ktxTexture2*>(m_ptr.get()),
                static_cast<ktx_texture_transcode_fmt_e>(fmt),
                static_cast<ktx_uint32_t>(decodeFlags));

            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to transcode: " << ktxErrorString(result) << std::endl;
                return;
            }
        }

        emscripten::val glUpload()
        {
            GLuint texture = 0;
            GLenum target = 0;
            GLenum error = 0;
            KTX_error_code result = ktxTexture_GLUpload(m_ptr.get(), &texture, &target, &error);
            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to GL upload: " << ktxErrorString(result) << std::endl;
            }

            emscripten::val ret = emscripten::val::object();
            ret.set("texture", texture);
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

EMSCRIPTEN_BINDINGS(ktx_wrapper)
{
    class_<ktx_wrapper::texture>("ktxTexture")
#if 0
        .class_property("KTX_TTF_ETC1_RGB", &ktx_wrapper::texture::KTX_TTF_ETC1_RGB)
        .class_property("KTX_TTF_ETC2_RGBA", &ktx_wrapper::texture::KTX_TTF_ETC2_RGBA)
        .class_property("KTX_TTF_BC1_RGB", &ktx_wrapper::texture::KTX_TTF_BC1_RGB)
        .class_property("KTX_TTF_BC3_RGBA", &ktx_wrapper::texture::KTX_TTF_BC3_RGBA)
        .class_property("KTX_TTF_BC4_R", &ktx_wrapper::texture::KTX_TTF_BC4_R)
        .class_property("KTX_TTF_BC5_RG", &ktx_wrapper::texture::KTX_TTF_BC5_RG)
        .class_property("KTX_TTF_BC7_M6_RGB", &ktx_wrapper::texture::KTX_TTF_BC7_M6_RGB)
        .class_property("KTX_TTF_BC7_M5_RGBA", &ktx_wrapper::texture::KTX_TTF_BC7_M5_RGBA)
        .class_property("KTX_TTF_PVRTC1_4_RGB", &ktx_wrapper::texture::KTX_TTF_PVRTC1_4_RGB)
        .class_property("KTX_TTF_PVRTC1_4_RGBA", &ktx_wrapper::texture::KTX_TTF_PVRTC1_4_RGBA)
        .class_property("KTX_TTF_ASTC_4x4_RGBA", &ktx_wrapper::texture::KTX_TTF_ASTC_4x4_RGBA)
        .class_property("KTX_TTF_PVRTC2_4_RGB", &ktx_wrapper::texture::KTX_TTF_PVRTC2_4_RGB)
        .class_property("KTX_TTF_PVRTC2_4_RGBA", &ktx_wrapper::texture::KTX_TTF_PVRTC2_4_RGBA)
        .class_property("KTX_TTF_ETC2_EAC_R11", &ktx_wrapper::texture::KTX_TTF_ETC2_EAC_R11)
        .class_property("KTX_TTF_ETC2_EAC_RG11", &ktx_wrapper::texture::KTX_TTF_ETC2_EAC_RG11)
        .class_property("KTX_TTF_RGBA32", &ktx_wrapper::texture::KTX_TTF_RGBA32)
        .class_property("KTX_TTF_RGB565", &ktx_wrapper::texture::KTX_TTF_RGB565)
        .class_property("KTX_TTF_BGR565", &ktx_wrapper::texture::KTX_TTF_BGR565)
        .class_property("KTX_TTF_RGBA4444", &ktx_wrapper::texture::KTX_TTF_RGBA4444)
        .class_property("KTX_TTF_ETC", &ktx_wrapper::texture::KTX_TTF_ETC)
        .class_property("KTX_TTF_BC1_OR_3", &ktx_wrapper::texture::KTX_TTF_BC1_OR_3)
#endif
        .constructor(&ktx_wrapper::texture::createFromMemory)
        //.class_function("createFromMemory", &ktx_wrapper::texture::createFromMemory)
        // .property("data", &ktx_wrapper::texture::getData)
        .property("baseWidth", &ktx_wrapper::texture::baseWidth)
        .property("baseHeight", &ktx_wrapper::texture::baseHeight)
        .function("transcodeBasis", &ktx_wrapper::texture::transcodeBasis)
        .function("glUpload", &ktx_wrapper::texture::glUpload)
    ;
}
