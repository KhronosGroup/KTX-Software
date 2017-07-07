/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
Copyright (c) 2010 The Khronos Group Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
unaltered in all copies or substantial portions of the Materials.
Any additions, deletions, or changes to the original source files
must be clearly indicated in accompanying documentation.

If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the
work of the Khronos Group".

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#ifndef KTXTEXTURE_H
#define KTXTEXTURE_H

#include "ktx.h"

/* XXX This should be moved into ktx.h */

/* This is from gli */

struct Texture {
        std::shared_ptr<storage_type> Storage;
        target_type Target;
        format_type Format;
        size_type BaseLayer;
        size_type MaxLayer;
        size_type BaseFace;
        size_type MaxFace;
        size_type BaseLevel;
        size_type MaxLevel;
        swizzles_type Swizzles;
};

typedef struct KTX_FormatSize {
	GlFormatSizeFlags	flags;
	unsigned int		paletteSizeInBits;
	unsigned int		blockSizeInBits;
	unsigned int		blockWidth;			// in texels
	unsigned int		blockHeight;		// in texels
	unsigned int		blockDepth;			// in texels
} KTX_FormatSize;

struct KTX_texture {
    ktx_uint32_t glType;
    ktx_uint32_t glTypeSize;
    ktx_uint32_t glFormat;
    ktx_uint32_t glInternalFormat;
    ktx_uint32_t glBaseInternalFormat;
    KTX_format_size formatSize;  /* Really want this to be opaque!! */
    KTX_dimensions dimensions;
    ktx_uint32_t dimension;  /* 1, 2 or 3 */
    ktx_uint32_t layers;
    ktx_uint32_t faces;
    ktx_uint32_t levels;
    ktx_bool_t   isArray;
    ktx_bool_t   isCubemap;
    ktx_bool_t   isCompressed;
    ktx_bool_t   isGenerateMipmaps;
};


/* XXX Load in constructor or have a load method? Since this is internal
   to the KTX library, I'm inclined to the latter. */

/* constructors */



/* functions */

        /// Return whether the texture instance is empty, no storage_type or description have been assigned to the instance.
        bool empty() const;

        /// Return the target of a texture instance.
        target_type target() const{return this->Target;}

        /// Return the texture instance format
        format_type format() const;

        swizzles_type swizzles() const;

        /// Return the base layer of the texture instance, effectively a memory offset in the actual texture storage_type to identify where to start reading the layers. 
        size_type base_layer() const;

        /// Return the max layer of the texture instance, effectively a memory offset to the beginning of the last layer in the actual texture storage_type that the texture instance can access. 
        size_type max_layer() const;

        /// Return max_layer() - base_layer() + 1
        size_type layers() const;

        /// Return the base face of the texture instance, effectively a memory offset in the actual texture storage_type to identify where to start reading the faces. 
        size_type base_face() const;

        /// Return the max face of the texture instance, effectively a memory offset to the beginning of the last face in the actual texture storage_type that the texture instance can access. 
        size_type max_face() const;

        /// Return max_face() - base_face() + 1
        size_type faces() const;

        /// Return the base level of the texture instance, effectively a memory offset in the actual texture storage_type to identify where to start reading the levels. 
        size_type base_level() const;

        /// Return the max level of the texture instance, effectively a memory offset to the beginning of the last level in the actual texture storage_type that the texture instance can access. 
        size_type max_level() const;

        /// Return max_level() - base_level() + 1.
        size_type levels() const;

        /// Return the size of a texture instance: width, height and depth.
        extent_type extent(size_type Level = 0) const;

        /// Return the memory size of a texture instance storage_type in bytes.
        size_type size() const;

        /// Return the number of blocks contained in a texture instance storage_type.
        /// genType size must match the block size conresponding to the texture format.
        template <typename genType>
        size_type size() const;

        /// Return the memory size of a specific level identified by Level.
        size_type size(size_type Level) const;

        /// Return the memory size of a specific level identified by Level.
        /// genType size must match the block size conresponding to the texture format.
        template <typename gen_type>
        size_type size(size_type Level) const;

        /// Return a pointer to the beginning of the texture instance data.
        void* data();

        /// Return a pointer of type genType which size must match the texture format block size
        template <typename gen_type>
        gen_type* data();

        /// Return a pointer to the beginning of the texture instance data.
        void const* data() const;

        /// Return a pointer of type genType which size must match the texture format block size
        template <typename gen_type>
        gen_type const* data() const;

        /// Return a pointer to the beginning of the texture instance data.
        void* data(size_type Layer, size_type Face, size_type Level);

        /// Return a pointer to the beginning of the texture instance data.
        void const* const data(size_type Layer, size_type Face, size_type Level) const;

        /// Return a pointer of type genType which size must match the texture format block size
        template <typename gen_type>
        gen_type* data(size_type Layer, size_type Face, size_type Level);

        /// Return a pointer of type genType which size must match the texture format block size
        template <typename gen_type>
        gen_type const* const data(size_type Layer, size_type Face, size_type Level) const;

        /// Clear the entire texture storage_linear with zeros
        void clear();

        /// Clear the entire texture storage_linear with Texel which type must match the texture storage_linear format block size
        /// If the type of gen_type doesn't match the type of the texture format, no conversion is performed and the data will be reinterpreted as if is was of the texture format. 
        template <typename gen_type>
        void clear(gen_type const& Texel);

        /// Clear a specific image of a texture.
        template <typename gen_type>
        void clear(size_type Layer, size_type Face, size_type Level, gen_type const& BlockData);

        /// Clear a subset of a specific image of a texture.
        template <typename gen_type>
        void clear(size_type Layer, size_type Face, size_type Level, extent_type const& TexelOffset, extent_type const& TexelExtent, gen_type const& BlockData);

        /// Copy a specific image of a texture 
        void copy(
            texture const& TextureSrc,
            size_t LayerSrc, size_t FaceSrc, size_t LevelSrc,
            size_t LayerDst, size_t FaceDst, size_t LevelDst);

        /// Copy a subset of a specific image of a texture 
        void copy(
            texture const& TextureSrc,
            size_t LayerSrc, size_t FaceSrc, size_t LevelSrc, extent_type const& OffsetSrc,
            size_t LayerDst, size_t FaceDst, size_t LevelDst, extent_type const& OffsetDst,
            extent_type const& Extent);

        /// Reorder the component in texture memory.
        template <typename gen_type>
        void swizzle(gli::swizzles const& Swizzles);

        /// Fetch a texel from a texture. The texture format must be uncompressed.
        template <typename gen_type>
        gen_type load(extent_type const & TexelCoord, size_type Layer, size_type Face, size_type Level) const;

        /// Write a texel to a texture. The texture format must be uncompressed.
        template <typename gen_type>
        void store(extent_type const& TexelCoord, size_type Layer, size_type Face, size_type Level, gen_type const& Texel);


