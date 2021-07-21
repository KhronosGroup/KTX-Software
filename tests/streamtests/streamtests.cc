// Copyright 2021 Paolo Jovon, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>
#include <iostream>
#include <fstream>
#include "gl_format.h"
#include "ktx.h"
#include "gtest/gtest.h"

namespace
{

constexpr const char SAMPLE_KTX1[] = "pattern_02_bc2.ktx";
constexpr const char SAMPLE_KTX2[] = "pattern_02_bc2.ktx2";

std::string testImagesPath;

std::unique_ptr<std::streambuf> testImageFilebuf(std::string name)
{
    std::string imagePath{testImagesPath};
    imagePath += '/';
    imagePath += name;
    
    auto filebuf = std::make_unique<std::filebuf>();
    filebuf->open(imagePath, std::ios::in | std::ios::binary);
    if (filebuf->is_open())
    {
        return filebuf;
    }
    return nullptr;
}

/// A ktxStream that wraps a C++ std::streambuf.
class StreambufStream
{
    // Doubt this will ever get triggered
    static_assert(sizeof(char) == sizeof(uint8_t), "Chars are != 1 byte in this platform");

public:
    StreambufStream(std::unique_ptr<std::streambuf> &&streambuf,
                    std::ios::openmode seek_mode = std::ios::in | std::ios::out)
        : _streambuf{std::move(streambuf)}
        , _seek_mode{seek_mode}
        , _stream{std::make_unique<ktxStream>()}
        , _destructed{false}
    {
        _stream->type = eStreamTypeCustom;
        _stream->closeOnDestruct = false;

        auto& custom_ptr = _stream->data.custom_ptr;
        custom_ptr.address = this;
        custom_ptr.allocatorAddress = nullptr; // N/A
        custom_ptr.size = 0; // N/A

        _stream->read = read;
        _stream->skip = skip;
        _stream->write = write;
        _stream->getpos = getpos;
        _stream->setpos = setpos;
        _stream->getsize = getsize;
        _stream->destruct = destruct;
    }

    StreambufStream(const StreambufStream&) = delete;
    StreambufStream &operator=(const StreambufStream&) = delete;

    StreambufStream(StreambufStream&&) = delete;
    StreambufStream &operator=(StreambufStream&&) = delete;

    virtual ~StreambufStream()
    {
        EXPECT_TRUE(_destructed) << "ktxStream should have been destructed";
    }

    inline ktxStream* stream() const
    {
        return _stream.get();
    }

    inline std::streambuf* streambuf() const
    {
        return _streambuf.get();
    }

    inline std::ios::openmode seek_mode() const
    {
        return _seek_mode;
    }

    inline void seek_mode(std::ios::openmode newmode)
    {
        _seek_mode = newmode;
    }

    inline bool destructed() const
    {
        return _destructed;
    }

protected:
    // C++ streambuf overrides

    // ktxStream vtable implementations

    inline static StreambufStream* parent(ktxStream *str)
    {
        return reinterpret_cast<StreambufStream*>(str->data.custom_ptr.address);
    }

    static KTX_error_code read(ktxStream* str, void* dst, ktx_size_t count)
    {
        auto self = parent(str);
        if (count == 0)
        {
            return KTX_SUCCESS;
        }
        std::cerr << "\t  read: " << count << 'B' << std::endl;

        const auto stdcount = std::streamsize(count);
        const std::streamsize nread = self->_streambuf->sgetn(reinterpret_cast<char*>(dst), stdcount);
        return (nread == stdcount) ? KTX_SUCCESS : KTX_FILE_UNEXPECTED_EOF;
    }

    static KTX_error_code skip(ktxStream* str, ktx_size_t count)
    {
        auto self = parent(str);
        if (count == 0)
        {
            return KTX_SUCCESS;
        }
        std::cerr << "\t  skip: " << count << 'B' << std::endl;

        const std::streampos curpos = self->_streambuf->pubseekoff(0, std::ios::cur, self->_seek_mode);
        const std::streampos newpos = self->_streambuf->pubseekoff(std::streamoff(count), std::ios::cur, self->_seek_mode);
        return (curpos > newpos) ? KTX_SUCCESS : KTX_FILE_SEEK_ERROR;
    }

    static KTX_error_code write(ktxStream* str, const void* src, ktx_size_t size, ktx_size_t count)
    {
        auto self = parent(str);
        if (size == 0 || count == 0)
        {
            return KTX_SUCCESS;
        }
        std::cerr << "\t write: " << count << "*" << size << "B" << std::endl;

        const auto ntotal = std::streamsize(size * count);
        const std::streamsize nput = self->_streambuf->sputn(reinterpret_cast<const char*>(src), ntotal);
        return (nput == ntotal) ? KTX_SUCCESS : KTX_FILE_WRITE_ERROR;
    }

