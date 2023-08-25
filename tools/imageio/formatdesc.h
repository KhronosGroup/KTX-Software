/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2023 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file formatdesc.h
 * @~English
 *
 * @brief Data Format Descriptor for imageio.
 *
 * @author Mark Callow
 */

#include <exception>
#include <iterator>
#include <string>
#include <vector>

#include <KHR/khr_df.h>


/// @brief Image format descriptor
///
/// Based on Khronos Data Format specification. Omits the parts needed for
/// serialization (size, descriptorType, etc.) and, since there is no worry
/// about writing & reading across compilers and platforms, uses a struct
/// instead of the khr\_df.h macros.
///
/// Note that @e samples are not @e channels, a.k.a @e components. @e samples
/// represent a series of contiguous bits in the bitstream representing
/// a pixel of the image. Since the various ImageInput derived classes
/// convert incoming data to local endianness most @e channels need only
/// a single sample. Only formats such as those with a shared exponent
/// need multiple samples per component.
///
/// This descriptor is way more general than is needed by the current set of
/// supported input formats for which there will always be one sample per
/// channel. We use this because it is familiar from use elsewhere in
/// KTX-Software and because of the large number of useful enums provided
/// by khr\_df.h.
///
/// @note This class uses the Data Format Specification nomenclature of
/// @e channel for consistency with thar spec. Elsewhere in KTX-Software
/// @e component is widely used.
///
struct FormatDescriptor {
    /// @internal
    /// @brief Basic descriptor.
    struct basicDescriptor {
        khr_df_model_e model: 8;
        khr_df_primaries_e primaries: 8;
        khr_df_transfer_e transfer: 8;
        khr_df_flags_e flags: 8;
        uint32_t texelBlockDimension0: 8;
        uint32_t texelBlockDimension1: 8;
        uint32_t texelBlockDimension2: 8;
        uint32_t texelBlockDimension3: 8;
        uint32_t bytesPlane0: 8;
        uint32_t bytesPlane1: 8;
        uint32_t bytesPlane2: 8;
        uint32_t bytesPlane3: 8;
        uint32_t bytesPlane4: 8;
        uint32_t bytesPlane5: 8;
        uint32_t bytesPlane6: 8;
        uint32_t bytesPlane7: 8;

        /// @brief Default constructor
        basicDescriptor() {
            memset(this, 0, sizeof(*this));
        }

        /// @brief Constructor for unpacked, non-compressed textures.
        basicDescriptor(uint32_t pixelByteCount,
                        khr_df_transfer_e t = KHR_DF_TRANSFER_UNSPECIFIED,
                        khr_df_primaries_e p = KHR_DF_PRIMARIES_BT709,
                        khr_df_model_e m = KHR_DF_MODEL_RGBSDA,
                        khr_df_flags_e f = KHR_DF_FLAG_ALPHA_STRAIGHT) {
            model = m;
            primaries = p;
            transfer = t;
            flags = f;
            texelBlockDimension0 = 0; // Uncompressed means only 1x1x1x1 blocks.
            texelBlockDimension1 = 0;
            texelBlockDimension2 = 0;
            texelBlockDimension3 = 0;
            bytesPlane0 = pixelByteCount;
            bytesPlane1 = bytesPlane2 = bytesPlane3 = 0;
            bytesPlane4 = bytesPlane5 = bytesPlane6 = bytesPlane7 = 0;
        }

        bool operator==(const basicDescriptor& rhs) const {
            const uint32_t* a = reinterpret_cast<const uint32_t*>(this);
            const uint32_t* b = reinterpret_cast<const uint32_t*>(&rhs);
            for (uint32_t i = 0; i < 4; i++) {
                if (a[i] != b[i]) return false;
            }
            return true;
        }
        bool operator!=(const basicDescriptor& rhs) const {
            return !(*this == rhs);
        }
    } basic;

    /// @internal
    /// @brief Extended descriptor.
    ///
    /// In a true DFD this would be an extension descriptor type complete with
    /// size, vendorId, descriptorType, etc.
    struct extendedDescriptor {
        uint32_t channelCount; /// Saved channel count to avoid having to loop
                               /// over samples to figure out the count.
        bool sameUnitAllChannels = false; /// All samples have same types and sizes.
        float oeGamma = -1; /// Power function exponent used when the image was
                            /// encoded, if one was used. -1 otherwise.
        /// @internal
        /// @brief ICC profile descriptor.
        struct iccProfileDescriptor {
            std::string name;
            std::vector<uint8_t> profile;