/* struct cache is protected. */

    // Pre compute at texture instance creation some information for faster access to texels
        struct cache
        {
        public:
            enum ctor
            {
                DEFAULT
            };

            explicit cache(ctor)
            {}

            cache
            (
                storage_type& Storage,
                format_type Format,
                size_type BaseLayer, size_type Layers,
                size_type BaseFace, size_type MaxFace,
                size_type BaseLevel, size_type MaxLevel
            )
                : Faces(MaxFace - BaseFace + 1)
                , Levels(MaxLevel - BaseLevel + 1)
            {
                GLI_ASSERT(static_cast<size_t>(gli::levels(Storage.extent(0))) < this->ImageMemorySize.size());

/***
 XXX The cache has this vector of BaseAddresses sized according to the texture.
***/
                this->BaseAddresses.resize(Layers * this->Faces * this->Levels);

                for(size_type Layer = 0; Layer < Layers; ++Layer)
                for(size_type Face = 0; Face < this->Faces; ++Face)
                for(size_type Level = 0; Level < this->Levels; ++Level)
                {
                    size_type const Index = index_cache(Layer, Face, Level);
                    this->BaseAddresses[Index] = Storage.data() + Storage.base_offset(
                        BaseLayer + Layer, BaseFace + Face, BaseLevel + Level);
                }

                for(size_type Level = 0; Level < this->Levels; ++Level)
                {
                    extent_type const& SrcExtent = Storage.extent(BaseLevel + Level);
                    extent_type const& DstExtent = SrcExtent * block_extent(Format) / Storage.block_extent();

                    this->ImageExtent[Level] = glm::max(DstExtent, extent_type(1));
                    this->ImageMemorySize[Level] = Storage.level_size(BaseLevel + Level);
                }
                
                this->GlobalMemorySize = Storage.layer_size(BaseFace, MaxFace, BaseLevel, MaxLevel) * Layers;
            }

            // Base addresses of each images of a texture.
            data_type* get_base_address(size_type Layer, size_type Face, size_type Level) const
            {
                return this->BaseAddresses[index_cache(Layer, Face, Level)];
            }

            // In texels
            extent_type get_extent(size_type Level) const
            {
                return this->ImageExtent[Level];
            };

            // In bytes
            size_type get_memory_size(size_type Level) const
            {
                return this->ImageMemorySize[Level];
            };

            // In bytes
            size_type get_memory_size() const
            {
                return this->GlobalMemorySize;
            };

        private:
            size_type index_cache(size_type Layer, size_type Face, size_type Level) const
            {
                return ((Layer * this->Faces) + Face) * this->Levels + Level;
            }

            size_type Faces;
            size_type Levels;
            std::vector<data_type*> BaseAddresses;
            std::array<extent_type, 16> ImageExtent;
            std::array<size_type, 16> ImageMemorySize;
            size_type GlobalMemorySize;
        } Cache;


