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
 * Modified from Paolo's original to make it a template so that the  reference to std::streambuf can
 * be passed either as a `unique_ptr` or a regular pointer.  As our use is to pass the streambuf from
 * an istream, potentially that from std::cin, or an istringstream that is not being not allocated by `new`
 * we can't use a `unique_ptr`. i.e. can't attempt to delete the stringbuf when done. Also the
 * destructor now destructs the stream, if not already destructed.
 *
 * @author Paolo Jovon
 * @author Mark Callow
 */

#include <iostream>
#include <memory>
#include <ktx.h>

static std::ostream cnull(0);
static std::ostream& logstream = cnull;
//static std::ostream logstream = std::cerr;

/// @brief Template for a ktxStream that wraps a C++ std::streambuf.
///
/// The template supports 2 ways of referencing the underlying std::streambuf:
/// as a @c unique_ptr or as a regular pointer. For the former case, the
/// @c unique_ptr std::streambuf and StreambufStream should be created
/// like this:
/// @code
///     // Can use stringbuf or any class derived from it.
///     auto filebuf = std::make_unique<std::filebuf>();
///     StreambufStream<std::unique_ptr<std::streambuf>> ktxStream(
///                                                        std::move(filebuf),
///                                                        ios::in);
/// @endcode
///
///
template <typename T>
class StreambufStream
{
    // Doubt this will ever get triggered
    static_assert(sizeof(char) == sizeof(uint8_t),
                  "Chars are != 1 byte in this platform");

public:
   StreambufStream(T streambuf,
                   std::ios::openmode seek_mode = std::ios::in | std::ios::out)
        : _streambuf{std::move(streambuf)}
        , _seek_mode{seek_mode}
        , _stream{std::make_unique<ktxStream>()}
    {
        initialize_stream();
    }

    StreambufStream(const StreambufStream&) = delete;
    StreambufStream &operator=(const StreambufStream&) = delete;

    StreambufStream(StreambufStream&&) = delete;
    StreambufStream &operator=(StreambufStream&&) = delete;

    inline ktxStream* stream() const
    {
        return _stream.get();
    }

    // Implementation depends on whether a std::unique_ptr or a raw pointer is
    // passed.
    std::streambuf* streambuf() const {
        if constexpr (std::is_pointer<T>::value)
            return _streambuf;
        else
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

protected:
    void initialize_stream() {
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
        _stream->destruct = destruct_noop;
    }

    // C++ streambuf overrides

    // ktxStream vtable implementations

    inline static StreambufStream* parent(ktxStream *str)
    {
        assert(str->type == eStreamTypeCustom);
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
        ktx_off_t seekoffval =
              ktx_off_t(self->_streambuf->pubseekoff(0, std::ios::cur, self->_seek_mode));
        logstream << "\tgetpos: " << seekoffval << std::endl;
        if (seekoffval != -1) {
            *offset = seekoffval;
            return KTX_SUCCESS;
        } else {
            return KTX_FILE_ISPIPE; // Not seekable.
        }
    }

    static KTX_error_code setpos(ktxStream* str, ktx_off_t pos)
    {
        auto self = parent(str);
        const auto newpos = std::streamoff(pos);
        const std::streampos setpos =
                self->_streambuf->pubseekoff(newpos, std::ios::beg, self->_seek_mode);
        logstream << "\tsetpos: " << pos << std::endl;
        if (setpos == -1) {
            // Not seekable
            if (pos > str->readpos)
                return str->skip(str, pos - str->readpos);
            else
                return KTX_FILE_ISPIPE;
        }

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

    // NOOP function to be passed to ktxStream's destruct
    static void destruct_noop(ktxStream*) {}

    T _streambuf;
    std::ios::openmode _seek_mode;

    // Note that ktxTexture?_CreateFromStream makes a copy of _stream, the
    // stream structure passed as its pStream parameter. The static stream
    // functions above will receive a pointer to that copy not to this _stream.
    std::unique_ptr<ktxStream> _stream;
};