            iccProfileDescriptor() { }
            iccProfileDescriptor(std::string& n, uint8_t* p, size_t ps) {
                name = n;
                profile.resize(ps);
                profile.insert(profile.begin(), p, &p[ps]);
            }

            bool operator==(const iccProfileDescriptor& rhs) const {
                bool result = !this->name.compare(rhs.name);
                return result && (this->profile == rhs.profile);
            }
            bool operator!=(const iccProfileDescriptor& rhs) const {
                return !(*this == rhs);
            }
        } iccProfile;

        extendedDescriptor(uint32_t cc = 0) :
            channelCount(cc) { }

        bool operator==(const extendedDescriptor& rhs) const {
            if (this->channelCount != rhs.channelCount) return false;
            if (this->sameUnitAllChannels != rhs.sameUnitAllChannels) return false;
            if (this->oeGamma != rhs.oeGamma) return false;
            return this->iccProfile == rhs.iccProfile;
        }
        bool operator!=(const extendedDescriptor& rhs) const {
            return !(*this == rhs);
        }
    } extended;

    struct sample {
        uint32_t bitOffset: 16;
        uint32_t bitLength: 8;
        // uint32_t channelType: 8;
        uint32_t channelType: 4;
        uint32_t qualifierLinear: 1;
        uint32_t qualifierExponent: 1;
        uint32_t qualifierSigned: 1;
        uint32_t qualifierFloat: 1;
        uint32_t samplePosition0: 8;
        uint32_t samplePosition1: 8;
        uint32_t samplePosition2: 8;
        uint32_t samplePosition3: 8;
        uint32_t lower;
        uint32_t upper;

        bool operator==(const sample& rhs) const {
            const uint32_t* a = reinterpret_cast<const uint32_t*>(this);
            const uint32_t* b = reinterpret_cast<const uint32_t*>(&rhs);
            for (uint32_t i = 0; i < 4; i++) {
                if (a[i] != b[i]) return false;
            }
            return true;
        }

        /// @brief Constructs an uninitialized sample object
        sample() = default;

        /// @brief Construct a sample with default sampleUpper and sampleLower.
        ///
        /// For uncompressed formats. Handle integer data as normalized. For
        /// unsigned use the full range of the number of bits. For signed set
        /// sampleUpper and sampleLower so 0 is representable.
        sample(uint32_t chanType,
               uint32_t bLength, uint32_t offset,
               khr_df_sample_datatype_qualifiers_e dataType
                  = static_cast<khr_df_sample_datatype_qualifiers_e>(0),
               khr_df_transfer_e oetf = KHR_DF_TRANSFER_UNSPECIFIED,
               khr_df_model_e m = KHR_DF_MODEL_RGBSDA) {
            bitOffset = offset;
            bitLength = bLength - 1;
            channelType = chanType;
            if (channelType == 3 && m != KHR_DF_MODEL_XYZW) {
                /// XYZW does not have an alpha chennel. *_ALPHA has the same
                /// value for all other 4-channel-capable uncompressed models.
                channelType = KHR_DF_CHANNEL_RGBSDA_ALPHA;
            }
            qualifierFloat = (dataType & KHR_DF_SAMPLE_DATATYPE_FLOAT) != 0;
            qualifierSigned = (dataType & KHR_DF_SAMPLE_DATATYPE_SIGNED) != 0;
            qualifierExponent = (dataType & KHR_DF_SAMPLE_DATATYPE_EXPONENT) != 0;
            qualifierLinear = (dataType & KHR_DF_SAMPLE_DATATYPE_LINEAR) != 0;
            if (oetf > KHR_DF_TRANSFER_LINEAR
                && channelType == KHR_DF_CHANNEL_RGBSDA_ALPHA) {
                channelType |= KHR_DF_SAMPLE_DATATYPE_LINEAR;
            }

            union {
                uint32_t i;
                float f;
            } uLower, uUpper;
            if (qualifierFloat) {
                if (qualifierSigned) {
                    uUpper.f = 1.0f;
                    uLower.f = -1.0f;
                } else {
                    uUpper.f = 1.0f;
                    uLower.f = 0.0f;
                }
            } else {
                if (qualifierSigned) {
                    // signed normalized
                    if (bitLength > 32) {
                        uUpper.i = 0x7FFFFFFF;
                    } else {
                        uUpper.i = (1U << (bLength - 1)) - 1;
                    }
                    uLower.i = ~uUpper.i;
                    uLower.i += 1;
                } else {
                    // unsigned normalized
                    if (bitLength > 32) {
                        uUpper.i = 0xFFFFFFFFU;
                    } else {
                        uUpper.i = (uint32_t)((1U << bLength) - 1U);
                    }
                    uLower.i = 0U;
                }
            }
            lower = uLower.i;
            upper = uUpper.i;
        }

