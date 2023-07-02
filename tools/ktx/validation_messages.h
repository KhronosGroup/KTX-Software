// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <cstdint>
#include <string_view>


// -------------------------------------------------------------------------------------------------

namespace ktx {

using IssueID = uint16_t;

enum class IssueType : uint8_t {
    warning,
    error,
    fatal,
};

[[nodiscard]] constexpr inline std::string_view toString(IssueType value) noexcept {
    switch (value) {
    case IssueType::warning:
        return "warning";
    case IssueType::error:
        return "error";
    case IssueType::fatal:
        return "fatal";
    }
    assert(false && "Invalid IssueType enum value");
    return "<<invalid>>";
}

struct Issue {
    IssueType type;
    IssueID id;
    std::string_view message;
    std::string_view detailsFmt;

    constexpr inline Issue(IssueType type, IssueID id, std::string_view message, std::string_view detailsFmt) :
            type(type),
            id(id),
            message(message),
            detailsFmt(detailsFmt) {}
};

struct IssueWarning : Issue {
    constexpr inline IssueWarning(IssueID id, std::string_view message, std::string_view detailsFmt) :
            Issue(IssueType::warning, id, message, detailsFmt) {}
};

struct IssueError : Issue {
    constexpr inline IssueError(IssueID id, std::string_view message, std::string_view detailsFmt) :
            Issue(IssueType::error, id, message, detailsFmt) {}
};

struct IssueFatal : Issue {
    constexpr inline IssueFatal(IssueID id, std::string_view message, std::string_view detailsFmt) :
            Issue(IssueType::fatal, id, message, detailsFmt) {}
};

// -------------------------------------------------------------------------------------------------

struct IOError {
    static constexpr IssueFatal FileOpen{
        1001, "Failed to open file.",
        "Failed to open file {}: {}."
    };
    static constexpr IssueFatal FileReadFailure{
        1002, "Failed to read from file.",
        "Requested {} bytes but only read {} byte(s) at offset {} to access {}. File error {}."
    };
    static constexpr IssueFatal UnexpectedEOF{
        1003, "Unexpected end of file.",
        "Unexpected end of file. Failed to read {} byte(s) at offset {} to access {}. Only able to read {} byte(s)."
    };
    // static constexpr IssueFatal FileSeekEndFailure{
    //     1004, "Failed to seek to the end of the file.",
    //     "Failed to seek to the end of the file: {}."
    // };
    // static constexpr IssueFatal FileTellFailure{
    //     1005, "Failed to determine the size of the file.",
    //     "Failed to determine the size of the file: {}."
    // };
    static constexpr IssueFatal RewindFailure{
        1006, "Failed to seek to the start of the file.",
        "Failed to seek to the start of the file: {}."
    };
    static constexpr IssueFatal FileSeekFailure{
        1007, "Failed to seek in the file.",
        "Failed to seek to {} to access {}. File error {}."
    };
};

struct FileError {
    static constexpr IssueFatal NotKTX2{
        2001, "Not a KTX2 file.",
        "Not a KTX2 file. The beginning of the file does not match the expected file identifier \"«KTX 20»\\r\\n\\x1A\\n\"."
    };
    // static constexpr IssueFatal IncorrectDataSize{
    //     2003, "Size of image data in file does not match size calculated from levelIndex."
    // };
};

struct HeaderData {
    // 30xx - KTX Header related issues

    static constexpr IssueError ProhibitedFormat{
        3001, "Prohibited VkFormat.",
        "VkFormat {} is prohibited in a KTX2 file."
    };
    static constexpr IssueError InvalidFormat{
        3002, "Invalid VkFormat.",
        "Invalid VkFormat {}."
    };
    static constexpr IssueWarning UnknownFormat{
        3003, "Unknown VkFormat. Possibly an extension format.",
        "Unknown VkFormat {}, possibly an extension format."
    };

    static constexpr IssueError VkFormatAndBasis{
        3004, "Invalid VkFormat. VkFormat must be VK_FORMAT_UNDEFINED for BASIS_LZ supercompression.",
        "VkFormat is {} but for supercompressionScheme BASIS_LZ it must be VK_FORMAT_UNDEFINED."
    };
    static constexpr IssueError TypeSizeNotOne{
        3005, "Invalid typeSize. typeSize must be 1 for block-compressed or supercompressed formats.",
        "typeSize is {} but for block-compressed or supercompressed format {} it must be 1."
    };

    static constexpr IssueError WidthZero{
        3006, "Invalid pixelWidth. pixelWidth must not be 0.",
        "pixelWidth is 0 but textures must have width."
    };
    static constexpr IssueError BlockCompressedNoHeight{
        3007, "Invalid pixelHeight. pixelHeight must not be 0 for a block compressed formats.",
        "pixelHeight is 0 but for block-compressed {} it must not be 0."
    };
    static constexpr IssueError CubeHeightWidthMismatch{
        3008, "Mismatching pixelWidth and pixelHeight for a cube map.",
        "pixelWidth is {} and pixelHeight is {} but for a cube map they must be equal."
    };
    static constexpr IssueError DepthNoHeight{
        3009, "Invalid pixelHeight. pixelHeight must not be 0 if pixelDepth is not also 0.",
        "pixelHeight is 0 and pixelDepth is {} but pixelHeight must not be 0 if pixelDepth is not 0 as well."
    };
    static constexpr IssueError DepthBlockCompressedNoDepth{
        3010, "Invalid pixelDepth. pixelDepth must not be 0 for block-compressed formats with non-zero block depth.",
        "pixelDepth is 0 but for {} (which is a block-compressed format with non-zero block depth) it must not be 0."
    };
    static constexpr IssueError DepthStencilFormatWithDepth{
        3011, "Invalid pixelDepth. pixelDepth must be 0 for depth or stencil formats.",
        "pixelDepth is {} but for depth or stencil format {} it must be 0."
    };

