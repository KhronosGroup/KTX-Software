// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2023-2024 The Khronos Group Inc.
// Copyright 2023-2024 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file
//!
//! @brief Internal Image Span container and iterator classes
//!

#pragma once

#include <iterator>
#include "imagecodec.hpp"

class ImageSpan {
public:
    template <typename PTR = uint8_t*>
    class TexelBlockPtr {
    public:
        using pointer = PTR;
        TexelBlockPtr(PTR ptr, const ImageSpan& span) noexcept : ptr(ptr), span(span) {}

        uint32_t getPackedElement(uint32_t index) const { return span.imageCodec().getPackedElement(ptr, index); }
        glm::uvec4 decodeUINT() const { return span.imageCodec().decodeUINT(ptr); }
        glm::ivec4 decodeSINT() const { return span.imageCodec().decodeSINT(ptr); }
        glm::vec4 decodeFLOAT() const { return span.imageCodec().decodeFLOAT(ptr); }

        const ImageCodec& imageCodec() const { return span.codec; }

        constexpr bool isBlockCompressed() const { return span.imageCodec().isBlockCompressed(); }
        constexpr bool isPacked() const { return span.imageCodec().isPacked(); }
        constexpr bool isFloat() const { return span.imageCodec().isFloat(); }
        constexpr bool isFloatHalf() const { return span.imageCodec().isFloatHalf(); }
        constexpr bool isSigned() const { return span.imageCodec().isSigned(); }
        constexpr bool isNormalized() const { return span.imageCodec().isNormalized(); }
        constexpr bool canDecodeUINT() const { return span.imageCodec().canDecodeUINT(); }
        constexpr bool canDecodeSINT() const { return span.imageCodec().canDecodeSINT(); }
        constexpr bool canDecodeFLOAT() const { return span.imageCodec().canDecodeFLOAT(); }

        glm::uvec4 getTexelBlockDimensions() const { return span.imageCodec().getTexelBlockDimensions(); }
        constexpr uint32_t getPackedElementByteSize() const { return span.imageCodec().getPackedElementByteSize(); }
        constexpr uint32_t getPackedElementCount() const { return span.imageCodec().getPackedElementCount(); }
        constexpr uint32_t getTexelBlockByteSize() const { return span.imageCodec().getTexelBlockByteSize(); }
        constexpr uint32_t getChannelCount() const { return span.imageCodec().getChannelCount(); }

        std::ptrdiff_t getTexelBlockByteOffset() const {
            return ptr - span.data();
        }

        glm::uvec4 getTexelBlockLocation() const {
            const std::ptrdiff_t blockPitch = span.imageCodec().getTexelBlockByteSize();
            const std::ptrdiff_t rowPitch = span.getTexelBlockWidth() * blockPitch;
            const std::ptrdiff_t slicePitch = span.getTexelBlockHeight() * rowPitch;
            glm::uvec4 loc;
            auto diff = getTexelBlockByteOffset();
            loc.w = 0;
            loc.z = static_cast<uint32_t>(diff / slicePitch);
            diff = diff % slicePitch;
            loc.y = static_cast<uint32_t>(diff / rowPitch);
            diff = diff % rowPitch;
            loc.x = static_cast<uint32_t>(diff / blockPitch);
            return loc;
        }

        glm::uvec4 getPixelLocation() const {
            return getTexelBlockLocation() * span.imageCodec().getTexelBlockDimensions();
        }

    private:
        PTR ptr;
        const ImageSpan& span;
    };

    template <typename TBPTR = TexelBlockPtr<uint8_t*>, bool REVERSE = false>
    class Iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = TBPTR;
        using pointer = TBPTR;
        using reference = TBPTR;

        Iterator& operator+=(difference_type rhs) noexcept { ptr = advance(rhs); return *this; }
        Iterator& operator-=(difference_type rhs) noexcept { ptr = advance(-rhs); return *this; }
        reference operator*() const { return reference(ptr, span); }
        pointer operator->() const { return pointer(ptr, span); }
        reference operator[](difference_type rhs) const { return *advance(rhs); }
        Iterator& operator++() noexcept { ptr = advance(1); return *this; }
        Iterator& operator--() noexcept { ptr = advance(-1); return *this; }
        Iterator operator++(int) const noexcept { return Iterator(span, advance(1), stride); }
        Iterator operator--(int) const noexcept { return Iterator(span, advance(-1), stride); }
        difference_type operator-(const Iterator& rhs) const noexcept { return diff(rhs.ptr); }
        Iterator operator+(difference_type rhs) const noexcept { return Iterator(span, advance(rhs), stride); }
        Iterator operator-(difference_type rhs) const noexcept { return Iterator(span, advance(-rhs), stride); }
        friend Iterator operator+(difference_type lhs, const Iterator& rhs) { return Iterator(rhs.span, rhs.advance(lhs), rhs.stride); }
        friend Iterator operator-(difference_type lhs, const Iterator& rhs) { return Iterator(rhs.span, rhs.advance(-lhs), rhs.stride); }