        /// @brief Construct a sample with custom sampleLower and sampleUpper.
        ///
        /// For uncompressed formats.
        sample(uint32_t chanType,
               uint32_t bitLength, uint32_t offset,
               uint32_t sampleLower, uint32_t sampleUpper,
               khr_df_sample_datatype_qualifiers_e dataType
                  = static_cast<khr_df_sample_datatype_qualifiers_e>(0),
               khr_df_transfer_e oetf = KHR_DF_TRANSFER_UNSPECIFIED,
               khr_df_model_e m = KHR_DF_MODEL_RGBSDA)
               : sample(chanType, bitLength, offset, dataType, oetf, m)
        {
            if (qualifierFloat) {
                throw std::runtime_error(
                    "Invalid use of constructor for float data"
                );
            }
            lower = sampleLower;
            upper = sampleUpper;
        }
    };
    std::vector<sample> samples;

    /// @brief Default constructor
    ///
    /// Will have zero samples which means format unknown.
    FormatDescriptor() { }

    /// @brief Constructor for unpacked, non-compressed data.
    ///
    /// All channels have the same number of bits and basic data type.
    /// As all wide data types will be in local endianness we need only
    /// one sample per channel.
    ///
    /// Data is assumed to be unsigned normalized. @c sampleUpper will be
    /// set to the max value representable by @a channelBitLength.
    ///
    /// @c channelType will be set to the standard channel types for @a channelCount
    /// and @a m.
    FormatDescriptor(uint32_t channelCount, uint32_t channelBitLength,
               khr_df_sample_datatype_qualifiers_e dt
                  = static_cast<khr_df_sample_datatype_qualifiers_e>(0),
               khr_df_transfer_e t = KHR_DF_TRANSFER_UNSPECIFIED,
               khr_df_primaries_e p = KHR_DF_PRIMARIES_BT709,
               khr_df_model_e m = KHR_DF_MODEL_RGBSDA,
               khr_df_flags_e f = KHR_DF_FLAG_ALPHA_STRAIGHT)
          : basic((channelBitLength * channelCount) / 8, t, p, m, f),
            extended(channelCount)
    {
        for (uint32_t s = 0; s < channelCount; s++) {
            samples.push_back(sample(s, channelBitLength,
                                     s * channelBitLength,
                                     dt, t, m));
        }
        if (m == KHR_DF_MODEL_YUVSDA && channelCount == 2) {
            samples[1].channelType = KHR_DF_CHANNEL_YUVSDA_ALPHA;
        }
        extended.sameUnitAllChannels = true;
    }

    /// @brief Constructor for unpacked, non-compressed data with custom
    ///        sampleLower and sampleUpper
    ///
    /// All channels have the same number of bits and basic data type.
    /// Use this for unnormalized integer data or normalized data that does not
    /// use the full range representable by @a channelBitLength.
    FormatDescriptor(uint32_t channelCount, uint32_t channelBitLength,
               uint32_t sampleLower, uint32_t sampleUpper,
               khr_df_sample_datatype_qualifiers_e dt
                  = static_cast<khr_df_sample_datatype_qualifiers_e>(0),
               khr_df_transfer_e t = KHR_DF_TRANSFER_UNSPECIFIED,
               khr_df_primaries_e p = KHR_DF_PRIMARIES_BT709,
               khr_df_model_e m = KHR_DF_MODEL_RGBSDA,
               khr_df_flags_e f = KHR_DF_FLAG_ALPHA_STRAIGHT)
          : basic((channelBitLength * channelCount) / 8, t, p, m, f),
            extended(channelCount)
    {
        for (uint32_t s = 0; s < channelCount; s++) {
            samples.push_back(sample(s, channelBitLength,
                                     s * channelBitLength,
                                     sampleLower, sampleUpper,
                                     dt, t, m));
        }
        if (m == KHR_DF_MODEL_YUVSDA && channelCount == 2) {
            samples[1].channelType = KHR_DF_CHANNEL_YUVSDA_ALPHA;
        }
        extended.sameUnitAllChannels = true;
    }