    static constexpr IssueError TypeSizeMismatch{
        3012, "Invalid typeSize. The value must match the expected typeSize of the VkFormat.",
        "typeSize is {} but for VkFormat {} it must be {}."
    };
    static constexpr IssueError CubeWithDepth{
        3013, "Invalid pixelDepth. pixelDepth must be 0 for cube maps.",
        "pixelDepth is {} but for cube maps it must be 0 (cube map faces must be 2D)."
    };
    static constexpr IssueWarning ThreeDArray{
        3014, "File contains a 3D array texture.",
        "File contains a 3D array texture. No API supports these."
    };
    static constexpr IssueError InvalidFaceCount{
        3015, "Invalid faceCount. faceCount must be either 6 for Cubemaps and Cubemap Arrays or 1 otherwise.",
        "faceCount is {} but it must be either 6 for Cubemaps and Cubemap Arrays or 1 otherwise."
    };
    static constexpr IssueError TooManyMipLevels{
        3016, "Too many mip levels.",
        "levelCount is {} but for the largest image dimension which is {} it is too many level."
    };
    static constexpr IssueError BlockCompressedNoLevel{
        3017, "Invalid levelCount. levelCount cannot be 0 for block-compressed formats.",
        "levelCount is 0 but for block-compressed {} it must not be 0."
    };
    static constexpr IssueWarning VendorSupercompression{
        3018, "Using vendor supercompressionScheme. Cannot validate.",
        "supercompressionScheme is {} which falls into the reserved vendor range. Cannot validate."
    };
    static constexpr IssueError InvalidSupercompression{
        3019, "Invalid supercompressionScheme.",
        "Invalid supercompressionScheme: {}."
    };

    // Header index related issues:

    static constexpr IssueError IndexDFDMissing{
        3020, "Missing Data Format Descriptor.",
        "Data Format Descriptor is mandatory but dfdByteOffset is {} and dfdByteLength is {}."
    };
    static constexpr IssueError IndexDFDInvalidOffset{
        3021, "Invalid dfdByteOffset.",
        "dfdByteOffset is {} but the Data Format Descriptor must immediately follow (with 4 byte alignment) the Level Index so it must be {}."
    };
    static constexpr IssueError IndexKVDInvalidOffset{
        3022, "Invalid kvdByteOffset.",
        "kvdByteOffset is {} but the Key/Value Data must immediately follow (with 4 byte alignment) the Data Format Descriptor so it must be {}."
    };
    static constexpr IssueError IndexKVDOffsetWithoutLength{
        3023, "kvdByteOffset must be 0 when kvdByteLength is 0.",
        "kvdByteOffset is {} but must be 0 when kvdByteLength is 0."
    };
    static constexpr IssueError IndexSGDInvalidOffset{
        3024, "Invalid sgdByteOffset.",
        "sgdByteOffset is {} but the Supercompression Global Data must immediately follow (with 8 byte alignment) the preceding block so it must be {}."
    };
    static constexpr IssueError IndexSGDOffsetWithoutLength{
        3025, "sgdByteOffset must be 0 when sgdByteLength is 0.",
        "sgdByteOffset is {} but must be 0 when sgdByteLength is 0."
    };
    static constexpr IssueError IndexSGDMissing{
        3026, "sgdByteLength must not be 0 for supercompression schemes with global data.",
        "sgdByteLength is 0 but for supercompression scheme {} (which has global data) it must not be 0."
    };
    static constexpr IssueError IndexSGDNotApplicable{
        3027, "sgdByteLength must be 0 for supercompression schemes with no global data.",
        "sgdByteLength is {} but for supercompression scheme {} (which does not have global data) it must be 0."
    };
    static constexpr IssueError IndexDFDInvalidLength{
        3028, "Invalid dfdByteLength. If there is Key/Value Data the dfdByteLength/dfdTotalSize must be equal to kvdByteOffset - dfdByteOffset.",
        "dfdByteLength is {} but it must be equal to kvdByteOffset - dfdByteOffset which is {}."
    };

    // 31xx - GLTF KHR_texture_basisu compatibility

    static constexpr IssueError InvalidSupercompressionGLTFBU{
        3101, "Invalid supercompressionScheme for KHR_texture_basisu compatibility.",
        "supercompressionScheme is {} but it must either be BASIS_LZ for ETC1S or either NONE or ZSTD for UASTC textures for KHR_texture_basisu compatibility."
    };
    static constexpr IssueError InvalidTextureTypeGLTFBU{
        3102, "Texture type must be 2D for KHR_texture_basisu compatibility.",
        "Texture type is not 2D as {} is {} instead of {} which is incompatible with KHR_texture_basisu requirements."
    };
    static constexpr IssueError InvalidPixelWidthHeightGLTFBU{
        3103, "pixelWidth and pixelHeight must be multiples of 4 for KHR_texture_basisu compatibility.",
        "{} is {} which is not an integer multiple of 4 as required for KHR_texture_basisu compatibility."
    };
    static constexpr IssueError InvalidLevelCountGLTFBU{
        3104, "When multiple mip levels are present KHR_texture_basisu requires a full mip pyramid.",
        "levelCount is {} but it must be 1 (single level) or {} (full mip pyramid) for KHR_texture_basisu compatibility."
    };
};

struct LevelIndex {
    // 40xx - Level index related issues