    static KTX_error_code getpos(ktxStream* str, ktx_off_t *offset)
    {
        auto self = parent(str);
        *offset = ktx_off_t(self->_streambuf->pubseekoff(0, std::ios::cur, self->_seek_mode));
        std::cerr << "\tgetpos: " << *offset << std::endl;
        return KTX_SUCCESS;
    }

    static KTX_error_code setpos(ktxStream* str, ktx_off_t offset)
    {
        auto self = parent(str);
        const auto newpos = std::streamoff(offset);
        const std::streampos setpos = self->_streambuf->pubseekoff(newpos, std::ios::beg, self->_seek_mode);
        std::cerr << "\tsetpos: " << offset << std::endl;
        return (setpos == newpos) ? KTX_SUCCESS : KTX_FILE_SEEK_ERROR;
    }

    static KTX_error_code getsize(ktxStream* str, ktx_size_t* size)
    {
        auto self = parent(str);
        const std::streampos oldpos = self->_streambuf->pubseekoff(0, std::ios::cur, self->_seek_mode);
        *size = ktx_size_t(self->_streambuf->pubseekoff(0, std::ios::end));
        const std::streampos newpos = self->_streambuf->pubseekoff(oldpos, std::ios::beg, self->_seek_mode);
        std::cerr << "\t  size: " << *size << 'B' << std::endl;
        return (oldpos == newpos) ? KTX_SUCCESS : KTX_FILE_SEEK_ERROR;
    }

    static void destruct(ktxStream* str)
    {
        auto self = parent(str);
        self->_destructed = true;
    }

    std::unique_ptr<std::streambuf> _streambuf;
    std::ios::openmode _seek_mode;
    std::unique_ptr<ktxStream> _stream;
    bool _destructed;
};

class ktxStreamTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _ktx1Streambuf = testImageFilebuf(SAMPLE_KTX1);
        ASSERT_TRUE(_ktx1Streambuf) << "Could not load sample KTX1";

        _ktx2Streambuf = testImageFilebuf(SAMPLE_KTX2);
        ASSERT_TRUE(_ktx2Streambuf) << "Could not load sample KTX2";
    }

    void TearDown() override
    {
        _ktx1Streambuf.reset();
        _ktx2Streambuf.reset();
    }

    std::unique_ptr<std::streambuf> _ktx1Streambuf;
    std::unique_ptr<std::streambuf> _ktx2Streambuf;
};

/// A RAIIfied ktxTexture.
template <typename T>
class KtxTexture final
{
public:
    KtxTexture(std::nullptr_t null = nullptr)
        : _handle{nullptr}
    {
        (void)null;
    }

    KtxTexture(T* handle)
        : _handle{handle}
    {
    }

    KtxTexture(const KtxTexture&) = delete;
    KtxTexture &operator=(const KtxTexture&) = delete;

    KtxTexture(KtxTexture&& toMove)
        : _handle{toMove._handle}
    {
        toMove._handle = nullptr;
    }

    KtxTexture &operator=(KtxTexture&& toMove)
    {
        _handle = toMove._handle;
        toMove._handle = nullptr;
        return *this;
    }

    ~KtxTexture()
    {
        if (_handle)
        {
            ktxTexture_Destroy(handle<ktxTexture>()); _handle = nullptr;
        }
    }

    template <typename U = T>
    inline U* handle() const
    {
        return reinterpret_cast<U*>(_handle);
    }

    template <typename U = T>
    inline U** pHandle()
    {
        return reinterpret_cast<U**>(&_handle);
    }

    inline operator T*() const
    {
        return _handle;
    }

private:
    T* _handle;
};