    static uint32_t totalBits(uint32_t sampleCount, std::vector<uint32_t>& bits) {
        uint32_t totalBits = 0;
        for (uint32_t s = 0; s < sampleCount; s++) {
            totalBits += bits[s];
        }
        return totalBits;
    }

    /// @brief Constructor for non-compressed textures with varying bit lengths or channel types.
    ///
    /// Each channel has the same basic data type.
    FormatDescriptor(uint32_t channelCount,
               std::vector<uint32_t>& channelBitLengths,
               std::vector<khr_df_model_channels_e>& channelTypes,
               khr_df_sample_datatype_qualifiers_e dt
                  = static_cast<khr_df_sample_datatype_qualifiers_e>(0),
               khr_df_transfer_e t = KHR_DF_TRANSFER_UNSPECIFIED,
               khr_df_primaries_e p = KHR_DF_PRIMARIES_BT709,
               khr_df_model_e m = KHR_DF_MODEL_RGBSDA,
               khr_df_flags_e f = KHR_DF_FLAG_ALPHA_STRAIGHT)
          : basic(totalBits(channelCount, channelBitLengths) >> 3, t, p, m, f),
            extended(channelCount)
    {
        if (channelCount > channelBitLengths.size()
            || channelCount > channelTypes.size()) {
            throw std::runtime_error(
                "Not enough channelBits or channelType specfications."
            );
        }
        uint32_t bitOffset = 0;
        bool bitLengthsEqual = true;
        uint32_t firstBitLength = channelBitLengths[0];
        for (uint32_t s = 0; s < channelCount; s++) {
            samples.push_back(sample(channelTypes[s], channelBitLengths[s],
                                     bitOffset, dt, t, m));
            bitOffset += channelBitLengths[s];
            if (firstBitLength != channelBitLengths[s]) {
                bitLengthsEqual = false;
            }
        }
        if (bitLengthsEqual) {
          extended.sameUnitAllChannels = true;
        }
        if (m == KHR_DF_MODEL_YUVSDA && channelCount == 2) {
            samples[1].channelType = KHR_DF_CHANNEL_YUVSDA_ALPHA;
        }
    }

    /// @brief Constructor for non-compressed textures with varying bit lengths
    ///        or channel types and custom sampleLower and sampleUpper.
    ///
    /// Each channel has the same basic data type.  Use this for unnormalized
    /// integer data or normalized data that does not use the full bit range.
    FormatDescriptor(uint32_t channelCount,
               std::vector<uint32_t>& channelBitLengths,
               std::vector<khr_df_model_channels_e>& channelTypes,
               std::vector<uint32_t>& samplesLower,
               std::vector<uint32_t>& samplesUpper,
               khr_df_sample_datatype_qualifiers_e dt
                  = static_cast<khr_df_sample_datatype_qualifiers_e>(0),
               khr_df_transfer_e t = KHR_DF_TRANSFER_UNSPECIFIED,
               khr_df_primaries_e p = KHR_DF_PRIMARIES_BT709,
               khr_df_model_e m = KHR_DF_MODEL_RGBSDA,
               khr_df_flags_e f = KHR_DF_FLAG_ALPHA_STRAIGHT)
          : basic(totalBits(channelCount, channelBitLengths) >> 3, t, p, m, f),
            extended(channelCount)
    {
        if (channelCount > channelBitLengths.size()
            || channelCount > channelTypes.size()) {
            throw std::runtime_error(
                "Not enough channelBits or channelType specfications."
            );
        }
        uint32_t bitOffset = 0;
        bool bitLengthsEqual = true;
        uint32_t firstBitLength = channelBitLengths[0];
        for (uint32_t s = 0; s < channelCount; s++) {
            samples.push_back(sample(channelTypes[s], channelBitLengths[s],
                                     samplesLower[s], samplesUpper[s],
                                     bitOffset, dt, t, m));
            bitOffset += channelBitLengths[s];
            if (firstBitLength != channelBitLengths[s]) {
                bitLengthsEqual = false;
            }
        }
        if (bitLengthsEqual) {
          extended.sameUnitAllChannels = true;
        }
        if (m == KHR_DF_MODEL_YUVSDA && channelCount == 2) {
            samples[1].channelType = KHR_DF_CHANNEL_YUVSDA_ALPHA;
        }
    }