    static constexpr IssueError IncorrectIndexOrder{
        4001, "Invalid Level Index. Indices must be sorted from the largest level to the smallest level.",
        "Indexes for level {} with byteLength {} and level {} with byteLength {} are incorrectly ordered."
    };
    static constexpr IssueError IncorrectLevelOrder{
        4002, "Invalid Level Index. Level images must be sorted from the smallest level to the largest level in the file.",
        "Level Image for level {} with byteOffset {} and level {} with byteOffset {} are incorrectly ordered."
    };

    static constexpr IssueError IncorrectByteOffsetUnaligned{
        4003, "Invalid byteOffset in Level Index. byteOffset has to be aligned to lcm(texel_block_size, 4) and must match expected value.",
        "Level {} byteOffset is {} but based on the vkFormat, DFD and image sizes the required alignment is {} and the expected value is {}."
    };
    static constexpr IssueError IncorrectByteOffset{
        4004, "Invalid byteOffset in Level Index. byteOffset must match the expected value.",
        "Level {} byteOffset is {} but based on the vkFormat, DFD and image sizes the expected value is {}."
    };

    static constexpr IssueError IncorrectByteLength{
        4005, "Invalid byteLength in Level Index. byteLength must match the expected value.",
        "Level {} byteLength is {} but based on the vkFormat, DFD and image sizes the expected value is {}."
    };

    static constexpr IssueError IncorrectUncompressedByteLength{
        4006, "Invalid uncompressedByteLength in Level Index. For non-supercompressed files the uncompressedByteLength must match the expected value of byteLength.",
        "Level {} uncompressedByteLength is {} but based on the vkFormat, DFD and image sizes the expected value is {}."
    };
    static constexpr IssueError NonZeroUBLForBLZE{
        4007, "Invalid uncompressedByteLength in Level Index. For BasisLZ supercompression uncompressedByteLength must be 0.",
        "Level {} uncompressedByteLength is {} but for BasisLZ supercompression it must be 0."
    };
    static constexpr IssueError UncompressedByteLengthMismatch{
        4008, "Mismatch between uncompresedByteLength in Level Index and actually decompressed bytes.",
        "Decompressing supercompression {} resulted in a different number of bytes than expected according to uncompressedByteLength."
    };
    static constexpr IssueError ZeroUncompressedLength{
        4009, "Invalid uncompressedByteLength in Level Index. For non-BasisLZ files with VK_FORMAT_UNDEFINED uncompressedByteLength must not be 0.",
        "Level {} uncompressedByteLength is 0 but for non-BasisLZ files with VK_FORMAT_UNDEFINED uncompressedByteLength must not be 0."
    };
    static constexpr IssueError InvalidUncompressedLength{
        4010, "Invalid uncompressedByteLength in Level Index. uncompressedByteLength must be equally divisible between every face and layer.",
        "Level {} uncompressedByteLength is {} but it must be divisible with faceCount * max(1, layerCount)."
    };
};

struct Validator {
    // 50xx - Validator or KTX Library related issues

    static constexpr IssueError CreateExpectedDFDFailure{
        5001, "Failed to create expected DFD for the given VkFormat.",
        "Failed to create expected DFD for the given VkFormat {}."
    };
    static constexpr IssueError CreateDFDRoundtripFailed{
        5002, "Failed to re-interpret expected DFD.",
        "DFD created for VkFormat {} confused interpretDFD()."
    };
    static constexpr IssueWarning UnsupportedFeature{
        5003, "Feature not supported by libktx.",
        "KTX 2.0 file is valid but it is not currently supported by libktx."
    };
    static constexpr IssueWarning SupportedNonConformantFile{
        5004, "Non-conformant texture file accepted by libktx.",
        "KTX 2.0 file does not conform to the specification but it is currently accepted by libktx."
    };
    static constexpr IssueFatal CreateFailure{
        5005, "Failed to load texture using libktx.",
        "KTX 2.0 file is valid but libktx loading returned error: {}"
    };
    static constexpr IssueError DecompressChecksumError{
        5006, "Checksum error during decompression.",
        "Decompressing supercompression {} resulted in a checksum error."
    };
    static constexpr IssueError TranscodeFailure{
        5007, "Failed to transcode texture.",
        "Transcoding of texture with color model {} failed with the error: {}"
    };
};

struct DFD {
    // 60xx - Generic DFD related issues:

    static constexpr IssueError SizeMismatch{
        6001, "Mismatching dfdTotalSize and dfdByteLength. dfdTotalSize must match dfdByteLength.",
        "dfdTotalSize is {} but dfdByteLength is {} and they must match."
    };
    static constexpr IssueWarning TooManyDFDBlocks{
        6002, "Too many DFD blocks. The number of DFD blocks exceeds the validator limit.",
        "The number of DFD blocks exceeds the validator limit of {}. Skipping validation of the remaining {} byte(s)."
    };
    static constexpr IssueWarning UnknownDFDBlock{
        6003, "Unrecognized DFD block.",
        "DFD block #{} vendorId {} and descriptorType {} is not recognized and thus ignored."
    };
    static constexpr IssueError NotEnoughDataForBlockHeader{
        6004, "Invalid DFD data. Not enough data left to process another DFD block header.",
        "DFD has {} byte(s) unprocessed but for a valid DFD at least 8 bytes are required."
    };
    static constexpr IssueWarning MultipleBDFD{
        6005, "Multiple basic DFD blocks.",
        "DFD block #{} is a basic DFD block but one was already processed before. It will be ignored."
    };
    static constexpr IssueError DescriptorBlockSizeTooSmall{
        6006, "DFD block descriptorBlockSize is too small.",
        "DFD block #{} descriptorBlockSize is {} but has to be at least 8 bytes."
    };
    static constexpr IssueError DescriptorBlockSizeTooBig{
        6007, "DFD block descriptorBlockSize is too big.",
        "DFD block #{} descriptorBlockSize is {} but only {} byte(s) left in the DFD."
    };
    static constexpr IssueError MissingBDFD{
        6008, "Missing basic DFD block.",
        "No basic data format descriptor block is found in the DFD, or it is not the first DFD block."
    };

