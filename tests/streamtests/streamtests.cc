// Copyright 2021 Paolo Jovon, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory>
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
    filebuf->open(imagePath, std::ios::in);
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
        std::cerr << "\t  read: " << count << 'B' << std::endl;

        auto self = parent(str);
        const std::streamsize nread = self->_streambuf->sgetn(reinterpret_cast<char*>(dst), std::streamsize(count));
        return (nread > 0) ? KTX_SUCCESS : KTX_FILE_UNEXPECTED_EOF;
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

// --- Tests ---

TEST_F(ktxStreamTest, CanCreateKtx1FromCppStream)
{
    StreambufStream ktx1Stream{std::move(_ktx1Streambuf), std::ios::in};
    ktxTexture1 *texture1{nullptr};

    KTX_error_code err = ktxTexture1_CreateFromStream(ktx1Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture1);
    EXPECT_EQ(err, KTX_SUCCESS) << "Failed to create KTX1 from C++ stream: " << ktxErrorString(err);
    ASSERT_NE(texture1, nullptr) << "Newly-created KTX1 is null";
    EXPECT_TRUE(ktx1Stream.destructed()) << "ktxStream should have been destructed (LOAD_IMAGE_DATA_BIT set)";

    ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(texture1));
}

TEST_F(ktxStreamTest, CanCreateKtx2FromCppStream)
{
    StreambufStream ktx2Stream{std::move(_ktx2Streambuf), std::ios::in};
    ktxTexture2 *texture2{nullptr};

    KTX_error_code err = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture2);
    EXPECT_EQ(err, KTX_SUCCESS) << "Failed to create KTX2 from C++ stream: " << ktxErrorString(err);
    ASSERT_NE(texture2, nullptr) << "Newly-created KTX2 is null";
    EXPECT_TRUE(ktx2Stream.destructed()) << "ktxStream should have been destructed (LOAD_IMAGE_DATA_BIT set)";

    ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(texture2));
}

TEST_F(ktxStreamTest, CanCreateAutoKtxFromCppStream)
{
    StreambufStream ktxStream{std::move(_ktx2Streambuf), std::ios::in}; // Or could use the KTx1, no difference
    ktxTexture *texture{nullptr};

    KTX_error_code err = ktxTexture_CreateFromStream(ktxStream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
    EXPECT_EQ(err, KTX_SUCCESS) << "Failed to create auto-detected KTX from C++ stream: " << ktxErrorString(err);
    ASSERT_NE(texture, nullptr) << "Newly-created auto-detected KTX is null";
    EXPECT_TRUE(ktxStream.destructed()) << "ktxStream should have been destructed (LOAD_IMAGE_DATA_BIT set)";

    ktxTexture_Destroy(texture);
}

TEST_F(ktxStreamTest, CanWriteKtx1AsKtx2ToCppStream)
{
    KTX_error_code err{KTX_INVALID_VALUE};
    auto dstStreambuf = std::make_unique<std::stringbuf>();
    StreambufStream dstStream{std::move(dstStreambuf)};

    {
        ktxTexture1 *srcTexture{nullptr};

        std::cerr << "Loading KTX1 from file" << std::endl;
        StreambufStream srcStream{std::move(_ktx1Streambuf), std::ios::in};
        err = ktxTexture1_CreateFromStream(srcStream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &srcTexture);
        EXPECT_EQ(err, KTX_SUCCESS) << "Failed to load source KTX1 from C++ stream: " << ktxErrorString(err);
        ASSERT_NE(srcTexture, nullptr) << "Source KTX1 is null";
        EXPECT_TRUE(srcStream.destructed()) << "ktxStream should have been destructed (LOAD_IMAGE_DATA_BIT set)";

        // We're about to write to `dstStream`
        dstStream.seek_mode(std::ios::out);

        std::cerr << "Converting KTX1 -> KTX2" << std::endl;
        err = ktxTexture1_WriteKTX2ToStream(srcTexture, dstStream.stream());
        EXPECT_EQ(err, KTX_SUCCESS) << "Failed to convert KTX1 -> KTX2 to C++ stream: " << ktxErrorString(err);

        // Not needed anymore - it already got copied to KTX2 format into dstStream
        ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(srcTexture)); srcTexture = nullptr;
    }
 
    // Rewind dstStream and set it up for reading
    dstStream.streambuf()->pubseekpos(0, std::ios::in);
    dstStream.seek_mode(std::ios::in);

    {
        ktxTexture2 *dstTexture{nullptr};

        std::cerr << "Loading the converted KTX2" << std::endl;
        err = ktxTexture2_CreateFromStream(dstStream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &dstTexture);
        EXPECT_EQ(err, KTX_SUCCESS) << "Failed to load converted KTX2 from C++ stream: " << ktxErrorString(err);
        ASSERT_NE(dstTexture, nullptr) << "Destination KTX2 is null";

        ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(dstTexture));
    }
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