    /// @brief Constructor for non-compressed, shared exponent format.
    ///
    /// Each channel is a floating point. All channels share the same exponent
    /// and have the same number of mantissa bits.
    // TODO: Handle whether there is an implicit 1 and a sign bit.
    FormatDescriptor(uint32_t channelCount,
               uint32_t mantissaBitLength,
               uint32_t exponentBitLength,
               khr_df_sample_datatype_qualifiers_e dt
                  = static_cast<khr_df_sample_datatype_qualifiers_e>(0),
               khr_df_transfer_e t = KHR_DF_TRANSFER_LINEAR,
               khr_df_primaries_e p = KHR_DF_PRIMARIES_BT709,
               khr_df_model_e m = KHR_DF_MODEL_RGBSDA,
               khr_df_flags_e f = KHR_DF_FLAG_ALPHA_STRAIGHT)
          : basic((channelCount * mantissaBitLength + exponentBitLength) >> 3,
                   t, p, m, f),
            extended(channelCount)
    {
        if (dt & KHR_DF_SAMPLE_DATATYPE_FLOAT) {
            throw std::runtime_error(
                "DATATYPE_FLOAT is set for a shared exponent format");
        }
        for (uint32_t s = 0; s < channelCount; s++) {
            uint32_t sampleLower = 0, sampleUpper;
            // sampleUpper and sampleLower values for the mantissa should be
            // set to indicate the representation of 1.0 and 0.0 (for unsigned
            // formats) or -1.0 (for signed formats) respectively when the
            // exponent is in a 0 position after any bias has been corrected.
            // If there is an implicit 1 bit, these values for the mantissa
            // will exceed what can be represented in the number of available
            // mantissa bits.
            sampleUpper = 1U << mantissaBitLength;
            samples.push_back(sample(s, mantissaBitLength,
                                     s * mantissaBitLength,
                                     sampleLower, sampleUpper,
                                     dt, t, m));
            // The sampleLower for the exponent should indicate the exponent
            // bias. That is, the mantissa should be scaled by two raised to
            // the power of the stored exponent minus this sampleLower value.
            //
            // The sampleUpper for the exponent indicates the maximum legal
            // exponent value. Values above this are used to encode infinities
            // and not-a-number (NaN) values. sampleUpper can therefore be used
            // to indicate whether or not the format supports these encodings.
            //sampleLower = exponentBias;
            //sampleUpper = maxLegalExponentValue;
            samples.push_back(sample(s, exponentBitLength,
                                     channelCount * mantissaBitLength,
                                     sampleLower, sampleUpper,
                                     static_cast<khr_df_sample_datatype_qualifiers_e>(dt | KHR_DF_SAMPLE_DATATYPE_EXPONENT),
                                     t, m));
        }
        extended.sameUnitAllChannels = true;
    }

    /// @brief Constructor from pre-constructed basic and sample descriptors
    FormatDescriptor(
            FormatDescriptor::basicDescriptor basic,
            std::vector<FormatDescriptor::sample> samples_)
          : basic(basic),
            extended(static_cast<uint32_t>(samples_.size())),
            samples(std::move(samples_))
    {
        extended.sameUnitAllChannels = true;

        if (!samples.empty()) {
            for (uint32_t i = 1; i < static_cast<uint32_t>(samples.size()); ++i) {
                if (samples[0].bitLength != samples[i].bitLength
                        || samples[0].qualifierLinear != samples[i].qualifierLinear
                        || samples[0].qualifierExponent != samples[i].qualifierExponent
                        || samples[0].qualifierSigned != samples[i].qualifierSigned
                        || samples[0].qualifierFloat != samples[i].qualifierFloat) {
                    extended.sameUnitAllChannels = false;
                    break;
                }
            }
        }
    }

    bool isUnknown() const noexcept {
        return samples.size() == 0;
    }
    bool sameUnitAllChannels() const noexcept {
        return extended.sameUnitAllChannels;
    }