//======================================================================
// The inline implementation part
//======================================================================

inline bool texture::empty() const
    {
        if(this->Storage.get() == nullptr)
            return true;

        return this->Storage->empty();
    }

    inline texture::format_type texture::format() const
    {
        return this->Format;
    }

    inline texture::swizzles_type texture::swizzles() const
    {
        swizzles_type const FormatSwizzle = detail::get_format_info(this->format()).Swizzles;
        swizzles_type const CustomSwizzle = this->Swizzles;

        swizzles_type ResultSwizzle(SWIZZLE_ZERO);
        ResultSwizzle.r = is_channel(CustomSwizzle.r) ? FormatSwizzle[CustomSwizzle.r] : CustomSwizzle.r;
        ResultSwizzle.g = is_channel(CustomSwizzle.g) ? FormatSwizzle[CustomSwizzle.g] : CustomSwizzle.g;
        ResultSwizzle.b = is_channel(CustomSwizzle.b) ? FormatSwizzle[CustomSwizzle.b] : CustomSwizzle.b;
        ResultSwizzle.a = is_channel(CustomSwizzle.a) ? FormatSwizzle[CustomSwizzle.a] : CustomSwizzle.a;
        return ResultSwizzle;
    }

    inline texture::size_type texture::base_layer() const
    {
        return this->BaseLayer;
    }

    inline texture::size_type texture::max_layer() const
    {
        return this->MaxLayer;
    }

    inline texture::size_type texture::layers() const
    {
        if(this->empty())
            return 0;
        return this->max_layer() - this->base_layer() + 1;
    }

    inline texture::size_type texture::base_face() const
    {
        return this->BaseFace;
    }

    inline texture::size_type texture::max_face() const
    {
        return this->MaxFace;
    }

    inline texture::size_type texture::faces() const
    {
        if(this->empty())
            return 0;
        return this->max_face() - this->base_face() + 1;
    }

    inline texture::size_type texture::base_level() const
    {
        return this->BaseLevel;
    }

    inline texture::size_type texture::max_level() const
    {
        return this->MaxLevel;
    }

    inline texture::size_type texture::levels() const
    {
        if(this->empty())
            return 0;
        return this->max_level() - this->base_level() + 1;
    }

    inline texture::size_type texture::size() const
    {
        GLI_ASSERT(!this->empty());

        return this->Cache.get_memory_size();
    }

    template <typename gen_type>
    inline texture::size_type texture::size() const
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(block_size(this->format()) == sizeof(gen_type));

        return this->size() / sizeof(gen_type);
    }

    inline texture::size_type texture::size(size_type Level) const
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(Level >= 0 && Level < this->levels());

        return this->Cache.get_memory_size(Level);
    }

    template <typename gen_type>
    inline texture::size_type texture::size(size_type Level) const
    {
        GLI_ASSERT(block_size(this->format()) == sizeof(gen_type));

        return this->size(Level) / sizeof(gen_type);
    }

    inline void* texture::data()
    {
        GLI_ASSERT(!this->empty());

        return this->Cache.get_base_address(0, 0, 0);
    }

    inline void const* texture::data() const
    {
        GLI_ASSERT(!this->empty());

        return this->Cache.get_base_address(0, 0, 0);
    }

    template <typename gen_type>
    inline gen_type* texture::data()
    {
        GLI_ASSERT(block_size(this->format()) >= sizeof(gen_type));

        return reinterpret_cast<gen_type*>(this->data());
    }

    template <typename gen_type>
    inline gen_type const* texture::data() const
    {
        GLI_ASSERT(block_size(this->format()) >= sizeof(gen_type));

        return reinterpret_cast<gen_type const*>(this->data());
    }

    inline void* texture::data(size_type Layer, size_type Face, size_type Level)
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(Layer >= 0 && Layer < this->layers() && Face >= 0 && Face < this->faces() && Level >= 0 && Level < this->levels());

        return this->Cache.get_base_address(Layer, Face, Level);
    }

    inline void const* const texture::data(size_type Layer, size_type Face, size_type Level) const
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(Layer >= 0 && Layer < this->layers() && Face >= 0 && Face < this->faces() && Level >= 0 && Level < this->levels());

        return this->Cache.get_base_address(Layer, Face, Level);
    }

    template <typename gen_type>
    inline gen_type* texture::data(size_type Layer, size_type Face, size_type Level)
    {
        GLI_ASSERT(block_size(this->format()) >= sizeof(gen_type));

        return reinterpret_cast<gen_type*>(this->data(Layer, Face, Level));
    }

    template <typename gen_type>
    inline gen_type const* const texture::data(size_type Layer, size_type Face, size_type Level) const
    {
        GLI_ASSERT(block_size(this->format()) >= sizeof(gen_type));

        return reinterpret_cast<gen_type const* const>(this->data(Layer, Face, Level));
    }

    inline texture::extent_type texture::extent(size_type Level) const
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(Level >= 0 && Level < this->levels());

        return this->Cache.get_extent(Level);
    }

    inline void texture::clear()
    {
        GLI_ASSERT(!this->empty());

        memset(this->data(), 0, this->size());
    }

    template <typename gen_type>
    inline void texture::clear(gen_type const& Texel)
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(block_size(this->format()) == sizeof(gen_type));

        gen_type* Data = this->data<gen_type>();
        size_type const BlockCount = this->size<gen_type>();

        for(size_type BlockIndex = 0; BlockIndex < BlockCount; ++BlockIndex)
            *(Data + BlockIndex) = Texel;
    }

    template <typename gen_type>
    inline void texture::clear(size_type Layer, size_type Face, size_type Level, gen_type const& BlockData)
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(block_size(this->format()) == sizeof(gen_type));
        GLI_ASSERT(Layer >= 0 && Layer < this->layers() && Face >= 0 && Face < this->faces() && Level >= 0 && Level < this->levels());

        size_type const BlockCount = this->Storage->level_size(Level) / sizeof(gen_type);
        gen_type* Data = this->data<gen_type>(Layer, Face, Level);
        for(size_type BlockIndex = 0; BlockIndex < BlockCount; ++BlockIndex)
            *(Data + BlockIndex) = BlockData;
    }

    template <typename gen_type>
    inline void texture::clear
    (
        size_type Layer, size_type Face, size_type Level,
        extent_type const& TexelOffset, extent_type const& TexelExtent,
        gen_type const& BlockData
    )
    {
        storage_type::size_type const BaseOffset = this->Storage->base_offset(Layer, Face, Level);
        storage_type::data_type* const BaseAddress = this->Storage->data() + BaseOffset;

        extent_type BlockOffset(TexelOffset / this->Storage->block_extent());
        extent_type const BlockExtent(TexelExtent / this->Storage->block_extent() + BlockOffset);
        for(; BlockOffset.z < BlockExtent.z; ++BlockOffset.z)
        for(; BlockOffset.y < BlockExtent.y; ++BlockOffset.y)
        for(; BlockOffset.x < BlockExtent.x; ++BlockOffset.x)
        {
            gli::size_t const Offset = this->Storage->image_offset(BlockOffset, this->extent(Level)) * this->Storage->block_size();
            gen_type* const BlockAddress = reinterpret_cast<gen_type* const>(BaseAddress + Offset);
            *BlockAddress = BlockData;
        }
    }

    inline void texture::copy
    (
        texture const& TextureSrc,
        size_t LayerSrc, size_t FaceSrc, size_t LevelSrc,
        size_t LayerDst, size_t FaceDst, size_t LevelDst
    )
    {
        GLI_ASSERT(this->size(LevelDst) == TextureSrc.size(LevelSrc));
        GLI_ASSERT(LayerSrc < TextureSrc.layers());
        GLI_ASSERT(LayerDst < this->layers());
        GLI_ASSERT(FaceSrc < TextureSrc.faces());
        GLI_ASSERT(FaceDst < this->faces());
        GLI_ASSERT(LevelSrc < TextureSrc.levels());
        GLI_ASSERT(LevelDst < this->levels());
        
        memcpy(
            this->data(LayerDst, FaceDst, LevelDst),
            TextureSrc.data(LayerSrc, FaceSrc, LevelSrc),
            this->size(LevelDst));
    }

    inline void texture::copy
    (
        texture const& TextureSrc,
        size_t LayerSrc, size_t FaceSrc, size_t LevelSrc, texture::extent_type const& OffsetSrc,
        size_t LayerDst, size_t FaceDst, size_t LevelDst, texture::extent_type const& OffsetDst,
        texture::extent_type const& Extent
    )
    {
        storage_type::extent_type const BlockExtent = this->Storage->block_extent();
        this->Storage->copy(
            *TextureSrc.Storage,
            LayerSrc, FaceSrc, LevelSrc, OffsetSrc / BlockExtent,
            LayerDst, FaceDst, LevelDst, OffsetDst / BlockExtent,
            Extent / BlockExtent);
    }

    template <typename gen_type>
    inline void texture::swizzle(gli::swizzles const& Swizzles)
    {
        for(size_type TexelIndex = 0, TexelCount = this->size<gen_type>(); TexelIndex < TexelCount; ++TexelIndex)
        {
            gen_type& TexelDst = *(this->data<gen_type>() + TexelIndex);
            gen_type const TexelSrc = TexelDst;
            for(typename gen_type::length_type Component = 0; Component < TexelDst.length(); ++Component)
            {
                GLI_ASSERT(static_cast<typename gen_type::length_type>(Swizzles[Component]) < TexelDst.length());
                TexelDst[Component] = TexelSrc[Swizzles[Component]];
            }
        }
    }

    template <typename gen_type>
    inline gen_type texture::load(extent_type const& TexelCoord, size_type Layer,  size_type Face, size_type Level) const
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(!is_compressed(this->format()));
        GLI_ASSERT(block_size(this->format()) == sizeof(gen_type));

        size_type const ImageOffset = this->Storage->image_offset(TexelCoord, this->extent(Level));
        GLI_ASSERT(ImageOffset < this->size<gen_type>(Level));

        return *(this->data<gen_type>(Layer, Face, Level) + ImageOffset);
    }

    template <typename gen_type>
    inline void texture::store(extent_type const& TexelCoord, size_type Layer,  size_type Face, size_type Level, gen_type const& Texel)
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(!is_compressed(this->format()));
        GLI_ASSERT(block_size(this->format()) == sizeof(gen_type));
        GLI_ASSERT(glm::all(glm::lessThan(TexelCoord, this->extent(Level))));

        size_type const ImageOffset = this->Storage->image_offset(TexelCoord, this->extent(Level));
        GLI_ASSERT(ImageOffset < this->size<gen_type>(Level));

        *(this->data<gen_type>(Layer, Face, Level) + ImageOffset) = Texel;
    }
}//namespace gli