        bool operator==(const Iterator& rhs) const noexcept { return (ptr == rhs.ptr) != REVERSE; }
        bool operator!=(const Iterator& rhs) const noexcept { return (ptr != rhs.ptr) != REVERSE; }
        bool operator>(const Iterator& rhs) const noexcept { return (ptr > rhs.ptr) != REVERSE; }
        bool operator<(const Iterator& rhs) const noexcept { return (ptr < rhs.ptr) != REVERSE; }
        bool operator>=(const Iterator& rhs) const noexcept { return (ptr >= rhs.ptr) != REVERSE; }
        bool operator<=(const Iterator& rhs) const noexcept { return (ptr <= rhs.ptr) != REVERSE; }

    private:
        const ImageSpan& span;
        typename TBPTR::pointer ptr;
        const uint32_t stride;

        Iterator(const ImageSpan& span, typename TBPTR::pointer ptr, uint32_t stride) noexcept
            : span(span), ptr(ptr), stride(stride) {}

        typename TBPTR::pointer advance(difference_type diff) const noexcept {
            return ptr + (REVERSE ? -1 : +1) * diff * stride;
        }

        difference_type diff(pointer other) const noexcept {
            return (REVERSE ? -1 : +1) * (ptr - other) / stride;
        }

        friend class ImageSpan;
    };

    using value_type = TexelBlockPtr<uint8_t*>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = TexelBlockPtr<uint8_t*>;
    using const_reference = TexelBlockPtr<const uint8_t*>;
    using pointer = TexelBlockPtr<uint8_t*>;
    using const_pointer = TexelBlockPtr<const uint8_t*>;
    using iterator = Iterator<TexelBlockPtr<uint8_t*>, false>;
    using const_iterator = Iterator<TexelBlockPtr<const uint8_t*>, false>;
    using reverse_iterator = Iterator<TexelBlockPtr<uint8_t*>, true>;
    using const_reverse_iterator = Iterator<TexelBlockPtr<const uint8_t*>, true>;

    ImageSpan(uint32_t width, uint32_t height, uint32_t depth, void* pixels, const ImageCodec& imageCodec)
      : texelBlockWidth((width + imageCodec.getTexelBlockDimensions()[0] - 1) / imageCodec.getTexelBlockDimensions()[0]),
        texelBlockHeight((height + imageCodec.getTexelBlockDimensions()[1] - 1) / imageCodec.getTexelBlockDimensions()[1]),
        texelBlockDepth((depth + imageCodec.getTexelBlockDimensions()[2] - 1) / imageCodec.getTexelBlockDimensions()[2]),
        pixels(reinterpret_cast<uint8_t*>(pixels)), codec(imageCodec) {}

    const ImageCodec& imageCodec() const { return codec; }

    reference at(uint32_t blockX, uint32_t blockY, uint32_t blockZ) {
        return reference(pixels + texelBlockByteOffset(blockX, blockY, blockZ), *this);
    }
    const_reference at(uint32_t blockX, uint32_t blockY, uint32_t blockZ) const {
        return const_reference(pixels + texelBlockByteOffset(blockX, blockY, blockZ), *this);
    }
    iterator begin() noexcept { return iterator(*this, pixels, codec.getTexelBlockByteSize()); }
    const_iterator begin() const noexcept { return const_iterator(*this, pixels, codec.getTexelBlockByteSize()); }
    const_iterator cbegin() const noexcept { return const_iterator(*this, pixels, codec.getTexelBlockByteSize()); }
    iterator end() noexcept { return iterator(*this, pixels, codec.getTexelBlockByteSize()) + size(); }
    const_iterator end() const noexcept { return const_iterator(*this, pixels, codec.getTexelBlockByteSize()) + size(); }
    const_iterator cend() const noexcept { return const_iterator(*this, pixels, codec.getTexelBlockByteSize()) + size(); }
    reverse_iterator rbegin() noexcept { return reverse_iterator(*this, pixels, codec.getTexelBlockByteSize()) - size() + 1; }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(*this, pixels, codec.getTexelBlockByteSize()) - size() + 1; }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(*this, pixels, codec.getTexelBlockByteSize()) - size() + 1; }
    reverse_iterator rend() noexcept { return reverse_iterator(*this, pixels , codec.getTexelBlockByteSize()) + 1; }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(*this, pixels, codec.getTexelBlockByteSize()) + 1; }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(*this, pixels, codec.getTexelBlockByteSize()) + 1; }
    uint8_t* data() noexcept { return pixels; }
    const uint8_t* data() const noexcept { return pixels; }
    constexpr size_type size() const noexcept { return texelBlockWidth * texelBlockHeight; }
    constexpr std::ptrdiff_t byteSize() const { return size() * codec.getTexelBlockByteSize(); }
    constexpr uint32_t getTexelBlockWidth() const { return texelBlockWidth; }
    constexpr uint32_t getTexelBlockHeight() const { return texelBlockHeight; }
    constexpr uint32_t getTexelBlockDepth() const { return texelBlockDepth; }

private:
    const uint32_t texelBlockWidth;
    const uint32_t texelBlockHeight;
    const uint32_t texelBlockDepth;
    uint8_t* const pixels;
    const ImageCodec& codec;

    constexpr std::ptrdiff_t texelBlockByteOffset(uint32_t blockX, uint32_t blockY, uint32_t blockZ) const {
        return (blockX + blockY * texelBlockWidth + blockZ * texelBlockWidth * texelBlockHeight) * codec.getTexelBlockByteSize();
    }
};