    // Basic Data Format Descriptor Block related issues:

    static constexpr IssueError BasicDescriptorBlockSizeInvalid{
        6009, "Basic DFD block descriptorBlockSize is invalid.",
        "DFD block #{} descriptorBlockSize is {} which does not fit the criteria (descriptorBlockSize - 24) % 16 == 0 for basic DFD blocks."
    };
    static constexpr IssueError BasicDescriptorBlockSizeTooSmall{
        6010, "Basic DFD block descriptorBlockSize is too small.",
        "DFD block #{} descriptorBlockSize is {} which is smaller than the minimum size of a basic DFD block (24 bytes)."
    };
    static constexpr IssueError BasicVersionNotSupported{
        6011, "Unsupported basic DFD block version.",
        "DFD block #{} versionNumber in basic DFD block is {} but it must be KHR_DF_VERSIONNUMBER_1_3."
    };
    static constexpr IssueError BasicInvalidTransferFunction{
        6012, "Invalid transferFunction in basic DFD block. It must be either KHR_DF_TRANSFER_LINEAR or KHR_DF_TRANSFER_SRGB.",
        "DFD block #{} transferFunction in basic DFD block is {} but it must be either KHR_DF_TRANSFER_LINEAR or KHR_DF_TRANSFER_SRGB."
    };
    static constexpr IssueError BasicSRGBMismatch{
        6013, "Invalid transferFunction in basic DFD block. For an sRGB VkFormat it must be KHR_DF_TRANSFER_SRGB.",
        "DFD block #{} transferFunction in basic DFD block is {} but for VkFormat {} it must be KHR_DF_TRANSFER_SRGB."
    };
    static constexpr IssueError BasicNotSRGBMismatch{
        6014, "Invalid transferFunction in basic DFD block. For a non-sRGB VkFormat with sRGB variants it must not be KHR_DF_TRANSFER_SRGB.",
        "DFD block #{} transferFunction is KHR_DF_TRANSFER_SRGB but for VkFormat {} it must not be KHR_DF_TRANSFER_SRGB."
    };
    static constexpr IssueError IncorrectModelForRGB{
        6015, "Invalid colorModel in basic DFD block for RGB VkFormat.",
        "DFD block #{} colorModel in basic DFD block is {} but for VkFormat {} it must be KHR_DF_MODEL_RGBSDA."
    };
    static constexpr IssueError IncorrectModelForBlock{
        6016, "Invalid colorModel in basic DFD block for block compressed VkFormat.",
        "DFD block #{} colorModel in basic DFD block is {} but for VkFormat {} it must be {}."
    };
    static constexpr IssueError IncorrectModelFor422{
        6017, "Invalid colorModel in basic DFD block for *_422_* VkFormat.",
        "DFD block #{} colorModel in basic DFD block is {} but for VkFormat {} it must be KHR_DF_MODEL_YUVSDA."
    };
    static constexpr IssueError IncorrectModelForBLZE{
        6018, "Invalid colorModel in basic DFD block for BasisLZ supercompression.",
        "DFD block #{} colorModel in basic DFD block is {} but for BasisLZ supercompression it must be KHR_DF_MODEL_ETC1S."
    };
    static constexpr IssueError InvalidColorPrimaries{
        6019, "Invalid colorPrimaries in basic DFD block.",
        "DFD block #{} colorPrimaries in basic DFD block is invalid: {}."
    };
    static constexpr IssueError InvalidTexelBlockDimension{
        6020, "Invalid texelBlockDimensions in basic DFD block.",
        "DFD block #{} texel block dimensions in basic DFD block are {}x{}x{}x{} but these must be {}x{}x{}x{} for {} textures."
    };
    static constexpr IssueError BytesPlanesMismatch{
        6021, "Invalid bytesPlanes in basic DFD block. The values do not match the expected values.",
        "DFD block #{} bytesPlanes in basic DFD block are {} {} {} {} {} {} {} {} but for {} textures these must be {} {} {} {} {} {} {} {}."
    };
    static constexpr IssueError BytesPlanesNotUnsized{
        6022, "Invalid bytesPlanes in basic DFD block. BytesPlanes must be 0 for supercompressed textures.",
        "DFD block #{} bytesPlanes in basic DFD block are {} {} {} {} {} {} {} {} but for {} supercompressed textures these must be zeros."
    };
    static constexpr IssueError BytesPlane0Zero{
        6023, "Invalid bytesPlane0 in basic DFD block. BytesPlane0 must be non-zero for non-supercompressed VK_FORMAT_UNDEFINED textures.",
        "DFD block #{} bytesPlane0 in basic DFD block is {} but it must be non-zero for non-supercompressed VK_FORMAT_UNDEFINED textures."
    };
    static constexpr IssueError MultiplaneFormatsNotSupported{
        6024, "Invalid bytesPlanes in basic DFD block. Multiplane formats are not supported.",
        "DFD block #{} bytesPlanes in basic DFD block are {} {} {} {} {} {} {} {} but bytesPlane[1-7] must be 0 as multiplane formats are not supported."
    };
    static constexpr IssueError SampleCountMismatch{
        6025, "Invalid sample count in basic DFD block. The sample count must match the expected sample count of the VkFormat.",
        "DFD block #{} sample count in basic DFD block is {} but for VkFormat {} it must be {}."
    };
    static constexpr IssueError InvalidSampleCount{
        6026, "Invalid sample count in basic DFD block. The sample count must match the expected sample count of the texture.",
        "DFD block #{} sample count in basic DFD block is {} but for {} textures it must be {}."
    };
    static constexpr IssueError ZeroSamples{
        6027, "Invalid sample count in basic DFD block. The sample count must be non-zero for non-supercompressed textures with VK_FORMAT_UNDEFINED.",
        "DFD block #{} sample count in basic DFD block is 0 but non-supercompressed VK_FORMAT_UNDEFINED textures must have sample information."
    };
    static constexpr IssueError FormatMismatch{
        6028, "Invalid sample in basic DFD block. The samples must match the expected samples of the VkFormat.",
        "DFD block #{} sample #{} {} in basic DFD block is {} but the expected value is {} for {}."
    };
    static constexpr IssueWarning TooManySample{
        6029, "Too many BDFD sample. The number of BDFD samples exceeds the validator limit.",
        "DFD block #{} sample count in basic DFD block is {} which exceeds the validator limit of {}. Skipping validation of the last {} sample(s) ({} byte(s))."
    };