/// Expects two textures to be equal in content (but not necessarily be the same texture).
bool expectSameTextureContent(const ktxTexture* tex1, const ktxTexture* tex2)
{
    bool ok = true;
#define EXPECT_EQ_OK(val1, val2) \
    ok = ok && (val1) == (val2); \
    EXPECT_EQ(val1, val2)

    EXPECT_EQ_OK(tex1->classId, tex2->classId) << "Mismatched texture type (KTX1 or KTX2)";

    EXPECT_EQ_OK(tex1->isArray, tex2->isArray) << "Both textures should [not] be array textures";
    EXPECT_EQ_OK(tex1->isCubemap, tex2->isCubemap) << "Both textures should [not] be cubemap [arrays]";
    EXPECT_EQ_OK(tex1->isCompressed, tex2->isCompressed) << "Both textures should [not] be compressed";

    EXPECT_EQ_OK(tex1->baseWidth, tex2->baseWidth) << "Mismatched base width";
    EXPECT_EQ_OK(tex1->baseHeight, tex2->baseHeight) << "Mismatched base height";
    EXPECT_EQ_OK(tex1->baseDepth, tex2->baseDepth) << "Mismatched base depth";
    EXPECT_EQ_OK(tex1->numDimensions, tex2->numDimensions) << "Mismatched # of texture dimensions";
    EXPECT_EQ_OK(tex1->numLevels, tex2->numLevels) << "Mismatched # of texture levels";
    EXPECT_EQ_OK(tex1->numLayers, tex2->numLayers) << "Mismatched # of texture layers";
    EXPECT_EQ_OK(tex1->numFaces, tex2->numFaces) << "Mismatched # of texture faces";

    EXPECT_EQ_OK(tex1->orientation.x, tex2->orientation.x) << "Mismatched X orientation";
    EXPECT_EQ_OK(tex1->orientation.y, tex2->orientation.y) << "Mismatched Y orientation";
    EXPECT_EQ_OK(tex1->orientation.z, tex2->orientation.z) << "Mismatched Z orientation";

    EXPECT_EQ_OK(tex1->kvDataLen, tex2->kvDataLen) << "Mismatched K/V data length";
    auto* e1 = ktxHashList_Next(tex1->kvDataHead);
    auto* e2 = ktxHashList_Next(tex2->kvDataHead);
    for(size_t i = 0; e1 && e2; e1 = ktxHashList_Next(e1), e2 = ktxHashList_Next(e2), i++)
    {
        unsigned int len1 = 0, len2 = 0;
        {
            char *key1 = nullptr, *key2 = nullptr;
            (void)ktxHashListEntry_GetKey(e1, &len1, &key1);
            (void)ktxHashListEntry_GetKey(e2, &len2, &key2);
            EXPECT_EQ_OK(strncmp(key1, key2, std::min(len1, len2)), 0) << i << "th key mismatch";
        }
        {
            void *val1 = nullptr, *val2 = nullptr;
            (void)ktxHashListEntry_GetValue(e1, &len1, &val1);
            (void)ktxHashListEntry_GetValue(e2, &len2, &val2);
            EXPECT_EQ_OK(memcmp(val1, val2, std::min(len1, len2)), 0) << i << "th value mismatch";
        }
    }

    EXPECT_EQ_OK(tex1->dataSize, tex2->dataSize) << "Mismatched image data size";
    EXPECT_EQ_OK(memcmp(tex1->pData, tex2->pData, std::min(tex1->dataSize, tex2->dataSize)), 0) << "Mismatched image data";

#undef EXPECT_EQ_OK
    return ok;
}

// --- Tests ---

TEST_F(ktxStreamTest, CanCreateKtx1FromCppStream)
{
    StreambufStream ktx1Stream{std::move(_ktx1Streambuf), std::ios::in};
    KtxTexture<ktxTexture1> texture1;

    KTX_error_code err = ktxTexture1_CreateFromStream(ktx1Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                                      texture1.pHandle());
    EXPECT_EQ(err, KTX_SUCCESS) << "Failed to create KTX1 from C++ stream: " << ktxErrorString(err);
    ASSERT_NE(texture1, nullptr) << "Newly-created KTX1 is null";
    EXPECT_TRUE(ktx1Stream.destructed()) << "ktxStream should have been destructed (LOAD_IMAGE_DATA_BIT set)";
}

TEST_F(ktxStreamTest, CanCreateKtx2FromCppStream)
{
    StreambufStream ktx2Stream{std::move(_ktx2Streambuf), std::ios::in};
    KtxTexture<ktxTexture2> texture2;

    KTX_error_code err = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture2.pHandle());
    EXPECT_EQ(err, KTX_SUCCESS) << "Failed to create KTX2 from C++ stream: " << ktxErrorString(err);
    ASSERT_NE(texture2, nullptr) << "Newly-created KTX2 is null";
    EXPECT_TRUE(ktx2Stream.destructed()) << "ktxStream should have been destructed (LOAD_IMAGE_DATA_BIT set)";
}