    bool operator==(const FormatDescriptor& rhs) const {
        if (this->basic != rhs.basic) return false;
        if (this->extended != rhs.extended) return false;
        return this->samples == rhs.samples;
    }
    bool operator!=(const FormatDescriptor& rhs) const {
        return !(*this == rhs);
    }

    khr_df_model_e model() const noexcept {
        return static_cast<khr_df_model_e>(basic.model);
    }
    khr_df_primaries_e primaries() const noexcept {
        return static_cast<khr_df_primaries_e>(basic.primaries);
    }
    khr_df_transfer_e transfer() const noexcept {
        return static_cast<khr_df_transfer_e>(basic.transfer);
    }
    khr_df_flags_e flags() const noexcept {
        return static_cast<khr_df_flags_e>(basic.flags);
    }
    float oeGamma() const noexcept {
        return extended.oeGamma;
    }
    const std::string& iccProfileName() const noexcept {
        return extended.iccProfile.name;
    }
    const std::vector<uint8_t>& iccProfile() const noexcept {
        return extended.iccProfile.profile;
    }
    void setModel(khr_df_model_e m) {
        basic.model = m;
    }
    void setPrimaries(khr_df_primaries_e p) {
        basic.primaries = p;
    }
    void setTransfer(khr_df_transfer_e t) {
        khr_df_transfer_e oldOetf = basic.transfer;
        basic.transfer = t;
        if ((oldOetf <= KHR_DF_TRANSFER_LINEAR) != (t <= KHR_DF_TRANSFER_LINEAR))
        {
            std::vector<sample>::iterator sit = samples.begin();
            for (; sit < samples.end(); sit++) {
                if (sit->channelType == KHR_DF_CHANNEL_RGBSDA_ALPHA) {
                    sit->qualifierLinear = t > KHR_DF_TRANSFER_LINEAR;
                }
            }
        }
    }
    uint32_t pixelByteCount() const noexcept {
        return basic.bytesPlane0;
    }
    uint32_t sampleCount() const noexcept {
        return static_cast<uint32_t>(samples.size());
    }
    uint32_t sampleBitLength(uint32_t s) const noexcept {
        return samples[s].bitLength + 1;
    }
    // TODO: remove?
    uint32_t sampleByteCount(uint32_t s) const noexcept {
        // Use integer division so 0 is returned when length is < a byte.
        return sampleBitLength(s) / 8;
    }
    uint32_t sampleUpper(uint32_t s) const noexcept {
       return samples[s].upper;
    }
    uint32_t channelCount() const noexcept {
        return extended.channelCount;
    }
    uint32_t channelBitLength(khr_df_model_channels_e c) const {
        std::vector<sample>::const_iterator it = samples.begin();
        uint32_t bitLength = 0;
        for (; it < samples.end(); it++) {
            if (it->channelType == static_cast<uint32_t>(c)) {
                bitLength += it->bitLength + 1;
            }
        }
        if (bitLength == 0) {
            throw std::runtime_error("No such channel.");
        }
        return bitLength;
    }
    uint32_t channelBitLength() const {
        if (!extended.sameUnitAllChannels) {
            throw std::runtime_error(
                "Differing size channels. Specify channel to query."
            );
        }
        return channelBitLength(KHR_DF_CHANNEL_RGBSDA_R);
    }
    uint32_t largestChannelBitLength() const {
        uint32_t maxBitLength = 0;
        for (uint32_t i = 0; i < 16; ++i) {
            uint32_t bitLength = 0;
            for (const auto& sample : samples)
                if (sample.channelType == i)
                    bitLength += sample.bitLength + 1;

            if (bitLength > maxBitLength)
                maxBitLength = bitLength;
        }
        return maxBitLength;
    }
    bool anyChannelBitLengthNotEqual(uint32_t bitLength) const {
        for (uint32_t i = 0; i < 16; ++i) {
            uint32_t channelBitLength = 0;
            for (const auto& sample : samples)
                if (sample.channelType == i)
                    channelBitLength += sample.bitLength + 1;

            if (bitLength != channelBitLength)
                return true;
        }
        return false;
    }
    khr_df_sample_datatype_qualifiers_e
    channelDataType(khr_df_model_channels_e c) const {
        // TODO: Fix for shared exponent case...
        std::vector<sample>::const_iterator it = samples.begin();
        for (; it < samples.end(); it++) {
            if (it->channelType == static_cast<uint32_t>(c)) {
                return static_cast<khr_df_sample_datatype_qualifiers_e>
                   (it->channelType & KHR_DF_SAMPLEMASK_QUALIFIERS);
            }
        }
        throw std::runtime_error("No such channel.");
    }

