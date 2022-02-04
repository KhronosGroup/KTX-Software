/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2021 Paolo Jovon.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief A ktxStream that wraps a C++ std::streambuf.
 *
 * Modified from Paolo's original to remove unique pointer to the streambuf. As our use it to pass the
 * streambuf from an istream or istringstream we can't be attempting to delete the stringbuf. Also
 * the destructor now destructs the stream, if not already destructed.
 *
 * @author Paolo Jovon
 * @author Mark Callow
 */

//#include <algorithm>
#include <iostream>
#include <memory>
//#include <utility>
//#include <vector>
#include <ktx.h>

static std::ostream cnull(0);
static std::ostream& logstream = cnull;
//static std::ostream logstream = std::cerr;

class StreambufStream
{
    // Doubt this will ever get triggered
    static_assert(sizeof(char) == sizeof(uint8_t),
                  "Chars are != 1 byte in this platform");

public:
    StreambufStream(std::streambuf* streambuf,
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
        // ktxStream will have been destructed if
        // KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT was passed to the
        // ktxTexture?_CreateFromStream function. Otherwise do it.
        if (!_destructed)
            stream()->destruct(stream());
    }

    inline ktxStream* stream() const
    {
        return _stream.get();
    }

    inline std::streambuf* streambuf() const
    {
        return _streambuf;
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
        logstream << "\t  read: " << count << 'B' << std::endl;

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
        logstream << "\t  skip: " << count << 'B' << std::endl;

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
        logstream << "\t write: " << count << "*" << size << "B" << std::endl;

        const auto ntotal = std::streamsize(size * count);
        const std::streamsize nput = self->_streambuf->sputn(reinterpret_cast<const char*>(src), ntotal);
        return (nput == ntotal) ? KTX_SUCCESS : KTX_FILE_WRITE_ERROR;
    }

    static KTX_error_code getpos(ktxStream* str, ktx_off_t *offset)
    {
        auto self = parent(str);
        *offset = ktx_off_t(self->_streambuf->pubseekoff(0, std::ios::cur, self->_seek_mode));
        logstream << "\tgetpos: " << *offset << std::endl;
        return KTX_SUCCESS;
    }

    static KTX_error_code setpos(ktxStream* str, ktx_off_t offset)
    {
        auto self = parent(str);
        const auto newpos = std::streamoff(offset);
        const std::streampos setpos = self->_streambuf->pubseekoff(newpos, std::ios::beg, self->_seek_mode);
        logstream << "\tsetpos: " << offset << std::endl;
        return (setpos == newpos) ? KTX_SUCCESS : KTX_FILE_SEEK_ERROR;
    }

    static KTX_error_code getsize(ktxStream* str, ktx_size_t* size)
    {
        auto self = parent(str);
        const std::streampos oldpos = self->_streambuf->pubseekoff(0, std::ios::cur, self->_seek_mode);
        *size = ktx_size_t(self->_streambuf->pubseekoff(0, std::ios::end));
        const std::streampos newpos = self->_streambuf->pubseekoff(oldpos, std::ios::beg, self->_seek_mode);
        logstream << "\t  size: " << *size << 'B' << std::endl;
        return (oldpos == newpos) ? KTX_SUCCESS : KTX_FILE_SEEK_ERROR;
    }

    static void destruct(ktxStream* str)
    {
        auto self = parent(str);
        self->_destructed = true;
    }

    std::streambuf* _streambuf;
    std::ios::openmode _seek_mode;
    std::unique_ptr<ktxStream> _stream;
    bool _destructed;
};