    // 61xx - Basic Data Format Descriptor Block sample related issues:

    static constexpr IssueError InvalidChannelForModel{
        6101, "Invalid sample channelType for colorModel in the basic DFD block.",
        "DFD block #{} sample #{} channelType in basic DFD block is {} which is not valid for colorModel {}."
    };
    static constexpr IssueError InvalidBitOffsetForUASTC{
        6102, "Invalid sample bitOffset for UASTC texture in the basic DFD block.",
        "DFD block #{} sample #{} bitOffset in basic DFD block is {} but for UASTC textures it must be 0."
    };
    static constexpr IssueError InvalidBitOffsetForBLZE{
        6103, "Invalid sample bitOffset for BasisLZ/ETC1S texture in the basic DFD block.",
        "DFD block #{} sample #{} bitOffset in basic DFD block is {} but for BasisLZ/ETC1S textures it must be {}."
    };
    static constexpr IssueError InvalidBitLengthForUASTC{
        6104, "Invalid sample bitLength for UASTC texture in the basic DFD block.",
        "DFD block #{} sample #{} bitLength in basic DFD block is {} but for UASTC textures it must be 127."
    };
    static constexpr IssueError InvalidBitLengthForBLZE{
        6105, "Invalid sample bitLength for BasisLZ/ETC1S texture in the basic DFD block.",
        "DFD block #{} sample #{} bitLength in basic DFD block is {} but for BasisLZ/ETC1S textures it must be 63."
    };
    static constexpr IssueError InvalidLower{
        6106, "Invalid sample lower for UASTC or BasisLZ/ETC1S texture in the basic DFD block.",
        "DFD block #{} sample #{} lower in basic DFD block is {} but for {} textures it must be {}."
    };
    static constexpr IssueError InvalidUpper{
        6107, "Invalid sample upper for UASTC or BasisLZ/ETC1S texture in the basic DFD block.",
        "DFD block #{} sample #{} upper in basic DFD block is {} but for {} textures it must be {}."
    };

    // 62xx - InterpretDFD related issues:

    static constexpr IssueError InterpretDFDMixedChannels{
        6203, "Mixed sample types. The Signed/Unsigned and Float/Integer flags of Basic DFD samples must be the consistent.",
        "DFD block #{} has mixed Signed/Unsigned or Float/Integer samples but they must be consistent."
    };
    static constexpr IssueError InterpretDFDMultisample{
        6204, "Unsupported multiple-sample format. Every sample position must be zero.",
        "DFD block #{} indicates multiple sample locations but multisample formats are not supported."
    };
    static constexpr IssueError InterpretDFDNonTrivialEndianness{
        6205, "Non-trivial endianness detected in the basic DFD block.",
        "DFD block #{} describes non little-endian or unsupported format."
    };

    // 63xx - GLTF KHR_texture_basisu compatibility

    static constexpr IssueError IncorrectModelGLTFBU{
        6301, "Invalid colorModel in basic DFD block for KHR_texture_basisu compatibility.",
        "DFD block #{} colorModel in basic DFD block is {} but for KHR_texture_basisu compatibility it must be either ETC1S or UASTC."
    };
    static constexpr IssueError IncompatibleModelGLTFBU{
        6302, "Incompatible supercompressionScheme and colorModel for KHR_texture_basisu compatibility.",
        "DFD block #{} colorModel is {} while supercompressionScheme is {} but KHR_texture_basisu requires supercompressionScheme {} for this colorModel."
    };
    static constexpr IssueError InvalidChannelGLTFBU{
        6303, "Invalid sample channelType for colorModel for KHR_texture_basisu compatibility.",
        "DFD block #{} colorModel is {} but sample #{} channelType is {} while KHR_texture_basisu requires {}."
    };
    static constexpr IssueError InvalidColorSpaceGLTFBU{
        6304, "Color space information is incompatible with KHR_texture_basisu.",
        "DFD block #{} primaries is {} and transfer is {} but KHR_texture_basisu requires either KHR_DF_PRIMARIES_BT709 with KHR_DF_TRANSFER_SRGB or KHR_DF_PRIMARIES_UNSPECIFIED with KHR_DF_TRANSFER_LINEAR."
    };
};

struct Metadata {
    // 70xx - Generic Key-Value related issues:

    static constexpr IssueWarning TooManyEntries{
        7001, "Too many Key/Value entries. The number of key-value entries exceeds the validator limit.",
        "The number of key-value entries exceeds the validator limit of {}. Skipping validation of the remaining {} byte(s)."
    };
    static constexpr IssueError NotEnoughDataForAnEntry{
        7002, "Invalid Key/Value Data. Not enough data left in Key/Value Data to process another key-value entry",
        "Key/Value Data has {} byte(s) unprocessed but for a key value entry at least 6 bytes are required (4 byte size + 1 byte key + null terminator)."
    };
    static constexpr IssueError KeyAndValueByteLengthTooLarge{
        7003, "Invalid keyAndValueByteLength. keyAndValueByteLength is greater than the amount of bytes left in the Key/Value Data.",
        "keyAndValueByteLength is {} but the Key/Value Data only has {} byte(s) left for the key-value pair."
    };
    static constexpr IssueError KeyAndValueByteLengthTooSmall{
        7004, "Invalid keyAndValueByteLength. keyAndValueByteLength must be at least 2.",
        "keyAndValueByteLength is {} but it must be at least 2 (1 byte key + null terminator)."
    };
    static constexpr IssueError KeyMissingNullTerminator{
        7005, "Invalid key in Key/Value Data. Key is missing the NULL terminator.",
        "The key-value entry \"{}\" is missing the NULL terminator but every key-value entry must have a NULL terminator separating the key from the value."
    };
    static constexpr IssueError KeyForbiddenBOM{
        7006, "Invalid key in Key/Value Data. Key must not contain BOM.",
        "The beginning of the key \"{}\" has forbidden BOM."
    };
    static constexpr IssueError KeyInvalidUTF8{
        7007, "Invalid key in Key/Value Data. Key must be a valid UTF-8 string.",
        "Key is \"{}\", which contains an invalid UTF-8 character at index: {}."
    };

    static constexpr IssueError SizesDontAddUp{
        7008, "kvdByteLength must add up to the sum of the size of the key-value entries with paddings.",
        "The processed Key/Value Data length is {} byte(s) but kvdByteLength is {} byte(s) and they must match."
    };
    static constexpr IssueError UnknownReservedKey{
        7009, "Invalid key in Key/Value Data. Keys with \"KTX\" or \"ktx\" prefix are reserved.",
        "The key is \"{}\" but its not recognized and keys with \"KTX\" or \"ktx\" prefix are reserved."
    };
    static constexpr IssueWarning CustomMetadata{
        7010, "Custom key in Key/Value Data.",
        "Custom key \"{}\" found in Key/Value Data."
    };
    static constexpr IssueError PaddingNotZero{
        7011, "Invalid padding byte value. Every padding byte's value must be 0.",
        "A padding byte value is {:d} {} but it must be 0."
    };

    static constexpr IssueError OutOfOrder{
        7012, "Invalid Key/Value Data. Key-value entries must be sorted by key.",
        "Key-value entries are not sorted but they must be sorted by key.",
    };
    static constexpr IssueError DuplicateKey{
        7013, "Invalid Key/Value Data. Keys must be unique.",
        "Duplicate key-value entry with key \"{}\"."
    };
    static constexpr IssueError KeyEmpty{
        7013, "Empty key in Key/Value Data.",
        "Key length is 0 byte in key-value entry."
    };

    static constexpr IssueError KTXcubemapIncompleteInvalidBitSet{
        7101, "Invalid KTXcubemapIncomplete value. The two MSB must be 0.",
        "The value is {:08b} but the two MSB must be 0 (00XXXXXX)."
    };
    static constexpr IssueWarning KTXcubemapIncompleteAllBitsSet{
        7102, "KTXcubemapIncomplete is not incomplete. Every face is marked present.",
        "Every face bit is set as present, KTXcubemapIncomplete key is unnecessary."
    };
    static constexpr IssueError KTXcubemapIncompleteNoBitSet{
        7103, "Invalid KTXcubemapIncomplete value. No face is marked present.",
        "No face bit is set as present but at least 1 face must be present."
    };
    static constexpr IssueError KTXcubemapIncompleteIncompatibleLayerCount{
        7104, "Incompatible KTXcubemapIncomplete and layerCount. layerCount must be the multiple of the number of faces present.",
        "layerCount is {} and KTXcubemapIncomplete indicates {} faces present but layerCount must the multiple of the number of faces present."
    };
    static constexpr IssueError KTXcubemapIncompleteWithFaceCountNot1{
        7105, "Invalid faceCount. faceCount must be 1 if KTXcubemapIncomplete is present.",
        "faceCount is {} but if KTXcubemapIncomplete is present it must be 1."
    };
    static constexpr IssueError KTXcubemapIncompleteInvalidSize{
        7106, "Invalid KTXcubemapIncomplete metadata. The size of the value must be 1 byte.",
        "The size of the KTXcubemapIncomplete value is {} byte(s) but it must be 1 byte."
    };
    static constexpr IssueError KTXorientationMissingNull{
        7107, "Invalid KTXorientation metadata. The value is missing the NULL terminator.",
        "The last byte of the value must be a NULL terminator."
    };
    static constexpr IssueError KTXorientationIncorrectDimension{
        7108, "Invalid KTXorientation value. The number of dimensions specified must match the number of dimensions in the texture type.",
        "The value has {} dimension but the texture type has {} and they must match."
    };
    static constexpr IssueError KTXorientationInvalidValue{
        7109, "Invalid KTXorientation value. The value must match /^[rl]$/ for 1D, /^[rl][du]$/ for 2D and /^[rl][du][oi]$/ for 3D texture types.",
        "Dimension {} is \"{}\" but it must be either \"{}\" or \"{}\"."
    };

    // 71xx - Known Key-Value related issues:

    static constexpr IssueError KTXglFormatInvalidSize{
        7110, "Invalid KTXglFormat metadata. The size of the value must be 12 bytes.",
        "The size of KTXglFormat value is {} byte(s) but it must be 12 bytes."
    };
    static constexpr IssueError KTXglFormatWithVkFormat{
        7111, "Incompatible KTXglFormat and VkFormat. If KTXglFormat is present vkFormat must be VK_FORMAT_UNDEFINED.",
        "vkFormat is {} but it must be VK_FORMAT_UNDEFINED if KTXglFormat is present."
    };
    static constexpr IssueError KTXglFormatInvalidValueForCompressed{
        7112, "Invalid KTXglFormatInvalidValue value. glFormat and glType must be zero for compressed formats.",
        "glFormat is {} and glType is {} but for compressed formats with {} both must be zero."
    };

    static constexpr IssueError KTXdxgiFormatInvalidSize{
        7113, "Invalid KTXdxgiFormat__ metadata. The size of the value must be 4 byte.",
        "The size of KTXdxgiFormat__ value is {} byte(s) but it must be 4 byte."
    };
    static constexpr IssueError KTXdxgiFormatWithVkFormat{
        7114, "Incompatible KTXdxgiFormat__ and VkFormat. If KTXdxgiFormat__ is present vkFormat must be VK_FORMAT_UNDEFINED.",
        "vkFormat is {} but it must be VK_FORMAT_UNDEFINED if KTXdxgiFormat__ is present."
    };

    static constexpr IssueError KTXmetalPixelFormatInvalidSize{
        7115, "Invalid KTXmetalPixelFormat metadata. The size of the value must be 4 byte.",
        "The size of KTXmetalPixelFormat value is {} byte(s) but it must be 4 byte."
    };
    static constexpr IssueError KTXmetalPixelFormatWithVkFormat{
        7116, "Incompatible KTXmetalPixelFormat and VkFormat. If KTXmetalPixelFormat is present vkFormat must be VK_FORMAT_UNDEFINED.",
        "vkFormat is {} but it must be VK_FORMAT_UNDEFINED if KTXmetalPixelFormat is present."
    };

    static constexpr IssueError KTXswizzleMissingNull{
        7117, "Invalid KTXswizzle value. The value is missing the NULL terminator.",
        "The last byte of the value must be a NULL terminator."
    };
    static constexpr IssueError KTXswizzleInvalidSize{
        7118, "Invalid KTXswizzle value. The size of the value must be 5 bytes (including the NULL terminator).",
        "The size of KTXswizzle value is {} byte(s) but it must be 5 bytes (including the NULL terminator)."
    };
    static constexpr IssueError KTXswizzleInvalidValue{
        7119, "Invalid KTXswizzle value. The value must match /^[rgba01]{4}$/.",
        "The character at index {} is \"{}\" but it must be one of \"rgba01\"."
    };
    // Unused 7120
    static constexpr IssueWarning KTXswizzleWithDepthOrStencil{
        7121, "KTXswizzle has no effect on depth or stencil texture formats.",
        "KTXswizzle is present but for VkFormat {} it has no effect."
    };

    static constexpr IssueError KTXwriterMissingNull{
        7122, "Invalid KTXwriter metadata. The value is missing the NULL terminator.",
        "The last byte of the value must be a NULL terminator."
    };
    static constexpr IssueError KTXwriterInvalidUTF8{
        7123, "Invalid KTXwriter value. The value must be a valid UTF8 string.",
        "The value contains an invalid UTF8 character at index: {}."
    };
    static constexpr IssueError KTXwriterRequiredButMissing{
        7124, "Missing KTXwriter metadata. When KTXwriterScParams is present KTXwriter must also be present.",
        "KTXwriter metadata is missing. When KTXwriterScParams is present KTXwriter must also be present."
    };
    static constexpr IssueWarning KTXwriterMissing{
        7125, "Missing KTXwriter metadata. Writers are strongly urged to identify themselves via this.",
        "KTXwriter metadata is missing. Writers are strongly urged to identify themselves via this."
    };

    static constexpr IssueError KTXwriterScParamsMissingNull{
        7126, "Invalid KTXwriterScParams metadata. The value is missing the NULL terminator.",
        "The last byte of the value must be a NULL terminator."
    };
    static constexpr IssueError KTXwriterScParamsInvalidUTF8{
        7127, "Invalid KTXwriterScParams value. The value must be a valid UTF8 string.",
        "The value contains an invalid UTF8 character at index: {}."
    };

    static constexpr IssueError KTXanimDataInvalidSize{
        7128, "Invalid KTXanimData metadata. The size of the value must be 12 bytes.",
        "The size of KTXanimData value is {} byte(s) but it must be 12 bytes."
    };
    static constexpr IssueError KTXanimDataNotArray{
        7129, "Invalid KTXanimData metadata. KTXanimData can only be used with array textures.",
        "KTXanimData is present but with layerCount {} the texture is not an array texture."
    };
    static constexpr IssueError KTXanimDataWithCubeIncomplete{
        7130, "Incompatible KTXanimData and KTXcubemapIncomplete metadata. KTXanimData and KTXcubemapIncomplete cannot be present at the same time.",
        "Both KTXanimData and KTXcubemapIncomplete is present but they are incompatible."
    };