    khr_df_sample_datatype_qualifiers_e channelDataType() const {
        if (!extended.sameUnitAllChannels) {
            throw std::runtime_error(
                "Differing size channels. Specify channel to query."
            );
        }
        return channelDataType(KHR_DF_CHANNEL_RGBSDA_R);
    }
    uint32_t channelUpper() const {
        if (extended.channelCount != samples.size()) {
            throw std::runtime_error(
                "Multiple samples per channel. Call sampleUpper(uint32_t s)."
            );
        }
        if (!extended.sameUnitAllChannels) {
            throw std::runtime_error(
                "Differing size channels. Call sampleUpper(uint32_t s)."
            );
        }
        return sampleUpper(KHR_DF_CHANNEL_RGBSDA_R);
    }
    void updateSampleInfo(uint32_t channelCount, uint32_t channelBitLength,
                  uint32_t sampleLower, uint32_t sampleUpper,
                  khr_df_sample_datatype_qualifiers_e dt
                      = static_cast<khr_df_sample_datatype_qualifiers_e>(0))
    {
        samples.clear();
        for (uint32_t s = 0; s < channelCount; s++) {
            samples.push_back(sample(s, channelBitLength,
                                     s * channelBitLength,
                                     sampleLower, sampleUpper,
                                     dt, basic.transfer, basic.model));
        }
        if (basic.model == KHR_DF_MODEL_YUVSDA && channelCount == 2) {
            samples[1].channelType = KHR_DF_CHANNEL_YUVSDA_ALPHA;
        }
        extended.channelCount = channelCount;
        extended.sameUnitAllChannels = true;
    }

    void updateSampleBitCounts(std::vector<uint32_t>& bits) {
        uint32_t b, s;
        uint32_t offset = 0;
        for (b = 0, s = 0; s < samples.size(); s++) {
            samples[s].bitLength = bits[b] - 1;
            samples[s].bitOffset = offset;
            offset += bits[b];
            if (b < bits.size() - 1)
                b++;
            // else set remaining sample sizes to last available bits value.
        }
    }
    void updateChannelCount(uint32_t newCount) {
        if (newCount == extended.channelCount)
            return;
        if (extended.channelCount != samples.size()) {
            // TODO: Either fix error handling or implement
            throw std::runtime_error(
               "changeChannelCount not supported when # samples != # channels");
        }
        extended.channelCount = newCount;
        if (newCount < samples.size()) {
            samples.erase(samples.begin() + newCount, samples.end());
            return;
        }
        uint32_t firstNewIndex = static_cast<uint32_t>(samples.size());
        uint32_t offset = samples.back().bitOffset
                        + samples.back().bitLength + 1;
        samples.resize(newCount, samples.back());
        std::vector<sample>::iterator sit = samples.begin() + firstNewIndex;
        for (; sit < samples.end(); sit++) {
            sit->bitOffset = offset;
            offset += (sit->bitLength + 1);
        }
    }

    [[nodiscard]] const sample* find(khr_df_model_channels_e channel) const {
        for (const auto& sample : samples)
            if (sample.channelType == static_cast<uint32_t>(channel))
                return &sample;
        return nullptr;
    }

    void removeLastChannel() {
        const auto numChannels = static_cast<uint32_t>(samples.size());
        assert(numChannels > 1);
        assert(basic.bytesPlane0 % numChannels == 0);
        samples.pop_back();
        basic.bytesPlane0 = basic.bytesPlane0 / numChannels * (numChannels - 1u);
        if (extended.channelCount != 0)
            --extended.channelCount;
    }

    friend std::ostream& operator<< (std::ostream& o, khr_df_sample_datatype_qualifiers_e q) {
        if (q & KHR_DF_SAMPLE_DATATYPE_SIGNED)
            o << " signed ";
        if (q & KHR_DF_SAMPLE_DATATYPE_FLOAT) {
            if (!(q & KHR_DF_SAMPLE_DATATYPE_SIGNED))
                o << " ";
            o << "float ";
        }
        return o;
    }
};