//======================================================================
//  This loop is used to load the data from a KTX file.
//======================================================================

        for(texture::size_type Level = 0, Levels = Texture.levels(); Level < Levels; ++Level)
        {
            Offset += sizeof(std::uint32_t);

            for(texture::size_type Layer = 0, Layers = Texture.layers(); Layer < Layers; ++Layer)
            for(texture::size_type Face = 0, Faces = Texture.faces(); Face < Faces; ++Face)
            {
                texture::size_type const FaceSize = Texture.size(Level);

                std::memcpy(Texture.data(Layer, Face, Level), Data + Offset, FaceSize);

                Offset += std::max(BlockSize, glm::ceilMultiple(FaceSize, static_cast<texture::size_type>(4)));
            }
        }

//======================================================================
//  Calculation of baseOffset.
//======================================================================

        inline storage_linear::size_type storage_linear::base_offset(size_type Layer, size_type Face, size_type Level) const
    {
        GLI_ASSERT(!this->empty());
        GLI_ASSERT(Layer >= 0 && Layer < this->layers() && Face >= 0 && Face < this->faces() && Level >= 0 && Level < this->levels());

        size_type const LayerSize = this->layer_size(0, this->faces() - 1, 0, this->levels() - 1);
        size_type const FaceSize = this->face_size(0, this->levels() - 1);
        size_type BaseOffset = LayerSize * Layer + FaceSize * Face;

        for(size_type LevelIndex = 0, LevelCount = Level; LevelIndex < LevelCount; ++LevelIndex)
            BaseOffset += this->level_size(LevelIndex);

        return BaseOffset;
    }

//======================================================================
//  And this is for the level_size. Very similar to what I have already.
//======================================================================


inline storage_linear::size_type storage_linear::level_size(size_type Level) const
    {
        GLI_ASSERT(Level >= 0 && Level < this->levels());

        return this->BlockSize * glm::compMul(this->block_count(Level));
    }


// XXX The big question is - do I need to cache the addresses or perform the above calculations
// each time. If I store the block size in the structure the calculation shouldn't take long.

#endif /* KTXTEXTURE_H */