    static constexpr IssueError KTXastcDecodeModeMissingNull{
        7131, "Invalid KTXastcDecodeMode metadata. The value is missing the NULL terminator.",
        "The last byte of the value must be a NULL terminator."
    };
    static constexpr IssueError KTXastcDecodeModeInvalidValue{
        7132, "Invalid KTXastcDecodeMode value. The value must be either \"rgb9e5\" or \"unorm8\".",
        "The value is \"{}\" but the value must be either \"rgb9e5\" or \"unorm8\"."
    };
    static constexpr IssueError KTXastcDecodeModeunorm8NotLDR{
        7133, "Invalid KTXastcDecodeMode value. \"unorm8\" is only valid for LDR formats.",
        "The value is \"unorm8\" but it is invalid for non-LDR VkFormat {}."
    };
    static constexpr IssueWarning KTXastcDecodeModeNotASTC{
        7134, "KTXastcDecodeMode has no effect on and should not be present in KTX files that use a non-ASTC formats.",
        "KTXastcDecodeMode is present but for colorModel {} it has no effect."
    };
    static constexpr IssueWarning KTXastcDecodeModeWithsRGB{
        7135, "KTXastcDecodeMode has no effect on and should not be present in KTX files that use the sRGB transfer function.",
        "KTXastcDecodeMode is present but for transferFunction {} it has no effect."
    };

    // 72xx - GLTF KHR_texture_basisu compatibility

    static constexpr IssueError KTXswizzleInvalidGLTFBU{
        7201, "Invalid KTXswizzle metadata for KHR_texture_basisu compatibility.",
        "KTXswizzle is \"{}\" but must be \"rgba\", if present, for KHR_texture_basisu compatibility."
    };
    static constexpr IssueError KTXorientationInvalidGLTFBU{
        7202, "Invalid KTXorientation metadata for KHR_texture_basisu compatibility.",
        "KTXorientation is \"{}\" but must be \"rd\", if present, for KHR_texture_basis compatibility."
    };
};

struct SGD {
    // 80xx - Generic SGD issues:
    // Currently none

    // 81xx - BASIS_LZ related issues:

    static constexpr IssueError BLZESizeTooSmallHeader{
        8101, "Invalid sgdByteLength for BasisLZ/ETC1S. sgdByteLength must be at least 20 bytes (sizeof ktxBasisLzGlobalHeader).",
        "sgdByteLength is {} but for BasisLZ/ETC1S textures it must be at least 20 bytes (sizeof ktxBasisLzGlobalHeader)."
    };
    static constexpr IssueError BLZESizeIncorrect{
        8102, "Invalid sgdByteLength for BasisLZ/ETC1S. sgdByteLength must be consistent with image count and BasisLzGlobalHeader.",
        "sgdByteLength is {} but based on image count of {} and the BasisLzGlobalHeader the expected value is {} (20 + 20 * imageCount + endpointsByteLength + selectorsByteLength + tablesByteLength + extendedByteLength)."
    };
    static constexpr IssueError BLZEExtendedByteLengthNotZero{
        8103, "Invalid extendedByteLength in BasisLzGlobalHeader. For BasisLZ/ETC1S the extendedByteLength must be 0.",
        "extendedByteLength is {} but for BasisLZ/ETC1S it must be 0."
    };
    static constexpr IssueError BLZEInvalidImageFlagBit{
        8104, "Invalid imageFlags in BasisLzEtc1sImageDesc.",
        "For Level {} Layer {} Face {} zSlice {} the imageFlags is 0x{:08X} which has an invalid bit set."
    };
    static constexpr IssueError BLZENoAnimationSequencesPFrame{
        8105, "Incompatible PFrame with missing KTXanimData. Only animation sequences can have PFrames.",
        "There is a PFrame in a BasisLzEtc1sImageDesc but the KTXanimData is missing."
    };
    static constexpr IssueError BLZEZeroRGBLength{
        8106, "Invalid rgbSliceByteLength in BasisLzEtc1sImageDesc. rgbSliceByteLength must not be 0.",
        "For Level {} Layer {} Face {} zSlice {} the rgbSliceByteLength is {} but it must not be 0."
    };
    static constexpr IssueError BLZEInvalidRGBSlice{
        8107, "Invalid rgbSliceByteOffset or rgbSliceByteLength. The defined byte region must be within the corresponding mip level.",
        "For Level {} Layer {} Face {} zSlice {} the rgbSliceByteOffset is {} and the rgbSliceByteLength is {} but the defined region must fit in the level's byteLength of {}."
    };
    static constexpr IssueError BLZEInvalidAlphaSlice{
        8108, "Invalid alphaSliceByteOffset or alphaSliceByteLength. The defined byte region must be within the corresponding mip level.",
        "For Level {} Layer {} Face {} zSlice {} the alphaSliceByteOffset is {} and the alphaSliceByteLength is {} but the defined region must fit in the level's byteLength of {}."
    };
    static constexpr IssueError BLZEDFDMismatchAlpha{
        8109, "Incompatible alphaSliceByteLength and DFD sampleCount. If DFD indicates an alpha slice the alphaSliceByteLength in BasisLzEtc1sImageDesc must not be 0.",
        "For Level {} Layer {} Face {} zSlice {} the alphaSliceByteLength is 0 but DFD indicates an alpha slice so it must not be 0."
    };
    static constexpr IssueError BLZEDFDMismatchNoAlpha{
        8110, "Incompatible alphaSliceByteLength and DFD sampleCount. If DFD indicates no alpha slice the alphaSliceByteLength in BasisLzEtc1sImageDesc must be 0.",
        "For Level {} Layer {} Face {} zSlice {} the alphaSliceByteLength is {} but DFD indicates no alpha slice so it must be 0."
    };
};

struct System {
    // 90xx - Generic System issues
    static constexpr IssueError OutOfMemory{
        9001, "System ran out of memory during a validation step.",
        "An allocation failed during {} validation: {}."
    };
};

// -------------------------------------------------------------------------------------------------

} // namespace ktx