TEST_F(ktxStreamTest, CanCreateAutoKtxFromCppStream)
{
    StreambufStream ktxStream{std::move(_ktx2Streambuf), std::ios::in}; // Or could use the KTx1, no difference
    KtxTexture<ktxTexture> texture;

    KTX_error_code err = ktxTexture_CreateFromStream(ktxStream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
    EXPECT_EQ(err, KTX_SUCCESS) << "Failed to create auto-detected KTX from C++ stream: " << ktxErrorString(err);
    ASSERT_NE(texture, nullptr) << "Newly-created auto-detected KTX is null";
    EXPECT_TRUE(ktxStream.destructed()) << "ktxStream should have been destructed (LOAD_IMAGE_DATA_BIT set)";
}

TEST_F(ktxStreamTest, CanWriteKtx1AsKtx2ToCppStream)
{
    KTX_error_code err{KTX_INVALID_VALUE};
    auto dstStreambuf = std::make_unique<std::stringbuf>();
    StreambufStream dstStream{std::move(dstStreambuf)};

    KtxTexture<ktxTexture1> srcTexture1{nullptr};
    KtxTexture<ktxTexture2> dstTexture2{nullptr};

    {
        std::cerr << "Loading KTX1 from file" << std::endl;

        StreambufStream srcStream{std::move(_ktx1Streambuf), std::ios::in};
        err = ktxTexture1_CreateFromStream(srcStream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, srcTexture1.pHandle());
        EXPECT_EQ(err, KTX_SUCCESS) << "Failed to load source KTX1 from C++ stream: " << ktxErrorString(err);
        ASSERT_NE(srcTexture1, nullptr) << "Source KTX1 is null";
        EXPECT_TRUE(srcStream.destructed()) << "ktxStream should have been destructed (LOAD_IMAGE_DATA_BIT set)";
    }
    {
        std::cerr << "Converting KTX1 -> KTX2" << std::endl;

        // We're about to write to `dstStream`
        dstStream.seek_mode(std::ios::out);

        err = ktxTexture1_WriteKTX2ToStream(srcTexture1, dstStream.stream());
        EXPECT_EQ(err, KTX_SUCCESS) << "Failed to convert KTX1 -> KTX2 to C++ stream: " << ktxErrorString(err);
    }
    {
        std::cerr << "Loading the converted KTX2" << std::endl;

        // Rewind dstStream and set it up for reading
        dstStream.streambuf()->pubseekpos(0, std::ios::in);
        dstStream.seek_mode(std::ios::in);

        err = ktxTexture2_CreateFromStream(dstStream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, dstTexture2.pHandle());
        EXPECT_EQ(err, KTX_SUCCESS) << "Failed to load converted KTX2 from C++ stream: " << ktxErrorString(err);
        ASSERT_NE(dstTexture2, nullptr) << "Destination KTX2 is null";
    }
}

TEST_F(ktxStreamTest, CanWriteKtx2ToCppStream)
{
    KTX_error_code err{KTX_INVALID_VALUE};
    auto dstStreambuf = std::make_unique<std::stringbuf>();
    StreambufStream dstStream{std::move(dstStreambuf)};

    KtxTexture<ktxTexture2> srcTexture2;
    KtxTexture<ktxTexture2> dstTexture2;

    {
        std::cerr << "Loading KTX2 from file" << std::endl;

        StreambufStream srcStream{std::move(_ktx2Streambuf), std::ios::in};
        err = ktxTexture2_CreateFromStream(srcStream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, srcTexture2.pHandle());
        EXPECT_EQ(err, KTX_SUCCESS) << "Failed to load source KTX2 from C++ stream: " << ktxErrorString(err);
        ASSERT_NE(srcTexture2, nullptr) << "Source KTX2 is null";
        EXPECT_TRUE(srcStream.destructed()) << "ktxStream should have been destructed (LOAD_IMAGE_DATA_BIT set)";
    }
    {
        std::cerr << "Writing KTX2 -> copied KTX2" << std::endl;

        // We're about to write to `dstStream`
        dstStream.seek_mode(std::ios::out);

        err = ktxTexture_WriteToStream(srcTexture2.handle<ktxTexture>(), dstStream.stream());
        EXPECT_EQ(err, KTX_SUCCESS) << "Failed to convert KTX1 -> KTX2 to C++ stream: " << ktxErrorString(err);
    }
    {
        std::cerr << "Loading the converted KTX2" << std::endl;

        // Rewind dstStream and set it up for reading
        dstStream.streambuf()->pubseekpos(0, std::ios::in);
        dstStream.seek_mode(std::ios::in);

        err = ktxTexture2_CreateFromStream(dstStream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, dstTexture2.pHandle());
        EXPECT_EQ(err, KTX_SUCCESS) << "Failed to load converted KTX2 from C++ stream: " << ktxErrorString(err);
        ASSERT_NE(dstTexture2, nullptr) << "Destination KTX2 is null";
    }

    // Should be a clone of the same texture
    expectSameTextureContent(srcTexture2.handle<ktxTexture>(), dstTexture2.handle<ktxTexture>());
}

}  // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    if (!::testing::FLAGS_gtest_list_tests)
    {
        if (argc != 2)
        {
            std::cerr << "Usage: " << argv[0] << " <test images path>\n";
            return -1;
        }

        testImagesPath = argv[1];

        struct stat info;
        if (stat(testImagesPath.data(), &info) != 0)
        {
            std::cerr << "Cannot access " << testImagesPath << '\n';
            return -2;
        }
        else if (!(info.st_mode & S_IFDIR))
        {
            std::cerr << testImagesPath << "is not a valid directory\n";
            return -3;
        }
    }

    return RUN_ALL_TESTS();
}
