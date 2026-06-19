//
//  EmulatedDouble.h
//  EmulatedDouble
//
//  Software-emulated IEEE 754 binary64 storage shared by Swift and Metal.
//
//  MIT License
//
//  Copyright (c) 2026 Yuki Kuwashima
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.

#ifndef EMULATED_DOUBLE_H
#define EMULATED_DOUBLE_H

#if !defined(__METAL_VERSION__)
#include <stdint.h>
#include <string.h>
#endif

#if defined(__METAL_VERSION__)
#pragma METAL fp math_mode(safe)
#pragma METAL fp contract(off)
#endif

/// Pointer address-space qualifier used by helpers shared between Metal and C-family importers.
///
/// Metal requires explicit address-space qualifiers for pointer parameters. C, C++, Objective-C,
/// and Swift's Clang importer do not know the Metal `thread` keyword, so this macro expands to an
/// empty qualifier outside Metal.
#if defined(__METAL_VERSION__)
#define EMULATED_DOUBLE_THREAD thread
#else
#define EMULATED_DOUBLE_THREAD
#endif

/// Mask for the sign bit stored in the high 32 bits of an IEEE 754 binary64 value.
#define EMULATED_DOUBLE_SIGN_MASK 0x80000000u

/// Mask for the 11-bit exponent field stored in the high 32 bits of an IEEE 754 binary64 value.
#define EMULATED_DOUBLE_EXPONENT_MASK 0x7FF00000u

/// Mask for the upper 20 bits of the binary64 fraction field stored in `EmulatedDouble.highBits`.
#define EMULATED_DOUBLE_FRACTION_HIGH_MASK 0x000FFFFFu

/// Number of bits to shift `EmulatedDouble.highBits` right to obtain the binary64 exponent field.
#define EMULATED_DOUBLE_EXPONENT_SHIFT 20u

/// IEEE 754 binary64 exponent bias.
#define EMULATED_DOUBLE_EXPONENT_BIAS 1023u

/// All-ones IEEE 754 binary64 exponent value used by infinities and NaNs.
#define EMULATED_DOUBLE_EXPONENT_MAX 0x7FFu

/// Number of explicit fraction bits in an IEEE 754 binary64 value.
#define EMULATED_DOUBLE_FRACTION_BITS 52u

/// Number of guard, round, and sticky bits carried by arithmetic rounding paths.
#define EMULATED_DOUBLE_ROUNDING_BITS 3u

/// Quiet-NaN payload bit stored in the upper fraction word of an IEEE 754 binary64 value.
#define EMULATED_DOUBLE_QUIET_NAN_BIT 0x00080000u

/// Mask for the sign bit stored in an IEEE 754 binary32 bit pattern.
#define EMULATED_FLOAT_SIGN_MASK 0x80000000u

/// Mask for the 8-bit exponent field stored in an IEEE 754 binary32 bit pattern.
#define EMULATED_FLOAT_EXPONENT_MASK 0x7F800000u

/// Mask for the 23-bit fraction field stored in an IEEE 754 binary32 bit pattern.
#define EMULATED_FLOAT_FRACTION_MASK 0x007FFFFFu

/// Number of bits to shift a binary32 bit pattern right to obtain its exponent field.
#define EMULATED_FLOAT_EXPONENT_SHIFT 23u

/// IEEE 754 binary32 exponent bias.
#define EMULATED_FLOAT_EXPONENT_BIAS 127u

/// All-ones IEEE 754 binary32 exponent value used by infinities and NaNs.
#define EMULATED_FLOAT_EXPONENT_MAX 0xFFu

/// Stores an IEEE 754 binary64 value as two 32-bit words for use from Swift, C, C++, and Metal.
///
/// `highBits` contains the sign bit, the 11-bit exponent, and the upper 20 bits of the fraction.
/// `lowBits` contains the lower 32 bits of the fraction. The represented bit pattern is equivalent
/// to `(uint64_t(highBits) << 32) | lowBits`, but the layout intentionally avoids requiring
/// 64-bit integer math in Metal kernels.
///
/// Quality: Covered by `emulatedDoubleMakePreservesWordsAndAccessors` and
/// `emulatedDoubleClassifiesBinary64SpecialValues`.
typedef struct EmulatedDouble {
    /// Lower 32 bits of the IEEE 754 binary64 fraction field.
    uint32_t lowBits;

    /// Sign bit, exponent field, and upper 20 bits of the IEEE 754 binary64 fraction field.
    uint32_t highBits;
} EmulatedDouble;

/// Creates an `EmulatedDouble` from its high and low 32-bit words.
///
/// - Parameters:
///   - highBits: Sign, exponent, and upper fraction bits of the binary64 representation.
///   - lowBits: Lower fraction bits of the binary64 representation.
/// - Returns: A value that preserves the supplied bit pattern without normalization or validation.
///
/// Quality: Covered by `emulatedDoubleMakePreservesWordsAndAccessors`.
static inline EmulatedDouble EmulatedDoubleMake(uint32_t highBits, uint32_t lowBits) {
    EmulatedDouble value;
    value.lowBits = lowBits;
    value.highBits = highBits;
    return value;
}

/// Returns the sign field of an `EmulatedDouble`.
///
/// - Parameter value: The encoded binary64 value to inspect.
/// - Returns: `1` for negative values and `0` for positive values, including signed zeroes.
///
/// Quality: Covered by `emulatedDoubleMakePreservesWordsAndAccessors` and
/// `emulatedDoubleClassifiesBinary64SpecialValues`.
static inline uint32_t EmulatedDoubleSignBit(EmulatedDouble value) {
    return value.highBits >> 31u;
}

/// Returns the biased exponent field of an `EmulatedDouble`.
///
/// - Parameter value: The encoded binary64 value to inspect.
/// - Returns: The raw 11-bit IEEE 754 binary64 exponent field.
///
/// Quality: Covered by `emulatedDoubleMakePreservesWordsAndAccessors` and
/// `emulatedDoubleClassifiesBinary64SpecialValues`.
static inline uint32_t EmulatedDoubleExponentBits(EmulatedDouble value) {
    return (value.highBits & EMULATED_DOUBLE_EXPONENT_MASK) >> EMULATED_DOUBLE_EXPONENT_SHIFT;
}

/// Returns the upper 20 bits of the binary64 fraction field.
///
/// - Parameter value: The encoded binary64 value to inspect.
/// - Returns: The fraction bits stored in `EmulatedDouble.highBits`, with sign and exponent removed.
///
/// Quality: Covered by `emulatedDoubleMakePreservesWordsAndAccessors`.
static inline uint32_t EmulatedDoubleFractionHighBits(EmulatedDouble value) {
    return value.highBits & EMULATED_DOUBLE_FRACTION_HIGH_MASK;
}

/// Returns the lower 32 bits of the binary64 fraction field.
///
/// - Parameter value: The encoded binary64 value to inspect.
/// - Returns: The fraction bits stored in `EmulatedDouble.lowBits`.
///
/// Quality: Covered by `emulatedDoubleMakePreservesWordsAndAccessors`.
static inline uint32_t EmulatedDoubleFractionLowBits(EmulatedDouble value) {
    return value.lowBits;
}

/// Reports whether the binary64 fraction field is zero.
///
/// - Parameter value: The encoded binary64 value to inspect.
/// - Returns: `1` when all 52 fraction bits are zero, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleMakePreservesWordsAndAccessors` and
/// `emulatedDoubleClassifiesBinary64SpecialValues`.
static inline uint32_t EmulatedDoubleHasZeroFraction(EmulatedDouble value) {
    return (EmulatedDoubleFractionHighBits(value) | value.lowBits) == 0u ? 1u : 0u;
}

/// Reports whether an `EmulatedDouble` encodes positive or negative zero.
///
/// - Parameter value: The encoded binary64 value to classify.
/// - Returns: `1` for `+0.0` or `-0.0`, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleClassifiesBinary64SpecialValues`.
static inline uint32_t EmulatedDoubleIsZero(EmulatedDouble value) {
    return ((value.highBits & ~EMULATED_DOUBLE_SIGN_MASK) | value.lowBits) == 0u ? 1u : 0u;
}

/// Reports whether an `EmulatedDouble` encodes a binary64 subnormal value.
///
/// - Parameter value: The encoded binary64 value to classify.
/// - Returns: `1` when the exponent is zero and the fraction is nonzero, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleClassifiesBinary64SpecialValues`.
static inline uint32_t EmulatedDoubleIsSubnormal(EmulatedDouble value) {
    return EmulatedDoubleExponentBits(value) == 0u && !EmulatedDoubleHasZeroFraction(value) ? 1u : 0u;
}

/// Reports whether an `EmulatedDouble` encodes positive or negative infinity.
///
/// - Parameter value: The encoded binary64 value to classify.
/// - Returns: `1` when the exponent is all ones and the fraction is zero, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleClassifiesBinary64SpecialValues`.
static inline uint32_t EmulatedDoubleIsInfinity(EmulatedDouble value) {
    return EmulatedDoubleExponentBits(value) == EMULATED_DOUBLE_EXPONENT_MAX &&
        EmulatedDoubleHasZeroFraction(value) ? 1u : 0u;
}

/// Reports whether an `EmulatedDouble` encodes a NaN.
///
/// - Parameter value: The encoded binary64 value to classify.
/// - Returns: `1` when the exponent is all ones and the fraction is nonzero, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleClassifiesBinary64SpecialValues`.
static inline uint32_t EmulatedDoubleIsNaN(EmulatedDouble value) {
    return EmulatedDoubleExponentBits(value) == EMULATED_DOUBLE_EXPONENT_MAX &&
        !EmulatedDoubleHasZeroFraction(value) ? 1u : 0u;
}

/// Counts the leading zero bits in a 32-bit unsigned integer.
///
/// - Parameter value: The integer to inspect.
/// - Returns: A value in `0...32`; returns `32` when `value` is zero.
///
/// Quality: Covered by `emulatedDoubleCountsLeadingZeros`.
static inline uint32_t EmulatedDoubleCountLeadingZeros32(uint32_t value) {
#if !defined(__METAL_VERSION__) && (defined(__clang__) || defined(__GNUC__))
    return value == 0u ? 32u : (uint32_t)__builtin_clz(value);
#else
    uint32_t count = 0u;
    for (uint32_t bit = 0x80000000u; bit != 0u && (value & bit) == 0u; bit >>= 1u) {
        count += 1u;
    }
    return count;
#endif
}

/// Packs a binary32 fraction into the fraction field of an `EmulatedDouble`.
///
/// This helper is used by `EmulatedDoubleFromFloatBits` after the binary32 sign and target
/// binary64 exponent have already been computed.
///
/// - Parameters:
///   - sign: The binary64 sign bit already positioned in bit 31 of the high word.
///   - exponent: The target biased binary64 exponent.
///   - fraction: The source binary32 fraction payload, without any hidden leading bit.
///   - fractionShift: Number of positions to shift the source fraction into the 52-bit binary64 field.
/// - Returns: An `EmulatedDouble` containing the supplied sign, exponent, and shifted fraction.
///
/// Quality: Covered by `emulatedDoublePackFromFloatFractionPlacesHighAndLowFractionBits` and
/// `emulatedDoubleFromFloatBitsPreservesRepresentativeValues`.
static inline EmulatedDouble EmulatedDoublePackFromFloatFraction(
    uint32_t sign,
    uint32_t exponent,
    uint32_t fraction,
    uint32_t fractionShift
) {
    uint32_t highFraction = 0u;
    uint32_t lowFraction = 0u;

    if (fractionShift >= 32u) {
        highFraction = fraction << (fractionShift - 32u);
    } else {
        highFraction = fraction >> (32u - fractionShift);
        lowFraction = fraction << fractionShift;
    }

    return EmulatedDoubleMake(
        sign | (exponent << EMULATED_DOUBLE_EXPONENT_SHIFT) | highFraction,
        lowFraction
    );
}

/// Converts an IEEE 754 binary32 bit pattern into an `EmulatedDouble` bit pattern.
///
/// The conversion is exact for all binary32 values, including zeroes, subnormals, normal finite
/// values, infinities, and NaNs. NaN payload bits are shifted into the binary64 fraction field.
///
/// - Parameter bits: Raw IEEE 754 binary32 bits.
/// - Returns: The corresponding IEEE 754 binary64 encoding stored as `EmulatedDouble`.
///
/// Quality: Covered by `emulatedDoubleFromFloatBitsPreservesRepresentativeValues`,
/// `emulatedDoubleRoundTripsFloatBits`, and
/// `emulatedDoubleFloatValueConversionRoundTripsBitPatterns`.
static inline EmulatedDouble EmulatedDoubleFromFloatBits(uint32_t bits) {
    uint32_t sign = bits & EMULATED_FLOAT_SIGN_MASK;
    uint32_t exponent = (bits & EMULATED_FLOAT_EXPONENT_MASK) >> EMULATED_FLOAT_EXPONENT_SHIFT;
    uint32_t fraction = bits & EMULATED_FLOAT_FRACTION_MASK;

    if (exponent == EMULATED_FLOAT_EXPONENT_MAX) {
        return EmulatedDoublePackFromFloatFraction(
            sign,
            EMULATED_DOUBLE_EXPONENT_MAX,
            fraction,
            29u
        );
    }

    if (exponent == 0u) {
        if (fraction == 0u) {
            return EmulatedDoubleMake(sign, 0u);
        }

        uint32_t highestBitIndex = 31u - EmulatedDoubleCountLeadingZeros32(fraction);
        uint32_t normalizedExponent = highestBitIndex + 874u;
        uint32_t normalizedFraction = fraction ^ (1u << highestBitIndex);
        return EmulatedDoublePackFromFloatFraction(
            sign,
            normalizedExponent,
            normalizedFraction,
            52u - highestBitIndex
        );
    }

    return EmulatedDoublePackFromFloatFraction(
        sign,
        exponent + (EMULATED_DOUBLE_EXPONENT_BIAS - EMULATED_FLOAT_EXPONENT_BIAS),
        fraction,
        29u
    );
}

/// Rounds a 53-bit significand rightward into a 32-bit integer using round-to-nearest ties-to-even.
///
/// `significandHigh` and `significandLow` together hold a binary64 significand where the high word
/// contains the leading significand bits. This helper is used when converting very small binary64
/// values into binary32 subnormal results.
///
/// - Parameters:
///   - significandHigh: High word of the 53-bit significand.
///   - significandLow: Low word of the 53-bit significand.
///   - shift: Number of bits to shift right before rounding. Valid values are `1...53`.
/// - Returns: The rounded integer result after the requested right shift.
///
/// Quality: Covered by `emulatedDoubleRoundShiftRight53ToUint32RoundsAcrossShiftRanges` and
/// `emulatedDoubleToFloatBitsRoundsToNearestEven`.
static inline uint32_t EmulatedDoubleRoundShiftRight53ToUint32(
    uint32_t significandHigh,
    uint32_t significandLow,
    uint32_t shift
) {
    if (shift > 53u) {
        return 0u;
    }

    uint32_t rounded = 0u;
    uint32_t greaterThanHalf = 0u;
    uint32_t exactlyHalf = 0u;

    if (shift < 32u) {
        uint32_t remainderMask = (1u << shift) - 1u;
        uint32_t remainder = significandLow & remainderMask;
        uint32_t halfway = 1u << (shift - 1u);
        rounded = (significandHigh << (32u - shift)) | (significandLow >> shift);
        greaterThanHalf = remainder > halfway ? 1u : 0u;
        exactlyHalf = remainder == halfway ? 1u : 0u;
    } else if (shift == 32u) {
        rounded = significandHigh;
        greaterThanHalf = significandLow > 0x80000000u ? 1u : 0u;
        exactlyHalf = significandLow == 0x80000000u ? 1u : 0u;
    } else {
        uint32_t highShift = shift - 32u;
        uint32_t highRemainderMask = (1u << highShift) - 1u;
        uint32_t highRemainder = significandHigh & highRemainderMask;
        uint32_t highHalf = 1u << (highShift - 1u);
        rounded = significandHigh >> highShift;
        greaterThanHalf = highRemainder > highHalf ||
            (highRemainder == highHalf && significandLow != 0u) ? 1u : 0u;
        exactlyHalf = highRemainder == highHalf && significandLow == 0u ? 1u : 0u;
    }

    if (greaterThanHalf || (exactlyHalf && (rounded & 1u) != 0u)) {
        rounded += 1u;
    }

    return rounded;
}

/// Converts an `EmulatedDouble` bit pattern to an IEEE 754 binary32 bit pattern.
///
/// Finite values are rounded using round-to-nearest ties-to-even. Overflow produces signed
/// infinity. Tiny values produce signed zero or a binary32 subnormal as appropriate. NaN and
/// infinity encodings are preserved as binary32 special values, with NaN payload bits truncated.
///
/// - Parameter value: The encoded binary64 value to convert.
/// - Returns: Raw IEEE 754 binary32 bits.
///
/// Quality: Covered by `emulatedDoubleToFloatBitsRoundsToNearestEven`,
/// `emulatedDoubleToFloatBitsMatchesSwiftForFiniteDoubleValues`,
/// `emulatedDoubleToFloatBitsHandlesSpecialValues`, `emulatedDoubleRoundTripsFloatBits`, and
/// `emulatedDoubleFloatValueConversionRoundTripsBitPatterns`.
static inline uint32_t EmulatedDoubleToFloatBits(EmulatedDouble value) {
    uint32_t sign = value.highBits & EMULATED_DOUBLE_SIGN_MASK;
    uint32_t exponent = EmulatedDoubleExponentBits(value);
    uint32_t fractionHigh = EmulatedDoubleFractionHighBits(value);
    uint32_t fractionLow = value.lowBits;

    if (exponent == EMULATED_DOUBLE_EXPONENT_MAX) {
        uint32_t floatFraction = (fractionHigh << 3u) | (fractionLow >> 29u);
        if (floatFraction == 0u && (fractionHigh | fractionLow) != 0u) {
            floatFraction = 0x00400000u;
        }
        return sign | EMULATED_FLOAT_EXPONENT_MASK | floatFraction;
    }

    if (exponent == 0u) {
        return sign;
    }

    int32_t unbiasedExponent = (int32_t)exponent - (int32_t)EMULATED_DOUBLE_EXPONENT_BIAS;

    if (unbiasedExponent > 127) {
        return sign | EMULATED_FLOAT_EXPONENT_MASK;
    }

    uint32_t significandHigh = 0x00100000u | fractionHigh;
    uint32_t significandLow = fractionLow;

    if (unbiasedExponent < -126) {
        if (unbiasedExponent < -150) {
            return sign;
        }

        uint32_t shift = (uint32_t)(-unbiasedExponent - 97);
        uint32_t roundedFraction = EmulatedDoubleRoundShiftRight53ToUint32(
            significandHigh,
            significandLow,
            shift
        );

        if (roundedFraction == 0u) {
            return sign;
        }
        if (roundedFraction >= 0x00800000u) {
            return sign | (1u << EMULATED_FLOAT_EXPONENT_SHIFT);
        }
        return sign | roundedFraction;
    }

    uint32_t floatExponent = (uint32_t)(unbiasedExponent + (int32_t)EMULATED_FLOAT_EXPONENT_BIAS);
    uint32_t floatSignificand = (1u << 23u) | (fractionHigh << 3u) | (fractionLow >> 29u);
    uint32_t remainder = fractionLow & 0x1FFFFFFFu;
    uint32_t greaterThanHalf = remainder > 0x10000000u ? 1u : 0u;
    uint32_t exactlyHalf = remainder == 0x10000000u ? 1u : 0u;

    if (greaterThanHalf || (exactlyHalf && (floatSignificand & 1u) != 0u)) {
        floatSignificand += 1u;
    }

    if (floatSignificand == 0x01000000u) {
        floatSignificand = 0x00800000u;
        floatExponent += 1u;
        if (floatExponent >= EMULATED_FLOAT_EXPONENT_MAX) {
            return sign | EMULATED_FLOAT_EXPONENT_MASK;
        }
    }

    return sign | (floatExponent << EMULATED_FLOAT_EXPONENT_SHIFT) |
        (floatSignificand & EMULATED_FLOAT_FRACTION_MASK);
}

/// Reinterprets a `float` value as its raw IEEE 754 binary32 bit pattern.
///
/// Metal uses `as_type<uint>` for a bit-preserving reinterpretation. C and C++ use `memcpy` to avoid
/// violating strict aliasing rules.
///
/// - Parameter value: The `float` value to reinterpret.
/// - Returns: The raw binary32 bits of `value`.
///
/// Quality: Covered by `emulatedDoubleFloatValueConversionRoundTripsBitPatterns`.
static inline uint32_t EmulatedDoubleFloatToBits(float value) {
#if defined(__METAL_VERSION__)
    return as_type<uint>(value);
#else
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
#endif
}

/// Reinterprets raw IEEE 754 binary32 bits as a `float` value.
///
/// Metal uses `as_type<float>` for a bit-preserving reinterpretation. C and C++ use `memcpy` to
/// avoid violating strict aliasing rules.
///
/// - Parameter bits: The raw binary32 bits to reinterpret.
/// - Returns: The `float` value represented by `bits`.
///
/// Quality: Covered by `emulatedDoubleFloatValueConversionRoundTripsBitPatterns`.
static inline float EmulatedDoubleBitsToFloat(uint32_t bits) {
#if defined(__METAL_VERSION__)
    return as_type<float>(bits);
#else
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
#endif
}

/// Converts a `float` value to an exact `EmulatedDouble` representation.
///
/// - Parameter value: The IEEE 754 binary32 value to widen.
/// - Returns: An `EmulatedDouble` containing the exact binary64 representation of `value`.
///
/// Quality: Covered by `emulatedDoubleFloatValueConversionRoundTripsBitPatterns`.
static inline EmulatedDouble EmulatedDoubleFromFloat(float value) {
    return EmulatedDoubleFromFloatBits(EmulatedDoubleFloatToBits(value));
}

/// Converts an `EmulatedDouble` value to `float`.
///
/// The result uses the same conversion path as `EmulatedDoubleToFloatBits`, then reinterprets the
/// resulting binary32 bits as a `float`.
///
/// - Parameter value: The encoded binary64 value to narrow.
/// - Returns: The nearest IEEE 754 binary32 value, with ties rounded to even.
///
/// Quality: Covered by `emulatedDoubleFloatValueConversionRoundTripsBitPatterns`.
static inline float EmulatedDoubleToFloat(EmulatedDouble value) {
    return EmulatedDoubleBitsToFloat(EmulatedDoubleToFloatBits(value));
}

/// Returns the implicit binary64 significand bit as a 64-bit integer.
///
/// - Returns: `1 << 52`, the hidden bit for normal binary64 values.
///
/// Quality: Covered by `emulatedDoubleArithmeticHelpersExposeExpectedConstants`.
static inline uint64_t EmulatedDoubleHiddenBit(void) {
    return (uint64_t)1u << EMULATED_DOUBLE_FRACTION_BITS;
}

/// Returns the hidden bit position after appending guard, round, and sticky bits.
///
/// - Returns: `1 << 55`, the hidden bit shifted left by `EMULATED_DOUBLE_ROUNDING_BITS`.
///
/// Quality: Covered by `emulatedDoubleArithmeticHelpersExposeExpectedConstants`.
static inline uint64_t EmulatedDoubleGuardedHiddenBit(void) {
    return EmulatedDoubleHiddenBit() << EMULATED_DOUBLE_ROUNDING_BITS;
}

/// Returns an `EmulatedDouble` quiet NaN used for invalid arithmetic results.
///
/// - Returns: A positive quiet NaN with zero payload beyond the quiet bit.
///
/// Quality: Covered by `emulatedDoubleArithmeticHandlesSpecialValues`.
static inline EmulatedDouble EmulatedDoubleDefaultNaN(void) {
    return EmulatedDoubleMake(
        EMULATED_DOUBLE_EXPONENT_MASK | EMULATED_DOUBLE_QUIET_NAN_BIT,
        0u
    );
}

/// Returns an `EmulatedDouble` infinity with the requested sign.
///
/// - Parameter sign: `1` for negative infinity, `0` for positive infinity.
/// - Returns: The corresponding IEEE 754 binary64 infinity encoding.
///
/// Quality: Covered by `emulatedDoubleArithmeticHandlesSpecialValues`.
static inline EmulatedDouble EmulatedDoubleInfinity(uint32_t sign) {
    return EmulatedDoubleMake(
        (sign != 0u ? EMULATED_DOUBLE_SIGN_MASK : 0u) | EMULATED_DOUBLE_EXPONENT_MASK,
        0u
    );
}

/// Returns an `EmulatedDouble` zero with the requested sign.
///
/// - Parameter sign: `1` for negative zero, `0` for positive zero.
/// - Returns: The corresponding IEEE 754 binary64 zero encoding.
///
/// Quality: Covered by `emulatedDoubleArithmeticHandlesSignedZeroes`.
static inline EmulatedDouble EmulatedDoubleZero(uint32_t sign) {
    return EmulatedDoubleMake(sign != 0u ? EMULATED_DOUBLE_SIGN_MASK : 0u, 0u);
}

/// Returns a quieted NaN while preserving the original NaN sign and payload bits.
///
/// - Parameter value: A NaN encoding to quiet.
/// - Returns: `value` with the binary64 quiet-NaN bit set, or the default quiet NaN if needed.
///
/// Quality: Covered by `emulatedDoubleArithmeticHandlesSpecialValues`.
static inline EmulatedDouble EmulatedDoubleQuietNaN(EmulatedDouble value) {
    if (!EmulatedDoubleIsNaN(value)) {
        return EmulatedDoubleDefaultNaN();
    }
    return EmulatedDoubleMake(value.highBits | EMULATED_DOUBLE_QUIET_NAN_BIT, value.lowBits);
}

/// Toggles the sign bit of an `EmulatedDouble` value.
///
/// - Parameter value: The encoded binary64 value whose sign should be flipped.
/// - Returns: `value` with bit 63 inverted.
///
/// Quality: Covered by `emulatedDoubleNegatedTogglesOnlySignBit` and
/// `emulatedDoubleSubtractMatchesSwiftForFiniteValues`.
static inline EmulatedDouble EmulatedDoubleNegated(EmulatedDouble value) {
    return EmulatedDoubleMake(value.highBits ^ EMULATED_DOUBLE_SIGN_MASK, value.lowBits);
}

/// Extracts the finite binary64 significand and unbiased exponent.
///
/// Zeroes return significand `0` and exponent `-1022`. Normal values include the hidden bit.
/// Subnormal values use exponent `-1022` and do not include a hidden bit.
///
/// - Parameters:
///   - value: A finite binary64 encoding.
///   - exponent: Output storage for the unbiased exponent.
/// - Returns: The unsigned significand matching `value * 2^(52 - exponent)`.
///
/// Quality: Covered by `emulatedDoubleFinitePartsMatchExpectedSignificands`.
static inline uint64_t EmulatedDoubleFiniteSignificand(EmulatedDouble value, EMULATED_DOUBLE_THREAD int32_t *exponent) {
    uint32_t exponentBits = EmulatedDoubleExponentBits(value);
    uint64_t fraction = ((uint64_t)EmulatedDoubleFractionHighBits(value) << 32u) |
        (uint64_t)value.lowBits;

    if (exponentBits == 0u) {
        *exponent = -1022;
        return fraction;
    }

    *exponent = (int32_t)exponentBits - (int32_t)EMULATED_DOUBLE_EXPONENT_BIAS;
    return EmulatedDoubleHiddenBit() | fraction;
}

/// Normalizes a nonzero finite significand so its hidden bit is present.
///
/// - Parameters:
///   - significand: In-out significand returned by `EmulatedDoubleFiniteSignificand`.
///   - exponent: In-out unbiased exponent paired with `significand`.
///
/// Quality: Covered by `emulatedDoubleNormalizeFiniteSignificandRestoresHiddenBit`.
static inline void EmulatedDoubleNormalizeFiniteSignificand(
    EMULATED_DOUBLE_THREAD uint64_t *significand,
    EMULATED_DOUBLE_THREAD int32_t *exponent
) {
    while (*significand != 0u && *significand < EmulatedDoubleHiddenBit()) {
        *significand <<= 1u;
        *exponent -= 1;
    }
}

/// Compares the magnitudes of two finite `EmulatedDouble` values.
///
/// - Parameters:
///   - lhs: Left finite binary64 value.
///   - rhs: Right finite binary64 value.
/// - Returns: `1` when `lhs` magnitude is larger, `-1` when `rhs` magnitude is larger, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleCompareMagnitudeOrdersFiniteValues`.
static inline int32_t EmulatedDoubleCompareMagnitude(EmulatedDouble lhs, EmulatedDouble rhs) {
    uint32_t lhsHigh = lhs.highBits & ~EMULATED_DOUBLE_SIGN_MASK;
    uint32_t rhsHigh = rhs.highBits & ~EMULATED_DOUBLE_SIGN_MASK;

    if (lhsHigh > rhsHigh) {
        return 1;
    }
    if (lhsHigh < rhsHigh) {
        return -1;
    }
    if (lhs.lowBits > rhs.lowBits) {
        return 1;
    }
    if (lhs.lowBits < rhs.lowBits) {
        return -1;
    }
    return 0;
}

/// Shifts a 64-bit unsigned integer right and jams any discarded nonzero bit into bit 0.
///
/// - Parameters:
///   - value: Value to shift.
///   - shift: Number of bits to shift right.
/// - Returns: `(value >> shift)` with sticky bit information ORed into the least significant bit.
///
/// Quality: Covered by `emulatedDoubleShiftRightJam64PreservesStickyBit`.
static inline uint64_t EmulatedDoubleShiftRightJam64(uint64_t value, uint32_t shift) {
    if (shift == 0u) {
        return value;
    }
    if (shift < 64u) {
        uint64_t shifted = value >> shift;
        uint64_t discarded = value << (64u - shift);
        return shifted | (discarded != 0u ? 1u : 0u);
    }
    return value != 0u ? 1u : 0u;
}

/// Shifts a 128-bit unsigned integer right and jams discarded nonzero bits into bit 0.
///
/// - Parameters:
///   - high: High 64 bits of the 128-bit value.
///   - low: Low 64 bits of the 128-bit value.
///   - shift: Number of bits to shift right.
/// - Returns: The low 64 bits of the shifted value with sticky information in bit 0.
///
/// Quality: Covered by `emulatedDoubleShiftRightJam128PreservesStickyBit`.
static inline uint64_t EmulatedDoubleShiftRightJam128(uint64_t high, uint64_t low, uint32_t shift) {
    if (shift == 0u) {
        return low;
    }
    if (shift < 64u) {
        uint64_t shifted = (high << (64u - shift)) | (low >> shift);
        uint64_t discarded = low << (64u - shift);
        return shifted | (discarded != 0u ? 1u : 0u);
    }
    if (shift == 64u) {
        return high | (low != 0u ? 1u : 0u);
    }
    if (shift < 128u) {
        uint32_t highShift = shift - 64u;
        uint64_t shifted = high >> highShift;
        uint64_t discardedHigh = high << (64u - highShift);
        return shifted | ((discardedHigh | low) != 0u ? 1u : 0u);
    }
    return (high | low) != 0u ? 1u : 0u;
}

/// Multiplies two 64-bit unsigned integers and returns the 128-bit product as two words.
///
/// - Parameters:
///   - lhs: Left multiplicand.
///   - rhs: Right multiplicand.
///   - high: Output storage for the high 64 bits of the product.
///   - low: Output storage for the low 64 bits of the product.
///
/// Quality: Covered by `emulatedDoubleMultiply64To128MatchesKnownProducts`.
static inline void EmulatedDoubleMultiply64To128(
    uint64_t lhs,
    uint64_t rhs,
    EMULATED_DOUBLE_THREAD uint64_t *high,
    EMULATED_DOUBLE_THREAD uint64_t *low
) {
    uint64_t mask32 = 0xffffffffu;
    uint64_t lhsLow = lhs & mask32;
    uint64_t lhsHigh = lhs >> 32u;
    uint64_t rhsLow = rhs & mask32;
    uint64_t rhsHigh = rhs >> 32u;

    uint64_t productLowLow = lhsLow * rhsLow;
    uint64_t productLowHigh = lhsLow * rhsHigh;
    uint64_t productHighLow = lhsHigh * rhsLow;
    uint64_t productHighHigh = lhsHigh * rhsHigh;

    uint64_t resultLow = productLowLow;
    uint64_t addend = productLowHigh << 32u;
    resultLow += addend;
    uint64_t carry = resultLow < addend ? 1u : 0u;

    addend = productHighLow << 32u;
    uint64_t previousLow = resultLow;
    resultLow += addend;
    carry += resultLow < previousLow ? 1u : 0u;

    *low = resultLow;
    *high = productHighHigh + (productLowHigh >> 32u) + (productHighLow >> 32u) + carry;
}

/// Packs a sign, unbiased exponent, and guarded significand into an `EmulatedDouble`.
///
/// The significand is expected to carry three low rounding bits. Rounding uses round-to-nearest
/// ties-to-even. Overflow produces signed infinity; underflow produces signed zero or subnormal.
///
/// - Parameters:
///   - sign: Result sign, `1` for negative and `0` for positive.
///   - exponent: Unbiased binary64 exponent associated with the guarded significand.
///   - significand: Significand with guard, round, and sticky bits in its low three bits.
/// - Returns: The rounded binary64 encoding.
///
/// Quality: Covered by `emulatedDoubleRoundAndPackHandlesNormalSubnormalOverflowAndUnderflow`.
static inline EmulatedDouble EmulatedDoubleRoundAndPack(uint32_t sign, int32_t exponent, uint64_t significand) {
    if (significand == 0u) {
        return EmulatedDoubleZero(sign);
    }

    if (significand >= (EmulatedDoubleGuardedHiddenBit() << 1u)) {
        significand = EmulatedDoubleShiftRightJam64(significand, 1u);
        exponent += 1;
    }

    if (exponent < -1022) {
        uint32_t shift = (uint32_t)(-1022 - exponent);
        significand = EmulatedDoubleShiftRightJam64(significand, shift);
        exponent = -1022;
    }

    uint64_t rounded = significand >> EMULATED_DOUBLE_ROUNDING_BITS;
    uint32_t roundBits = (uint32_t)(significand & 0x7u);
    if (roundBits > 0x4u || (roundBits == 0x4u && (rounded & 1u) != 0u)) {
        rounded += 1u;
    }

    if (rounded >= (EmulatedDoubleHiddenBit() << 1u)) {
        rounded >>= 1u;
        exponent += 1;
    }

    uint32_t signBits = sign != 0u ? EMULATED_DOUBLE_SIGN_MASK : 0u;
    if (exponent > 1023) {
        return EmulatedDoubleInfinity(sign);
    }

    if (exponent == -1022 && rounded < EmulatedDoubleHiddenBit()) {
        if (rounded == 0u) {
            return EmulatedDoubleZero(sign);
        }
        return EmulatedDoubleMake(
            signBits | (uint32_t)(rounded >> 32u),
            (uint32_t)(rounded & 0xffffffffu)
        );
    }

    uint32_t exponentBits = (uint32_t)(exponent + (int32_t)EMULATED_DOUBLE_EXPONENT_BIAS);
    uint64_t fraction = rounded & (EmulatedDoubleHiddenBit() - 1u);
    return EmulatedDoubleMake(
        signBits | (exponentBits << EMULATED_DOUBLE_EXPONENT_SHIFT) | (uint32_t)(fraction >> 32u),
        (uint32_t)(fraction & 0xffffffffu)
    );
}

/// Adds two `EmulatedDouble` values using software IEEE 754 binary64 arithmetic.
///
/// The operation handles finite values, signed zeroes, infinities, and NaNs. Finite results use
/// round-to-nearest ties-to-even. Metal kernels using this helper require 64-bit integer math
/// support, which the bundled feature table lists as Metal 3 / Apple3.
///
/// - Parameters:
///   - lhs: Left addend.
///   - rhs: Right addend.
/// - Returns: `lhs + rhs` encoded as `EmulatedDouble`.
///
/// Quality: Covered by `emulatedDoubleAddMatchesSwiftForFiniteValues`,
/// `emulatedDoubleArithmeticMatchesSwiftAcrossGeneratedFiniteValues`,
/// `emulatedDoubleArithmeticHandlesSignedZeroes`, and `emulatedDoubleArithmeticHandlesSpecialValues`.
static inline EmulatedDouble EmulatedDoubleAdd(EmulatedDouble lhs, EmulatedDouble rhs) {
    if (EmulatedDoubleIsNaN(lhs)) {
        return EmulatedDoubleQuietNaN(lhs);
    }
    if (EmulatedDoubleIsNaN(rhs)) {
        return EmulatedDoubleQuietNaN(rhs);
    }

    uint32_t lhsSign = EmulatedDoubleSignBit(lhs);
    uint32_t rhsSign = EmulatedDoubleSignBit(rhs);

    if (EmulatedDoubleIsInfinity(lhs) || EmulatedDoubleIsInfinity(rhs)) {
        if (EmulatedDoubleIsInfinity(lhs) && EmulatedDoubleIsInfinity(rhs) && lhsSign != rhsSign) {
            return EmulatedDoubleDefaultNaN();
        }
        return EmulatedDoubleIsInfinity(lhs) ? lhs : rhs;
    }

    int32_t lhsExponent = 0;
    int32_t rhsExponent = 0;
    uint64_t lhsSignificand = EmulatedDoubleFiniteSignificand(lhs, &lhsExponent);
    uint64_t rhsSignificand = EmulatedDoubleFiniteSignificand(rhs, &rhsExponent);

    if (lhsSignificand == 0u && rhsSignificand == 0u) {
        return EmulatedDoubleZero(lhsSign & rhsSign);
    }
    if (lhsSignificand == 0u) {
        return rhs;
    }
    if (rhsSignificand == 0u) {
        return lhs;
    }

    uint64_t lhsGuarded = lhsSignificand << EMULATED_DOUBLE_ROUNDING_BITS;
    uint64_t rhsGuarded = rhsSignificand << EMULATED_DOUBLE_ROUNDING_BITS;
    int32_t resultExponent = lhsExponent;

    if (lhsExponent > rhsExponent) {
        rhsGuarded = EmulatedDoubleShiftRightJam64(rhsGuarded, (uint32_t)(lhsExponent - rhsExponent));
        resultExponent = lhsExponent;
    } else if (rhsExponent > lhsExponent) {
        lhsGuarded = EmulatedDoubleShiftRightJam64(lhsGuarded, (uint32_t)(rhsExponent - lhsExponent));
        resultExponent = rhsExponent;
    }

    uint32_t resultSign = lhsSign;
    uint64_t resultSignificand = 0u;

    if (lhsSign == rhsSign) {
        resultSignificand = lhsGuarded + rhsGuarded;
        resultSign = lhsSign;
    } else {
        int32_t comparison = EmulatedDoubleCompareMagnitude(lhs, rhs);
        if (comparison == 0) {
            return EmulatedDoubleZero(0u);
        }
        if (comparison > 0) {
            resultSignificand = lhsGuarded - rhsGuarded;
            resultSign = lhsSign;
        } else {
            resultSignificand = rhsGuarded - lhsGuarded;
            resultSign = rhsSign;
        }
    }

    if (resultSignificand == 0u) {
        return EmulatedDoubleZero(0u);
    }

    while (resultExponent > -1022 && resultSignificand < EmulatedDoubleGuardedHiddenBit()) {
        resultSignificand <<= 1u;
        resultExponent -= 1;
    }

    return EmulatedDoubleRoundAndPack(resultSign, resultExponent, resultSignificand);
}

/// Subtracts two `EmulatedDouble` values using software IEEE 754 binary64 arithmetic.
///
/// - Parameters:
///   - lhs: Minuend.
///   - rhs: Subtrahend.
/// - Returns: `lhs - rhs` encoded as `EmulatedDouble`.
///
/// Quality: Covered by `emulatedDoubleSubtractMatchesSwiftForFiniteValues`,
/// `emulatedDoubleArithmeticMatchesSwiftAcrossGeneratedFiniteValues`,
/// `emulatedDoubleArithmeticHandlesSignedZeroes`, and `emulatedDoubleArithmeticHandlesSpecialValues`.
static inline EmulatedDouble EmulatedDoubleSubtract(EmulatedDouble lhs, EmulatedDouble rhs) {
    return EmulatedDoubleAdd(lhs, EmulatedDoubleNegated(rhs));
}

/// Multiplies two `EmulatedDouble` values using software IEEE 754 binary64 arithmetic.
///
/// Finite results use round-to-nearest ties-to-even. Metal kernels using this helper require
/// 64-bit integer math support, which the bundled feature table lists as Metal 3 / Apple3.
///
/// - Parameters:
///   - lhs: Left factor.
///   - rhs: Right factor.
/// - Returns: `lhs * rhs` encoded as `EmulatedDouble`.
///
/// Quality: Covered by `emulatedDoubleMultiplyMatchesSwiftForFiniteValues`,
/// `emulatedDoubleArithmeticMatchesSwiftAcrossGeneratedFiniteValues`, and
/// `emulatedDoubleArithmeticHandlesSpecialValues`.
static inline EmulatedDouble EmulatedDoubleMultiply(EmulatedDouble lhs, EmulatedDouble rhs) {
    if (EmulatedDoubleIsNaN(lhs)) {
        return EmulatedDoubleQuietNaN(lhs);
    }
    if (EmulatedDoubleIsNaN(rhs)) {
        return EmulatedDoubleQuietNaN(rhs);
    }

    uint32_t resultSign = EmulatedDoubleSignBit(lhs) ^ EmulatedDoubleSignBit(rhs);
    uint32_t lhsIsInfinity = EmulatedDoubleIsInfinity(lhs);
    uint32_t rhsIsInfinity = EmulatedDoubleIsInfinity(rhs);
    uint32_t lhsIsZero = EmulatedDoubleIsZero(lhs);
    uint32_t rhsIsZero = EmulatedDoubleIsZero(rhs);

    if ((lhsIsInfinity && rhsIsZero) || (rhsIsInfinity && lhsIsZero)) {
        return EmulatedDoubleDefaultNaN();
    }
    if (lhsIsInfinity || rhsIsInfinity) {
        return EmulatedDoubleInfinity(resultSign);
    }
    if (lhsIsZero || rhsIsZero) {
        return EmulatedDoubleZero(resultSign);
    }

    int32_t lhsExponent = 0;
    int32_t rhsExponent = 0;
    uint64_t lhsSignificand = EmulatedDoubleFiniteSignificand(lhs, &lhsExponent);
    uint64_t rhsSignificand = EmulatedDoubleFiniteSignificand(rhs, &rhsExponent);
    EmulatedDoubleNormalizeFiniteSignificand(&lhsSignificand, &lhsExponent);
    EmulatedDoubleNormalizeFiniteSignificand(&rhsSignificand, &rhsExponent);

    uint64_t productHigh = 0u;
    uint64_t productLow = 0u;
    EmulatedDoubleMultiply64To128(lhsSignificand, rhsSignificand, &productHigh, &productLow);

    int32_t resultExponent = lhsExponent + rhsExponent;
    uint32_t shift = 49u;
    if ((productHigh & ((uint64_t)1u << 41u)) != 0u) {
        shift = 50u;
        resultExponent += 1;
    }

    uint64_t resultSignificand = EmulatedDoubleShiftRightJam128(productHigh, productLow, shift);
    return EmulatedDoubleRoundAndPack(resultSign, resultExponent, resultSignificand);
}

/// Divides two `EmulatedDouble` values using software IEEE 754 binary64 arithmetic.
///
/// Finite results use round-to-nearest ties-to-even. Metal kernels using this helper require
/// 64-bit integer math support, which the bundled feature table lists as Metal 3 / Apple3.
///
/// - Parameters:
///   - lhs: Dividend.
///   - rhs: Divisor.
/// - Returns: `lhs / rhs` encoded as `EmulatedDouble`.
///
/// Quality: Covered by `emulatedDoubleDivideMatchesSwiftForFiniteValues`,
/// `emulatedDoubleArithmeticMatchesSwiftAcrossGeneratedFiniteValues`, and
/// `emulatedDoubleArithmeticHandlesSpecialValues`.
static inline EmulatedDouble EmulatedDoubleDivide(EmulatedDouble lhs, EmulatedDouble rhs) {
    if (EmulatedDoubleIsNaN(lhs)) {
        return EmulatedDoubleQuietNaN(lhs);
    }
    if (EmulatedDoubleIsNaN(rhs)) {
        return EmulatedDoubleQuietNaN(rhs);
    }

    uint32_t resultSign = EmulatedDoubleSignBit(lhs) ^ EmulatedDoubleSignBit(rhs);
    uint32_t lhsIsInfinity = EmulatedDoubleIsInfinity(lhs);
    uint32_t rhsIsInfinity = EmulatedDoubleIsInfinity(rhs);
    uint32_t lhsIsZero = EmulatedDoubleIsZero(lhs);
    uint32_t rhsIsZero = EmulatedDoubleIsZero(rhs);

    if ((lhsIsZero && rhsIsZero) || (lhsIsInfinity && rhsIsInfinity)) {
        return EmulatedDoubleDefaultNaN();
    }
    if (lhsIsInfinity) {
        return EmulatedDoubleInfinity(resultSign);
    }
    if (rhsIsInfinity) {
        return EmulatedDoubleZero(resultSign);
    }
    if (rhsIsZero) {
        return EmulatedDoubleInfinity(resultSign);
    }
    if (lhsIsZero) {
        return EmulatedDoubleZero(resultSign);
    }

    int32_t lhsExponent = 0;
    int32_t rhsExponent = 0;
    uint64_t lhsSignificand = EmulatedDoubleFiniteSignificand(lhs, &lhsExponent);
    uint64_t rhsSignificand = EmulatedDoubleFiniteSignificand(rhs, &rhsExponent);
    EmulatedDoubleNormalizeFiniteSignificand(&lhsSignificand, &lhsExponent);
    EmulatedDoubleNormalizeFiniteSignificand(&rhsSignificand, &rhsExponent);

    int32_t resultExponent = lhsExponent - rhsExponent;
    uint32_t quotientShift = 55u;
    if (lhsSignificand < rhsSignificand) {
        quotientShift = 56u;
        resultExponent -= 1;
    }

    uint64_t quotient = lhsSignificand / rhsSignificand;
    uint64_t remainder = lhsSignificand - quotient * rhsSignificand;

    for (uint32_t index = 0u; index < quotientShift; index += 1u) {
        quotient <<= 1u;
        remainder <<= 1u;
        if (remainder >= rhsSignificand) {
            remainder -= rhsSignificand;
            quotient |= 1u;
        }
    }

    if (remainder != 0u) {
        quotient |= 1u;
    }

    return EmulatedDoubleRoundAndPack(resultSign, resultExponent, quotient);
}

/// Compares two unsigned 128-bit integer values represented as high and low 64-bit words.
///
/// This helper supports correctly rounded square root without requiring native 128-bit integer
/// types in Metal.
///
/// - Parameters:
///   - lhsHigh: High word of the left value.
///   - lhsLow: Low word of the left value.
///   - rhsHigh: High word of the right value.
///   - rhsLow: Low word of the right value.
/// - Returns: `1` when the left value is larger, `-1` when the right value is larger, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleSqrtMatchesSwiftForRepresentativeValues`.
static inline int32_t EmulatedDoubleCompareUnsigned128(
    uint64_t lhsHigh,
    uint64_t lhsLow,
    uint64_t rhsHigh,
    uint64_t rhsLow
) {
    if (lhsHigh > rhsHigh) {
        return 1;
    }
    if (lhsHigh < rhsHigh) {
        return -1;
    }
    if (lhsLow > rhsLow) {
        return 1;
    }
    if (lhsLow < rhsLow) {
        return -1;
    }
    return 0;
}

/// Adds a 64-bit unsigned integer to an unsigned 128-bit value.
///
/// - Parameters:
///   - high: High word of the 128-bit addend.
///   - low: Low word of the 128-bit addend.
///   - addend: 64-bit value to add to the low word.
///   - resultHigh: Output high word.
///   - resultLow: Output low word.
///
/// Quality: Covered by `emulatedDoubleSqrtMatchesSwiftForRepresentativeValues`.
static inline void EmulatedDoubleAdd64ToUnsigned128(
    uint64_t high,
    uint64_t low,
    uint64_t addend,
    EMULATED_DOUBLE_THREAD uint64_t *resultHigh,
    EMULATED_DOUBLE_THREAD uint64_t *resultLow
) {
    uint64_t summedLow = low + addend;
    *resultLow = summedLow;
    *resultHigh = high + (summedLow < low ? 1u : 0u);
}

/// Computes `floor(sqrt(value))` for an unsigned 128-bit integer.
///
/// The result is limited to 53 bits because this helper is used for binary64 significand square
/// root construction.
///
/// - Parameters:
///   - high: High word of the radicand.
///   - low: Low word of the radicand.
/// - Returns: The largest integer `r` such that `r * r <= value`.
///
/// Quality: Covered by `emulatedDoubleSqrtMatchesSwiftForRepresentativeValues`.
static inline uint64_t EmulatedDoubleSqrtFloorUnsigned128(uint64_t high, uint64_t low) {
    uint64_t lower = 0u;
    uint64_t upper = (uint64_t)1u << 53u;

    while (lower < upper) {
        uint64_t mid = (lower + upper + 1u) >> 1u;
        uint64_t squareHigh = 0u;
        uint64_t squareLow = 0u;
        EmulatedDoubleMultiply64To128(mid, mid, &squareHigh, &squareLow);
        if (EmulatedDoubleCompareUnsigned128(squareHigh, squareLow, high, low) <= 0) {
            lower = mid;
        } else {
            upper = mid - 1u;
        }
    }

    return lower;
}

/// Computes the IEEE 754 square root of an `EmulatedDouble`.
///
/// Finite nonnegative values are rounded to nearest ties-to-even using integer arithmetic. NaNs are
/// quieted, positive infinity is preserved, negative nonzero values produce the default quiet NaN,
/// and signed zero is preserved.
///
/// - Parameter value: The encoded binary64 value.
/// - Returns: `sqrt(value)` encoded as `EmulatedDouble`.
///
/// Quality: Covered by `emulatedDoubleSqrtMatchesSwiftForRepresentativeValues` and
/// `emulatedDoubleSqrtHandlesSpecialValues`.
static inline EmulatedDouble EmulatedDoubleSqrt(EmulatedDouble value) {
    if (EmulatedDoubleIsNaN(value)) {
        return EmulatedDoubleQuietNaN(value);
    }
    if (EmulatedDoubleIsZero(value)) {
        return value;
    }
    if (EmulatedDoubleSignBit(value)) {
        return EmulatedDoubleDefaultNaN();
    }
    if (EmulatedDoubleIsInfinity(value)) {
        return value;
    }

    int32_t exponent = 0;
    uint64_t significand = EmulatedDoubleFiniteSignificand(value, &exponent);
    EmulatedDoubleNormalizeFiniteSignificand(&significand, &exponent);

    uint32_t exponentIsOdd = ((uint32_t)exponent) & 1u;
    if (exponentIsOdd) {
        exponent -= 1;
    }
    uint32_t shift = exponentIsOdd ? 53u : 52u;
    int32_t resultExponent = exponent / 2;

    uint64_t radicandHigh = significand >> (64u - shift);
    uint64_t radicandLow = significand << shift;
    uint64_t root = EmulatedDoubleSqrtFloorUnsigned128(radicandHigh, radicandLow);

    uint64_t squareHigh = 0u;
    uint64_t squareLow = 0u;
    uint64_t thresholdHigh = 0u;
    uint64_t thresholdLow = 0u;
    EmulatedDoubleMultiply64To128(root, root, &squareHigh, &squareLow);
    EmulatedDoubleAdd64ToUnsigned128(squareHigh, squareLow, root, &thresholdHigh, &thresholdLow);
    if (EmulatedDoubleCompareUnsigned128(radicandHigh, radicandLow, thresholdHigh, thresholdLow) > 0) {
        root += 1u;
    }

    if (root >= (EmulatedDoubleHiddenBit() << 1u)) {
        root >>= 1u;
        resultExponent += 1;
    }

    return EmulatedDoubleRoundAndPack(0u, resultExponent, root << EMULATED_DOUBLE_ROUNDING_BITS);
}

/// Reports whether an `EmulatedDouble` compares less than positive zero.
///
/// NaNs and both signed zeroes return `0`, matching IEEE comparison behavior for the
/// `value < 0.0` predicate used by geometric helpers.
///
/// - Parameter value: The encoded binary64 value to compare.
/// - Returns: `1` when `value < 0.0`, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleIsLessThanZeroFollowsIEEEComparisonRules`.
static inline uint32_t EmulatedDoubleIsLessThanZero(EmulatedDouble value) {
    return !EmulatedDoubleIsNaN(value) && EmulatedDoubleSignBit(value) && !EmulatedDoubleIsZero(value) ? 1u : 0u;
}

/// Returns the `EmulatedDouble` representation of `0.5`.
///
/// - Returns: The exact IEEE 754 binary64 encoding of `0.5`.
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleHalf(void) {
    return EmulatedDoubleMake(0x3fe00000u, 0x00000000u);
}

/// Returns the `EmulatedDouble` representation of `1.0`.
///
/// - Returns: The exact IEEE 754 binary64 encoding of `1.0`.
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleOne(void) {
    return EmulatedDoubleMake(0x3ff00000u, 0x00000000u);
}

/// Returns the `EmulatedDouble` representation of `2.0`.
///
/// - Returns: The exact IEEE 754 binary64 encoding of `2.0`.
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleTwo(void) {
    return EmulatedDoubleMake(0x40000000u, 0x00000000u);
}

/// Returns the `EmulatedDouble` representation of `3.0`.
///
/// - Returns: The exact IEEE 754 binary64 encoding of `3.0`.
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleThree(void) {
    return EmulatedDoubleMake(0x40080000u, 0x00000000u);
}

/// Returns the `EmulatedDouble` representation of π.
///
/// - Returns: The nearest IEEE 754 binary64 encoding of π.
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoublePi(void) {
    return EmulatedDoubleMake(0x400921fbu, 0x54442d18u);
}

/// Returns the `EmulatedDouble` representation of π/2.
///
/// - Returns: The nearest IEEE 754 binary64 encoding of π/2.
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleHalfPi(void) {
    return EmulatedDoubleMake(0x3ff921fbu, 0x54442d18u);
}

/// Returns the `EmulatedDouble` representation of π/4.
///
/// - Returns: The nearest IEEE 754 binary64 encoding of π/4.
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleQuarterPi(void) {
    return EmulatedDoubleMake(0x3fe921fbu, 0x54442d18u);
}

/// Returns the `EmulatedDouble` representation of 2π.
///
/// - Returns: The nearest IEEE 754 binary64 encoding of 2π.
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleTwoPi(void) {
    return EmulatedDoubleMake(0x401921fbu, 0x54442d18u);
}

/// Returns the `EmulatedDouble` representation of 2/π.
///
/// - Returns: The nearest IEEE 754 binary64 encoding of 2/π.
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleTwoOverPi(void) {
    return EmulatedDoubleMake(0x3fe45f30u, 0x6dc9c883u);
}

/// Returns the `EmulatedDouble` representation of ln(2).
///
/// - Returns: The nearest IEEE 754 binary64 encoding of ln(2).
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleLn2(void) {
    return EmulatedDoubleMake(0x3fe62e42u, 0xfefa39efu);
}

/// Returns the `EmulatedDouble` representation of 1/ln(2).
///
/// - Returns: The nearest IEEE 754 binary64 encoding of 1/ln(2).
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleInvLn2(void) {
    return EmulatedDoubleMake(0x3ff71547u, 0x652b82feu);
}

/// Returns the `EmulatedDouble` representation of 1/ln(10).
///
/// - Returns: The nearest IEEE 754 binary64 encoding of 1/ln(10).
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleInvLn10(void) {
    return EmulatedDoubleMake(0x3fdbcb7bu, 0x1526e50du);
}

/// Returns the `EmulatedDouble` representation of log2(10).
///
/// - Returns: The nearest IEEE 754 binary64 encoding of log2(10).
///
/// Quality: Covered by `emulatedDoubleMathConstantsExposeExpectedBitPatterns`.
static inline EmulatedDouble EmulatedDoubleLog2_10(void) {
    return EmulatedDoubleMake(0x400a934fu, 0x0979a371u);
}

/// Returns whether an `EmulatedDouble` is finite.
///
/// - Parameter value: The encoded binary64 value to classify.
/// - Returns: `1` for zeroes, subnormals, and normal finite values; otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleRelationalPredicatesFollowIEEEClassification`.
static inline uint32_t EmulatedDoubleIsFinite(EmulatedDouble value) {
    return EmulatedDoubleExponentBits(value) != EMULATED_DOUBLE_EXPONENT_MAX ? 1u : 0u;
}

/// Reports whether an `EmulatedDouble` encodes positive or negative infinity.
///
/// This alias mirrors Metal's `isinf` spelling while preserving the existing
/// `EmulatedDoubleIsInfinity` API.
///
/// Quality: Covered by `emulatedDoubleRelationalPredicatesFollowIEEEClassification`.
static inline uint32_t EmulatedDoubleIsInf(EmulatedDouble value) {
    return EmulatedDoubleIsInfinity(value);
}

/// Returns whether an `EmulatedDouble` is a normal finite value.
///
/// - Parameter value: The encoded binary64 value to classify.
/// - Returns: `1` when the exponent is neither zero nor all ones, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleRelationalPredicatesFollowIEEEClassification`.
static inline uint32_t EmulatedDoubleIsNormal(EmulatedDouble value) {
    uint32_t exponent = EmulatedDoubleExponentBits(value);
    return exponent != 0u && exponent != EMULATED_DOUBLE_EXPONENT_MAX ? 1u : 0u;
}

/// Returns whether two `EmulatedDouble` values are ordered.
///
/// - Parameters:
///   - lhs: Left operand.
///   - rhs: Right operand.
/// - Returns: `1` when neither operand is NaN, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleRelationalPredicatesFollowIEEEClassification`.
static inline uint32_t EmulatedDoubleIsOrdered(EmulatedDouble lhs, EmulatedDouble rhs) {
    return !EmulatedDoubleIsNaN(lhs) && !EmulatedDoubleIsNaN(rhs) ? 1u : 0u;
}

/// Returns whether two `EmulatedDouble` values are unordered.
///
/// - Parameters:
///   - lhs: Left operand.
///   - rhs: Right operand.
/// - Returns: `1` when either operand is NaN, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleRelationalPredicatesFollowIEEEClassification`.
static inline uint32_t EmulatedDoubleIsUnordered(EmulatedDouble lhs, EmulatedDouble rhs) {
    return EmulatedDoubleIsNaN(lhs) || EmulatedDoubleIsNaN(rhs) ? 1u : 0u;
}

/// Selects between two `EmulatedDouble` values using a scalar predicate.
///
/// - Parameters:
///   - falseValue: Value returned when `condition` is zero.
///   - trueValue: Value returned when `condition` is nonzero.
///   - condition: Scalar predicate.
/// - Returns: `condition ? trueValue : falseValue`.
///
/// Quality: Covered by `emulatedDoubleSelectAndLogicalHelpersFollowScalarRules`.
static inline EmulatedDouble EmulatedDoubleSelect(EmulatedDouble falseValue, EmulatedDouble trueValue, uint32_t condition) {
    return condition != 0u ? trueValue : falseValue;
}

/// Applies scalar logical NOT to a predicate value.
///
/// - Parameter value: Predicate encoded as zero or nonzero.
/// - Returns: `1` when `value` is zero, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleSelectAndLogicalHelpersFollowScalarRules`.
static inline uint32_t EmulatedDoubleNot(uint32_t value) {
    return value == 0u ? 1u : 0u;
}

/// Applies scalar logical AND to two predicate values.
///
/// - Parameters:
///   - lhs: Left predicate encoded as zero or nonzero.
///   - rhs: Right predicate encoded as zero or nonzero.
/// - Returns: `1` when both predicates are nonzero, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleSelectAndLogicalHelpersFollowScalarRules`.
static inline uint32_t EmulatedDoubleAll(uint32_t lhs, uint32_t rhs) {
    return lhs != 0u && rhs != 0u ? 1u : 0u;
}

/// Applies scalar logical OR to two predicate values.
///
/// - Parameters:
///   - lhs: Left predicate encoded as zero or nonzero.
///   - rhs: Right predicate encoded as zero or nonzero.
/// - Returns: `1` when either predicate is nonzero, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleSelectAndLogicalHelpersFollowScalarRules`.
static inline uint32_t EmulatedDoubleAny(uint32_t lhs, uint32_t rhs) {
    return lhs != 0u || rhs != 0u ? 1u : 0u;
}

/// Compares two ordered `EmulatedDouble` values for equality.
///
/// Signed zeroes compare equal. NaNs compare unequal to every value, including themselves.
///
/// - Parameters:
///   - lhs: Left operand.
///   - rhs: Right operand.
/// - Returns: `1` when `lhs == rhs`, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleComparisonsFollowIEEEOrdering`.
static inline uint32_t EmulatedDoubleIsEqual(EmulatedDouble lhs, EmulatedDouble rhs) {
    if (!EmulatedDoubleIsOrdered(lhs, rhs)) {
        return 0u;
    }
    if (EmulatedDoubleIsZero(lhs) && EmulatedDoubleIsZero(rhs)) {
        return 1u;
    }
    return lhs.highBits == rhs.highBits && lhs.lowBits == rhs.lowBits ? 1u : 0u;
}

/// Compares two ordered `EmulatedDouble` values with the IEEE `<` predicate.
///
/// - Parameters:
///   - lhs: Left operand.
///   - rhs: Right operand.
/// - Returns: `1` when `lhs < rhs`, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleComparisonsFollowIEEEOrdering`.
static inline uint32_t EmulatedDoubleIsLessThan(EmulatedDouble lhs, EmulatedDouble rhs) {
    if (!EmulatedDoubleIsOrdered(lhs, rhs) || (EmulatedDoubleIsZero(lhs) && EmulatedDoubleIsZero(rhs))) {
        return 0u;
    }

    uint32_t lhsSign = EmulatedDoubleSignBit(lhs);
    uint32_t rhsSign = EmulatedDoubleSignBit(rhs);
    if (lhsSign != rhsSign) {
        return lhsSign > rhsSign ? 1u : 0u;
    }

    int32_t magnitude = EmulatedDoubleCompareMagnitude(lhs, rhs);
    if (lhsSign) {
        return magnitude > 0 ? 1u : 0u;
    }
    return magnitude < 0 ? 1u : 0u;
}

/// Compares two ordered `EmulatedDouble` values with the IEEE `<=` predicate.
///
/// Quality: Covered by `emulatedDoubleComparisonsFollowIEEEOrdering`.
static inline uint32_t EmulatedDoubleIsLessThanOrEqual(EmulatedDouble lhs, EmulatedDouble rhs) {
    return EmulatedDoubleIsLessThan(lhs, rhs) || EmulatedDoubleIsEqual(lhs, rhs) ? 1u : 0u;
}

/// Compares two ordered `EmulatedDouble` values with the IEEE `>` predicate.
///
/// Quality: Covered by `emulatedDoubleComparisonsFollowIEEEOrdering`.
static inline uint32_t EmulatedDoubleIsGreaterThan(EmulatedDouble lhs, EmulatedDouble rhs) {
    return EmulatedDoubleIsLessThan(rhs, lhs);
}

/// Compares two ordered `EmulatedDouble` values with the IEEE `>=` predicate.
///
/// Quality: Covered by `emulatedDoubleComparisonsFollowIEEEOrdering`.
static inline uint32_t EmulatedDoubleIsGreaterThanOrEqual(EmulatedDouble lhs, EmulatedDouble rhs) {
    return EmulatedDoubleIsGreaterThan(lhs, rhs) || EmulatedDoubleIsEqual(lhs, rhs) ? 1u : 0u;
}

/// Converts a signed 32-bit integer to `EmulatedDouble`.
///
/// - Parameter value: Integer value to encode.
/// - Returns: The exact binary64 representation of `value`.
///
/// Quality: Covered by `emulatedDoubleIntegerConversionRoundTripsRepresentativeValues`.
static inline EmulatedDouble EmulatedDoubleFromInt32(int32_t value) {
    if (value == 0) {
        return EmulatedDoubleZero(0u);
    }

    uint32_t sign = value < 0 ? 1u : 0u;
    uint32_t magnitude = sign ? (uint32_t)(-(value + 1)) + 1u : (uint32_t)value;
    uint32_t highestBitIndex = 31u - EmulatedDoubleCountLeadingZeros32(magnitude);
    uint64_t significand = (uint64_t)magnitude << (EMULATED_DOUBLE_FRACTION_BITS - highestBitIndex);
    return EmulatedDoubleRoundAndPack(sign, (int32_t)highestBitIndex, significand << EMULATED_DOUBLE_ROUNDING_BITS);
}

/// Converts an `EmulatedDouble` to a signed 32-bit integer with saturation.
///
/// Fractional bits are discarded toward zero. NaNs and zeroes return `0`. Values outside the
/// signed 32-bit range saturate to `INT32_MIN` or `INT32_MAX`.
///
/// - Parameter value: Encoded binary64 value to narrow.
/// - Returns: Saturating signed 32-bit integer result.
///
/// Quality: Covered by `emulatedDoubleIntegerConversionRoundTripsRepresentativeValues`.
static inline int32_t EmulatedDoubleToInt32Saturating(EmulatedDouble value) {
    if (!EmulatedDoubleIsFinite(value) || EmulatedDoubleIsZero(value)) {
        return 0;
    }

    uint32_t sign = EmulatedDoubleSignBit(value);
    int32_t exponent = 0;
    uint64_t significand = EmulatedDoubleFiniteSignificand(value, &exponent);
    EmulatedDoubleNormalizeFiniteSignificand(&significand, &exponent);

    if (exponent < 0) {
        return 0;
    }
    if (exponent > 30) {
        return sign ? (int32_t)0x80000000u : (int32_t)0x7fffffffu;
    }

    uint64_t magnitude = significand >> (EMULATED_DOUBLE_FRACTION_BITS - (uint32_t)exponent);
    if (sign) {
        return magnitude == 0x80000000u ? (int32_t)0x80000000u : -(int32_t)magnitude;
    }
    return (int32_t)magnitude;
}

/// Returns the absolute value of an `EmulatedDouble`.
///
/// - Parameter value: Encoded binary64 value.
/// - Returns: `value` with its sign bit cleared. NaN payload bits are preserved.
///
/// Quality: Covered by `emulatedDoubleAbsCopySignAndSignPreserveExpectedBits`.
static inline EmulatedDouble EmulatedDoubleAbs(EmulatedDouble value) {
    return EmulatedDoubleMake(value.highBits & ~EMULATED_DOUBLE_SIGN_MASK, value.lowBits);
}

/// Returns the absolute value of an `EmulatedDouble`.
///
/// This alias mirrors Metal's `fabs` name.
///
/// Quality: Covered by `emulatedDoubleAbsCopySignAndSignPreserveExpectedBits`.
static inline EmulatedDouble EmulatedDoubleFabs(EmulatedDouble value) {
    return EmulatedDoubleAbs(value);
}

/// Copies the sign bit from one `EmulatedDouble` to another.
///
/// - Parameters:
///   - magnitude: Value that supplies exponent and fraction bits.
///   - signSource: Value that supplies the sign bit.
/// - Returns: `magnitude` with its sign changed to match `signSource`.
///
/// Quality: Covered by `emulatedDoubleAbsCopySignAndSignPreserveExpectedBits`.
static inline EmulatedDouble EmulatedDoubleCopySign(EmulatedDouble magnitude, EmulatedDouble signSource) {
    return EmulatedDoubleMake(
        (magnitude.highBits & ~EMULATED_DOUBLE_SIGN_MASK) | (signSource.highBits & EMULATED_DOUBLE_SIGN_MASK),
        magnitude.lowBits
    );
}

/// Returns `-1.0`, signed zero, `1.0`, or `+0.0` for NaN according to Metal's `sign` behavior.
///
/// - Parameter value: Encoded binary64 value.
/// - Returns: The sign classification encoded as `EmulatedDouble`.
///
/// Quality: Covered by `emulatedDoubleAbsCopySignAndSignPreserveExpectedBits`.
static inline EmulatedDouble EmulatedDoubleSign(EmulatedDouble value) {
    if (EmulatedDoubleIsNaN(value)) {
        return EmulatedDoubleZero(0u);
    }
    if (EmulatedDoubleIsZero(value)) {
        return value;
    }
    return EmulatedDoubleSignBit(value) ? EmulatedDoubleMake(0xbff00000u, 0u) : EmulatedDoubleOne();
}

/// Returns the lesser of two `EmulatedDouble` values using Metal `fmin` NaN rules.
///
/// - Parameters:
///   - lhs: Left operand.
///   - rhs: Right operand.
/// - Returns: The non-NaN operand when exactly one operand is NaN; a quiet NaN when both are NaN.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleMin(EmulatedDouble lhs, EmulatedDouble rhs) {
    if (EmulatedDoubleIsNaN(lhs) && EmulatedDoubleIsNaN(rhs)) {
        return EmulatedDoubleQuietNaN(lhs);
    }
    if (EmulatedDoubleIsNaN(lhs)) {
        return rhs;
    }
    if (EmulatedDoubleIsNaN(rhs)) {
        return lhs;
    }
    return EmulatedDoubleIsLessThan(rhs, lhs) ? rhs : lhs;
}

/// Returns the lesser of two `EmulatedDouble` values using Metal `fmin` NaN rules.
///
/// This alias mirrors Metal's `fmin` name.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleFmin(EmulatedDouble lhs, EmulatedDouble rhs) {
    return EmulatedDoubleMin(lhs, rhs);
}

/// Returns the greater of two `EmulatedDouble` values using Metal `fmax` NaN rules.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleMax(EmulatedDouble lhs, EmulatedDouble rhs) {
    if (EmulatedDoubleIsNaN(lhs) && EmulatedDoubleIsNaN(rhs)) {
        return EmulatedDoubleQuietNaN(lhs);
    }
    if (EmulatedDoubleIsNaN(lhs)) {
        return rhs;
    }
    if (EmulatedDoubleIsNaN(rhs)) {
        return lhs;
    }
    return EmulatedDoubleIsLessThan(lhs, rhs) ? rhs : lhs;
}

/// Returns the greater of two `EmulatedDouble` values using Metal `fmax` NaN rules.
///
/// This alias mirrors Metal's `fmax` name.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleFmax(EmulatedDouble lhs, EmulatedDouble rhs) {
    return EmulatedDoubleMax(lhs, rhs);
}

/// Returns the least of three `EmulatedDouble` values.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleMin3(EmulatedDouble x, EmulatedDouble y, EmulatedDouble z) {
    return EmulatedDoubleMin(x, EmulatedDoubleMin(y, z));
}

/// Returns the least of three `EmulatedDouble` values.
///
/// This alias mirrors Metal's `fmin3` name.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleFmin3(EmulatedDouble x, EmulatedDouble y, EmulatedDouble z) {
    return EmulatedDoubleMin3(x, y, z);
}

/// Returns the greatest of three `EmulatedDouble` values.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleMax3(EmulatedDouble x, EmulatedDouble y, EmulatedDouble z) {
    return EmulatedDoubleMax(x, EmulatedDoubleMax(y, z));
}

/// Returns the greatest of three `EmulatedDouble` values.
///
/// This alias mirrors Metal's `fmax3` name.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleFmax3(EmulatedDouble x, EmulatedDouble y, EmulatedDouble z) {
    return EmulatedDoubleMax3(x, y, z);
}

/// Returns the middle value among three `EmulatedDouble` values.
///
/// NaNs are treated as missing data, matching the Metal `median3` description. If all operands are
/// NaNs, the result is a quiet NaN.
///
/// Quality: Covered by `emulatedDoubleMedian3TreatsNaNsAsMissingData`.
static inline EmulatedDouble EmulatedDoubleMedian3(EmulatedDouble x, EmulatedDouble y, EmulatedDouble z) {
    if (EmulatedDoubleIsNaN(x) && EmulatedDoubleIsNaN(y) && EmulatedDoubleIsNaN(z)) {
        return EmulatedDoubleQuietNaN(x);
    }
    if (EmulatedDoubleIsNaN(x)) {
        return EmulatedDoubleMin(y, z);
    }
    if (EmulatedDoubleIsNaN(y)) {
        return EmulatedDoubleMin(x, z);
    }
    if (EmulatedDoubleIsNaN(z)) {
        return EmulatedDoubleMin(x, y);
    }
    return EmulatedDoubleMax(EmulatedDoubleMin(x, y), EmulatedDoubleMin(EmulatedDoubleMax(x, y), z));
}

/// Returns the middle value among three `EmulatedDouble` values.
///
/// This alias mirrors Metal's `fmedian3` name.
///
/// Quality: Covered by `emulatedDoubleMedian3TreatsNaNsAsMissingData`.
static inline EmulatedDouble EmulatedDoubleFMedian3(EmulatedDouble x, EmulatedDouble y, EmulatedDouble z) {
    return EmulatedDoubleMedian3(x, y, z);
}

/// Clamps a value to the closed range `[minimumValue, maximumValue]`.
///
/// The implementation follows Metal's precise definition, `fmin(fmax(x, minval), maxval)`.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleClamp(
    EmulatedDouble value,
    EmulatedDouble minimumValue,
    EmulatedDouble maximumValue
) {
    return EmulatedDoubleMin(EmulatedDoubleMax(value, minimumValue), maximumValue);
}

/// Clamps a value to the closed range `[0.0, 1.0]`.
///
/// Quality: Covered by `emulatedDoubleMinMaxClampAndSaturateFollowMetalRules`.
static inline EmulatedDouble EmulatedDoubleSaturate(EmulatedDouble value) {
    return EmulatedDoubleClamp(value, EmulatedDoubleZero(0u), EmulatedDoubleOne());
}

/// Linearly interpolates between two values.
///
/// Computes `x + (y - x) * a`, matching the Metal `mix` definition.
///
/// Quality: Covered by `emulatedDoubleStepMixAndSmoothstepMatchMetalDefinitions`.
static inline EmulatedDouble EmulatedDoubleMix(EmulatedDouble x, EmulatedDouble y, EmulatedDouble a) {
    return EmulatedDoubleAdd(x, EmulatedDoubleMultiply(EmulatedDoubleSubtract(y, x), a));
}

/// Returns `0.0` when `x < edge`, otherwise `1.0`.
///
/// Quality: Covered by `emulatedDoubleStepMixAndSmoothstepMatchMetalDefinitions`.
static inline EmulatedDouble EmulatedDoubleStep(EmulatedDouble edge, EmulatedDouble x) {
    return EmulatedDoubleIsLessThan(x, edge) ? EmulatedDoubleZero(0u) : EmulatedDoubleOne();
}

/// Performs smooth Hermite interpolation between two edges.
///
/// Computes `t * t * (3 - 2 * t)` where `t = clamp((x - edge0) / (edge1 - edge0), 0, 1)`.
///
/// Quality: Covered by `emulatedDoubleStepMixAndSmoothstepMatchMetalDefinitions`.
static inline EmulatedDouble EmulatedDoubleSmoothstep(EmulatedDouble edge0, EmulatedDouble edge1, EmulatedDouble x) {
    EmulatedDouble t = EmulatedDoubleSaturate(EmulatedDoubleDivide(EmulatedDoubleSubtract(x, edge0), EmulatedDoubleSubtract(edge1, edge0)));
    return EmulatedDoubleMultiply(
        EmulatedDoubleMultiply(t, t),
        EmulatedDoubleSubtract(EmulatedDoubleThree(), EmulatedDoubleMultiply(EmulatedDoubleTwo(), t))
    );
}

/// Returns whether a finite value has any fractional bits.
///
/// - Parameter value: A finite encoded binary64 value.
/// - Returns: `1` when `value` is not mathematically integral, otherwise `0`.
///
/// Quality: Covered by `emulatedDoubleRoundingFunctionsMatchSwiftForRepresentativeValues`.
static inline uint32_t EmulatedDoubleHasFractionalPart(EmulatedDouble value) {
    if (!EmulatedDoubleIsFinite(value) || EmulatedDoubleIsZero(value)) {
        return 0u;
    }

    int32_t exponent = (int32_t)EmulatedDoubleExponentBits(value) - (int32_t)EMULATED_DOUBLE_EXPONENT_BIAS;
    if (EmulatedDoubleExponentBits(value) == 0u) {
        exponent = -1022;
    }
    if (exponent < 0) {
        return 1u;
    }
    if (exponent >= 52) {
        return 0u;
    }

    uint64_t fraction = ((uint64_t)EmulatedDoubleFractionHighBits(value) << 32u) | (uint64_t)value.lowBits;
    uint32_t fractionalBits = (uint32_t)(52 - exponent);
    uint64_t mask = ((uint64_t)1u << fractionalBits) - 1u;
    return (fraction & mask) != 0u ? 1u : 0u;
}

/// Rounds a finite value toward zero.
///
/// - Parameter value: Encoded binary64 value.
/// - Returns: Integral value with the fractional bits discarded.
///
/// Quality: Covered by `emulatedDoubleRoundingFunctionsMatchSwiftForRepresentativeValues`.
static inline EmulatedDouble EmulatedDoubleTrunc(EmulatedDouble value) {
    if (!EmulatedDoubleIsFinite(value) || EmulatedDoubleIsZero(value)) {
        return value;
    }

    int32_t exponent = (int32_t)EmulatedDoubleExponentBits(value) - (int32_t)EMULATED_DOUBLE_EXPONENT_BIAS;
    if (EmulatedDoubleExponentBits(value) == 0u || exponent < 0) {
        return EmulatedDoubleZero(EmulatedDoubleSignBit(value));
    }
    if (exponent >= 52) {
        return value;
    }

    uint32_t fractionalBits = (uint32_t)(52 - exponent);
    uint64_t fraction = ((uint64_t)EmulatedDoubleFractionHighBits(value) << 32u) | (uint64_t)value.lowBits;
    uint64_t mask = ((uint64_t)1u << fractionalBits) - 1u;
    fraction &= ~mask;
    return EmulatedDoubleMake(
        (value.highBits & (EMULATED_DOUBLE_SIGN_MASK | EMULATED_DOUBLE_EXPONENT_MASK)) | (uint32_t)(fraction >> 32u),
        (uint32_t)(fraction & 0xffffffffu)
    );
}

/// Rounds a value toward negative infinity.
///
/// Quality: Covered by `emulatedDoubleRoundingFunctionsMatchSwiftForRepresentativeValues`.
static inline EmulatedDouble EmulatedDoubleFloor(EmulatedDouble value) {
    if (!EmulatedDoubleIsFinite(value) || EmulatedDoubleIsZero(value)) {
        return value;
    }
    EmulatedDouble truncated = EmulatedDoubleTrunc(value);
    if (EmulatedDoubleSignBit(value) && EmulatedDoubleHasFractionalPart(value)) {
        return EmulatedDoubleSubtract(truncated, EmulatedDoubleOne());
    }
    return truncated;
}

/// Rounds a value toward positive infinity.
///
/// Quality: Covered by `emulatedDoubleRoundingFunctionsMatchSwiftForRepresentativeValues`.
static inline EmulatedDouble EmulatedDoubleCeil(EmulatedDouble value) {
    if (!EmulatedDoubleIsFinite(value) || EmulatedDoubleIsZero(value)) {
        return value;
    }
    EmulatedDouble truncated = EmulatedDoubleTrunc(value);
    if (!EmulatedDoubleSignBit(value) && EmulatedDoubleHasFractionalPart(value)) {
        return EmulatedDoubleAdd(truncated, EmulatedDoubleOne());
    }
    return truncated;
}

/// Returns whether a finite `EmulatedDouble` is mathematically integral.
///
/// Quality: Covered by `emulatedDoubleIntegralPredicatesDetectIntegralAndOddValues`.
static inline uint32_t EmulatedDoubleIsIntegral(EmulatedDouble value) {
    return EmulatedDoubleIsFinite(value) && !EmulatedDoubleHasFractionalPart(value) ? 1u : 0u;
}

/// Returns whether a finite integral `EmulatedDouble` is odd.
///
/// Quality: Covered by `emulatedDoubleIntegralPredicatesDetectIntegralAndOddValues`.
static inline uint32_t EmulatedDoubleIsOddInteger(EmulatedDouble value) {
    if (!EmulatedDoubleIsIntegral(value) || EmulatedDoubleIsZero(value)) {
        return 0u;
    }

    int32_t exponent = 0;
    uint64_t significand = EmulatedDoubleFiniteSignificand(value, &exponent);
    EmulatedDoubleNormalizeFiniteSignificand(&significand, &exponent);
    if (exponent < 0 || exponent > 52) {
        return 0u;
    }
    return ((significand >> (EMULATED_DOUBLE_FRACTION_BITS - (uint32_t)exponent)) & 1u) != 0u ? 1u : 0u;
}

/// Rounds a value to the nearest integral value, with halfway cases rounded away from zero.
///
/// Quality: Covered by `emulatedDoubleRoundingFunctionsMatchSwiftForRepresentativeValues`.
static inline EmulatedDouble EmulatedDoubleRound(EmulatedDouble value) {
    if (!EmulatedDoubleIsFinite(value) || EmulatedDoubleIsZero(value)) {
        return value;
    }
    EmulatedDouble truncated = EmulatedDoubleTrunc(value);
    EmulatedDouble fraction = EmulatedDoubleAbs(EmulatedDoubleSubtract(value, truncated));
    if (EmulatedDoubleIsLessThan(fraction, EmulatedDoubleHalf())) {
        return truncated;
    }
    return EmulatedDoubleSignBit(value) ?
        EmulatedDoubleSubtract(truncated, EmulatedDoubleOne()) :
        EmulatedDoubleAdd(truncated, EmulatedDoubleOne());
}

/// Rounds a value to the nearest integral value, with halfway cases rounded to even.
///
/// Quality: Covered by `emulatedDoubleRoundingFunctionsMatchSwiftForRepresentativeValues`.
static inline EmulatedDouble EmulatedDoubleRint(EmulatedDouble value) {
    if (!EmulatedDoubleIsFinite(value) || EmulatedDoubleIsZero(value)) {
        return value;
    }
    EmulatedDouble truncated = EmulatedDoubleTrunc(value);
    EmulatedDouble fraction = EmulatedDoubleAbs(EmulatedDoubleSubtract(value, truncated));
    if (EmulatedDoubleIsLessThan(fraction, EmulatedDoubleHalf())) {
        return truncated;
    }
    if (EmulatedDoubleIsGreaterThan(fraction, EmulatedDoubleHalf())) {
        return EmulatedDoubleSignBit(value) ?
            EmulatedDoubleSubtract(truncated, EmulatedDoubleOne()) :
            EmulatedDoubleAdd(truncated, EmulatedDoubleOne());
    }
    if (EmulatedDoubleIsOddInteger(truncated)) {
        return EmulatedDoubleSignBit(value) ?
            EmulatedDoubleSubtract(truncated, EmulatedDoubleOne()) :
            EmulatedDoubleAdd(truncated, EmulatedDoubleOne());
    }
    return truncated;
}

/// Returns the fractional part of a value in the range `[0, 1)`.
///
/// Quality: Covered by `emulatedDoubleFmodFractAndModfFollowDefinitions`.
static inline EmulatedDouble EmulatedDoubleFract(EmulatedDouble value) {
    if (!EmulatedDoubleIsFinite(value)) {
        return EmulatedDoubleDefaultNaN();
    }
    EmulatedDouble result = EmulatedDoubleSubtract(value, EmulatedDoubleFloor(value));
    return EmulatedDoubleIsZero(result) ? EmulatedDoubleZero(0u) : result;
}

/// Computes the floating-point remainder `x - y * trunc(x / y)`.
///
/// Quality: Covered by `emulatedDoubleFmodFractAndModfFollowDefinitions`.
static inline EmulatedDouble EmulatedDoubleFmod(EmulatedDouble x, EmulatedDouble y) {
    if (EmulatedDoubleIsNaN(x)) {
        return EmulatedDoubleQuietNaN(x);
    }
    if (EmulatedDoubleIsNaN(y)) {
        return EmulatedDoubleQuietNaN(y);
    }
    if (EmulatedDoubleIsInfinity(x) || EmulatedDoubleIsZero(y)) {
        return EmulatedDoubleDefaultNaN();
    }
    if (EmulatedDoubleIsInfinity(y) || EmulatedDoubleIsZero(x)) {
        return x;
    }
    return EmulatedDoubleSubtract(x, EmulatedDoubleMultiply(y, EmulatedDoubleTrunc(EmulatedDoubleDivide(x, y))));
}

/// Extracts a mantissa in `[0.5, 1)` and a base-2 exponent.
///
/// The result satisfies `value == mantissa * 2^exponent` for finite values. Zero stores exponent
/// `0` and returns signed zero.
///
/// Quality: Covered by `emulatedDoubleFrexpLdexpAndIlogbDecomposeFiniteValues`.
static inline EmulatedDouble EmulatedDoubleFrexp(EmulatedDouble value, EMULATED_DOUBLE_THREAD int32_t *exponent) {
    if (EmulatedDoubleIsZero(value)) {
        *exponent = 0;
        return value;
    }
    if (!EmulatedDoubleIsFinite(value)) {
        *exponent = 0;
        return value;
    }

    int32_t valueExponent = 0;
    uint64_t significand = EmulatedDoubleFiniteSignificand(value, &valueExponent);
    EmulatedDoubleNormalizeFiniteSignificand(&significand, &valueExponent);
    *exponent = valueExponent + 1;
    return EmulatedDoubleRoundAndPack(EmulatedDoubleSignBit(value), -1, significand << EMULATED_DOUBLE_ROUNDING_BITS);
}

/// Multiplies an `EmulatedDouble` by `2^exponent`.
///
/// Quality: Covered by `emulatedDoubleFrexpLdexpAndIlogbDecomposeFiniteValues`.
static inline EmulatedDouble EmulatedDoubleLdexp(EmulatedDouble value, int32_t exponent) {
    if (!EmulatedDoubleIsFinite(value) || EmulatedDoubleIsZero(value)) {
        return value;
    }

    int32_t valueExponent = 0;
    uint64_t significand = EmulatedDoubleFiniteSignificand(value, &valueExponent);
    EmulatedDoubleNormalizeFiniteSignificand(&significand, &valueExponent);
    return EmulatedDoubleRoundAndPack(EmulatedDoubleSignBit(value), valueExponent + exponent, significand << EMULATED_DOUBLE_ROUNDING_BITS);
}

/// Returns the unbiased binary exponent of a value as an integer.
///
/// Zero and NaN return `INT32_MIN`; infinities return `INT32_MAX`.
///
/// Quality: Covered by `emulatedDoubleFrexpLdexpAndIlogbDecomposeFiniteValues`.
static inline int32_t EmulatedDoubleIlogb(EmulatedDouble value) {
    if (EmulatedDoubleIsZero(value) || EmulatedDoubleIsNaN(value)) {
        return (int32_t)0x80000000u;
    }
    if (EmulatedDoubleIsInfinity(value)) {
        return (int32_t)0x7fffffffu;
    }

    int32_t exponent = 0;
    uint64_t significand = EmulatedDoubleFiniteSignificand(value, &exponent);
    EmulatedDoubleNormalizeFiniteSignificand(&significand, &exponent);
    return exponent;
}

/// Decomposes a value into signed fractional and integral parts.
///
/// - Parameters:
///   - value: Encoded binary64 value.
///   - integerPart: Output storage for the integral part.
/// - Returns: The signed fractional part.
///
/// Quality: Covered by `emulatedDoubleFmodFractAndModfFollowDefinitions`.
static inline EmulatedDouble EmulatedDoubleModf(EmulatedDouble value, EMULATED_DOUBLE_THREAD EmulatedDouble *integerPart) {
    if (EmulatedDoubleIsNaN(value)) {
        *integerPart = EmulatedDoubleQuietNaN(value);
        return EmulatedDoubleQuietNaN(value);
    }
    if (EmulatedDoubleIsInfinity(value)) {
        *integerPart = value;
        return EmulatedDoubleZero(EmulatedDoubleSignBit(value));
    }
    *integerPart = EmulatedDoubleTrunc(value);
    EmulatedDouble fraction = EmulatedDoubleSubtract(value, *integerPart);
    return EmulatedDoubleIsZero(fraction) ? EmulatedDoubleZero(EmulatedDoubleSignBit(value)) : fraction;
}

/// Returns the next representable value after `x` in the direction of `y`.
///
/// Quality: Covered by `emulatedDoubleNextafterMovesOneRepresentableStep`.
static inline EmulatedDouble EmulatedDoubleNextafter(EmulatedDouble x, EmulatedDouble y) {
    if (EmulatedDoubleIsNaN(x)) {
        return EmulatedDoubleQuietNaN(x);
    }
    if (EmulatedDoubleIsNaN(y)) {
        return EmulatedDoubleQuietNaN(y);
    }
    if (EmulatedDoubleIsEqual(x, y)) {
        return y;
    }
    if (EmulatedDoubleIsZero(x)) {
        return EmulatedDoubleMake(y.highBits & EMULATED_DOUBLE_SIGN_MASK, 1u);
    }

    uint32_t moveUp = EmulatedDoubleIsLessThan(x, y);
    uint32_t incrementBits = EmulatedDoubleSignBit(x) ? !moveUp : moveUp;
    EmulatedDouble result = x;
    if (incrementBits) {
        result.lowBits += 1u;
        if (result.lowBits == 0u) {
            result.highBits += 1u;
        }
    } else {
        if (result.lowBits == 0u) {
            result.highBits -= 1u;
        }
        result.lowBits -= 1u;
    }
    return result;
}

/// Computes the positive difference `x - y` when `x > y`, otherwise positive zero.
///
/// Quality: Covered by `emulatedDoubleFdimAndFmaFollowDefinitions`.
static inline EmulatedDouble EmulatedDoubleFdim(EmulatedDouble x, EmulatedDouble y) {
    if (EmulatedDoubleIsNaN(x)) {
        return EmulatedDoubleQuietNaN(x);
    }
    if (EmulatedDoubleIsNaN(y)) {
        return EmulatedDoubleQuietNaN(y);
    }
    return EmulatedDoubleIsGreaterThan(x, y) ? EmulatedDoubleSubtract(x, y) : EmulatedDoubleZero(0u);
}

/// Computes `a * b + c` using `EmulatedDouble` arithmetic.
///
/// The product and sum are evaluated through the software binary64 multiply and add helpers. This
/// keeps the implementation independent of native `double` and Metal's native floating-point
/// functions.
///
/// Quality: Covered by `emulatedDoubleFdimAndFmaFollowDefinitions`.
static inline EmulatedDouble EmulatedDoubleFma(EmulatedDouble a, EmulatedDouble b, EmulatedDouble c) {
    return EmulatedDoubleAdd(EmulatedDoubleMultiply(a, b), c);
}

/// Computes the reciprocal square root of an `EmulatedDouble`.
///
/// Quality: Covered by `emulatedDoubleRsqrtMatchesReciprocalSqrtDefinition`.
static inline EmulatedDouble EmulatedDoubleRsqrt(EmulatedDouble value) {
    return EmulatedDoubleDivide(EmulatedDoubleOne(), EmulatedDoubleSqrt(value));
}

/// Reduces a finite angle to a small remainder around zero and returns the quadrant.
///
/// The reduction uses only `EmulatedDouble` arithmetic and integer conversion after reducing by
/// `2π`. It is used by `sin`, `cos`, and `tan` helpers to avoid native math dependencies.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues`.
static inline int32_t EmulatedDoubleReduceAngle(EmulatedDouble value, EMULATED_DOUBLE_THREAD EmulatedDouble *remainder) {
    EmulatedDouble wrapped = EmulatedDoubleFmod(value, EmulatedDoubleTwoPi());
    EmulatedDouble scaled = EmulatedDoubleDivide(wrapped, EmulatedDoubleHalfPi());
    EmulatedDouble nearest = EmulatedDoubleRint(scaled);
    int32_t quadrant = EmulatedDoubleToInt32Saturating(nearest);
    *remainder = EmulatedDoubleSubtract(wrapped, EmulatedDoubleMultiply(EmulatedDoubleFromInt32(quadrant), EmulatedDoubleHalfPi()));
    int32_t mod = quadrant % 4;
    return mod < 0 ? mod + 4 : mod;
}

/// Evaluates the sine kernel on a reduced argument near zero.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleSinKernel(EmulatedDouble x) {
    EmulatedDouble x2 = EmulatedDoubleMultiply(x, x);
    EmulatedDouble result = EmulatedDoubleMake(0x3bd71b8eu, 0xf6dcf572u);
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbc62f49bu, 0x46814157u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0x3ce952c7u, 0x7030ad4au));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbd6ae7f3u, 0xe733b81fu));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0x3de61246u, 0x13a86d09u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbe5ae645u, 0x67f544e4u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0x3ec71de3u, 0xa556c734u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbf2a01a0u, 0x1a01a01au));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0x3f811111u, 0x11111111u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbfc55555u, 0x55555555u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleOne());
    return EmulatedDoubleMultiply(x, result);
}

/// Evaluates the cosine kernel on a reduced argument near zero.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleCosKernel(EmulatedDouble x) {
    EmulatedDouble x2 = EmulatedDoubleMultiply(x, x);
    EmulatedDouble result = EmulatedDoubleMake(0x3c1e542bu, 0xa4020225u);
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbca68278u, 0x63b97d97u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0x3d2ae7f3u, 0xe733b81fu));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbda93974u, 0xa8c07c9du));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0x3e21eed8u, 0xeff8d898u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbe927e4fu, 0xb7789f5cu));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0x3efa01a0u, 0x1a01a01au));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbf56c16cu, 0x16c16c17u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0x3fa55555u, 0x55555555u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleMake(0xbfe00000u, 0x00000000u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x2), EmulatedDoubleOne());
    return result;
}

/// Computes sine using reduced-argument polynomial kernels.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues` and
/// `emulatedDoubleTranscendentalFunctionsHandleSpecialValues`.
static inline EmulatedDouble EmulatedDoubleSin(EmulatedDouble value) {
    if (EmulatedDoubleIsNaN(value)) {
        return EmulatedDoubleQuietNaN(value);
    }
    if (EmulatedDoubleIsInfinity(value)) {
        return EmulatedDoubleDefaultNaN();
    }
    EmulatedDouble reduced = EmulatedDoubleZero(0u);
    int32_t quadrant = EmulatedDoubleReduceAngle(value, &reduced);
    if (quadrant == 0) {
        return EmulatedDoubleSinKernel(reduced);
    }
    if (quadrant == 1) {
        return EmulatedDoubleCosKernel(reduced);
    }
    if (quadrant == 2) {
        return EmulatedDoubleNegated(EmulatedDoubleSinKernel(reduced));
    }
    return EmulatedDoubleNegated(EmulatedDoubleCosKernel(reduced));
}

/// Computes cosine using reduced-argument polynomial kernels.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues` and
/// `emulatedDoubleTranscendentalFunctionsHandleSpecialValues`.
static inline EmulatedDouble EmulatedDoubleCos(EmulatedDouble value) {
    if (EmulatedDoubleIsNaN(value)) {
        return EmulatedDoubleQuietNaN(value);
    }
    if (EmulatedDoubleIsInfinity(value)) {
        return EmulatedDoubleDefaultNaN();
    }
    EmulatedDouble reduced = EmulatedDoubleZero(0u);
    int32_t quadrant = EmulatedDoubleReduceAngle(value, &reduced);
    if (quadrant == 0) {
        return EmulatedDoubleCosKernel(reduced);
    }
    if (quadrant == 1) {
        return EmulatedDoubleNegated(EmulatedDoubleSinKernel(reduced));
    }
    if (quadrant == 2) {
        return EmulatedDoubleNegated(EmulatedDoubleCosKernel(reduced));
    }
    return EmulatedDoubleSinKernel(reduced);
}

/// Computes sine and cosine together.
///
/// - Parameters:
///   - value: Input angle in radians.
///   - cosineValue: Output storage for `cos(value)`.
/// - Returns: `sin(value)`.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleSincos(EmulatedDouble value, EMULATED_DOUBLE_THREAD EmulatedDouble *cosineValue) {
    *cosineValue = EmulatedDoubleCos(value);
    return EmulatedDoubleSin(value);
}

/// Computes tangent as `sin(x) / cos(x)`.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleTan(EmulatedDouble value) {
    return EmulatedDoubleDivide(EmulatedDoubleSin(value), EmulatedDoubleCos(value));
}

/// Computes `sin(πx)`.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleSinPi(EmulatedDouble value) {
    return EmulatedDoubleSin(EmulatedDoubleMultiply(value, EmulatedDoublePi()));
}

/// Computes `cos(πx)`.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleCosPi(EmulatedDouble value) {
    return EmulatedDoubleCos(EmulatedDoubleMultiply(value, EmulatedDoublePi()));
}

/// Computes `tan(πx)`.
///
/// Quality: Covered by `emulatedDoubleTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleTanPi(EmulatedDouble value) {
    return EmulatedDoubleTan(EmulatedDoubleMultiply(value, EmulatedDoublePi()));
}

/// Evaluates `exp(x)` for a small reduced argument.
///
/// Quality: Covered by `emulatedDoubleExpLogAndPowMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleExpReduced(EmulatedDouble x) {
    EmulatedDouble result = EmulatedDoubleMake(0x3c1e542bu, 0xa4020225u);
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3c62f49bu, 0x46814157u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3ca68278u, 0x63b97d97u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3ce952c7u, 0x7030ad4au));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3d2ae7f3u, 0xe733b81fu));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3d6ae7f3u, 0xe733b81fu));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3da93974u, 0xa8c07c9du));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3de61246u, 0x13a86d09u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3e21eed8u, 0xeff8d898u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3e5ae645u, 0x67f544e4u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3e927e4fu, 0xb7789f5cu));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3ec71de3u, 0xa556c734u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3efa01a0u, 0x1a01a01au));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3f2a01a0u, 0x1a01a01au));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3f56c16cu, 0x16c16c17u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3f811111u, 0x11111111u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3fa55555u, 0x55555555u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleMake(0x3fc55555u, 0x55555555u));
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleHalf());
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleOne());
    result = EmulatedDoubleAdd(EmulatedDoubleMultiply(result, x), EmulatedDoubleOne());
    return result;
}

/// Computes the base-2 exponential.
///
/// Quality: Covered by `emulatedDoubleExpLogAndPowMatchReferenceValues` and
/// `emulatedDoubleTranscendentalFunctionsHandleSpecialValues`.
static inline EmulatedDouble EmulatedDoubleExp2(EmulatedDouble value) {
    if (EmulatedDoubleIsNaN(value)) {
        return EmulatedDoubleQuietNaN(value);
    }
    if (EmulatedDoubleIsInfinity(value)) {
        return EmulatedDoubleSignBit(value) ? EmulatedDoubleZero(0u) : EmulatedDoubleInfinity(0u);
    }
    if (EmulatedDoubleIsGreaterThan(value, EmulatedDoubleMake(0x40900000u, 0x00000000u))) {
        return EmulatedDoubleInfinity(0u);
    }
    if (EmulatedDoubleIsLessThan(value, EmulatedDoubleMake(0xc090e000u, 0x00000000u))) {
        return EmulatedDoubleZero(0u);
    }

    EmulatedDouble integral = EmulatedDoubleFloor(value);
    EmulatedDouble fraction = EmulatedDoubleSubtract(value, integral);
    int32_t exponent = EmulatedDoubleToInt32Saturating(integral);
    return EmulatedDoubleLdexp(EmulatedDoubleExpReduced(EmulatedDoubleMultiply(fraction, EmulatedDoubleLn2())), exponent);
}

/// Computes the natural exponential.
///
/// Quality: Covered by `emulatedDoubleExpLogAndPowMatchReferenceValues` and
/// `emulatedDoubleTranscendentalFunctionsHandleSpecialValues`.
static inline EmulatedDouble EmulatedDoubleExp(EmulatedDouble value) {
    if (EmulatedDoubleIsGreaterThan(value, EmulatedDoubleMake(0x40862e42u, 0xfefa39efu))) {
        return EmulatedDoubleInfinity(0u);
    }
    if (EmulatedDoubleIsLessThan(value, EmulatedDoubleMake(0xc0874910u, 0xd52d3051u))) {
        return EmulatedDoubleZero(0u);
    }
    return EmulatedDoubleExp2(EmulatedDoubleMultiply(value, EmulatedDoubleInvLn2()));
}

/// Computes the base-10 exponential.
///
/// Quality: Covered by `emulatedDoubleExpLogAndPowMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleExp10(EmulatedDouble value) {
    return EmulatedDoubleExp2(EmulatedDoubleMultiply(value, EmulatedDoubleLog2_10()));
}

/// Computes natural logarithm using binary exponent extraction and an atanh series.
///
/// Quality: Covered by `emulatedDoubleExpLogAndPowMatchReferenceValues` and
/// `emulatedDoubleTranscendentalFunctionsHandleSpecialValues`.
static inline EmulatedDouble EmulatedDoubleLog(EmulatedDouble value) {
    if (EmulatedDoubleIsNaN(value)) {
        return EmulatedDoubleQuietNaN(value);
    }
    if (EmulatedDoubleIsZero(value)) {
        return EmulatedDoubleInfinity(1u);
    }
    if (EmulatedDoubleSignBit(value)) {
        return EmulatedDoubleDefaultNaN();
    }
    if (EmulatedDoubleIsInfinity(value)) {
        return value;
    }

    int32_t exponent = 0;
    EmulatedDouble mantissa = EmulatedDoubleFrexp(value, &exponent);
    EmulatedDouble sqrtHalf = EmulatedDoubleMake(0x3fe6a09eu, 0x667f3bcdu);
    if (EmulatedDoubleIsLessThan(mantissa, sqrtHalf)) {
        mantissa = EmulatedDoubleMultiply(mantissa, EmulatedDoubleTwo());
        exponent -= 1;
    }

    EmulatedDouble z = EmulatedDoubleDivide(
        EmulatedDoubleSubtract(mantissa, EmulatedDoubleOne()),
        EmulatedDoubleAdd(mantissa, EmulatedDoubleOne())
    );
    EmulatedDouble z2 = EmulatedDoubleMultiply(z, z);
    EmulatedDouble term = z;
    EmulatedDouble sum = z;
    for (int32_t denominator = 3; denominator <= 79; denominator += 2) {
        term = EmulatedDoubleMultiply(term, z2);
        sum = EmulatedDoubleAdd(sum, EmulatedDoubleDivide(term, EmulatedDoubleFromInt32(denominator)));
    }
    EmulatedDouble logMantissa = EmulatedDoubleMultiply(EmulatedDoubleTwo(), sum);
    return EmulatedDoubleAdd(logMantissa, EmulatedDoubleMultiply(EmulatedDoubleFromInt32(exponent), EmulatedDoubleLn2()));
}

/// Computes base-2 logarithm.
///
/// Quality: Covered by `emulatedDoubleExpLogAndPowMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleLog2(EmulatedDouble value) {
    return EmulatedDoubleMultiply(EmulatedDoubleLog(value), EmulatedDoubleInvLn2());
}

/// Computes base-10 logarithm.
///
/// Quality: Covered by `emulatedDoubleExpLogAndPowMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleLog10(EmulatedDouble value) {
    return EmulatedDoubleMultiply(EmulatedDoubleLog(value), EmulatedDoubleInvLn10());
}

/// Computes arctangent for a reduced argument with `|x| <= 1`.
///
/// Quality: Covered by `emulatedDoubleInverseTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleAtanReduced(EmulatedDouble value) {
    EmulatedDouble x2 = EmulatedDoubleMultiply(value, value);
    EmulatedDouble term = value;
    EmulatedDouble sum = value;
    uint32_t subtractTerm = 1u;
    for (int32_t denominator = 3; denominator <= 81; denominator += 2) {
        term = EmulatedDoubleMultiply(term, x2);
        EmulatedDouble addend = EmulatedDoubleDivide(term, EmulatedDoubleFromInt32(denominator));
        sum = subtractTerm ? EmulatedDoubleSubtract(sum, addend) : EmulatedDoubleAdd(sum, addend);
        subtractTerm = !subtractTerm;
    }
    return sum;
}

/// Computes arctangent.
///
/// Quality: Covered by `emulatedDoubleInverseTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleAtan(EmulatedDouble value) {
    if (EmulatedDoubleIsNaN(value)) {
        return EmulatedDoubleQuietNaN(value);
    }
    if (EmulatedDoubleIsInfinity(value)) {
        return EmulatedDoubleCopySign(EmulatedDoubleHalfPi(), value);
    }

    uint32_t sign = EmulatedDoubleSignBit(value);
    EmulatedDouble x = EmulatedDoubleAbs(value);
    EmulatedDouble result = EmulatedDoubleZero(0u);
    EmulatedDouble sqrt2MinusOne = EmulatedDoubleMake(0x3fda8279u, 0x99fcef34u);

    if (EmulatedDoubleIsGreaterThan(x, EmulatedDoubleOne())) {
        result = EmulatedDoubleSubtract(EmulatedDoubleHalfPi(), EmulatedDoubleAtanReduced(EmulatedDoubleDivide(EmulatedDoubleOne(), x)));
    } else if (EmulatedDoubleIsGreaterThan(x, sqrt2MinusOne)) {
        result = EmulatedDoubleAdd(
            EmulatedDoubleQuarterPi(),
            EmulatedDoubleAtanReduced(EmulatedDoubleDivide(EmulatedDoubleSubtract(x, EmulatedDoubleOne()), EmulatedDoubleAdd(x, EmulatedDoubleOne())))
        );
    } else {
        result = EmulatedDoubleAtanReduced(x);
    }
    return sign ? EmulatedDoubleNegated(result) : result;
}

/// Computes arctangent of `y/x` using quadrant information.
///
/// Quality: Covered by `emulatedDoubleInverseTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleAtan2(EmulatedDouble y, EmulatedDouble x) {
    if (EmulatedDoubleIsNaN(y)) {
        return EmulatedDoubleQuietNaN(y);
    }
    if (EmulatedDoubleIsNaN(x)) {
        return EmulatedDoubleQuietNaN(x);
    }
    if (EmulatedDoubleIsZero(x)) {
        if (EmulatedDoubleIsZero(y)) {
            return EmulatedDoubleCopySign(EmulatedDoubleZero(0u), y);
        }
        return EmulatedDoubleCopySign(EmulatedDoubleHalfPi(), y);
    }
    EmulatedDouble base = EmulatedDoubleAtan(EmulatedDoubleDivide(y, x));
    if (!EmulatedDoubleSignBit(x)) {
        return base;
    }
    return EmulatedDoubleSignBit(y) ? EmulatedDoubleSubtract(base, EmulatedDoublePi()) : EmulatedDoubleAdd(base, EmulatedDoublePi());
}

/// Computes arcsine.
///
/// Quality: Covered by `emulatedDoubleInverseTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleAsin(EmulatedDouble value) {
    EmulatedDouble magnitude = EmulatedDoubleAbs(value);
    if (EmulatedDoubleIsGreaterThan(magnitude, EmulatedDoubleOne())) {
        return EmulatedDoubleDefaultNaN();
    }
    if (EmulatedDoubleIsGreaterThan(magnitude, EmulatedDoubleHalf())) {
        EmulatedDouble reduced = EmulatedDoubleSqrt(
            EmulatedDoubleMultiply(EmulatedDoubleHalf(), EmulatedDoubleSubtract(EmulatedDoubleOne(), magnitude))
        );
        EmulatedDouble result = EmulatedDoubleSubtract(
            EmulatedDoubleHalfPi(),
            EmulatedDoubleMultiply(EmulatedDoubleTwo(), EmulatedDoubleAsin(reduced))
        );
        return EmulatedDoubleSignBit(value) ? EmulatedDoubleNegated(result) : result;
    }
    return EmulatedDoubleAtan2(value, EmulatedDoubleSqrt(EmulatedDoubleSubtract(EmulatedDoubleOne(), EmulatedDoubleMultiply(value, value))));
}

/// Computes arccosine.
///
/// Quality: Covered by `emulatedDoubleInverseTrigonometricFunctionsMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoubleAcos(EmulatedDouble value) {
    if (EmulatedDoubleIsGreaterThan(EmulatedDoubleAbs(value), EmulatedDoubleOne())) {
        return EmulatedDoubleDefaultNaN();
    }
    return EmulatedDoubleSubtract(EmulatedDoubleHalfPi(), EmulatedDoubleAsin(value));
}

/// Computes hyperbolic sine.
///
/// Quality: Covered by `emulatedDoubleHyperbolicFunctionsMatchDefinitions`.
static inline EmulatedDouble EmulatedDoubleSinh(EmulatedDouble value) {
    EmulatedDouble positive = EmulatedDoubleExp(value);
    EmulatedDouble negative = EmulatedDoubleExp(EmulatedDoubleNegated(value));
    return EmulatedDoubleMultiply(EmulatedDoubleHalf(), EmulatedDoubleSubtract(positive, negative));
}

/// Computes hyperbolic cosine.
///
/// Quality: Covered by `emulatedDoubleHyperbolicFunctionsMatchDefinitions`.
static inline EmulatedDouble EmulatedDoubleCosh(EmulatedDouble value) {
    EmulatedDouble positive = EmulatedDoubleExp(value);
    EmulatedDouble negative = EmulatedDoubleExp(EmulatedDoubleNegated(value));
    return EmulatedDoubleMultiply(EmulatedDoubleHalf(), EmulatedDoubleAdd(positive, negative));
}

/// Computes hyperbolic tangent.
///
/// Quality: Covered by `emulatedDoubleHyperbolicFunctionsMatchDefinitions`.
static inline EmulatedDouble EmulatedDoubleTanh(EmulatedDouble value) {
    EmulatedDouble doubled = EmulatedDoubleAdd(value, value);
    EmulatedDouble e = EmulatedDoubleExp(doubled);
    return EmulatedDoubleDivide(EmulatedDoubleSubtract(e, EmulatedDoubleOne()), EmulatedDoubleAdd(e, EmulatedDoubleOne()));
}

/// Computes inverse hyperbolic sine.
///
/// Quality: Covered by `emulatedDoubleHyperbolicFunctionsMatchDefinitions`.
static inline EmulatedDouble EmulatedDoubleAsinh(EmulatedDouble value) {
    return EmulatedDoubleLog(EmulatedDoubleAdd(value, EmulatedDoubleSqrt(EmulatedDoubleAdd(EmulatedDoubleMultiply(value, value), EmulatedDoubleOne()))));
}

/// Computes inverse hyperbolic cosine.
///
/// Quality: Covered by `emulatedDoubleHyperbolicFunctionsMatchDefinitions`.
static inline EmulatedDouble EmulatedDoubleAcosh(EmulatedDouble value) {
    if (EmulatedDoubleIsLessThan(value, EmulatedDoubleOne())) {
        return EmulatedDoubleDefaultNaN();
    }
    return EmulatedDoubleLog(EmulatedDoubleAdd(value, EmulatedDoubleSqrt(EmulatedDoubleSubtract(EmulatedDoubleMultiply(value, value), EmulatedDoubleOne()))));
}

/// Computes inverse hyperbolic tangent.
///
/// Quality: Covered by `emulatedDoubleHyperbolicFunctionsMatchDefinitions`.
static inline EmulatedDouble EmulatedDoubleAtanh(EmulatedDouble value) {
    if (!EmulatedDoubleIsLessThan(EmulatedDoubleAbs(value), EmulatedDoubleOne())) {
        if (EmulatedDoubleIsEqual(EmulatedDoubleAbs(value), EmulatedDoubleOne())) {
            return EmulatedDoubleInfinity(EmulatedDoubleSignBit(value));
        }
        return EmulatedDoubleDefaultNaN();
    }
    return EmulatedDoubleMultiply(
        EmulatedDoubleHalf(),
        EmulatedDoubleLog(EmulatedDoubleDivide(EmulatedDoubleAdd(EmulatedDoubleOne(), value), EmulatedDoubleSubtract(EmulatedDoubleOne(), value)))
    );
}

/// Computes `x` raised to `y`.
///
/// Negative bases are supported when `y` is integral. Nonintegral exponents with a negative base
/// produce NaN.
///
/// Quality: Covered by `emulatedDoubleExpLogAndPowMatchReferenceValues` and
/// `emulatedDoubleTranscendentalFunctionsHandleSpecialValues`.
static inline EmulatedDouble EmulatedDoublePow(EmulatedDouble x, EmulatedDouble y) {
    if (EmulatedDoubleIsNaN(x)) {
        return EmulatedDoubleQuietNaN(x);
    }
    if (EmulatedDoubleIsNaN(y)) {
        return EmulatedDoubleQuietNaN(y);
    }
    if (EmulatedDoubleIsZero(y)) {
        return EmulatedDoubleOne();
    }
    if (EmulatedDoubleIsZero(x)) {
        if (EmulatedDoubleSignBit(y)) {
            return EmulatedDoubleInfinity(EmulatedDoubleSignBit(x) && EmulatedDoubleIsOddInteger(y));
        }
        return EmulatedDoubleZero(EmulatedDoubleSignBit(x) && EmulatedDoubleIsOddInteger(y));
    }

    uint32_t negateResult = 0u;
    EmulatedDouble base = x;
    if (EmulatedDoubleSignBit(x)) {
        if (!EmulatedDoubleIsIntegral(y)) {
            return EmulatedDoubleDefaultNaN();
        }
        negateResult = EmulatedDoubleIsOddInteger(y);
        base = EmulatedDoubleAbs(x);
    }

    EmulatedDouble result = EmulatedDoubleExp2(EmulatedDoubleMultiply(y, EmulatedDoubleLog2(base)));
    return negateResult ? EmulatedDoubleNegated(result) : result;
}

/// Computes `x` raised to `y` for nonnegative `x`.
///
/// Quality: Covered by `emulatedDoubleExpLogAndPowMatchReferenceValues`.
static inline EmulatedDouble EmulatedDoublePowr(EmulatedDouble x, EmulatedDouble y) {
    if (EmulatedDoubleSignBit(x) && !EmulatedDoubleIsZero(x)) {
        return EmulatedDoubleDefaultNaN();
    }
    return EmulatedDoublePow(x, y);
}

/// Two-component vector of `EmulatedDouble` values.
///
/// Components are named to match Metal's `float2` convention.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
typedef struct EmulatedDouble2 {
    /// First component.
    EmulatedDouble x;

    /// Second component.
    EmulatedDouble y;
} EmulatedDouble2;

/// Three-component vector of `EmulatedDouble` values.
///
/// Components are named to match Metal's `float3` convention.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
typedef struct EmulatedDouble3 {
    /// First component.
    EmulatedDouble x;

    /// Second component.
    EmulatedDouble y;

    /// Third component.
    EmulatedDouble z;
} EmulatedDouble3;

/// Four-component vector of `EmulatedDouble` values.
///
/// Components are named to match Metal's `float4` convention.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
typedef struct EmulatedDouble4 {
    /// First component.
    EmulatedDouble x;

    /// Second component.
    EmulatedDouble y;

    /// Third component.
    EmulatedDouble z;

    /// Fourth component.
    EmulatedDouble w;
} EmulatedDouble4;

/// Creates an `EmulatedDouble2` from two components.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
static inline EmulatedDouble2 EmulatedDouble2Make(EmulatedDouble x, EmulatedDouble y) {
    EmulatedDouble2 value;
    value.x = x;
    value.y = y;
    return value;
}

/// Creates an `EmulatedDouble3` from three components.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
static inline EmulatedDouble3 EmulatedDouble3Make(EmulatedDouble x, EmulatedDouble y, EmulatedDouble z) {
    EmulatedDouble3 value;
    value.x = x;
    value.y = y;
    value.z = z;
    return value;
}

/// Creates an `EmulatedDouble4` from four components.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
static inline EmulatedDouble4 EmulatedDouble4Make(
    EmulatedDouble x,
    EmulatedDouble y,
    EmulatedDouble z,
    EmulatedDouble w
) {
    EmulatedDouble4 value;
    value.x = x;
    value.y = y;
    value.z = z;
    value.w = w;
    return value;
}

/// Creates an `EmulatedDouble2` with all components set to `value`.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
static inline EmulatedDouble2 EmulatedDouble2Splat(EmulatedDouble value) {
    return EmulatedDouble2Make(value, value);
}

/// Creates an `EmulatedDouble3` with all components set to `value`.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
static inline EmulatedDouble3 EmulatedDouble3Splat(EmulatedDouble value) {
    return EmulatedDouble3Make(value, value, value);
}

/// Creates an `EmulatedDouble4` with all components set to `value`.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
static inline EmulatedDouble4 EmulatedDouble4Splat(EmulatedDouble value) {
    return EmulatedDouble4Make(value, value, value, value);
}

/// Creates an `EmulatedDouble2` positive-zero vector.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
static inline EmulatedDouble2 EmulatedDouble2Zero(void) {
    return EmulatedDouble2Splat(EmulatedDoubleZero(0u));
}

/// Creates an `EmulatedDouble3` positive-zero vector.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
static inline EmulatedDouble3 EmulatedDouble3Zero(void) {
    return EmulatedDouble3Splat(EmulatedDoubleZero(0u));
}

/// Creates an `EmulatedDouble4` positive-zero vector.
///
/// Quality: Covered by `emulatedDoubleVectorConstructorsPreserveComponents`.
static inline EmulatedDouble4 EmulatedDouble4Zero(void) {
    return EmulatedDouble4Splat(EmulatedDoubleZero(0u));
}

/// Applies `EmulatedDoubleAbs` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Abs(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleAbs(value.x),
        EmulatedDoubleAbs(value.y)
    );
}

/// Applies `EmulatedDoubleAbs` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Abs(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleAbs(value.x),
        EmulatedDoubleAbs(value.y),
        EmulatedDoubleAbs(value.z)
    );
}

/// Applies `EmulatedDoubleAbs` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Abs(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleAbs(value.x),
        EmulatedDoubleAbs(value.y),
        EmulatedDoubleAbs(value.z),
        EmulatedDoubleAbs(value.w)
    );
}

/// Applies `EmulatedDoubleFabs` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Fabs(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleFabs(value.x),
        EmulatedDoubleFabs(value.y)
    );
}

/// Applies `EmulatedDoubleFabs` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Fabs(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleFabs(value.x),
        EmulatedDoubleFabs(value.y),
        EmulatedDoubleFabs(value.z)
    );
}

/// Applies `EmulatedDoubleFabs` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Fabs(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleFabs(value.x),
        EmulatedDoubleFabs(value.y),
        EmulatedDoubleFabs(value.z),
        EmulatedDoubleFabs(value.w)
    );
}

/// Applies `EmulatedDoubleSign` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Sign(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleSign(value.x),
        EmulatedDoubleSign(value.y)
    );
}

/// Applies `EmulatedDoubleSign` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Sign(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleSign(value.x),
        EmulatedDoubleSign(value.y),
        EmulatedDoubleSign(value.z)
    );
}

/// Applies `EmulatedDoubleSign` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Sign(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleSign(value.x),
        EmulatedDoubleSign(value.y),
        EmulatedDoubleSign(value.z),
        EmulatedDoubleSign(value.w)
    );
}

/// Applies `EmulatedDoubleFloor` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Floor(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleFloor(value.x),
        EmulatedDoubleFloor(value.y)
    );
}

/// Applies `EmulatedDoubleFloor` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Floor(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleFloor(value.x),
        EmulatedDoubleFloor(value.y),
        EmulatedDoubleFloor(value.z)
    );
}

/// Applies `EmulatedDoubleFloor` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Floor(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleFloor(value.x),
        EmulatedDoubleFloor(value.y),
        EmulatedDoubleFloor(value.z),
        EmulatedDoubleFloor(value.w)
    );
}

/// Applies `EmulatedDoubleCeil` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Ceil(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleCeil(value.x),
        EmulatedDoubleCeil(value.y)
    );
}

/// Applies `EmulatedDoubleCeil` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Ceil(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleCeil(value.x),
        EmulatedDoubleCeil(value.y),
        EmulatedDoubleCeil(value.z)
    );
}

/// Applies `EmulatedDoubleCeil` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Ceil(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleCeil(value.x),
        EmulatedDoubleCeil(value.y),
        EmulatedDoubleCeil(value.z),
        EmulatedDoubleCeil(value.w)
    );
}

/// Applies `EmulatedDoubleTrunc` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Trunc(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleTrunc(value.x),
        EmulatedDoubleTrunc(value.y)
    );
}

/// Applies `EmulatedDoubleTrunc` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Trunc(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleTrunc(value.x),
        EmulatedDoubleTrunc(value.y),
        EmulatedDoubleTrunc(value.z)
    );
}

/// Applies `EmulatedDoubleTrunc` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Trunc(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleTrunc(value.x),
        EmulatedDoubleTrunc(value.y),
        EmulatedDoubleTrunc(value.z),
        EmulatedDoubleTrunc(value.w)
    );
}

/// Applies `EmulatedDoubleRint` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Rint(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleRint(value.x),
        EmulatedDoubleRint(value.y)
    );
}

/// Applies `EmulatedDoubleRint` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Rint(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleRint(value.x),
        EmulatedDoubleRint(value.y),
        EmulatedDoubleRint(value.z)
    );
}

/// Applies `EmulatedDoubleRint` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Rint(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleRint(value.x),
        EmulatedDoubleRint(value.y),
        EmulatedDoubleRint(value.z),
        EmulatedDoubleRint(value.w)
    );
}

/// Applies `EmulatedDoubleRound` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Round(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleRound(value.x),
        EmulatedDoubleRound(value.y)
    );
}

/// Applies `EmulatedDoubleRound` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Round(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleRound(value.x),
        EmulatedDoubleRound(value.y),
        EmulatedDoubleRound(value.z)
    );
}

/// Applies `EmulatedDoubleRound` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Round(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleRound(value.x),
        EmulatedDoubleRound(value.y),
        EmulatedDoubleRound(value.z),
        EmulatedDoubleRound(value.w)
    );
}

/// Applies `EmulatedDoubleFract` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Fract(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleFract(value.x),
        EmulatedDoubleFract(value.y)
    );
}

/// Applies `EmulatedDoubleFract` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Fract(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleFract(value.x),
        EmulatedDoubleFract(value.y),
        EmulatedDoubleFract(value.z)
    );
}

/// Applies `EmulatedDoubleFract` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Fract(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleFract(value.x),
        EmulatedDoubleFract(value.y),
        EmulatedDoubleFract(value.z),
        EmulatedDoubleFract(value.w)
    );
}

/// Applies `EmulatedDoubleSqrt` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Sqrt(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleSqrt(value.x),
        EmulatedDoubleSqrt(value.y)
    );
}

/// Applies `EmulatedDoubleSqrt` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Sqrt(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleSqrt(value.x),
        EmulatedDoubleSqrt(value.y),
        EmulatedDoubleSqrt(value.z)
    );
}

/// Applies `EmulatedDoubleSqrt` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Sqrt(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleSqrt(value.x),
        EmulatedDoubleSqrt(value.y),
        EmulatedDoubleSqrt(value.z),
        EmulatedDoubleSqrt(value.w)
    );
}

/// Applies `EmulatedDoubleRsqrt` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Rsqrt(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleRsqrt(value.x),
        EmulatedDoubleRsqrt(value.y)
    );
}

/// Applies `EmulatedDoubleRsqrt` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Rsqrt(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleRsqrt(value.x),
        EmulatedDoubleRsqrt(value.y),
        EmulatedDoubleRsqrt(value.z)
    );
}

/// Applies `EmulatedDoubleRsqrt` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Rsqrt(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleRsqrt(value.x),
        EmulatedDoubleRsqrt(value.y),
        EmulatedDoubleRsqrt(value.z),
        EmulatedDoubleRsqrt(value.w)
    );
}

/// Applies `EmulatedDoubleSin` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Sin(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleSin(value.x),
        EmulatedDoubleSin(value.y)
    );
}

/// Applies `EmulatedDoubleSin` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Sin(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleSin(value.x),
        EmulatedDoubleSin(value.y),
        EmulatedDoubleSin(value.z)
    );
}

/// Applies `EmulatedDoubleSin` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Sin(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleSin(value.x),
        EmulatedDoubleSin(value.y),
        EmulatedDoubleSin(value.z),
        EmulatedDoubleSin(value.w)
    );
}

/// Applies `EmulatedDoubleCos` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Cos(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleCos(value.x),
        EmulatedDoubleCos(value.y)
    );
}

/// Applies `EmulatedDoubleCos` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Cos(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleCos(value.x),
        EmulatedDoubleCos(value.y),
        EmulatedDoubleCos(value.z)
    );
}

/// Applies `EmulatedDoubleCos` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Cos(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleCos(value.x),
        EmulatedDoubleCos(value.y),
        EmulatedDoubleCos(value.z),
        EmulatedDoubleCos(value.w)
    );
}

/// Applies `EmulatedDoubleTan` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Tan(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleTan(value.x),
        EmulatedDoubleTan(value.y)
    );
}

/// Applies `EmulatedDoubleTan` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Tan(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleTan(value.x),
        EmulatedDoubleTan(value.y),
        EmulatedDoubleTan(value.z)
    );
}

/// Applies `EmulatedDoubleTan` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Tan(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleTan(value.x),
        EmulatedDoubleTan(value.y),
        EmulatedDoubleTan(value.z),
        EmulatedDoubleTan(value.w)
    );
}

/// Applies `EmulatedDoubleSinPi` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2SinPi(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleSinPi(value.x),
        EmulatedDoubleSinPi(value.y)
    );
}

/// Applies `EmulatedDoubleSinPi` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3SinPi(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleSinPi(value.x),
        EmulatedDoubleSinPi(value.y),
        EmulatedDoubleSinPi(value.z)
    );
}

/// Applies `EmulatedDoubleSinPi` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4SinPi(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleSinPi(value.x),
        EmulatedDoubleSinPi(value.y),
        EmulatedDoubleSinPi(value.z),
        EmulatedDoubleSinPi(value.w)
    );
}

/// Applies `EmulatedDoubleCosPi` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2CosPi(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleCosPi(value.x),
        EmulatedDoubleCosPi(value.y)
    );
}

/// Applies `EmulatedDoubleCosPi` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3CosPi(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleCosPi(value.x),
        EmulatedDoubleCosPi(value.y),
        EmulatedDoubleCosPi(value.z)
    );
}

/// Applies `EmulatedDoubleCosPi` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4CosPi(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleCosPi(value.x),
        EmulatedDoubleCosPi(value.y),
        EmulatedDoubleCosPi(value.z),
        EmulatedDoubleCosPi(value.w)
    );
}

/// Applies `EmulatedDoubleTanPi` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2TanPi(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleTanPi(value.x),
        EmulatedDoubleTanPi(value.y)
    );
}

/// Applies `EmulatedDoubleTanPi` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3TanPi(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleTanPi(value.x),
        EmulatedDoubleTanPi(value.y),
        EmulatedDoubleTanPi(value.z)
    );
}

/// Applies `EmulatedDoubleTanPi` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4TanPi(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleTanPi(value.x),
        EmulatedDoubleTanPi(value.y),
        EmulatedDoubleTanPi(value.z),
        EmulatedDoubleTanPi(value.w)
    );
}

/// Applies `EmulatedDoubleExp` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Exp(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleExp(value.x),
        EmulatedDoubleExp(value.y)
    );
}

/// Applies `EmulatedDoubleExp` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Exp(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleExp(value.x),
        EmulatedDoubleExp(value.y),
        EmulatedDoubleExp(value.z)
    );
}

/// Applies `EmulatedDoubleExp` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Exp(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleExp(value.x),
        EmulatedDoubleExp(value.y),
        EmulatedDoubleExp(value.z),
        EmulatedDoubleExp(value.w)
    );
}

/// Applies `EmulatedDoubleExp2` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Exp2(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleExp2(value.x),
        EmulatedDoubleExp2(value.y)
    );
}

/// Applies `EmulatedDoubleExp2` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Exp2(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleExp2(value.x),
        EmulatedDoubleExp2(value.y),
        EmulatedDoubleExp2(value.z)
    );
}

/// Applies `EmulatedDoubleExp2` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Exp2(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleExp2(value.x),
        EmulatedDoubleExp2(value.y),
        EmulatedDoubleExp2(value.z),
        EmulatedDoubleExp2(value.w)
    );
}

/// Applies `EmulatedDoubleExp10` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Exp10(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleExp10(value.x),
        EmulatedDoubleExp10(value.y)
    );
}

/// Applies `EmulatedDoubleExp10` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Exp10(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleExp10(value.x),
        EmulatedDoubleExp10(value.y),
        EmulatedDoubleExp10(value.z)
    );
}

/// Applies `EmulatedDoubleExp10` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Exp10(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleExp10(value.x),
        EmulatedDoubleExp10(value.y),
        EmulatedDoubleExp10(value.z),
        EmulatedDoubleExp10(value.w)
    );
}

/// Applies `EmulatedDoubleLog` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Log(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleLog(value.x),
        EmulatedDoubleLog(value.y)
    );
}

/// Applies `EmulatedDoubleLog` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Log(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleLog(value.x),
        EmulatedDoubleLog(value.y),
        EmulatedDoubleLog(value.z)
    );
}

/// Applies `EmulatedDoubleLog` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Log(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleLog(value.x),
        EmulatedDoubleLog(value.y),
        EmulatedDoubleLog(value.z),
        EmulatedDoubleLog(value.w)
    );
}

/// Applies `EmulatedDoubleLog2` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Log2(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleLog2(value.x),
        EmulatedDoubleLog2(value.y)
    );
}

/// Applies `EmulatedDoubleLog2` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Log2(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleLog2(value.x),
        EmulatedDoubleLog2(value.y),
        EmulatedDoubleLog2(value.z)
    );
}

/// Applies `EmulatedDoubleLog2` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Log2(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleLog2(value.x),
        EmulatedDoubleLog2(value.y),
        EmulatedDoubleLog2(value.z),
        EmulatedDoubleLog2(value.w)
    );
}

/// Applies `EmulatedDoubleLog10` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Log10(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleLog10(value.x),
        EmulatedDoubleLog10(value.y)
    );
}

/// Applies `EmulatedDoubleLog10` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Log10(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleLog10(value.x),
        EmulatedDoubleLog10(value.y),
        EmulatedDoubleLog10(value.z)
    );
}

/// Applies `EmulatedDoubleLog10` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Log10(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleLog10(value.x),
        EmulatedDoubleLog10(value.y),
        EmulatedDoubleLog10(value.z),
        EmulatedDoubleLog10(value.w)
    );
}

/// Applies `EmulatedDoubleAtan` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Atan(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleAtan(value.x),
        EmulatedDoubleAtan(value.y)
    );
}

/// Applies `EmulatedDoubleAtan` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Atan(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleAtan(value.x),
        EmulatedDoubleAtan(value.y),
        EmulatedDoubleAtan(value.z)
    );
}

/// Applies `EmulatedDoubleAtan` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Atan(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleAtan(value.x),
        EmulatedDoubleAtan(value.y),
        EmulatedDoubleAtan(value.z),
        EmulatedDoubleAtan(value.w)
    );
}

/// Applies `EmulatedDoubleAsin` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Asin(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleAsin(value.x),
        EmulatedDoubleAsin(value.y)
    );
}

/// Applies `EmulatedDoubleAsin` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Asin(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleAsin(value.x),
        EmulatedDoubleAsin(value.y),
        EmulatedDoubleAsin(value.z)
    );
}

/// Applies `EmulatedDoubleAsin` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Asin(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleAsin(value.x),
        EmulatedDoubleAsin(value.y),
        EmulatedDoubleAsin(value.z),
        EmulatedDoubleAsin(value.w)
    );
}

/// Applies `EmulatedDoubleAcos` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Acos(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleAcos(value.x),
        EmulatedDoubleAcos(value.y)
    );
}

/// Applies `EmulatedDoubleAcos` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Acos(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleAcos(value.x),
        EmulatedDoubleAcos(value.y),
        EmulatedDoubleAcos(value.z)
    );
}

/// Applies `EmulatedDoubleAcos` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Acos(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleAcos(value.x),
        EmulatedDoubleAcos(value.y),
        EmulatedDoubleAcos(value.z),
        EmulatedDoubleAcos(value.w)
    );
}

/// Applies `EmulatedDoubleSinh` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Sinh(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleSinh(value.x),
        EmulatedDoubleSinh(value.y)
    );
}

/// Applies `EmulatedDoubleSinh` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Sinh(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleSinh(value.x),
        EmulatedDoubleSinh(value.y),
        EmulatedDoubleSinh(value.z)
    );
}

/// Applies `EmulatedDoubleSinh` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Sinh(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleSinh(value.x),
        EmulatedDoubleSinh(value.y),
        EmulatedDoubleSinh(value.z),
        EmulatedDoubleSinh(value.w)
    );
}

/// Applies `EmulatedDoubleCosh` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Cosh(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleCosh(value.x),
        EmulatedDoubleCosh(value.y)
    );
}

/// Applies `EmulatedDoubleCosh` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Cosh(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleCosh(value.x),
        EmulatedDoubleCosh(value.y),
        EmulatedDoubleCosh(value.z)
    );
}

/// Applies `EmulatedDoubleCosh` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Cosh(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleCosh(value.x),
        EmulatedDoubleCosh(value.y),
        EmulatedDoubleCosh(value.z),
        EmulatedDoubleCosh(value.w)
    );
}

/// Applies `EmulatedDoubleTanh` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Tanh(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleTanh(value.x),
        EmulatedDoubleTanh(value.y)
    );
}

/// Applies `EmulatedDoubleTanh` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Tanh(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleTanh(value.x),
        EmulatedDoubleTanh(value.y),
        EmulatedDoubleTanh(value.z)
    );
}

/// Applies `EmulatedDoubleTanh` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Tanh(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleTanh(value.x),
        EmulatedDoubleTanh(value.y),
        EmulatedDoubleTanh(value.z),
        EmulatedDoubleTanh(value.w)
    );
}

/// Applies `EmulatedDoubleAsinh` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Asinh(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleAsinh(value.x),
        EmulatedDoubleAsinh(value.y)
    );
}

/// Applies `EmulatedDoubleAsinh` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Asinh(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleAsinh(value.x),
        EmulatedDoubleAsinh(value.y),
        EmulatedDoubleAsinh(value.z)
    );
}

/// Applies `EmulatedDoubleAsinh` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Asinh(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleAsinh(value.x),
        EmulatedDoubleAsinh(value.y),
        EmulatedDoubleAsinh(value.z),
        EmulatedDoubleAsinh(value.w)
    );
}

/// Applies `EmulatedDoubleAcosh` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Acosh(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleAcosh(value.x),
        EmulatedDoubleAcosh(value.y)
    );
}

/// Applies `EmulatedDoubleAcosh` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Acosh(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleAcosh(value.x),
        EmulatedDoubleAcosh(value.y),
        EmulatedDoubleAcosh(value.z)
    );
}

/// Applies `EmulatedDoubleAcosh` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Acosh(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleAcosh(value.x),
        EmulatedDoubleAcosh(value.y),
        EmulatedDoubleAcosh(value.z),
        EmulatedDoubleAcosh(value.w)
    );
}

/// Applies `EmulatedDoubleAtanh` componentwise to an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Atanh(EmulatedDouble2 value) {
    return EmulatedDouble2Make(
        EmulatedDoubleAtanh(value.x),
        EmulatedDoubleAtanh(value.y)
    );
}

/// Applies `EmulatedDoubleAtanh` componentwise to an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Atanh(EmulatedDouble3 value) {
    return EmulatedDouble3Make(
        EmulatedDoubleAtanh(value.x),
        EmulatedDoubleAtanh(value.y),
        EmulatedDoubleAtanh(value.z)
    );
}

/// Applies `EmulatedDoubleAtanh` componentwise to an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Atanh(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleAtanh(value.x),
        EmulatedDoubleAtanh(value.y),
        EmulatedDoubleAtanh(value.z),
        EmulatedDoubleAtanh(value.w)
    );
}

/// Applies `EmulatedDoubleMin` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Min(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleMin(lhs.x, rhs.x),
        EmulatedDoubleMin(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleMin` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Min(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleMin(lhs.x, rhs.x),
        EmulatedDoubleMin(lhs.y, rhs.y),
        EmulatedDoubleMin(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleMin` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Min(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleMin(lhs.x, rhs.x),
        EmulatedDoubleMin(lhs.y, rhs.y),
        EmulatedDoubleMin(lhs.z, rhs.z),
        EmulatedDoubleMin(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoubleFmin` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Fmin(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleFmin(lhs.x, rhs.x),
        EmulatedDoubleFmin(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleFmin` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Fmin(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleFmin(lhs.x, rhs.x),
        EmulatedDoubleFmin(lhs.y, rhs.y),
        EmulatedDoubleFmin(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleFmin` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Fmin(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleFmin(lhs.x, rhs.x),
        EmulatedDoubleFmin(lhs.y, rhs.y),
        EmulatedDoubleFmin(lhs.z, rhs.z),
        EmulatedDoubleFmin(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoubleMax` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Max(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleMax(lhs.x, rhs.x),
        EmulatedDoubleMax(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleMax` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Max(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleMax(lhs.x, rhs.x),
        EmulatedDoubleMax(lhs.y, rhs.y),
        EmulatedDoubleMax(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleMax` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Max(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleMax(lhs.x, rhs.x),
        EmulatedDoubleMax(lhs.y, rhs.y),
        EmulatedDoubleMax(lhs.z, rhs.z),
        EmulatedDoubleMax(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoubleFmax` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Fmax(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleFmax(lhs.x, rhs.x),
        EmulatedDoubleFmax(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleFmax` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Fmax(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleFmax(lhs.x, rhs.x),
        EmulatedDoubleFmax(lhs.y, rhs.y),
        EmulatedDoubleFmax(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleFmax` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Fmax(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleFmax(lhs.x, rhs.x),
        EmulatedDoubleFmax(lhs.y, rhs.y),
        EmulatedDoubleFmax(lhs.z, rhs.z),
        EmulatedDoubleFmax(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoubleFdim` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Fdim(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleFdim(lhs.x, rhs.x),
        EmulatedDoubleFdim(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleFdim` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Fdim(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleFdim(lhs.x, rhs.x),
        EmulatedDoubleFdim(lhs.y, rhs.y),
        EmulatedDoubleFdim(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleFdim` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Fdim(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleFdim(lhs.x, rhs.x),
        EmulatedDoubleFdim(lhs.y, rhs.y),
        EmulatedDoubleFdim(lhs.z, rhs.z),
        EmulatedDoubleFdim(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoubleFmod` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Fmod(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleFmod(lhs.x, rhs.x),
        EmulatedDoubleFmod(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleFmod` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Fmod(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleFmod(lhs.x, rhs.x),
        EmulatedDoubleFmod(lhs.y, rhs.y),
        EmulatedDoubleFmod(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleFmod` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Fmod(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleFmod(lhs.x, rhs.x),
        EmulatedDoubleFmod(lhs.y, rhs.y),
        EmulatedDoubleFmod(lhs.z, rhs.z),
        EmulatedDoubleFmod(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoublePow` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Pow(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoublePow(lhs.x, rhs.x),
        EmulatedDoublePow(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoublePow` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Pow(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoublePow(lhs.x, rhs.x),
        EmulatedDoublePow(lhs.y, rhs.y),
        EmulatedDoublePow(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoublePow` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Pow(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoublePow(lhs.x, rhs.x),
        EmulatedDoublePow(lhs.y, rhs.y),
        EmulatedDoublePow(lhs.z, rhs.z),
        EmulatedDoublePow(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoublePowr` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Powr(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoublePowr(lhs.x, rhs.x),
        EmulatedDoublePowr(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoublePowr` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Powr(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoublePowr(lhs.x, rhs.x),
        EmulatedDoublePowr(lhs.y, rhs.y),
        EmulatedDoublePowr(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoublePowr` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Powr(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoublePowr(lhs.x, rhs.x),
        EmulatedDoublePowr(lhs.y, rhs.y),
        EmulatedDoublePowr(lhs.z, rhs.z),
        EmulatedDoublePowr(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoubleAtan2` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Atan2(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleAtan2(lhs.x, rhs.x),
        EmulatedDoubleAtan2(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleAtan2` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Atan2(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleAtan2(lhs.x, rhs.x),
        EmulatedDoubleAtan2(lhs.y, rhs.y),
        EmulatedDoubleAtan2(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleAtan2` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Atan2(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleAtan2(lhs.x, rhs.x),
        EmulatedDoubleAtan2(lhs.y, rhs.y),
        EmulatedDoubleAtan2(lhs.z, rhs.z),
        EmulatedDoubleAtan2(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoubleStep` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Step(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleStep(lhs.x, rhs.x),
        EmulatedDoubleStep(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleStep` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Step(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleStep(lhs.x, rhs.x),
        EmulatedDoubleStep(lhs.y, rhs.y),
        EmulatedDoubleStep(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleStep` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Step(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleStep(lhs.x, rhs.x),
        EmulatedDoubleStep(lhs.y, rhs.y),
        EmulatedDoubleStep(lhs.z, rhs.z),
        EmulatedDoubleStep(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoubleCopySign` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2CopySign(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleCopySign(lhs.x, rhs.x),
        EmulatedDoubleCopySign(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleCopySign` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3CopySign(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleCopySign(lhs.x, rhs.x),
        EmulatedDoubleCopySign(lhs.y, rhs.y),
        EmulatedDoubleCopySign(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleCopySign` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4CopySign(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleCopySign(lhs.x, rhs.x),
        EmulatedDoubleCopySign(lhs.y, rhs.y),
        EmulatedDoubleCopySign(lhs.z, rhs.z),
        EmulatedDoubleCopySign(lhs.w, rhs.w)
    );
}

/// Applies `EmulatedDoubleNextafter` componentwise to two `EmulatedDouble2` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Nextafter(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleNextafter(lhs.x, rhs.x),
        EmulatedDoubleNextafter(lhs.y, rhs.y)
    );
}

/// Applies `EmulatedDoubleNextafter` componentwise to two `EmulatedDouble3` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Nextafter(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleNextafter(lhs.x, rhs.x),
        EmulatedDoubleNextafter(lhs.y, rhs.y),
        EmulatedDoubleNextafter(lhs.z, rhs.z)
    );
}

/// Applies `EmulatedDoubleNextafter` componentwise to two `EmulatedDouble4` vectors.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Nextafter(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleNextafter(lhs.x, rhs.x),
        EmulatedDoubleNextafter(lhs.y, rhs.y),
        EmulatedDoubleNextafter(lhs.z, rhs.z),
        EmulatedDoubleNextafter(lhs.w, rhs.w)
    );
}

/// Componentwise vector `min3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Min3(EmulatedDouble2 x, EmulatedDouble2 y, EmulatedDouble2 z) {
    return EmulatedDouble2Make(EmulatedDoubleMin3(x.x, y.x, z.x), EmulatedDoubleMin3(x.y, y.y, z.y));
}

/// Componentwise vector `min3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Min3(EmulatedDouble3 x, EmulatedDouble3 y, EmulatedDouble3 z) {
    return EmulatedDouble3Make(
        EmulatedDoubleMin3(x.x, y.x, z.x),
        EmulatedDoubleMin3(x.y, y.y, z.y),
        EmulatedDoubleMin3(x.z, y.z, z.z)
    );
}

/// Componentwise vector `min3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Min3(EmulatedDouble4 x, EmulatedDouble4 y, EmulatedDouble4 z) {
    return EmulatedDouble4Make(
        EmulatedDoubleMin3(x.x, y.x, z.x),
        EmulatedDoubleMin3(x.y, y.y, z.y),
        EmulatedDoubleMin3(x.z, y.z, z.z),
        EmulatedDoubleMin3(x.w, y.w, z.w)
    );
}

/// Componentwise vector `max3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Max3(EmulatedDouble2 x, EmulatedDouble2 y, EmulatedDouble2 z) {
    return EmulatedDouble2Make(EmulatedDoubleMax3(x.x, y.x, z.x), EmulatedDoubleMax3(x.y, y.y, z.y));
}

/// Componentwise vector `max3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Max3(EmulatedDouble3 x, EmulatedDouble3 y, EmulatedDouble3 z) {
    return EmulatedDouble3Make(
        EmulatedDoubleMax3(x.x, y.x, z.x),
        EmulatedDoubleMax3(x.y, y.y, z.y),
        EmulatedDoubleMax3(x.z, y.z, z.z)
    );
}

/// Componentwise vector `max3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Max3(EmulatedDouble4 x, EmulatedDouble4 y, EmulatedDouble4 z) {
    return EmulatedDouble4Make(
        EmulatedDoubleMax3(x.x, y.x, z.x),
        EmulatedDoubleMax3(x.y, y.y, z.y),
        EmulatedDoubleMax3(x.z, y.z, z.z),
        EmulatedDoubleMax3(x.w, y.w, z.w)
    );
}

/// Componentwise vector `median3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Median3(EmulatedDouble2 x, EmulatedDouble2 y, EmulatedDouble2 z) {
    return EmulatedDouble2Make(EmulatedDoubleMedian3(x.x, y.x, z.x), EmulatedDoubleMedian3(x.y, y.y, z.y));
}

/// Componentwise vector `median3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Median3(EmulatedDouble3 x, EmulatedDouble3 y, EmulatedDouble3 z) {
    return EmulatedDouble3Make(
        EmulatedDoubleMedian3(x.x, y.x, z.x),
        EmulatedDoubleMedian3(x.y, y.y, z.y),
        EmulatedDoubleMedian3(x.z, y.z, z.z)
    );
}

/// Componentwise vector `median3`.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Median3(EmulatedDouble4 x, EmulatedDouble4 y, EmulatedDouble4 z) {
    return EmulatedDouble4Make(
        EmulatedDoubleMedian3(x.x, y.x, z.x),
        EmulatedDoubleMedian3(x.y, y.y, z.y),
        EmulatedDoubleMedian3(x.z, y.z, z.z),
        EmulatedDoubleMedian3(x.w, y.w, z.w)
    );
}

/// Componentwise vector `ldexp` with a shared exponent.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Ldexp(EmulatedDouble2 value, int32_t exponent) {
    return EmulatedDouble2Make(EmulatedDoubleLdexp(value.x, exponent), EmulatedDoubleLdexp(value.y, exponent));
}

/// Componentwise vector `ldexp` with a shared exponent.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Ldexp(EmulatedDouble3 value, int32_t exponent) {
    return EmulatedDouble3Make(
        EmulatedDoubleLdexp(value.x, exponent),
        EmulatedDoubleLdexp(value.y, exponent),
        EmulatedDoubleLdexp(value.z, exponent)
    );
}

/// Componentwise vector `ldexp` with a shared exponent.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Ldexp(EmulatedDouble4 value, int32_t exponent) {
    return EmulatedDouble4Make(
        EmulatedDoubleLdexp(value.x, exponent),
        EmulatedDoubleLdexp(value.y, exponent),
        EmulatedDoubleLdexp(value.z, exponent),
        EmulatedDoubleLdexp(value.w, exponent)
    );
}

/// Componentwise vector clamp.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Clamp(EmulatedDouble2 value, EmulatedDouble2 minimumValue, EmulatedDouble2 maximumValue) {
    return EmulatedDouble2Make(
        EmulatedDoubleClamp(value.x, minimumValue.x, maximumValue.x),
        EmulatedDoubleClamp(value.y, minimumValue.y, maximumValue.y)
    );
}

/// Componentwise vector clamp.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Clamp(EmulatedDouble3 value, EmulatedDouble3 minimumValue, EmulatedDouble3 maximumValue) {
    return EmulatedDouble3Make(
        EmulatedDoubleClamp(value.x, minimumValue.x, maximumValue.x),
        EmulatedDoubleClamp(value.y, minimumValue.y, maximumValue.y),
        EmulatedDoubleClamp(value.z, minimumValue.z, maximumValue.z)
    );
}

/// Componentwise vector clamp.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Clamp(EmulatedDouble4 value, EmulatedDouble4 minimumValue, EmulatedDouble4 maximumValue) {
    return EmulatedDouble4Make(
        EmulatedDoubleClamp(value.x, minimumValue.x, maximumValue.x),
        EmulatedDoubleClamp(value.y, minimumValue.y, maximumValue.y),
        EmulatedDoubleClamp(value.z, minimumValue.z, maximumValue.z),
        EmulatedDoubleClamp(value.w, minimumValue.w, maximumValue.w)
    );
}

/// Componentwise vector saturate.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Saturate(EmulatedDouble2 value) {
    return EmulatedDouble2Make(EmulatedDoubleSaturate(value.x), EmulatedDoubleSaturate(value.y));
}

/// Componentwise vector saturate.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Saturate(EmulatedDouble3 value) {
    return EmulatedDouble3Make(EmulatedDoubleSaturate(value.x), EmulatedDoubleSaturate(value.y), EmulatedDoubleSaturate(value.z));
}

/// Componentwise vector saturate.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Saturate(EmulatedDouble4 value) {
    return EmulatedDouble4Make(EmulatedDoubleSaturate(value.x), EmulatedDoubleSaturate(value.y), EmulatedDoubleSaturate(value.z), EmulatedDoubleSaturate(value.w));
}

/// Componentwise vector mix.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Mix(EmulatedDouble2 x, EmulatedDouble2 y, EmulatedDouble2 a) {
    return EmulatedDouble2Make(EmulatedDoubleMix(x.x, y.x, a.x), EmulatedDoubleMix(x.y, y.y, a.y));
}

/// Componentwise vector mix.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Mix(EmulatedDouble3 x, EmulatedDouble3 y, EmulatedDouble3 a) {
    return EmulatedDouble3Make(EmulatedDoubleMix(x.x, y.x, a.x), EmulatedDoubleMix(x.y, y.y, a.y), EmulatedDoubleMix(x.z, y.z, a.z));
}

/// Componentwise vector mix.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Mix(EmulatedDouble4 x, EmulatedDouble4 y, EmulatedDouble4 a) {
    return EmulatedDouble4Make(
        EmulatedDoubleMix(x.x, y.x, a.x),
        EmulatedDoubleMix(x.y, y.y, a.y),
        EmulatedDoubleMix(x.z, y.z, a.z),
        EmulatedDoubleMix(x.w, y.w, a.w)
    );
}

/// Componentwise vector smoothstep.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Smoothstep(EmulatedDouble2 edge0, EmulatedDouble2 edge1, EmulatedDouble2 x) {
    return EmulatedDouble2Make(EmulatedDoubleSmoothstep(edge0.x, edge1.x, x.x), EmulatedDoubleSmoothstep(edge0.y, edge1.y, x.y));
}

/// Componentwise vector smoothstep.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Smoothstep(EmulatedDouble3 edge0, EmulatedDouble3 edge1, EmulatedDouble3 x) {
    return EmulatedDouble3Make(
        EmulatedDoubleSmoothstep(edge0.x, edge1.x, x.x),
        EmulatedDoubleSmoothstep(edge0.y, edge1.y, x.y),
        EmulatedDoubleSmoothstep(edge0.z, edge1.z, x.z)
    );
}

/// Componentwise vector smoothstep.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Smoothstep(EmulatedDouble4 edge0, EmulatedDouble4 edge1, EmulatedDouble4 x) {
    return EmulatedDouble4Make(
        EmulatedDoubleSmoothstep(edge0.x, edge1.x, x.x),
        EmulatedDoubleSmoothstep(edge0.y, edge1.y, x.y),
        EmulatedDoubleSmoothstep(edge0.z, edge1.z, x.z),
        EmulatedDoubleSmoothstep(edge0.w, edge1.w, x.w)
    );
}

/// Componentwise vector multiply-add helper.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Fma(EmulatedDouble2 a, EmulatedDouble2 b, EmulatedDouble2 c) {
    return EmulatedDouble2Make(EmulatedDoubleFma(a.x, b.x, c.x), EmulatedDoubleFma(a.y, b.y, c.y));
}

/// Componentwise vector multiply-add helper.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Fma(EmulatedDouble3 a, EmulatedDouble3 b, EmulatedDouble3 c) {
    return EmulatedDouble3Make(EmulatedDoubleFma(a.x, b.x, c.x), EmulatedDoubleFma(a.y, b.y, c.y), EmulatedDoubleFma(a.z, b.z, c.z));
}

/// Componentwise vector multiply-add helper.
///
/// Quality: Covered by `emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Fma(EmulatedDouble4 a, EmulatedDouble4 b, EmulatedDouble4 c) {
    return EmulatedDouble4Make(
        EmulatedDoubleFma(a.x, b.x, c.x),
        EmulatedDoubleFma(a.y, b.y, c.y),
        EmulatedDoubleFma(a.z, b.z, c.z),
        EmulatedDoubleFma(a.w, b.w, c.w)
    );
}

/// Componentwise vector `frexp`.
///
/// The `exponents` pointer must reference storage for at least two `int32_t` values.
///
/// Quality: Covered by `emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Frexp(EmulatedDouble2 value, EMULATED_DOUBLE_THREAD int32_t *exponents) {
    return EmulatedDouble2Make(EmulatedDoubleFrexp(value.x, &exponents[0]), EmulatedDoubleFrexp(value.y, &exponents[1]));
}

/// Componentwise vector `frexp`.
///
/// The `exponents` pointer must reference storage for at least three `int32_t` values.
///
/// Quality: Covered by `emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Frexp(EmulatedDouble3 value, EMULATED_DOUBLE_THREAD int32_t *exponents) {
    return EmulatedDouble3Make(
        EmulatedDoubleFrexp(value.x, &exponents[0]),
        EmulatedDoubleFrexp(value.y, &exponents[1]),
        EmulatedDoubleFrexp(value.z, &exponents[2])
    );
}

/// Componentwise vector `frexp`.
///
/// The `exponents` pointer must reference storage for at least four `int32_t` values.
///
/// Quality: Covered by `emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Frexp(EmulatedDouble4 value, EMULATED_DOUBLE_THREAD int32_t *exponents) {
    return EmulatedDouble4Make(
        EmulatedDoubleFrexp(value.x, &exponents[0]),
        EmulatedDoubleFrexp(value.y, &exponents[1]),
        EmulatedDoubleFrexp(value.z, &exponents[2]),
        EmulatedDoubleFrexp(value.w, &exponents[3])
    );
}

/// Componentwise vector `modf`.
///
/// Quality: Covered by `emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Modf(EmulatedDouble2 value, EMULATED_DOUBLE_THREAD EmulatedDouble2 *integerPart) {
    return EmulatedDouble2Make(EmulatedDoubleModf(value.x, &integerPart->x), EmulatedDoubleModf(value.y, &integerPart->y));
}

/// Componentwise vector `modf`.
///
/// Quality: Covered by `emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Modf(EmulatedDouble3 value, EMULATED_DOUBLE_THREAD EmulatedDouble3 *integerPart) {
    return EmulatedDouble3Make(
        EmulatedDoubleModf(value.x, &integerPart->x),
        EmulatedDoubleModf(value.y, &integerPart->y),
        EmulatedDoubleModf(value.z, &integerPart->z)
    );
}

/// Componentwise vector `modf`.
///
/// Quality: Covered by `emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Modf(EmulatedDouble4 value, EMULATED_DOUBLE_THREAD EmulatedDouble4 *integerPart) {
    return EmulatedDouble4Make(
        EmulatedDoubleModf(value.x, &integerPart->x),
        EmulatedDoubleModf(value.y, &integerPart->y),
        EmulatedDoubleModf(value.z, &integerPart->z),
        EmulatedDoubleModf(value.w, &integerPart->w)
    );
}

/// Componentwise vector `sincos`.
///
/// Quality: Covered by `emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise`.
static inline EmulatedDouble2 EmulatedDouble2Sincos(EmulatedDouble2 value, EMULATED_DOUBLE_THREAD EmulatedDouble2 *cosineValue) {
    return EmulatedDouble2Make(EmulatedDoubleSincos(value.x, &cosineValue->x), EmulatedDoubleSincos(value.y, &cosineValue->y));
}

/// Componentwise vector `sincos`.
///
/// Quality: Covered by `emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise`.
static inline EmulatedDouble3 EmulatedDouble3Sincos(EmulatedDouble3 value, EMULATED_DOUBLE_THREAD EmulatedDouble3 *cosineValue) {
    return EmulatedDouble3Make(
        EmulatedDoubleSincos(value.x, &cosineValue->x),
        EmulatedDoubleSincos(value.y, &cosineValue->y),
        EmulatedDoubleSincos(value.z, &cosineValue->z)
    );
}

/// Componentwise vector `sincos`.
///
/// Quality: Covered by `emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise`.
static inline EmulatedDouble4 EmulatedDouble4Sincos(EmulatedDouble4 value, EMULATED_DOUBLE_THREAD EmulatedDouble4 *cosineValue) {
    return EmulatedDouble4Make(
        EmulatedDoubleSincos(value.x, &cosineValue->x),
        EmulatedDoubleSincos(value.y, &cosineValue->y),
        EmulatedDoubleSincos(value.z, &cosineValue->z),
        EmulatedDoubleSincos(value.w, &cosineValue->w)
    );
}

/// Adds two `EmulatedDouble2` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2Add(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleAdd(lhs.x, rhs.x),
        EmulatedDoubleAdd(lhs.y, rhs.y)
    );
}

/// Subtracts two `EmulatedDouble2` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2Subtract(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleSubtract(lhs.x, rhs.x),
        EmulatedDoubleSubtract(lhs.y, rhs.y)
    );
}

/// Multiplies two `EmulatedDouble2` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2Multiply(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleMultiply(lhs.x, rhs.x),
        EmulatedDoubleMultiply(lhs.y, rhs.y)
    );
}

/// Divides two `EmulatedDouble2` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2Divide(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleDivide(lhs.x, rhs.x),
        EmulatedDoubleDivide(lhs.y, rhs.y)
    );
}

/// Adds two `EmulatedDouble3` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3Add(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleAdd(lhs.x, rhs.x),
        EmulatedDoubleAdd(lhs.y, rhs.y),
        EmulatedDoubleAdd(lhs.z, rhs.z)
    );
}

/// Subtracts two `EmulatedDouble3` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3Subtract(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleSubtract(lhs.x, rhs.x),
        EmulatedDoubleSubtract(lhs.y, rhs.y),
        EmulatedDoubleSubtract(lhs.z, rhs.z)
    );
}

/// Multiplies two `EmulatedDouble3` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3Multiply(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleMultiply(lhs.x, rhs.x),
        EmulatedDoubleMultiply(lhs.y, rhs.y),
        EmulatedDoubleMultiply(lhs.z, rhs.z)
    );
}

/// Divides two `EmulatedDouble3` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3Divide(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleDivide(lhs.x, rhs.x),
        EmulatedDoubleDivide(lhs.y, rhs.y),
        EmulatedDoubleDivide(lhs.z, rhs.z)
    );
}

/// Adds two `EmulatedDouble4` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4Add(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleAdd(lhs.x, rhs.x),
        EmulatedDoubleAdd(lhs.y, rhs.y),
        EmulatedDoubleAdd(lhs.z, rhs.z),
        EmulatedDoubleAdd(lhs.w, rhs.w)
    );
}

/// Subtracts two `EmulatedDouble4` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4Subtract(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleSubtract(lhs.x, rhs.x),
        EmulatedDoubleSubtract(lhs.y, rhs.y),
        EmulatedDoubleSubtract(lhs.z, rhs.z),
        EmulatedDoubleSubtract(lhs.w, rhs.w)
    );
}

/// Multiplies two `EmulatedDouble4` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4Multiply(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleMultiply(lhs.x, rhs.x),
        EmulatedDoubleMultiply(lhs.y, rhs.y),
        EmulatedDoubleMultiply(lhs.z, rhs.z),
        EmulatedDoubleMultiply(lhs.w, rhs.w)
    );
}

/// Divides two `EmulatedDouble4` vectors componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4Divide(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleDivide(lhs.x, rhs.x),
        EmulatedDoubleDivide(lhs.y, rhs.y),
        EmulatedDoubleDivide(lhs.z, rhs.z),
        EmulatedDoubleDivide(lhs.w, rhs.w)
    );
}

/// Adds an `EmulatedDouble2` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2AddScalar(EmulatedDouble2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleAdd(lhs.x, rhs),
        EmulatedDoubleAdd(lhs.y, rhs)
    );
}

/// Subtracts an `EmulatedDouble2` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2SubtractScalar(EmulatedDouble2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleSubtract(lhs.x, rhs),
        EmulatedDoubleSubtract(lhs.y, rhs)
    );
}

/// Multiplies an `EmulatedDouble2` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2MultiplyScalar(EmulatedDouble2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleMultiply(lhs.x, rhs),
        EmulatedDoubleMultiply(lhs.y, rhs)
    );
}

/// Divides an `EmulatedDouble2` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2DivideScalar(EmulatedDouble2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2Make(
        EmulatedDoubleDivide(lhs.x, rhs),
        EmulatedDoubleDivide(lhs.y, rhs)
    );
}

/// Adds an `EmulatedDouble3` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3AddScalar(EmulatedDouble3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleAdd(lhs.x, rhs),
        EmulatedDoubleAdd(lhs.y, rhs),
        EmulatedDoubleAdd(lhs.z, rhs)
    );
}

/// Subtracts an `EmulatedDouble3` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3SubtractScalar(EmulatedDouble3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleSubtract(lhs.x, rhs),
        EmulatedDoubleSubtract(lhs.y, rhs),
        EmulatedDoubleSubtract(lhs.z, rhs)
    );
}

/// Multiplies an `EmulatedDouble3` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3MultiplyScalar(EmulatedDouble3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleMultiply(lhs.x, rhs),
        EmulatedDoubleMultiply(lhs.y, rhs),
        EmulatedDoubleMultiply(lhs.z, rhs)
    );
}

/// Divides an `EmulatedDouble3` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3DivideScalar(EmulatedDouble3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleDivide(lhs.x, rhs),
        EmulatedDoubleDivide(lhs.y, rhs),
        EmulatedDoubleDivide(lhs.z, rhs)
    );
}

/// Adds an `EmulatedDouble4` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4AddScalar(EmulatedDouble4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleAdd(lhs.x, rhs),
        EmulatedDoubleAdd(lhs.y, rhs),
        EmulatedDoubleAdd(lhs.z, rhs),
        EmulatedDoubleAdd(lhs.w, rhs)
    );
}

/// Subtracts an `EmulatedDouble4` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4SubtractScalar(EmulatedDouble4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleSubtract(lhs.x, rhs),
        EmulatedDoubleSubtract(lhs.y, rhs),
        EmulatedDoubleSubtract(lhs.z, rhs),
        EmulatedDoubleSubtract(lhs.w, rhs)
    );
}

/// Multiplies an `EmulatedDouble4` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4MultiplyScalar(EmulatedDouble4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleMultiply(lhs.x, rhs),
        EmulatedDoubleMultiply(lhs.y, rhs),
        EmulatedDoubleMultiply(lhs.z, rhs),
        EmulatedDoubleMultiply(lhs.w, rhs)
    );
}

/// Divides an `EmulatedDouble4` vector and an `EmulatedDouble` scalar componentwise.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4DivideScalar(EmulatedDouble4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleDivide(lhs.x, rhs),
        EmulatedDoubleDivide(lhs.y, rhs),
        EmulatedDoubleDivide(lhs.z, rhs),
        EmulatedDoubleDivide(lhs.w, rhs)
    );
}

/// Computes `lhs - rhs` for a scalar and an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2ScalarSubtract(EmulatedDouble lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(EmulatedDoubleSubtract(lhs, rhs.x), EmulatedDoubleSubtract(lhs, rhs.y));
}

/// Computes `lhs - rhs` for a scalar and an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3ScalarSubtract(EmulatedDouble lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleSubtract(lhs, rhs.x),
        EmulatedDoubleSubtract(lhs, rhs.y),
        EmulatedDoubleSubtract(lhs, rhs.z)
    );
}

/// Computes `lhs - rhs` for a scalar and an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4ScalarSubtract(EmulatedDouble lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleSubtract(lhs, rhs.x),
        EmulatedDoubleSubtract(lhs, rhs.y),
        EmulatedDoubleSubtract(lhs, rhs.z),
        EmulatedDoubleSubtract(lhs, rhs.w)
    );
}

/// Computes `lhs / rhs` for a scalar and an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2ScalarDivide(EmulatedDouble lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Make(EmulatedDoubleDivide(lhs, rhs.x), EmulatedDoubleDivide(lhs, rhs.y));
}

/// Computes `lhs / rhs` for a scalar and an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3ScalarDivide(EmulatedDouble lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleDivide(lhs, rhs.x),
        EmulatedDoubleDivide(lhs, rhs.y),
        EmulatedDoubleDivide(lhs, rhs.z)
    );
}

/// Computes `lhs / rhs` for a scalar and an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4ScalarDivide(EmulatedDouble lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDoubleDivide(lhs, rhs.x),
        EmulatedDoubleDivide(lhs, rhs.y),
        EmulatedDoubleDivide(lhs, rhs.z),
        EmulatedDoubleDivide(lhs, rhs.w)
    );
}

/// Negates every component of an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble2 EmulatedDouble2Negated(EmulatedDouble2 value) {
    return EmulatedDouble2Make(EmulatedDoubleNegated(value.x), EmulatedDoubleNegated(value.y));
}

/// Negates every component of an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble3 EmulatedDouble3Negated(EmulatedDouble3 value) {
    return EmulatedDouble3Make(EmulatedDoubleNegated(value.x), EmulatedDoubleNegated(value.y), EmulatedDoubleNegated(value.z));
}

/// Negates every component of an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorArithmeticMatchesScalarArithmetic`.
static inline EmulatedDouble4 EmulatedDouble4Negated(EmulatedDouble4 value) {
    return EmulatedDouble4Make(
        EmulatedDoubleNegated(value.x),
        EmulatedDoubleNegated(value.y),
        EmulatedDoubleNegated(value.z),
        EmulatedDoubleNegated(value.w)
    );
}

/// Computes the dot product of two `EmulatedDouble2` values.
///
/// Quality: Covered by `emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic`.
static inline EmulatedDouble EmulatedDouble2Dot(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDoubleAdd(EmulatedDoubleMultiply(lhs.x, rhs.x), EmulatedDoubleMultiply(lhs.y, rhs.y));
}

/// Computes the dot product of two `EmulatedDouble3` values.
///
/// Quality: Covered by `emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic`.
static inline EmulatedDouble EmulatedDouble3Dot(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDoubleAdd(
        EmulatedDoubleAdd(EmulatedDoubleMultiply(lhs.x, rhs.x), EmulatedDoubleMultiply(lhs.y, rhs.y)),
        EmulatedDoubleMultiply(lhs.z, rhs.z)
    );
}

/// Computes the dot product of two `EmulatedDouble4` values.
///
/// Quality: Covered by `emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic`.
static inline EmulatedDouble EmulatedDouble4Dot(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDoubleAdd(
        EmulatedDoubleAdd(EmulatedDoubleMultiply(lhs.x, rhs.x), EmulatedDoubleMultiply(lhs.y, rhs.y)),
        EmulatedDoubleAdd(EmulatedDoubleMultiply(lhs.z, rhs.z), EmulatedDoubleMultiply(lhs.w, rhs.w))
    );
}

/// Computes the squared length of an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic`.
static inline EmulatedDouble EmulatedDouble2LengthSquared(EmulatedDouble2 value) {
    return EmulatedDouble2Dot(value, value);
}

/// Computes the squared length of an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic`.
static inline EmulatedDouble EmulatedDouble3LengthSquared(EmulatedDouble3 value) {
    return EmulatedDouble3Dot(value, value);
}

/// Computes the squared length of an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic`.
static inline EmulatedDouble EmulatedDouble4LengthSquared(EmulatedDouble4 value) {
    return EmulatedDouble4Dot(value, value);
}

/// Computes the squared distance between two `EmulatedDouble2` values.
///
/// Quality: Covered by `emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic`.
static inline EmulatedDouble EmulatedDouble2DistanceSquared(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2LengthSquared(EmulatedDouble2Subtract(lhs, rhs));
}

/// Computes the squared distance between two `EmulatedDouble3` values.
///
/// Quality: Covered by `emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic`.
static inline EmulatedDouble EmulatedDouble3DistanceSquared(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3LengthSquared(EmulatedDouble3Subtract(lhs, rhs));
}

/// Computes the squared distance between two `EmulatedDouble4` values.
///
/// Quality: Covered by `emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic`.
static inline EmulatedDouble EmulatedDouble4DistanceSquared(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4LengthSquared(EmulatedDouble4Subtract(lhs, rhs));
}

/// Computes the length of an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt`.
static inline EmulatedDouble EmulatedDouble2Length(EmulatedDouble2 value) {
    return EmulatedDoubleSqrt(EmulatedDouble2LengthSquared(value));
}

/// Computes the length of an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt`.
static inline EmulatedDouble EmulatedDouble3Length(EmulatedDouble3 value) {
    return EmulatedDoubleSqrt(EmulatedDouble3LengthSquared(value));
}

/// Computes the length of an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt`.
static inline EmulatedDouble EmulatedDouble4Length(EmulatedDouble4 value) {
    return EmulatedDoubleSqrt(EmulatedDouble4LengthSquared(value));
}

/// Computes the distance between two `EmulatedDouble2` values.
///
/// Quality: Covered by `emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt`.
static inline EmulatedDouble EmulatedDouble2Distance(EmulatedDouble2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDoubleSqrt(EmulatedDouble2DistanceSquared(lhs, rhs));
}

/// Computes the distance between two `EmulatedDouble3` values.
///
/// Quality: Covered by `emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt`.
static inline EmulatedDouble EmulatedDouble3Distance(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDoubleSqrt(EmulatedDouble3DistanceSquared(lhs, rhs));
}

/// Computes the distance between two `EmulatedDouble4` values.
///
/// Quality: Covered by `emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt`.
static inline EmulatedDouble EmulatedDouble4Distance(EmulatedDouble4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDoubleSqrt(EmulatedDouble4DistanceSquared(lhs, rhs));
}

/// Normalizes an `EmulatedDouble2`.
///
/// Quality: Covered by `emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt`.
static inline EmulatedDouble2 EmulatedDouble2Normalize(EmulatedDouble2 value) {
    return EmulatedDouble2DivideScalar(value, EmulatedDouble2Length(value));
}

/// Normalizes an `EmulatedDouble3`.
///
/// Quality: Covered by `emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt`.
static inline EmulatedDouble3 EmulatedDouble3Normalize(EmulatedDouble3 value) {
    return EmulatedDouble3DivideScalar(value, EmulatedDouble3Length(value));
}

/// Normalizes an `EmulatedDouble4`.
///
/// Quality: Covered by `emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt`.
static inline EmulatedDouble4 EmulatedDouble4Normalize(EmulatedDouble4 value) {
    return EmulatedDouble4DivideScalar(value, EmulatedDouble4Length(value));
}

/// Orients an `EmulatedDouble2` normal away from an incident vector.
///
/// Returns `normal` when `dot(referenceNormal, incident) < 0.0`; otherwise returns `-normal`.
///
/// Quality: Covered by `emulatedDoubleVectorFaceforwardMatchesMetalDefinition`.
static inline EmulatedDouble2 EmulatedDouble2Faceforward(
    EmulatedDouble2 normal,
    EmulatedDouble2 incident,
    EmulatedDouble2 referenceNormal
) {
    return EmulatedDoubleIsLessThanZero(EmulatedDouble2Dot(referenceNormal, incident)) ?
        normal : EmulatedDouble2Negated(normal);
}

/// Orients an `EmulatedDouble3` normal away from an incident vector.
///
/// Quality: Covered by `emulatedDoubleVectorFaceforwardMatchesMetalDefinition`.
static inline EmulatedDouble3 EmulatedDouble3Faceforward(
    EmulatedDouble3 normal,
    EmulatedDouble3 incident,
    EmulatedDouble3 referenceNormal
) {
    return EmulatedDoubleIsLessThanZero(EmulatedDouble3Dot(referenceNormal, incident)) ?
        normal : EmulatedDouble3Negated(normal);
}

/// Orients an `EmulatedDouble4` normal away from an incident vector.
///
/// Quality: Covered by `emulatedDoubleVectorFaceforwardMatchesMetalDefinition`.
static inline EmulatedDouble4 EmulatedDouble4Faceforward(
    EmulatedDouble4 normal,
    EmulatedDouble4 incident,
    EmulatedDouble4 referenceNormal
) {
    return EmulatedDoubleIsLessThanZero(EmulatedDouble4Dot(referenceNormal, incident)) ?
        normal : EmulatedDouble4Negated(normal);
}

/// Reflects an `EmulatedDouble2` incident vector around a surface normal.
///
/// The surface normal is normalized before applying Metal's `I - 2 * dot(NN, I) * NN` formula.
///
/// Quality: Covered by `emulatedDoubleVectorReflectMatchesMetalDefinition`.
static inline EmulatedDouble2 EmulatedDouble2Reflect(EmulatedDouble2 incident, EmulatedDouble2 normal) {
    EmulatedDouble2 normalizedNormal = EmulatedDouble2Normalize(normal);
    EmulatedDouble scale = EmulatedDoubleMultiply(
        EmulatedDoubleMake(0x40000000u, 0u),
        EmulatedDouble2Dot(normalizedNormal, incident)
    );
    return EmulatedDouble2Subtract(incident, EmulatedDouble2MultiplyScalar(normalizedNormal, scale));
}

/// Reflects an `EmulatedDouble3` incident vector around a surface normal.
///
/// Quality: Covered by `emulatedDoubleVectorReflectMatchesMetalDefinition`.
static inline EmulatedDouble3 EmulatedDouble3Reflect(EmulatedDouble3 incident, EmulatedDouble3 normal) {
    EmulatedDouble3 normalizedNormal = EmulatedDouble3Normalize(normal);
    EmulatedDouble scale = EmulatedDoubleMultiply(
        EmulatedDoubleMake(0x40000000u, 0u),
        EmulatedDouble3Dot(normalizedNormal, incident)
    );
    return EmulatedDouble3Subtract(incident, EmulatedDouble3MultiplyScalar(normalizedNormal, scale));
}

/// Reflects an `EmulatedDouble4` incident vector around a surface normal.
///
/// Quality: Covered by `emulatedDoubleVectorReflectMatchesMetalDefinition`.
static inline EmulatedDouble4 EmulatedDouble4Reflect(EmulatedDouble4 incident, EmulatedDouble4 normal) {
    EmulatedDouble4 normalizedNormal = EmulatedDouble4Normalize(normal);
    EmulatedDouble scale = EmulatedDoubleMultiply(
        EmulatedDoubleMake(0x40000000u, 0u),
        EmulatedDouble4Dot(normalizedNormal, incident)
    );
    return EmulatedDouble4Subtract(incident, EmulatedDouble4MultiplyScalar(normalizedNormal, scale));
}

/// Refracts an `EmulatedDouble2` incident vector through a surface with ratio `eta`.
///
/// The incident vector and normal are expected to already be normalized, matching Metal's
/// `refract` contract.
///
/// Quality: Covered by `emulatedDoubleVectorRefractMatchesMetalDefinition`.
static inline EmulatedDouble2 EmulatedDouble2Refract(
    EmulatedDouble2 incident,
    EmulatedDouble2 normal,
    EmulatedDouble eta
) {
    EmulatedDouble one = EmulatedDoubleMake(0x3ff00000u, 0u);
    EmulatedDouble normalDotIncident = EmulatedDouble2Dot(normal, incident);
    EmulatedDouble k = EmulatedDoubleSubtract(
        one,
        EmulatedDoubleMultiply(
            EmulatedDoubleMultiply(eta, eta),
            EmulatedDoubleSubtract(one, EmulatedDoubleMultiply(normalDotIncident, normalDotIncident))
        )
    );
    if (EmulatedDoubleIsLessThanZero(k)) {
        return EmulatedDouble2Zero();
    }
    return EmulatedDouble2Subtract(
        EmulatedDouble2MultiplyScalar(incident, eta),
        EmulatedDouble2MultiplyScalar(normal, EmulatedDoubleAdd(EmulatedDoubleMultiply(eta, normalDotIncident), EmulatedDoubleSqrt(k)))
    );
}

/// Refracts an `EmulatedDouble3` incident vector through a surface with ratio `eta`.
///
/// Quality: Covered by `emulatedDoubleVectorRefractMatchesMetalDefinition`.
static inline EmulatedDouble3 EmulatedDouble3Refract(
    EmulatedDouble3 incident,
    EmulatedDouble3 normal,
    EmulatedDouble eta
) {
    EmulatedDouble one = EmulatedDoubleMake(0x3ff00000u, 0u);
    EmulatedDouble normalDotIncident = EmulatedDouble3Dot(normal, incident);
    EmulatedDouble k = EmulatedDoubleSubtract(
        one,
        EmulatedDoubleMultiply(
            EmulatedDoubleMultiply(eta, eta),
            EmulatedDoubleSubtract(one, EmulatedDoubleMultiply(normalDotIncident, normalDotIncident))
        )
    );
    if (EmulatedDoubleIsLessThanZero(k)) {
        return EmulatedDouble3Zero();
    }
    return EmulatedDouble3Subtract(
        EmulatedDouble3MultiplyScalar(incident, eta),
        EmulatedDouble3MultiplyScalar(normal, EmulatedDoubleAdd(EmulatedDoubleMultiply(eta, normalDotIncident), EmulatedDoubleSqrt(k)))
    );
}

/// Refracts an `EmulatedDouble4` incident vector through a surface with ratio `eta`.
///
/// Quality: Covered by `emulatedDoubleVectorRefractMatchesMetalDefinition`.
static inline EmulatedDouble4 EmulatedDouble4Refract(
    EmulatedDouble4 incident,
    EmulatedDouble4 normal,
    EmulatedDouble eta
) {
    EmulatedDouble one = EmulatedDoubleMake(0x3ff00000u, 0u);
    EmulatedDouble normalDotIncident = EmulatedDouble4Dot(normal, incident);
    EmulatedDouble k = EmulatedDoubleSubtract(
        one,
        EmulatedDoubleMultiply(
            EmulatedDoubleMultiply(eta, eta),
            EmulatedDoubleSubtract(one, EmulatedDoubleMultiply(normalDotIncident, normalDotIncident))
        )
    );
    if (EmulatedDoubleIsLessThanZero(k)) {
        return EmulatedDouble4Zero();
    }
    return EmulatedDouble4Subtract(
        EmulatedDouble4MultiplyScalar(incident, eta),
        EmulatedDouble4MultiplyScalar(normal, EmulatedDoubleAdd(EmulatedDoubleMultiply(eta, normalDotIncident), EmulatedDoubleSqrt(k)))
    );
}

/// Computes the cross product of two `EmulatedDouble3` values.
///
/// Quality: Covered by `emulatedDoubleVectorCrossProductMatchesRightHandRule`.
static inline EmulatedDouble3 EmulatedDouble3Cross(EmulatedDouble3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDoubleSubtract(EmulatedDoubleMultiply(lhs.y, rhs.z), EmulatedDoubleMultiply(lhs.z, rhs.y)),
        EmulatedDoubleSubtract(EmulatedDoubleMultiply(lhs.z, rhs.x), EmulatedDoubleMultiply(lhs.x, rhs.z)),
        EmulatedDoubleSubtract(EmulatedDoubleMultiply(lhs.x, rhs.y), EmulatedDoubleMultiply(lhs.y, rhs.x))
    );
}

/// `EmulatedDouble` matrix with 2 columns and 2 rows, matching Metal's `float2x2` shape.
///
/// Matrices are stored as column vectors: `c0` is column 0, `c1` is column 1.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
typedef struct EmulatedDouble2x2 {
    /// First column.
    EmulatedDouble2 c0;

    /// Second column.
    EmulatedDouble2 c1;
} EmulatedDouble2x2;

/// `EmulatedDouble` matrix with 2 columns and 3 rows, matching Metal's `float2x3` shape.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
typedef struct EmulatedDouble2x3 {
    EmulatedDouble3 c0;
    EmulatedDouble3 c1;
} EmulatedDouble2x3;

/// `EmulatedDouble` matrix with 2 columns and 4 rows, matching Metal's `float2x4` shape.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
typedef struct EmulatedDouble2x4 {
    EmulatedDouble4 c0;
    EmulatedDouble4 c1;
} EmulatedDouble2x4;

/// `EmulatedDouble` matrix with 3 columns and 2 rows, matching Metal's `float3x2` shape.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
typedef struct EmulatedDouble3x2 {
    EmulatedDouble2 c0;
    EmulatedDouble2 c1;
    EmulatedDouble2 c2;
} EmulatedDouble3x2;

/// `EmulatedDouble` matrix with 3 columns and 3 rows, matching Metal's `float3x3` shape.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
typedef struct EmulatedDouble3x3 {
    EmulatedDouble3 c0;
    EmulatedDouble3 c1;
    EmulatedDouble3 c2;
} EmulatedDouble3x3;

/// `EmulatedDouble` matrix with 3 columns and 4 rows, matching Metal's `float3x4` shape.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
typedef struct EmulatedDouble3x4 {
    EmulatedDouble4 c0;
    EmulatedDouble4 c1;
    EmulatedDouble4 c2;
} EmulatedDouble3x4;

/// `EmulatedDouble` matrix with 4 columns and 2 rows, matching Metal's `float4x2` shape.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
typedef struct EmulatedDouble4x2 {
    EmulatedDouble2 c0;
    EmulatedDouble2 c1;
    EmulatedDouble2 c2;
    EmulatedDouble2 c3;
} EmulatedDouble4x2;

/// `EmulatedDouble` matrix with 4 columns and 3 rows, matching Metal's `float4x3` shape.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
typedef struct EmulatedDouble4x3 {
    EmulatedDouble3 c0;
    EmulatedDouble3 c1;
    EmulatedDouble3 c2;
    EmulatedDouble3 c3;
} EmulatedDouble4x3;

/// `EmulatedDouble` matrix with 4 columns and 4 rows, matching Metal's `float4x4` shape.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
typedef struct EmulatedDouble4x4 {
    EmulatedDouble4 c0;
    EmulatedDouble4 c1;
    EmulatedDouble4 c2;
    EmulatedDouble4 c3;
} EmulatedDouble4x4;

/// Creates an `EmulatedDouble2x2` matrix from column vectors.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble2x2 EmulatedDouble2x2Make(EmulatedDouble2 c0, EmulatedDouble2 c1) {
    EmulatedDouble2x2 value;
    value.c0 = c0;
    value.c1 = c1;
    return value;
}

/// Creates an `EmulatedDouble2x2` matrix with every component set to `value`.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble2x2 EmulatedDouble2x2Filled(EmulatedDouble value) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2Splat(value),
        EmulatedDouble2Splat(value)
    );
}

/// Creates an `EmulatedDouble2x3` matrix from column vectors.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble2x3 EmulatedDouble2x3Make(EmulatedDouble3 c0, EmulatedDouble3 c1) {
    EmulatedDouble2x3 value;
    value.c0 = c0;
    value.c1 = c1;
    return value;
}

/// Creates an `EmulatedDouble2x3` matrix with every component set to `value`.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble2x3 EmulatedDouble2x3Filled(EmulatedDouble value) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3Splat(value),
        EmulatedDouble3Splat(value)
    );
}

/// Creates an `EmulatedDouble2x4` matrix from column vectors.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble2x4 EmulatedDouble2x4Make(EmulatedDouble4 c0, EmulatedDouble4 c1) {
    EmulatedDouble2x4 value;
    value.c0 = c0;
    value.c1 = c1;
    return value;
}

/// Creates an `EmulatedDouble2x4` matrix with every component set to `value`.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble2x4 EmulatedDouble2x4Filled(EmulatedDouble value) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4Splat(value),
        EmulatedDouble4Splat(value)
    );
}

/// Creates an `EmulatedDouble3x2` matrix from column vectors.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble3x2 EmulatedDouble3x2Make(EmulatedDouble2 c0, EmulatedDouble2 c1, EmulatedDouble2 c2) {
    EmulatedDouble3x2 value;
    value.c0 = c0;
    value.c1 = c1;
    value.c2 = c2;
    return value;
}

/// Creates an `EmulatedDouble3x2` matrix with every component set to `value`.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble3x2 EmulatedDouble3x2Filled(EmulatedDouble value) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2Splat(value),
        EmulatedDouble2Splat(value),
        EmulatedDouble2Splat(value)
    );
}

/// Creates an `EmulatedDouble3x3` matrix from column vectors.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble3x3 EmulatedDouble3x3Make(EmulatedDouble3 c0, EmulatedDouble3 c1, EmulatedDouble3 c2) {
    EmulatedDouble3x3 value;
    value.c0 = c0;
    value.c1 = c1;
    value.c2 = c2;
    return value;
}

/// Creates an `EmulatedDouble3x3` matrix with every component set to `value`.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble3x3 EmulatedDouble3x3Filled(EmulatedDouble value) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3Splat(value),
        EmulatedDouble3Splat(value),
        EmulatedDouble3Splat(value)
    );
}

/// Creates an `EmulatedDouble3x4` matrix from column vectors.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble3x4 EmulatedDouble3x4Make(EmulatedDouble4 c0, EmulatedDouble4 c1, EmulatedDouble4 c2) {
    EmulatedDouble3x4 value;
    value.c0 = c0;
    value.c1 = c1;
    value.c2 = c2;
    return value;
}

/// Creates an `EmulatedDouble3x4` matrix with every component set to `value`.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble3x4 EmulatedDouble3x4Filled(EmulatedDouble value) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4Splat(value),
        EmulatedDouble4Splat(value),
        EmulatedDouble4Splat(value)
    );
}

/// Creates an `EmulatedDouble4x2` matrix from column vectors.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble4x2 EmulatedDouble4x2Make(EmulatedDouble2 c0, EmulatedDouble2 c1, EmulatedDouble2 c2, EmulatedDouble2 c3) {
    EmulatedDouble4x2 value;
    value.c0 = c0;
    value.c1 = c1;
    value.c2 = c2;
    value.c3 = c3;
    return value;
}

/// Creates an `EmulatedDouble4x2` matrix with every component set to `value`.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble4x2 EmulatedDouble4x2Filled(EmulatedDouble value) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2Splat(value),
        EmulatedDouble2Splat(value),
        EmulatedDouble2Splat(value),
        EmulatedDouble2Splat(value)
    );
}

/// Creates an `EmulatedDouble4x3` matrix from column vectors.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble4x3 EmulatedDouble4x3Make(EmulatedDouble3 c0, EmulatedDouble3 c1, EmulatedDouble3 c2, EmulatedDouble3 c3) {
    EmulatedDouble4x3 value;
    value.c0 = c0;
    value.c1 = c1;
    value.c2 = c2;
    value.c3 = c3;
    return value;
}

/// Creates an `EmulatedDouble4x3` matrix with every component set to `value`.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble4x3 EmulatedDouble4x3Filled(EmulatedDouble value) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3Splat(value),
        EmulatedDouble3Splat(value),
        EmulatedDouble3Splat(value),
        EmulatedDouble3Splat(value)
    );
}

/// Creates an `EmulatedDouble4x4` matrix from column vectors.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble4x4 EmulatedDouble4x4Make(EmulatedDouble4 c0, EmulatedDouble4 c1, EmulatedDouble4 c2, EmulatedDouble4 c3) {
    EmulatedDouble4x4 value;
    value.c0 = c0;
    value.c1 = c1;
    value.c2 = c2;
    value.c3 = c3;
    return value;
}

/// Creates an `EmulatedDouble4x4` matrix with every component set to `value`.
///
/// Quality: Covered by `emulatedDoubleMatrixConstructorsUseColumnMajorLayout`.
static inline EmulatedDouble4x4 EmulatedDouble4x4Filled(EmulatedDouble value) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4Splat(value),
        EmulatedDouble4Splat(value),
        EmulatedDouble4Splat(value),
        EmulatedDouble4Splat(value)
    );
}

/// Creates an `EmulatedDouble2x2` diagonal matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble2x2 EmulatedDouble2x2Diagonal(EmulatedDouble value) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    return EmulatedDouble2x2Make(
        EmulatedDouble2Make(value, zero),
        EmulatedDouble2Make(zero, value)
    );
}

/// Creates an `EmulatedDouble2x3` diagonal matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble2x3 EmulatedDouble2x3Diagonal(EmulatedDouble value) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    return EmulatedDouble2x3Make(
        EmulatedDouble3Make(value, zero, zero),
        EmulatedDouble3Make(zero, value, zero)
    );
}

/// Creates an `EmulatedDouble2x4` diagonal matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble2x4 EmulatedDouble2x4Diagonal(EmulatedDouble value) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    return EmulatedDouble2x4Make(
        EmulatedDouble4Make(value, zero, zero, zero),
        EmulatedDouble4Make(zero, value, zero, zero)
    );
}

/// Creates an `EmulatedDouble3x2` diagonal matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble3x2 EmulatedDouble3x2Diagonal(EmulatedDouble value) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    return EmulatedDouble3x2Make(
        EmulatedDouble2Make(value, zero),
        EmulatedDouble2Make(zero, value),
        EmulatedDouble2Make(zero, zero)
    );
}

/// Creates an `EmulatedDouble3x3` diagonal matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble3x3 EmulatedDouble3x3Diagonal(EmulatedDouble value) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    return EmulatedDouble3x3Make(
        EmulatedDouble3Make(value, zero, zero),
        EmulatedDouble3Make(zero, value, zero),
        EmulatedDouble3Make(zero, zero, value)
    );
}

/// Creates an `EmulatedDouble3x4` diagonal matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble3x4 EmulatedDouble3x4Diagonal(EmulatedDouble value) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    return EmulatedDouble3x4Make(
        EmulatedDouble4Make(value, zero, zero, zero),
        EmulatedDouble4Make(zero, value, zero, zero),
        EmulatedDouble4Make(zero, zero, value, zero)
    );
}

/// Creates an `EmulatedDouble4x2` diagonal matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble4x2 EmulatedDouble4x2Diagonal(EmulatedDouble value) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    return EmulatedDouble4x2Make(
        EmulatedDouble2Make(value, zero),
        EmulatedDouble2Make(zero, value),
        EmulatedDouble2Make(zero, zero),
        EmulatedDouble2Make(zero, zero)
    );
}

/// Creates an `EmulatedDouble4x3` diagonal matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble4x3 EmulatedDouble4x3Diagonal(EmulatedDouble value) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    return EmulatedDouble4x3Make(
        EmulatedDouble3Make(value, zero, zero),
        EmulatedDouble3Make(zero, value, zero),
        EmulatedDouble3Make(zero, zero, value),
        EmulatedDouble3Make(zero, zero, zero)
    );
}

/// Creates an `EmulatedDouble4x4` diagonal matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble4x4 EmulatedDouble4x4Diagonal(EmulatedDouble value) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    return EmulatedDouble4x4Make(
        EmulatedDouble4Make(value, zero, zero, zero),
        EmulatedDouble4Make(zero, value, zero, zero),
        EmulatedDouble4Make(zero, zero, value, zero),
        EmulatedDouble4Make(zero, zero, zero, value)
    );
}

/// Creates an `EmulatedDouble2x2` identity matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble2x2 EmulatedDouble2x2Identity(void) {
    return EmulatedDouble2x2Diagonal(EmulatedDoubleMake(0x3ff00000u, 0u));
}

/// Creates an `EmulatedDouble3x3` identity matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble3x3 EmulatedDouble3x3Identity(void) {
    return EmulatedDouble3x3Diagonal(EmulatedDoubleMake(0x3ff00000u, 0u));
}

/// Creates an `EmulatedDouble4x4` identity matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixDiagonalAndIdentityConstructors`.
static inline EmulatedDouble4x4 EmulatedDouble4x4Identity(void) {
    return EmulatedDouble4x4Diagonal(EmulatedDoubleMake(0x3ff00000u, 0u));
}

/// Adds two `EmulatedDouble2x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x2 EmulatedDouble2x2Add(EmulatedDouble2x2 lhs, EmulatedDouble2x2 rhs) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2Add(lhs.c0, rhs.c0),
        EmulatedDouble2Add(lhs.c1, rhs.c1)
    );
}

/// Subtracts two `EmulatedDouble2x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x2 EmulatedDouble2x2Subtract(EmulatedDouble2x2 lhs, EmulatedDouble2x2 rhs) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2Subtract(lhs.c0, rhs.c0),
        EmulatedDouble2Subtract(lhs.c1, rhs.c1)
    );
}

/// Multiplies two `EmulatedDouble2x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x2 EmulatedDouble2x2MultiplyComponents(EmulatedDouble2x2 lhs, EmulatedDouble2x2 rhs) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2Multiply(lhs.c0, rhs.c0),
        EmulatedDouble2Multiply(lhs.c1, rhs.c1)
    );
}

/// Divides two `EmulatedDouble2x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x2 EmulatedDouble2x2DivideComponents(EmulatedDouble2x2 lhs, EmulatedDouble2x2 rhs) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2Divide(lhs.c0, rhs.c0),
        EmulatedDouble2Divide(lhs.c1, rhs.c1)
    );
}

/// Multiplies every component of an `EmulatedDouble2x2` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x2 EmulatedDouble2x2MultiplyScalar(EmulatedDouble2x2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2MultiplyScalar(lhs.c0, rhs),
        EmulatedDouble2MultiplyScalar(lhs.c1, rhs)
    );
}

/// Divides every component of an `EmulatedDouble2x2` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x2 EmulatedDouble2x2DivideScalar(EmulatedDouble2x2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2DivideScalar(lhs.c0, rhs),
        EmulatedDouble2DivideScalar(lhs.c1, rhs)
    );
}

/// Negates every component of an `EmulatedDouble2x2`.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x2 EmulatedDouble2x2Negated(EmulatedDouble2x2 value) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2Negated(value.c0),
        EmulatedDouble2Negated(value.c1)
    );
}

/// Adds two `EmulatedDouble2x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x3 EmulatedDouble2x3Add(EmulatedDouble2x3 lhs, EmulatedDouble2x3 rhs) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3Add(lhs.c0, rhs.c0),
        EmulatedDouble3Add(lhs.c1, rhs.c1)
    );
}

/// Subtracts two `EmulatedDouble2x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x3 EmulatedDouble2x3Subtract(EmulatedDouble2x3 lhs, EmulatedDouble2x3 rhs) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3Subtract(lhs.c0, rhs.c0),
        EmulatedDouble3Subtract(lhs.c1, rhs.c1)
    );
}

/// Multiplies two `EmulatedDouble2x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x3 EmulatedDouble2x3MultiplyComponents(EmulatedDouble2x3 lhs, EmulatedDouble2x3 rhs) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3Multiply(lhs.c0, rhs.c0),
        EmulatedDouble3Multiply(lhs.c1, rhs.c1)
    );
}

/// Divides two `EmulatedDouble2x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x3 EmulatedDouble2x3DivideComponents(EmulatedDouble2x3 lhs, EmulatedDouble2x3 rhs) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3Divide(lhs.c0, rhs.c0),
        EmulatedDouble3Divide(lhs.c1, rhs.c1)
    );
}

/// Multiplies every component of an `EmulatedDouble2x3` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x3 EmulatedDouble2x3MultiplyScalar(EmulatedDouble2x3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3MultiplyScalar(lhs.c0, rhs),
        EmulatedDouble3MultiplyScalar(lhs.c1, rhs)
    );
}

/// Divides every component of an `EmulatedDouble2x3` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x3 EmulatedDouble2x3DivideScalar(EmulatedDouble2x3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3DivideScalar(lhs.c0, rhs),
        EmulatedDouble3DivideScalar(lhs.c1, rhs)
    );
}

/// Negates every component of an `EmulatedDouble2x3`.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x3 EmulatedDouble2x3Negated(EmulatedDouble2x3 value) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3Negated(value.c0),
        EmulatedDouble3Negated(value.c1)
    );
}

/// Adds two `EmulatedDouble2x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x4 EmulatedDouble2x4Add(EmulatedDouble2x4 lhs, EmulatedDouble2x4 rhs) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4Add(lhs.c0, rhs.c0),
        EmulatedDouble4Add(lhs.c1, rhs.c1)
    );
}

/// Subtracts two `EmulatedDouble2x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x4 EmulatedDouble2x4Subtract(EmulatedDouble2x4 lhs, EmulatedDouble2x4 rhs) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4Subtract(lhs.c0, rhs.c0),
        EmulatedDouble4Subtract(lhs.c1, rhs.c1)
    );
}

/// Multiplies two `EmulatedDouble2x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x4 EmulatedDouble2x4MultiplyComponents(EmulatedDouble2x4 lhs, EmulatedDouble2x4 rhs) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4Multiply(lhs.c0, rhs.c0),
        EmulatedDouble4Multiply(lhs.c1, rhs.c1)
    );
}

/// Divides two `EmulatedDouble2x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x4 EmulatedDouble2x4DivideComponents(EmulatedDouble2x4 lhs, EmulatedDouble2x4 rhs) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4Divide(lhs.c0, rhs.c0),
        EmulatedDouble4Divide(lhs.c1, rhs.c1)
    );
}

/// Multiplies every component of an `EmulatedDouble2x4` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x4 EmulatedDouble2x4MultiplyScalar(EmulatedDouble2x4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4MultiplyScalar(lhs.c0, rhs),
        EmulatedDouble4MultiplyScalar(lhs.c1, rhs)
    );
}

/// Divides every component of an `EmulatedDouble2x4` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x4 EmulatedDouble2x4DivideScalar(EmulatedDouble2x4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4DivideScalar(lhs.c0, rhs),
        EmulatedDouble4DivideScalar(lhs.c1, rhs)
    );
}

/// Negates every component of an `EmulatedDouble2x4`.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble2x4 EmulatedDouble2x4Negated(EmulatedDouble2x4 value) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4Negated(value.c0),
        EmulatedDouble4Negated(value.c1)
    );
}

/// Adds two `EmulatedDouble3x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x2 EmulatedDouble3x2Add(EmulatedDouble3x2 lhs, EmulatedDouble3x2 rhs) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2Add(lhs.c0, rhs.c0),
        EmulatedDouble2Add(lhs.c1, rhs.c1),
        EmulatedDouble2Add(lhs.c2, rhs.c2)
    );
}

/// Subtracts two `EmulatedDouble3x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x2 EmulatedDouble3x2Subtract(EmulatedDouble3x2 lhs, EmulatedDouble3x2 rhs) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2Subtract(lhs.c0, rhs.c0),
        EmulatedDouble2Subtract(lhs.c1, rhs.c1),
        EmulatedDouble2Subtract(lhs.c2, rhs.c2)
    );
}

/// Multiplies two `EmulatedDouble3x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x2 EmulatedDouble3x2MultiplyComponents(EmulatedDouble3x2 lhs, EmulatedDouble3x2 rhs) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2Multiply(lhs.c0, rhs.c0),
        EmulatedDouble2Multiply(lhs.c1, rhs.c1),
        EmulatedDouble2Multiply(lhs.c2, rhs.c2)
    );
}

/// Divides two `EmulatedDouble3x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x2 EmulatedDouble3x2DivideComponents(EmulatedDouble3x2 lhs, EmulatedDouble3x2 rhs) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2Divide(lhs.c0, rhs.c0),
        EmulatedDouble2Divide(lhs.c1, rhs.c1),
        EmulatedDouble2Divide(lhs.c2, rhs.c2)
    );
}

/// Multiplies every component of an `EmulatedDouble3x2` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x2 EmulatedDouble3x2MultiplyScalar(EmulatedDouble3x2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2MultiplyScalar(lhs.c0, rhs),
        EmulatedDouble2MultiplyScalar(lhs.c1, rhs),
        EmulatedDouble2MultiplyScalar(lhs.c2, rhs)
    );
}

/// Divides every component of an `EmulatedDouble3x2` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x2 EmulatedDouble3x2DivideScalar(EmulatedDouble3x2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2DivideScalar(lhs.c0, rhs),
        EmulatedDouble2DivideScalar(lhs.c1, rhs),
        EmulatedDouble2DivideScalar(lhs.c2, rhs)
    );
}

/// Negates every component of an `EmulatedDouble3x2`.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x2 EmulatedDouble3x2Negated(EmulatedDouble3x2 value) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2Negated(value.c0),
        EmulatedDouble2Negated(value.c1),
        EmulatedDouble2Negated(value.c2)
    );
}

/// Adds two `EmulatedDouble3x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x3 EmulatedDouble3x3Add(EmulatedDouble3x3 lhs, EmulatedDouble3x3 rhs) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3Add(lhs.c0, rhs.c0),
        EmulatedDouble3Add(lhs.c1, rhs.c1),
        EmulatedDouble3Add(lhs.c2, rhs.c2)
    );
}

/// Subtracts two `EmulatedDouble3x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x3 EmulatedDouble3x3Subtract(EmulatedDouble3x3 lhs, EmulatedDouble3x3 rhs) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3Subtract(lhs.c0, rhs.c0),
        EmulatedDouble3Subtract(lhs.c1, rhs.c1),
        EmulatedDouble3Subtract(lhs.c2, rhs.c2)
    );
}

/// Multiplies two `EmulatedDouble3x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x3 EmulatedDouble3x3MultiplyComponents(EmulatedDouble3x3 lhs, EmulatedDouble3x3 rhs) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3Multiply(lhs.c0, rhs.c0),
        EmulatedDouble3Multiply(lhs.c1, rhs.c1),
        EmulatedDouble3Multiply(lhs.c2, rhs.c2)
    );
}

/// Divides two `EmulatedDouble3x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x3 EmulatedDouble3x3DivideComponents(EmulatedDouble3x3 lhs, EmulatedDouble3x3 rhs) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3Divide(lhs.c0, rhs.c0),
        EmulatedDouble3Divide(lhs.c1, rhs.c1),
        EmulatedDouble3Divide(lhs.c2, rhs.c2)
    );
}

/// Multiplies every component of an `EmulatedDouble3x3` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x3 EmulatedDouble3x3MultiplyScalar(EmulatedDouble3x3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3MultiplyScalar(lhs.c0, rhs),
        EmulatedDouble3MultiplyScalar(lhs.c1, rhs),
        EmulatedDouble3MultiplyScalar(lhs.c2, rhs)
    );
}

/// Divides every component of an `EmulatedDouble3x3` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x3 EmulatedDouble3x3DivideScalar(EmulatedDouble3x3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3DivideScalar(lhs.c0, rhs),
        EmulatedDouble3DivideScalar(lhs.c1, rhs),
        EmulatedDouble3DivideScalar(lhs.c2, rhs)
    );
}

/// Negates every component of an `EmulatedDouble3x3`.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x3 EmulatedDouble3x3Negated(EmulatedDouble3x3 value) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3Negated(value.c0),
        EmulatedDouble3Negated(value.c1),
        EmulatedDouble3Negated(value.c2)
    );
}

/// Adds two `EmulatedDouble3x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x4 EmulatedDouble3x4Add(EmulatedDouble3x4 lhs, EmulatedDouble3x4 rhs) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4Add(lhs.c0, rhs.c0),
        EmulatedDouble4Add(lhs.c1, rhs.c1),
        EmulatedDouble4Add(lhs.c2, rhs.c2)
    );
}

/// Subtracts two `EmulatedDouble3x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x4 EmulatedDouble3x4Subtract(EmulatedDouble3x4 lhs, EmulatedDouble3x4 rhs) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4Subtract(lhs.c0, rhs.c0),
        EmulatedDouble4Subtract(lhs.c1, rhs.c1),
        EmulatedDouble4Subtract(lhs.c2, rhs.c2)
    );
}

/// Multiplies two `EmulatedDouble3x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x4 EmulatedDouble3x4MultiplyComponents(EmulatedDouble3x4 lhs, EmulatedDouble3x4 rhs) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4Multiply(lhs.c0, rhs.c0),
        EmulatedDouble4Multiply(lhs.c1, rhs.c1),
        EmulatedDouble4Multiply(lhs.c2, rhs.c2)
    );
}

/// Divides two `EmulatedDouble3x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x4 EmulatedDouble3x4DivideComponents(EmulatedDouble3x4 lhs, EmulatedDouble3x4 rhs) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4Divide(lhs.c0, rhs.c0),
        EmulatedDouble4Divide(lhs.c1, rhs.c1),
        EmulatedDouble4Divide(lhs.c2, rhs.c2)
    );
}

/// Multiplies every component of an `EmulatedDouble3x4` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x4 EmulatedDouble3x4MultiplyScalar(EmulatedDouble3x4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4MultiplyScalar(lhs.c0, rhs),
        EmulatedDouble4MultiplyScalar(lhs.c1, rhs),
        EmulatedDouble4MultiplyScalar(lhs.c2, rhs)
    );
}

/// Divides every component of an `EmulatedDouble3x4` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x4 EmulatedDouble3x4DivideScalar(EmulatedDouble3x4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4DivideScalar(lhs.c0, rhs),
        EmulatedDouble4DivideScalar(lhs.c1, rhs),
        EmulatedDouble4DivideScalar(lhs.c2, rhs)
    );
}

/// Negates every component of an `EmulatedDouble3x4`.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble3x4 EmulatedDouble3x4Negated(EmulatedDouble3x4 value) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4Negated(value.c0),
        EmulatedDouble4Negated(value.c1),
        EmulatedDouble4Negated(value.c2)
    );
}

/// Adds two `EmulatedDouble4x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x2 EmulatedDouble4x2Add(EmulatedDouble4x2 lhs, EmulatedDouble4x2 rhs) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2Add(lhs.c0, rhs.c0),
        EmulatedDouble2Add(lhs.c1, rhs.c1),
        EmulatedDouble2Add(lhs.c2, rhs.c2),
        EmulatedDouble2Add(lhs.c3, rhs.c3)
    );
}

/// Subtracts two `EmulatedDouble4x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x2 EmulatedDouble4x2Subtract(EmulatedDouble4x2 lhs, EmulatedDouble4x2 rhs) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2Subtract(lhs.c0, rhs.c0),
        EmulatedDouble2Subtract(lhs.c1, rhs.c1),
        EmulatedDouble2Subtract(lhs.c2, rhs.c2),
        EmulatedDouble2Subtract(lhs.c3, rhs.c3)
    );
}

/// Multiplies two `EmulatedDouble4x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x2 EmulatedDouble4x2MultiplyComponents(EmulatedDouble4x2 lhs, EmulatedDouble4x2 rhs) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2Multiply(lhs.c0, rhs.c0),
        EmulatedDouble2Multiply(lhs.c1, rhs.c1),
        EmulatedDouble2Multiply(lhs.c2, rhs.c2),
        EmulatedDouble2Multiply(lhs.c3, rhs.c3)
    );
}

/// Divides two `EmulatedDouble4x2` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x2 EmulatedDouble4x2DivideComponents(EmulatedDouble4x2 lhs, EmulatedDouble4x2 rhs) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2Divide(lhs.c0, rhs.c0),
        EmulatedDouble2Divide(lhs.c1, rhs.c1),
        EmulatedDouble2Divide(lhs.c2, rhs.c2),
        EmulatedDouble2Divide(lhs.c3, rhs.c3)
    );
}

/// Multiplies every component of an `EmulatedDouble4x2` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x2 EmulatedDouble4x2MultiplyScalar(EmulatedDouble4x2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2MultiplyScalar(lhs.c0, rhs),
        EmulatedDouble2MultiplyScalar(lhs.c1, rhs),
        EmulatedDouble2MultiplyScalar(lhs.c2, rhs),
        EmulatedDouble2MultiplyScalar(lhs.c3, rhs)
    );
}

/// Divides every component of an `EmulatedDouble4x2` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x2 EmulatedDouble4x2DivideScalar(EmulatedDouble4x2 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2DivideScalar(lhs.c0, rhs),
        EmulatedDouble2DivideScalar(lhs.c1, rhs),
        EmulatedDouble2DivideScalar(lhs.c2, rhs),
        EmulatedDouble2DivideScalar(lhs.c3, rhs)
    );
}

/// Negates every component of an `EmulatedDouble4x2`.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x2 EmulatedDouble4x2Negated(EmulatedDouble4x2 value) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2Negated(value.c0),
        EmulatedDouble2Negated(value.c1),
        EmulatedDouble2Negated(value.c2),
        EmulatedDouble2Negated(value.c3)
    );
}

/// Adds two `EmulatedDouble4x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x3 EmulatedDouble4x3Add(EmulatedDouble4x3 lhs, EmulatedDouble4x3 rhs) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3Add(lhs.c0, rhs.c0),
        EmulatedDouble3Add(lhs.c1, rhs.c1),
        EmulatedDouble3Add(lhs.c2, rhs.c2),
        EmulatedDouble3Add(lhs.c3, rhs.c3)
    );
}

/// Subtracts two `EmulatedDouble4x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x3 EmulatedDouble4x3Subtract(EmulatedDouble4x3 lhs, EmulatedDouble4x3 rhs) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3Subtract(lhs.c0, rhs.c0),
        EmulatedDouble3Subtract(lhs.c1, rhs.c1),
        EmulatedDouble3Subtract(lhs.c2, rhs.c2),
        EmulatedDouble3Subtract(lhs.c3, rhs.c3)
    );
}

/// Multiplies two `EmulatedDouble4x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x3 EmulatedDouble4x3MultiplyComponents(EmulatedDouble4x3 lhs, EmulatedDouble4x3 rhs) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3Multiply(lhs.c0, rhs.c0),
        EmulatedDouble3Multiply(lhs.c1, rhs.c1),
        EmulatedDouble3Multiply(lhs.c2, rhs.c2),
        EmulatedDouble3Multiply(lhs.c3, rhs.c3)
    );
}

/// Divides two `EmulatedDouble4x3` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x3 EmulatedDouble4x3DivideComponents(EmulatedDouble4x3 lhs, EmulatedDouble4x3 rhs) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3Divide(lhs.c0, rhs.c0),
        EmulatedDouble3Divide(lhs.c1, rhs.c1),
        EmulatedDouble3Divide(lhs.c2, rhs.c2),
        EmulatedDouble3Divide(lhs.c3, rhs.c3)
    );
}

/// Multiplies every component of an `EmulatedDouble4x3` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x3 EmulatedDouble4x3MultiplyScalar(EmulatedDouble4x3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3MultiplyScalar(lhs.c0, rhs),
        EmulatedDouble3MultiplyScalar(lhs.c1, rhs),
        EmulatedDouble3MultiplyScalar(lhs.c2, rhs),
        EmulatedDouble3MultiplyScalar(lhs.c3, rhs)
    );
}

/// Divides every component of an `EmulatedDouble4x3` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x3 EmulatedDouble4x3DivideScalar(EmulatedDouble4x3 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3DivideScalar(lhs.c0, rhs),
        EmulatedDouble3DivideScalar(lhs.c1, rhs),
        EmulatedDouble3DivideScalar(lhs.c2, rhs),
        EmulatedDouble3DivideScalar(lhs.c3, rhs)
    );
}

/// Negates every component of an `EmulatedDouble4x3`.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x3 EmulatedDouble4x3Negated(EmulatedDouble4x3 value) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3Negated(value.c0),
        EmulatedDouble3Negated(value.c1),
        EmulatedDouble3Negated(value.c2),
        EmulatedDouble3Negated(value.c3)
    );
}

/// Adds two `EmulatedDouble4x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x4 EmulatedDouble4x4Add(EmulatedDouble4x4 lhs, EmulatedDouble4x4 rhs) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4Add(lhs.c0, rhs.c0),
        EmulatedDouble4Add(lhs.c1, rhs.c1),
        EmulatedDouble4Add(lhs.c2, rhs.c2),
        EmulatedDouble4Add(lhs.c3, rhs.c3)
    );
}

/// Subtracts two `EmulatedDouble4x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x4 EmulatedDouble4x4Subtract(EmulatedDouble4x4 lhs, EmulatedDouble4x4 rhs) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4Subtract(lhs.c0, rhs.c0),
        EmulatedDouble4Subtract(lhs.c1, rhs.c1),
        EmulatedDouble4Subtract(lhs.c2, rhs.c2),
        EmulatedDouble4Subtract(lhs.c3, rhs.c3)
    );
}

/// Multiplies two `EmulatedDouble4x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x4 EmulatedDouble4x4MultiplyComponents(EmulatedDouble4x4 lhs, EmulatedDouble4x4 rhs) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4Multiply(lhs.c0, rhs.c0),
        EmulatedDouble4Multiply(lhs.c1, rhs.c1),
        EmulatedDouble4Multiply(lhs.c2, rhs.c2),
        EmulatedDouble4Multiply(lhs.c3, rhs.c3)
    );
}

/// Divides two `EmulatedDouble4x4` matrices componentwise.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x4 EmulatedDouble4x4DivideComponents(EmulatedDouble4x4 lhs, EmulatedDouble4x4 rhs) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4Divide(lhs.c0, rhs.c0),
        EmulatedDouble4Divide(lhs.c1, rhs.c1),
        EmulatedDouble4Divide(lhs.c2, rhs.c2),
        EmulatedDouble4Divide(lhs.c3, rhs.c3)
    );
}

/// Multiplies every component of an `EmulatedDouble4x4` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x4 EmulatedDouble4x4MultiplyScalar(EmulatedDouble4x4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4MultiplyScalar(lhs.c0, rhs),
        EmulatedDouble4MultiplyScalar(lhs.c1, rhs),
        EmulatedDouble4MultiplyScalar(lhs.c2, rhs),
        EmulatedDouble4MultiplyScalar(lhs.c3, rhs)
    );
}

/// Divides every component of an `EmulatedDouble4x4` by a scalar.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x4 EmulatedDouble4x4DivideScalar(EmulatedDouble4x4 lhs, EmulatedDouble rhs) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4DivideScalar(lhs.c0, rhs),
        EmulatedDouble4DivideScalar(lhs.c1, rhs),
        EmulatedDouble4DivideScalar(lhs.c2, rhs),
        EmulatedDouble4DivideScalar(lhs.c3, rhs)
    );
}

/// Negates every component of an `EmulatedDouble4x4`.
///
/// Quality: Covered by `emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic`.
static inline EmulatedDouble4x4 EmulatedDouble4x4Negated(EmulatedDouble4x4 value) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4Negated(value.c0),
        EmulatedDouble4Negated(value.c1),
        EmulatedDouble4Negated(value.c2),
        EmulatedDouble4Negated(value.c3)
    );
}

/// Transposes an `EmulatedDouble2x2`.
///
/// Quality: Covered by `emulatedDoubleMatrixTransposeSwapsColumnsAndRows`.
static inline EmulatedDouble2x2 EmulatedDouble2x2Transpose(EmulatedDouble2x2 value) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2Make(value.c0.x, value.c1.x),
        EmulatedDouble2Make(value.c0.y, value.c1.y)
    );
}

/// Transposes an `EmulatedDouble2x3`.
///
/// Quality: Covered by `emulatedDoubleMatrixTransposeSwapsColumnsAndRows`.
static inline EmulatedDouble3x2 EmulatedDouble2x3Transpose(EmulatedDouble2x3 value) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2Make(value.c0.x, value.c1.x),
        EmulatedDouble2Make(value.c0.y, value.c1.y),
        EmulatedDouble2Make(value.c0.z, value.c1.z)
    );
}

/// Transposes an `EmulatedDouble2x4`.
///
/// Quality: Covered by `emulatedDoubleMatrixTransposeSwapsColumnsAndRows`.
static inline EmulatedDouble4x2 EmulatedDouble2x4Transpose(EmulatedDouble2x4 value) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2Make(value.c0.x, value.c1.x),
        EmulatedDouble2Make(value.c0.y, value.c1.y),
        EmulatedDouble2Make(value.c0.z, value.c1.z),
        EmulatedDouble2Make(value.c0.w, value.c1.w)
    );
}

/// Transposes an `EmulatedDouble3x2`.
///
/// Quality: Covered by `emulatedDoubleMatrixTransposeSwapsColumnsAndRows`.
static inline EmulatedDouble2x3 EmulatedDouble3x2Transpose(EmulatedDouble3x2 value) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3Make(value.c0.x, value.c1.x, value.c2.x),
        EmulatedDouble3Make(value.c0.y, value.c1.y, value.c2.y)
    );
}

/// Transposes an `EmulatedDouble3x3`.
///
/// Quality: Covered by `emulatedDoubleMatrixTransposeSwapsColumnsAndRows`.
static inline EmulatedDouble3x3 EmulatedDouble3x3Transpose(EmulatedDouble3x3 value) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3Make(value.c0.x, value.c1.x, value.c2.x),
        EmulatedDouble3Make(value.c0.y, value.c1.y, value.c2.y),
        EmulatedDouble3Make(value.c0.z, value.c1.z, value.c2.z)
    );
}

/// Transposes an `EmulatedDouble3x4`.
///
/// Quality: Covered by `emulatedDoubleMatrixTransposeSwapsColumnsAndRows`.
static inline EmulatedDouble4x3 EmulatedDouble3x4Transpose(EmulatedDouble3x4 value) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3Make(value.c0.x, value.c1.x, value.c2.x),
        EmulatedDouble3Make(value.c0.y, value.c1.y, value.c2.y),
        EmulatedDouble3Make(value.c0.z, value.c1.z, value.c2.z),
        EmulatedDouble3Make(value.c0.w, value.c1.w, value.c2.w)
    );
}

/// Transposes an `EmulatedDouble4x2`.
///
/// Quality: Covered by `emulatedDoubleMatrixTransposeSwapsColumnsAndRows`.
static inline EmulatedDouble2x4 EmulatedDouble4x2Transpose(EmulatedDouble4x2 value) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4Make(value.c0.x, value.c1.x, value.c2.x, value.c3.x),
        EmulatedDouble4Make(value.c0.y, value.c1.y, value.c2.y, value.c3.y)
    );
}

/// Transposes an `EmulatedDouble4x3`.
///
/// Quality: Covered by `emulatedDoubleMatrixTransposeSwapsColumnsAndRows`.
static inline EmulatedDouble3x4 EmulatedDouble4x3Transpose(EmulatedDouble4x3 value) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4Make(value.c0.x, value.c1.x, value.c2.x, value.c3.x),
        EmulatedDouble4Make(value.c0.y, value.c1.y, value.c2.y, value.c3.y),
        EmulatedDouble4Make(value.c0.z, value.c1.z, value.c2.z, value.c3.z)
    );
}

/// Transposes an `EmulatedDouble4x4`.
///
/// Quality: Covered by `emulatedDoubleMatrixTransposeSwapsColumnsAndRows`.
static inline EmulatedDouble4x4 EmulatedDouble4x4Transpose(EmulatedDouble4x4 value) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4Make(value.c0.x, value.c1.x, value.c2.x, value.c3.x),
        EmulatedDouble4Make(value.c0.y, value.c1.y, value.c2.y, value.c3.y),
        EmulatedDouble4Make(value.c0.z, value.c1.z, value.c2.z, value.c3.z),
        EmulatedDouble4Make(value.c0.w, value.c1.w, value.c2.w, value.c3.w)
    );
}

/// Multiplies an `EmulatedDouble2x2` matrix by an `EmulatedDouble2` vector.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble2 EmulatedDouble2x2MultiplyVector(EmulatedDouble2x2 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble2Add(
        EmulatedDouble2MultiplyScalar(lhs.c0, rhs.x),
        EmulatedDouble2MultiplyScalar(lhs.c1, rhs.y)
    );
}

/// Multiplies an `EmulatedDouble2` row vector by an `EmulatedDouble2x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble2 EmulatedDouble2x2LeftMultiplyVector(EmulatedDouble2 lhs, EmulatedDouble2x2 rhs) {
    return EmulatedDouble2Make(
        EmulatedDouble2Dot(lhs, rhs.c0),
        EmulatedDouble2Dot(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble2x3` matrix by an `EmulatedDouble2` vector.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble3 EmulatedDouble2x3MultiplyVector(EmulatedDouble2x3 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble3Add(
        EmulatedDouble3MultiplyScalar(lhs.c0, rhs.x),
        EmulatedDouble3MultiplyScalar(lhs.c1, rhs.y)
    );
}

/// Multiplies an `EmulatedDouble3` row vector by an `EmulatedDouble2x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble2 EmulatedDouble2x3LeftMultiplyVector(EmulatedDouble3 lhs, EmulatedDouble2x3 rhs) {
    return EmulatedDouble2Make(
        EmulatedDouble3Dot(lhs, rhs.c0),
        EmulatedDouble3Dot(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble2x4` matrix by an `EmulatedDouble2` vector.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble4 EmulatedDouble2x4MultiplyVector(EmulatedDouble2x4 lhs, EmulatedDouble2 rhs) {
    return EmulatedDouble4Add(
        EmulatedDouble4MultiplyScalar(lhs.c0, rhs.x),
        EmulatedDouble4MultiplyScalar(lhs.c1, rhs.y)
    );
}

/// Multiplies an `EmulatedDouble4` row vector by an `EmulatedDouble2x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble2 EmulatedDouble2x4LeftMultiplyVector(EmulatedDouble4 lhs, EmulatedDouble2x4 rhs) {
    return EmulatedDouble2Make(
        EmulatedDouble4Dot(lhs, rhs.c0),
        EmulatedDouble4Dot(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble3x2` matrix by an `EmulatedDouble3` vector.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble2 EmulatedDouble3x2MultiplyVector(EmulatedDouble3x2 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble2Add(
        EmulatedDouble2Add(
            EmulatedDouble2MultiplyScalar(lhs.c0, rhs.x),
            EmulatedDouble2MultiplyScalar(lhs.c1, rhs.y)
        ),
        EmulatedDouble2MultiplyScalar(lhs.c2, rhs.z)
    );
}

/// Multiplies an `EmulatedDouble2` row vector by an `EmulatedDouble3x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble3 EmulatedDouble3x2LeftMultiplyVector(EmulatedDouble2 lhs, EmulatedDouble3x2 rhs) {
    return EmulatedDouble3Make(
        EmulatedDouble2Dot(lhs, rhs.c0),
        EmulatedDouble2Dot(lhs, rhs.c1),
        EmulatedDouble2Dot(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble3x3` matrix by an `EmulatedDouble3` vector.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble3 EmulatedDouble3x3MultiplyVector(EmulatedDouble3x3 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble3Add(
        EmulatedDouble3Add(
            EmulatedDouble3MultiplyScalar(lhs.c0, rhs.x),
            EmulatedDouble3MultiplyScalar(lhs.c1, rhs.y)
        ),
        EmulatedDouble3MultiplyScalar(lhs.c2, rhs.z)
    );
}

/// Multiplies an `EmulatedDouble3` row vector by an `EmulatedDouble3x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble3 EmulatedDouble3x3LeftMultiplyVector(EmulatedDouble3 lhs, EmulatedDouble3x3 rhs) {
    return EmulatedDouble3Make(
        EmulatedDouble3Dot(lhs, rhs.c0),
        EmulatedDouble3Dot(lhs, rhs.c1),
        EmulatedDouble3Dot(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble3x4` matrix by an `EmulatedDouble3` vector.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble4 EmulatedDouble3x4MultiplyVector(EmulatedDouble3x4 lhs, EmulatedDouble3 rhs) {
    return EmulatedDouble4Add(
        EmulatedDouble4Add(
            EmulatedDouble4MultiplyScalar(lhs.c0, rhs.x),
            EmulatedDouble4MultiplyScalar(lhs.c1, rhs.y)
        ),
        EmulatedDouble4MultiplyScalar(lhs.c2, rhs.z)
    );
}

/// Multiplies an `EmulatedDouble4` row vector by an `EmulatedDouble3x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble3 EmulatedDouble3x4LeftMultiplyVector(EmulatedDouble4 lhs, EmulatedDouble3x4 rhs) {
    return EmulatedDouble3Make(
        EmulatedDouble4Dot(lhs, rhs.c0),
        EmulatedDouble4Dot(lhs, rhs.c1),
        EmulatedDouble4Dot(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble4x2` matrix by an `EmulatedDouble4` vector.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble2 EmulatedDouble4x2MultiplyVector(EmulatedDouble4x2 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble2Add(
        EmulatedDouble2Add(
            EmulatedDouble2MultiplyScalar(lhs.c0, rhs.x),
            EmulatedDouble2MultiplyScalar(lhs.c1, rhs.y)
        ),
        EmulatedDouble2Add(
            EmulatedDouble2MultiplyScalar(lhs.c2, rhs.z),
            EmulatedDouble2MultiplyScalar(lhs.c3, rhs.w)
        )
    );
}

/// Multiplies an `EmulatedDouble2` row vector by an `EmulatedDouble4x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble4 EmulatedDouble4x2LeftMultiplyVector(EmulatedDouble2 lhs, EmulatedDouble4x2 rhs) {
    return EmulatedDouble4Make(
        EmulatedDouble2Dot(lhs, rhs.c0),
        EmulatedDouble2Dot(lhs, rhs.c1),
        EmulatedDouble2Dot(lhs, rhs.c2),
        EmulatedDouble2Dot(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble4x3` matrix by an `EmulatedDouble4` vector.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble3 EmulatedDouble4x3MultiplyVector(EmulatedDouble4x3 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble3Add(
        EmulatedDouble3Add(
            EmulatedDouble3MultiplyScalar(lhs.c0, rhs.x),
            EmulatedDouble3MultiplyScalar(lhs.c1, rhs.y)
        ),
        EmulatedDouble3Add(
            EmulatedDouble3MultiplyScalar(lhs.c2, rhs.z),
            EmulatedDouble3MultiplyScalar(lhs.c3, rhs.w)
        )
    );
}

/// Multiplies an `EmulatedDouble3` row vector by an `EmulatedDouble4x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble4 EmulatedDouble4x3LeftMultiplyVector(EmulatedDouble3 lhs, EmulatedDouble4x3 rhs) {
    return EmulatedDouble4Make(
        EmulatedDouble3Dot(lhs, rhs.c0),
        EmulatedDouble3Dot(lhs, rhs.c1),
        EmulatedDouble3Dot(lhs, rhs.c2),
        EmulatedDouble3Dot(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble4x4` matrix by an `EmulatedDouble4` vector.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble4 EmulatedDouble4x4MultiplyVector(EmulatedDouble4x4 lhs, EmulatedDouble4 rhs) {
    return EmulatedDouble4Add(
        EmulatedDouble4Add(
            EmulatedDouble4MultiplyScalar(lhs.c0, rhs.x),
            EmulatedDouble4MultiplyScalar(lhs.c1, rhs.y)
        ),
        EmulatedDouble4Add(
            EmulatedDouble4MultiplyScalar(lhs.c2, rhs.z),
            EmulatedDouble4MultiplyScalar(lhs.c3, rhs.w)
        )
    );
}

/// Multiplies an `EmulatedDouble4` row vector by an `EmulatedDouble4x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules`.
static inline EmulatedDouble4 EmulatedDouble4x4LeftMultiplyVector(EmulatedDouble4 lhs, EmulatedDouble4x4 rhs) {
    return EmulatedDouble4Make(
        EmulatedDouble4Dot(lhs, rhs.c0),
        EmulatedDouble4Dot(lhs, rhs.c1),
        EmulatedDouble4Dot(lhs, rhs.c2),
        EmulatedDouble4Dot(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble2x2` matrix by an `EmulatedDouble2x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble2x2 EmulatedDouble2x2MultiplyEmulatedDouble2x2(EmulatedDouble2x2 lhs, EmulatedDouble2x2 rhs) {
    return EmulatedDouble2x2Make(
        EmulatedDouble2x2MultiplyVector(lhs, rhs.c0),
        EmulatedDouble2x2MultiplyVector(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble2x2` matrix by an `EmulatedDouble3x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble3x2 EmulatedDouble2x2MultiplyEmulatedDouble3x2(EmulatedDouble2x2 lhs, EmulatedDouble3x2 rhs) {
    return EmulatedDouble3x2Make(
        EmulatedDouble2x2MultiplyVector(lhs, rhs.c0),
        EmulatedDouble2x2MultiplyVector(lhs, rhs.c1),
        EmulatedDouble2x2MultiplyVector(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble2x2` matrix by an `EmulatedDouble4x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble4x2 EmulatedDouble2x2MultiplyEmulatedDouble4x2(EmulatedDouble2x2 lhs, EmulatedDouble4x2 rhs) {
    return EmulatedDouble4x2Make(
        EmulatedDouble2x2MultiplyVector(lhs, rhs.c0),
        EmulatedDouble2x2MultiplyVector(lhs, rhs.c1),
        EmulatedDouble2x2MultiplyVector(lhs, rhs.c2),
        EmulatedDouble2x2MultiplyVector(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble2x3` matrix by an `EmulatedDouble2x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble2x3 EmulatedDouble2x3MultiplyEmulatedDouble2x2(EmulatedDouble2x3 lhs, EmulatedDouble2x2 rhs) {
    return EmulatedDouble2x3Make(
        EmulatedDouble2x3MultiplyVector(lhs, rhs.c0),
        EmulatedDouble2x3MultiplyVector(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble2x3` matrix by an `EmulatedDouble3x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble3x3 EmulatedDouble2x3MultiplyEmulatedDouble3x2(EmulatedDouble2x3 lhs, EmulatedDouble3x2 rhs) {
    return EmulatedDouble3x3Make(
        EmulatedDouble2x3MultiplyVector(lhs, rhs.c0),
        EmulatedDouble2x3MultiplyVector(lhs, rhs.c1),
        EmulatedDouble2x3MultiplyVector(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble2x3` matrix by an `EmulatedDouble4x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble4x3 EmulatedDouble2x3MultiplyEmulatedDouble4x2(EmulatedDouble2x3 lhs, EmulatedDouble4x2 rhs) {
    return EmulatedDouble4x3Make(
        EmulatedDouble2x3MultiplyVector(lhs, rhs.c0),
        EmulatedDouble2x3MultiplyVector(lhs, rhs.c1),
        EmulatedDouble2x3MultiplyVector(lhs, rhs.c2),
        EmulatedDouble2x3MultiplyVector(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble2x4` matrix by an `EmulatedDouble2x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble2x4 EmulatedDouble2x4MultiplyEmulatedDouble2x2(EmulatedDouble2x4 lhs, EmulatedDouble2x2 rhs) {
    return EmulatedDouble2x4Make(
        EmulatedDouble2x4MultiplyVector(lhs, rhs.c0),
        EmulatedDouble2x4MultiplyVector(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble2x4` matrix by an `EmulatedDouble3x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble3x4 EmulatedDouble2x4MultiplyEmulatedDouble3x2(EmulatedDouble2x4 lhs, EmulatedDouble3x2 rhs) {
    return EmulatedDouble3x4Make(
        EmulatedDouble2x4MultiplyVector(lhs, rhs.c0),
        EmulatedDouble2x4MultiplyVector(lhs, rhs.c1),
        EmulatedDouble2x4MultiplyVector(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble2x4` matrix by an `EmulatedDouble4x2` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble4x4 EmulatedDouble2x4MultiplyEmulatedDouble4x2(EmulatedDouble2x4 lhs, EmulatedDouble4x2 rhs) {
    return EmulatedDouble4x4Make(
        EmulatedDouble2x4MultiplyVector(lhs, rhs.c0),
        EmulatedDouble2x4MultiplyVector(lhs, rhs.c1),
        EmulatedDouble2x4MultiplyVector(lhs, rhs.c2),
        EmulatedDouble2x4MultiplyVector(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble3x2` matrix by an `EmulatedDouble2x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble2x2 EmulatedDouble3x2MultiplyEmulatedDouble2x3(EmulatedDouble3x2 lhs, EmulatedDouble2x3 rhs) {
    return EmulatedDouble2x2Make(
        EmulatedDouble3x2MultiplyVector(lhs, rhs.c0),
        EmulatedDouble3x2MultiplyVector(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble3x2` matrix by an `EmulatedDouble3x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble3x2 EmulatedDouble3x2MultiplyEmulatedDouble3x3(EmulatedDouble3x2 lhs, EmulatedDouble3x3 rhs) {
    return EmulatedDouble3x2Make(
        EmulatedDouble3x2MultiplyVector(lhs, rhs.c0),
        EmulatedDouble3x2MultiplyVector(lhs, rhs.c1),
        EmulatedDouble3x2MultiplyVector(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble3x2` matrix by an `EmulatedDouble4x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble4x2 EmulatedDouble3x2MultiplyEmulatedDouble4x3(EmulatedDouble3x2 lhs, EmulatedDouble4x3 rhs) {
    return EmulatedDouble4x2Make(
        EmulatedDouble3x2MultiplyVector(lhs, rhs.c0),
        EmulatedDouble3x2MultiplyVector(lhs, rhs.c1),
        EmulatedDouble3x2MultiplyVector(lhs, rhs.c2),
        EmulatedDouble3x2MultiplyVector(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble3x3` matrix by an `EmulatedDouble2x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble2x3 EmulatedDouble3x3MultiplyEmulatedDouble2x3(EmulatedDouble3x3 lhs, EmulatedDouble2x3 rhs) {
    return EmulatedDouble2x3Make(
        EmulatedDouble3x3MultiplyVector(lhs, rhs.c0),
        EmulatedDouble3x3MultiplyVector(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble3x3` matrix by an `EmulatedDouble3x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble3x3 EmulatedDouble3x3MultiplyEmulatedDouble3x3(EmulatedDouble3x3 lhs, EmulatedDouble3x3 rhs) {
    return EmulatedDouble3x3Make(
        EmulatedDouble3x3MultiplyVector(lhs, rhs.c0),
        EmulatedDouble3x3MultiplyVector(lhs, rhs.c1),
        EmulatedDouble3x3MultiplyVector(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble3x3` matrix by an `EmulatedDouble4x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble4x3 EmulatedDouble3x3MultiplyEmulatedDouble4x3(EmulatedDouble3x3 lhs, EmulatedDouble4x3 rhs) {
    return EmulatedDouble4x3Make(
        EmulatedDouble3x3MultiplyVector(lhs, rhs.c0),
        EmulatedDouble3x3MultiplyVector(lhs, rhs.c1),
        EmulatedDouble3x3MultiplyVector(lhs, rhs.c2),
        EmulatedDouble3x3MultiplyVector(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble3x4` matrix by an `EmulatedDouble2x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble2x4 EmulatedDouble3x4MultiplyEmulatedDouble2x3(EmulatedDouble3x4 lhs, EmulatedDouble2x3 rhs) {
    return EmulatedDouble2x4Make(
        EmulatedDouble3x4MultiplyVector(lhs, rhs.c0),
        EmulatedDouble3x4MultiplyVector(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble3x4` matrix by an `EmulatedDouble3x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble3x4 EmulatedDouble3x4MultiplyEmulatedDouble3x3(EmulatedDouble3x4 lhs, EmulatedDouble3x3 rhs) {
    return EmulatedDouble3x4Make(
        EmulatedDouble3x4MultiplyVector(lhs, rhs.c0),
        EmulatedDouble3x4MultiplyVector(lhs, rhs.c1),
        EmulatedDouble3x4MultiplyVector(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble3x4` matrix by an `EmulatedDouble4x3` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble4x4 EmulatedDouble3x4MultiplyEmulatedDouble4x3(EmulatedDouble3x4 lhs, EmulatedDouble4x3 rhs) {
    return EmulatedDouble4x4Make(
        EmulatedDouble3x4MultiplyVector(lhs, rhs.c0),
        EmulatedDouble3x4MultiplyVector(lhs, rhs.c1),
        EmulatedDouble3x4MultiplyVector(lhs, rhs.c2),
        EmulatedDouble3x4MultiplyVector(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble4x2` matrix by an `EmulatedDouble2x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble2x2 EmulatedDouble4x2MultiplyEmulatedDouble2x4(EmulatedDouble4x2 lhs, EmulatedDouble2x4 rhs) {
    return EmulatedDouble2x2Make(
        EmulatedDouble4x2MultiplyVector(lhs, rhs.c0),
        EmulatedDouble4x2MultiplyVector(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble4x2` matrix by an `EmulatedDouble3x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble3x2 EmulatedDouble4x2MultiplyEmulatedDouble3x4(EmulatedDouble4x2 lhs, EmulatedDouble3x4 rhs) {
    return EmulatedDouble3x2Make(
        EmulatedDouble4x2MultiplyVector(lhs, rhs.c0),
        EmulatedDouble4x2MultiplyVector(lhs, rhs.c1),
        EmulatedDouble4x2MultiplyVector(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble4x2` matrix by an `EmulatedDouble4x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble4x2 EmulatedDouble4x2MultiplyEmulatedDouble4x4(EmulatedDouble4x2 lhs, EmulatedDouble4x4 rhs) {
    return EmulatedDouble4x2Make(
        EmulatedDouble4x2MultiplyVector(lhs, rhs.c0),
        EmulatedDouble4x2MultiplyVector(lhs, rhs.c1),
        EmulatedDouble4x2MultiplyVector(lhs, rhs.c2),
        EmulatedDouble4x2MultiplyVector(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble4x3` matrix by an `EmulatedDouble2x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble2x3 EmulatedDouble4x3MultiplyEmulatedDouble2x4(EmulatedDouble4x3 lhs, EmulatedDouble2x4 rhs) {
    return EmulatedDouble2x3Make(
        EmulatedDouble4x3MultiplyVector(lhs, rhs.c0),
        EmulatedDouble4x3MultiplyVector(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble4x3` matrix by an `EmulatedDouble3x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble3x3 EmulatedDouble4x3MultiplyEmulatedDouble3x4(EmulatedDouble4x3 lhs, EmulatedDouble3x4 rhs) {
    return EmulatedDouble3x3Make(
        EmulatedDouble4x3MultiplyVector(lhs, rhs.c0),
        EmulatedDouble4x3MultiplyVector(lhs, rhs.c1),
        EmulatedDouble4x3MultiplyVector(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble4x3` matrix by an `EmulatedDouble4x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble4x3 EmulatedDouble4x3MultiplyEmulatedDouble4x4(EmulatedDouble4x3 lhs, EmulatedDouble4x4 rhs) {
    return EmulatedDouble4x3Make(
        EmulatedDouble4x3MultiplyVector(lhs, rhs.c0),
        EmulatedDouble4x3MultiplyVector(lhs, rhs.c1),
        EmulatedDouble4x3MultiplyVector(lhs, rhs.c2),
        EmulatedDouble4x3MultiplyVector(lhs, rhs.c3)
    );
}

/// Multiplies an `EmulatedDouble4x4` matrix by an `EmulatedDouble2x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble2x4 EmulatedDouble4x4MultiplyEmulatedDouble2x4(EmulatedDouble4x4 lhs, EmulatedDouble2x4 rhs) {
    return EmulatedDouble2x4Make(
        EmulatedDouble4x4MultiplyVector(lhs, rhs.c0),
        EmulatedDouble4x4MultiplyVector(lhs, rhs.c1)
    );
}

/// Multiplies an `EmulatedDouble4x4` matrix by an `EmulatedDouble3x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble3x4 EmulatedDouble4x4MultiplyEmulatedDouble3x4(EmulatedDouble4x4 lhs, EmulatedDouble3x4 rhs) {
    return EmulatedDouble3x4Make(
        EmulatedDouble4x4MultiplyVector(lhs, rhs.c0),
        EmulatedDouble4x4MultiplyVector(lhs, rhs.c1),
        EmulatedDouble4x4MultiplyVector(lhs, rhs.c2)
    );
}

/// Multiplies an `EmulatedDouble4x4` matrix by an `EmulatedDouble4x4` matrix.
///
/// Quality: Covered by `emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes`.
static inline EmulatedDouble4x4 EmulatedDouble4x4MultiplyEmulatedDouble4x4(EmulatedDouble4x4 lhs, EmulatedDouble4x4 rhs) {
    return EmulatedDouble4x4Make(
        EmulatedDouble4x4MultiplyVector(lhs, rhs.c0),
        EmulatedDouble4x4MultiplyVector(lhs, rhs.c1),
        EmulatedDouble4x4MultiplyVector(lhs, rhs.c2),
        EmulatedDouble4x4MultiplyVector(lhs, rhs.c3)
    );
}

/// Computes the determinant of an `EmulatedDouble2x2`.
///
/// Quality: Covered by `emulatedDoubleMatrixDeterminantsMatchKnownValues`.
static inline EmulatedDouble EmulatedDouble2x2Determinant(EmulatedDouble2x2 value) {
    return EmulatedDoubleSubtract(
        EmulatedDoubleMultiply(value.c0.x, value.c1.y),
        EmulatedDoubleMultiply(value.c1.x, value.c0.y)
    );
}

/// Computes the determinant of an `EmulatedDouble3x3`.
///
/// Quality: Covered by `emulatedDoubleMatrixDeterminantsMatchKnownValues`.
static inline EmulatedDouble EmulatedDouble3x3Determinant(EmulatedDouble3x3 value) {
    EmulatedDouble a = value.c0.x;
    EmulatedDouble b = value.c1.x;
    EmulatedDouble c = value.c2.x;
    EmulatedDouble d = value.c0.y;
    EmulatedDouble e = value.c1.y;
    EmulatedDouble f = value.c2.y;
    EmulatedDouble g = value.c0.z;
    EmulatedDouble h = value.c1.z;
    EmulatedDouble i = value.c2.z;
    EmulatedDouble ei_fh = EmulatedDoubleSubtract(EmulatedDoubleMultiply(e, i), EmulatedDoubleMultiply(f, h));
    EmulatedDouble di_fg = EmulatedDoubleSubtract(EmulatedDoubleMultiply(d, i), EmulatedDoubleMultiply(f, g));
    EmulatedDouble dh_eg = EmulatedDoubleSubtract(EmulatedDoubleMultiply(d, h), EmulatedDoubleMultiply(e, g));
    return EmulatedDoubleAdd(
        EmulatedDoubleSubtract(EmulatedDoubleMultiply(a, ei_fh), EmulatedDoubleMultiply(b, di_fg)),
        EmulatedDoubleMultiply(c, dh_eg)
    );
}

/// Computes the determinant of an `EmulatedDouble4x4`.
///
/// Quality: Covered by `emulatedDoubleMatrixDeterminantsMatchKnownValues`.
static inline EmulatedDouble EmulatedDouble4x4Determinant(EmulatedDouble4x4 value) {
    EmulatedDouble a00 = value.c0.x;
    EmulatedDouble a01 = value.c1.x;
    EmulatedDouble a02 = value.c2.x;
    EmulatedDouble a03 = value.c3.x;
    EmulatedDouble a10 = value.c0.y;
    EmulatedDouble a11 = value.c1.y;
    EmulatedDouble a12 = value.c2.y;
    EmulatedDouble a13 = value.c3.y;
    EmulatedDouble a20 = value.c0.z;
    EmulatedDouble a21 = value.c1.z;
    EmulatedDouble a22 = value.c2.z;
    EmulatedDouble a23 = value.c3.z;
    EmulatedDouble a30 = value.c0.w;
    EmulatedDouble a31 = value.c1.w;
    EmulatedDouble a32 = value.c2.w;
    EmulatedDouble a33 = value.c3.w;

    EmulatedDouble m0 = EmulatedDouble3x3Determinant(EmulatedDouble3x3Make(
        EmulatedDouble3Make(a11, a21, a31),
        EmulatedDouble3Make(a12, a22, a32),
        EmulatedDouble3Make(a13, a23, a33)
    ));
    EmulatedDouble m1 = EmulatedDouble3x3Determinant(EmulatedDouble3x3Make(
        EmulatedDouble3Make(a10, a20, a30),
        EmulatedDouble3Make(a12, a22, a32),
        EmulatedDouble3Make(a13, a23, a33)
    ));
    EmulatedDouble m2 = EmulatedDouble3x3Determinant(EmulatedDouble3x3Make(
        EmulatedDouble3Make(a10, a20, a30),
        EmulatedDouble3Make(a11, a21, a31),
        EmulatedDouble3Make(a13, a23, a33)
    ));
    EmulatedDouble m3 = EmulatedDouble3x3Determinant(EmulatedDouble3x3Make(
        EmulatedDouble3Make(a10, a20, a30),
        EmulatedDouble3Make(a11, a21, a31),
        EmulatedDouble3Make(a12, a22, a32)
    ));

    return EmulatedDoubleSubtract(
        EmulatedDoubleAdd(EmulatedDoubleMultiply(a00, m0), EmulatedDoubleMultiply(a02, m2)),
        EmulatedDoubleAdd(EmulatedDoubleMultiply(a01, m1), EmulatedDoubleMultiply(a03, m3))
    );
}

/// Scratch storage used by the fixed-size one-sided Jacobi SVD implementation.
///
/// `a[column][row]` stores the working matrix columns, `v[column][row]` stores the accumulated
/// right singular vector matrix, and `s[column]` stores the singular values after convergence.
/// The public SVD functions keep this storage local and return typed matrix/vector results.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
typedef struct EmulatedDoubleJacobiSVDState {
    EmulatedDouble a[4][4];
    EmulatedDouble v[4][4];
    EmulatedDouble s[4];
} EmulatedDoubleJacobiSVDState;

/// Initializes Jacobi SVD scratch storage for a matrix with up to four rows and columns.
///
/// The working matrix and singular values are cleared. The leading `columns x columns` block of
/// `v` is initialized to identity so right singular vector rotations can be accumulated in place.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes` and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline void EmulatedDoubleJacobiSVDInitialize(
    EMULATED_DOUBLE_THREAD EmulatedDoubleJacobiSVDState *state,
    uint32_t rows,
    uint32_t columns
) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    EmulatedDouble one = EmulatedDoubleOne();

    for (uint32_t column = 0u; column < 4u; column += 1u) {
        state->s[column] = zero;
        for (uint32_t row = 0u; row < 4u; row += 1u) {
            state->a[column][row] = zero;
            state->v[column][row] = zero;
        }
    }

    for (uint32_t column = 0u; column < columns; column += 1u) {
        for (uint32_t row = 0u; row < rows; row += 1u) {
            state->a[column][row] = zero;
        }
        state->v[column][column] = one;
    }
}

/// Computes the dot product of two working matrix columns in Jacobi SVD scratch storage.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
static inline EmulatedDouble EmulatedDoubleJacobiSVDColumnDot(
    EMULATED_DOUBLE_THREAD EmulatedDoubleJacobiSVDState *state,
    uint32_t rows,
    uint32_t lhsColumn,
    uint32_t rhsColumn
) {
    EmulatedDouble sum = EmulatedDoubleZero(0u);
    for (uint32_t row = 0u; row < rows; row += 1u) {
        sum = EmulatedDoubleAdd(
            sum,
            EmulatedDoubleMultiply(state->a[lhsColumn][row], state->a[rhsColumn][row])
        );
    }
    return sum;
}

/// Applies a two-column Jacobi rotation to the working matrix and accumulated `v` matrix.
///
/// The rotation is the right-side update `A = A * J`, with the same column update applied to `v`.
/// It is used after `EmulatedDoubleJacobiSVDRotateColumnPair` computes the cosine and sine terms.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
static inline void EmulatedDoubleJacobiSVDApplyRotation(
    EMULATED_DOUBLE_THREAD EmulatedDoubleJacobiSVDState *state,
    uint32_t rows,
    uint32_t columns,
    uint32_t p,
    uint32_t q,
    EmulatedDouble cosine,
    EmulatedDouble sine
) {
    for (uint32_t row = 0u; row < rows; row += 1u) {
        EmulatedDouble oldP = state->a[p][row];
        EmulatedDouble oldQ = state->a[q][row];
        state->a[p][row] = EmulatedDoubleSubtract(
            EmulatedDoubleMultiply(cosine, oldP),
            EmulatedDoubleMultiply(sine, oldQ)
        );
        state->a[q][row] = EmulatedDoubleAdd(
            EmulatedDoubleMultiply(sine, oldP),
            EmulatedDoubleMultiply(cosine, oldQ)
        );
    }

    for (uint32_t row = 0u; row < columns; row += 1u) {
        EmulatedDouble oldP = state->v[p][row];
        EmulatedDouble oldQ = state->v[q][row];
        state->v[p][row] = EmulatedDoubleSubtract(
            EmulatedDoubleMultiply(cosine, oldP),
            EmulatedDoubleMultiply(sine, oldQ)
        );
        state->v[q][row] = EmulatedDoubleAdd(
            EmulatedDoubleMultiply(sine, oldP),
            EmulatedDoubleMultiply(cosine, oldQ)
        );
    }
}

/// Orthogonalizes one pair of working matrix columns using a stable Jacobi tangent formula.
///
/// The rotation diagonalizes the two-by-two Gram block formed by columns `p` and `q`. Pairs with
/// an already-zero cross term are skipped.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
static inline void EmulatedDoubleJacobiSVDRotateColumnPair(
    EMULATED_DOUBLE_THREAD EmulatedDoubleJacobiSVDState *state,
    uint32_t rows,
    uint32_t columns,
    uint32_t p,
    uint32_t q
) {
    EmulatedDouble alpha = EmulatedDoubleJacobiSVDColumnDot(state, rows, p, p);
    EmulatedDouble beta = EmulatedDoubleJacobiSVDColumnDot(state, rows, q, q);
    EmulatedDouble gamma = EmulatedDoubleJacobiSVDColumnDot(state, rows, p, q);

    if (EmulatedDoubleIsZero(gamma)) {
        return;
    }

    EmulatedDouble tau = EmulatedDoubleDivide(
        EmulatedDoubleSubtract(beta, alpha),
        EmulatedDoubleMultiply(EmulatedDoubleTwo(), gamma)
    );
    EmulatedDouble tangentMagnitude = EmulatedDoubleDivide(
        EmulatedDoubleOne(),
        EmulatedDoubleAdd(
            EmulatedDoubleAbs(tau),
            EmulatedDoubleSqrt(EmulatedDoubleAdd(EmulatedDoubleOne(), EmulatedDoubleMultiply(tau, tau)))
        )
    );
    EmulatedDouble tangent = EmulatedDoubleIsLessThanZero(tau) ?
        EmulatedDoubleNegated(tangentMagnitude) : tangentMagnitude;
    EmulatedDouble cosine = EmulatedDoubleDivide(
        EmulatedDoubleOne(),
        EmulatedDoubleSqrt(EmulatedDoubleAdd(EmulatedDoubleOne(), EmulatedDoubleMultiply(tangent, tangent)))
    );
    EmulatedDouble sine = EmulatedDoubleMultiply(cosine, tangent);

    EmulatedDoubleJacobiSVDApplyRotation(state, rows, columns, p, q, cosine, sine);
}

/// Computes singular values as the norms of the orthogonalized working matrix columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes` and
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`.
static inline void EmulatedDoubleJacobiSVDComputeSingularValues(
    EMULATED_DOUBLE_THREAD EmulatedDoubleJacobiSVDState *state,
    uint32_t rows,
    uint32_t columns
) {
    for (uint32_t column = 0u; column < columns; column += 1u) {
        state->s[column] = EmulatedDoubleSqrt(EmulatedDoubleJacobiSVDColumnDot(state, rows, column, column));
    }
}

/// Swaps two Jacobi SVD columns in the working matrix, singular value vector, and `v` matrix.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDSortsSingularValuesDescending`.
static inline void EmulatedDoubleJacobiSVDSwapColumns(
    EMULATED_DOUBLE_THREAD EmulatedDoubleJacobiSVDState *state,
    uint32_t rows,
    uint32_t columns,
    uint32_t lhsColumn,
    uint32_t rhsColumn
) {
    EmulatedDouble singularValue = state->s[lhsColumn];
    state->s[lhsColumn] = state->s[rhsColumn];
    state->s[rhsColumn] = singularValue;

    for (uint32_t row = 0u; row < rows; row += 1u) {
        EmulatedDouble value = state->a[lhsColumn][row];
        state->a[lhsColumn][row] = state->a[rhsColumn][row];
        state->a[rhsColumn][row] = value;
    }

    for (uint32_t row = 0u; row < columns; row += 1u) {
        EmulatedDouble value = state->v[lhsColumn][row];
        state->v[lhsColumn][row] = state->v[rhsColumn][row];
        state->v[rhsColumn][row] = value;
    }
}

/// Sorts singular values and their associated columns in descending order.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDSortsSingularValuesDescending`.
static inline void EmulatedDoubleJacobiSVDSortDescending(
    EMULATED_DOUBLE_THREAD EmulatedDoubleJacobiSVDState *state,
    uint32_t rows,
    uint32_t columns
) {
    for (uint32_t pass = 0u; pass < columns; pass += 1u) {
        for (uint32_t column = 0u; column + 1u < columns; column += 1u) {
            if (EmulatedDoubleIsLessThan(state->s[column], state->s[column + 1u])) {
                EmulatedDoubleJacobiSVDSwapColumns(state, rows, columns, column, column + 1u);
            }
        }
    }
}

/// Normalizes working matrix columns into left singular vector columns.
///
/// Columns with zero singular values are filled with positive zero. Reconstruction is unaffected
/// because those columns are multiplied by a zero singular value.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes` and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline void EmulatedDoubleJacobiSVDNormalizeColumns(
    EMULATED_DOUBLE_THREAD EmulatedDoubleJacobiSVDState *state,
    uint32_t rows,
    uint32_t columns
) {
    EmulatedDouble zero = EmulatedDoubleZero(0u);
    for (uint32_t column = 0u; column < columns; column += 1u) {
        if (EmulatedDoubleIsZero(state->s[column])) {
            for (uint32_t row = 0u; row < rows; row += 1u) {
                state->a[column][row] = zero;
            }
        } else {
            EmulatedDouble inverseSingularValue = EmulatedDoubleDivide(EmulatedDoubleOne(), state->s[column]);
            for (uint32_t row = 0u; row < rows; row += 1u) {
                state->a[column][row] = EmulatedDoubleMultiply(state->a[column][row], inverseSingularValue);
            }
        }
    }
}

/// Runs fixed-sweep one-sided Jacobi SVD on a matrix stored in scratch state.
///
/// Sixteen sweeps are used because the largest supported shape has four columns, making the fixed
/// upper bound small while avoiding convergence checks that would complicate Metal use. The result
/// leaves normalized left singular vector columns in `a`, descending singular values in `s`, and
/// right singular vectors in `v`.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline void EmulatedDoubleJacobiSVDRun(
    EMULATED_DOUBLE_THREAD EmulatedDoubleJacobiSVDState *state,
    uint32_t rows,
    uint32_t columns
) {
    for (uint32_t sweep = 0u; sweep < 16u; sweep += 1u) {
        for (uint32_t p = 0u; p < columns; p += 1u) {
            for (uint32_t q = p + 1u; q < columns; q += 1u) {
                EmulatedDoubleJacobiSVDRotateColumnPair(state, rows, columns, p, q);
            }
        }
    }

    EmulatedDoubleJacobiSVDComputeSingularValues(state, rows, columns);
    EmulatedDoubleJacobiSVDSortDescending(state, rows, columns);
    EmulatedDoubleJacobiSVDNormalizeColumns(state, rows, columns);
}

/// Result of one-sided Jacobi SVD for an `EmulatedDouble2x2` matrix.
///
/// `u` has the same shape as the input matrix, `s` contains one descending singular value per
/// input column, and `v` is a square right-singular-vector matrix. The original matrix is
/// reconstructed by `u * diag(s) * transpose(v)` using the first `2` singular columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
typedef struct EmulatedDouble2x2SVD {
    /// Left singular vector columns stored in the input matrix shape.
    EmulatedDouble2x2 u;

    /// Singular values sorted in descending order.
    EmulatedDouble2 s;

    /// Right singular vector columns.
    EmulatedDouble2x2 v;
} EmulatedDouble2x2SVD;

/// Result of one-sided Jacobi SVD for an `EmulatedDouble2x3` matrix.
///
/// `u` has the same shape as the input matrix, `s` contains one descending singular value per
/// input column, and `v` is a square right-singular-vector matrix. The original matrix is
/// reconstructed by `u * diag(s) * transpose(v)` using the first `2` singular columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
typedef struct EmulatedDouble2x3SVD {
    /// Left singular vector columns stored in the input matrix shape.
    EmulatedDouble2x3 u;

    /// Singular values sorted in descending order.
    EmulatedDouble2 s;

    /// Right singular vector columns.
    EmulatedDouble2x2 v;
} EmulatedDouble2x3SVD;

/// Result of one-sided Jacobi SVD for an `EmulatedDouble2x4` matrix.
///
/// `u` has the same shape as the input matrix, `s` contains one descending singular value per
/// input column, and `v` is a square right-singular-vector matrix. The original matrix is
/// reconstructed by `u * diag(s) * transpose(v)` using the first `2` singular columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
typedef struct EmulatedDouble2x4SVD {
    /// Left singular vector columns stored in the input matrix shape.
    EmulatedDouble2x4 u;

    /// Singular values sorted in descending order.
    EmulatedDouble2 s;

    /// Right singular vector columns.
    EmulatedDouble2x2 v;
} EmulatedDouble2x4SVD;

/// Result of one-sided Jacobi SVD for an `EmulatedDouble3x2` matrix.
///
/// `u` has the same shape as the input matrix, `s` contains one descending singular value per
/// input column, and `v` is a square right-singular-vector matrix. The original matrix is
/// reconstructed by `u * diag(s) * transpose(v)` using the first `3` singular columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
typedef struct EmulatedDouble3x2SVD {
    /// Left singular vector columns stored in the input matrix shape.
    EmulatedDouble3x2 u;

    /// Singular values sorted in descending order.
    EmulatedDouble3 s;

    /// Right singular vector columns.
    EmulatedDouble3x3 v;
} EmulatedDouble3x2SVD;

/// Result of one-sided Jacobi SVD for an `EmulatedDouble3x3` matrix.
///
/// `u` has the same shape as the input matrix, `s` contains one descending singular value per
/// input column, and `v` is a square right-singular-vector matrix. The original matrix is
/// reconstructed by `u * diag(s) * transpose(v)` using the first `3` singular columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
typedef struct EmulatedDouble3x3SVD {
    /// Left singular vector columns stored in the input matrix shape.
    EmulatedDouble3x3 u;

    /// Singular values sorted in descending order.
    EmulatedDouble3 s;

    /// Right singular vector columns.
    EmulatedDouble3x3 v;
} EmulatedDouble3x3SVD;

/// Result of one-sided Jacobi SVD for an `EmulatedDouble3x4` matrix.
///
/// `u` has the same shape as the input matrix, `s` contains one descending singular value per
/// input column, and `v` is a square right-singular-vector matrix. The original matrix is
/// reconstructed by `u * diag(s) * transpose(v)` using the first `3` singular columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
typedef struct EmulatedDouble3x4SVD {
    /// Left singular vector columns stored in the input matrix shape.
    EmulatedDouble3x4 u;

    /// Singular values sorted in descending order.
    EmulatedDouble3 s;

    /// Right singular vector columns.
    EmulatedDouble3x3 v;
} EmulatedDouble3x4SVD;

/// Result of one-sided Jacobi SVD for an `EmulatedDouble4x2` matrix.
///
/// `u` has the same shape as the input matrix, `s` contains one descending singular value per
/// input column, and `v` is a square right-singular-vector matrix. The original matrix is
/// reconstructed by `u * diag(s) * transpose(v)` using the first `4` singular columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
typedef struct EmulatedDouble4x2SVD {
    /// Left singular vector columns stored in the input matrix shape.
    EmulatedDouble4x2 u;

    /// Singular values sorted in descending order.
    EmulatedDouble4 s;

    /// Right singular vector columns.
    EmulatedDouble4x4 v;
} EmulatedDouble4x2SVD;

/// Result of one-sided Jacobi SVD for an `EmulatedDouble4x3` matrix.
///
/// `u` has the same shape as the input matrix, `s` contains one descending singular value per
/// input column, and `v` is a square right-singular-vector matrix. The original matrix is
/// reconstructed by `u * diag(s) * transpose(v)` using the first `4` singular columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
typedef struct EmulatedDouble4x3SVD {
    /// Left singular vector columns stored in the input matrix shape.
    EmulatedDouble4x3 u;

    /// Singular values sorted in descending order.
    EmulatedDouble4 s;

    /// Right singular vector columns.
    EmulatedDouble4x4 v;
} EmulatedDouble4x3SVD;

/// Result of one-sided Jacobi SVD for an `EmulatedDouble4x4` matrix.
///
/// `u` has the same shape as the input matrix, `s` contains one descending singular value per
/// input column, and `v` is a square right-singular-vector matrix. The original matrix is
/// reconstructed by `u * diag(s) * transpose(v)` using the first `4` singular columns.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`.
typedef struct EmulatedDouble4x4SVD {
    /// Left singular vector columns stored in the input matrix shape.
    EmulatedDouble4x4 u;

    /// Singular values sorted in descending order.
    EmulatedDouble4 s;

    /// Right singular vector columns.
    EmulatedDouble4x4 v;
} EmulatedDouble4x4SVD;

/// Computes a fixed-sweep one-sided Jacobi SVD of an `EmulatedDouble2x2` matrix.
///
/// The factorization returns singular values in descending order and satisfies
/// `value ~= result.u * diag(result.s) * transpose(result.v)` for finite inputs. The implementation
/// uses only `EmulatedDouble` arithmetic and does not call native `float`, native `double`, host
/// `math.h`, or Metal standard math functions for result computation.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline EmulatedDouble2x2SVD EmulatedDouble2x2JacobiSVD(EmulatedDouble2x2 value) {
    EmulatedDoubleJacobiSVDState state;
    EmulatedDoubleJacobiSVDInitialize(&state, 2u, 2u);
    state.a[0][0] = value.c0.x;
    state.a[0][1] = value.c0.y;
    state.a[1][0] = value.c1.x;
    state.a[1][1] = value.c1.y;
    EmulatedDoubleJacobiSVDRun(&state, 2u, 2u);

    EmulatedDouble2 u0 = EmulatedDouble2Make(
        state.a[0][0],
        state.a[0][1]
    );
    EmulatedDouble2 u1 = EmulatedDouble2Make(
        state.a[1][0],
        state.a[1][1]
    );

    EmulatedDouble2 v0 = EmulatedDouble2Make(
        state.v[0][0],
        state.v[0][1]
    );
    EmulatedDouble2 v1 = EmulatedDouble2Make(
        state.v[1][0],
        state.v[1][1]
    );

    EmulatedDouble2x2SVD result;
    result.u = EmulatedDouble2x2Make(
        u0,
        u1
    );
    result.s = EmulatedDouble2Make(
        state.s[0],
        state.s[1]
    );
    result.v = EmulatedDouble2x2Make(
        v0,
        v1
    );
    return result;
}

/// Computes a fixed-sweep one-sided Jacobi SVD of an `EmulatedDouble2x3` matrix.
///
/// The factorization returns singular values in descending order and satisfies
/// `value ~= result.u * diag(result.s) * transpose(result.v)` for finite inputs. The implementation
/// uses only `EmulatedDouble` arithmetic and does not call native `float`, native `double`, host
/// `math.h`, or Metal standard math functions for result computation.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline EmulatedDouble2x3SVD EmulatedDouble2x3JacobiSVD(EmulatedDouble2x3 value) {
    EmulatedDoubleJacobiSVDState state;
    EmulatedDoubleJacobiSVDInitialize(&state, 3u, 2u);
    state.a[0][0] = value.c0.x;
    state.a[0][1] = value.c0.y;
    state.a[0][2] = value.c0.z;
    state.a[1][0] = value.c1.x;
    state.a[1][1] = value.c1.y;
    state.a[1][2] = value.c1.z;
    EmulatedDoubleJacobiSVDRun(&state, 3u, 2u);

    EmulatedDouble3 u0 = EmulatedDouble3Make(
        state.a[0][0],
        state.a[0][1],
        state.a[0][2]
    );
    EmulatedDouble3 u1 = EmulatedDouble3Make(
        state.a[1][0],
        state.a[1][1],
        state.a[1][2]
    );

    EmulatedDouble2 v0 = EmulatedDouble2Make(
        state.v[0][0],
        state.v[0][1]
    );
    EmulatedDouble2 v1 = EmulatedDouble2Make(
        state.v[1][0],
        state.v[1][1]
    );

    EmulatedDouble2x3SVD result;
    result.u = EmulatedDouble2x3Make(
        u0,
        u1
    );
    result.s = EmulatedDouble2Make(
        state.s[0],
        state.s[1]
    );
    result.v = EmulatedDouble2x2Make(
        v0,
        v1
    );
    return result;
}

/// Computes a fixed-sweep one-sided Jacobi SVD of an `EmulatedDouble2x4` matrix.
///
/// The factorization returns singular values in descending order and satisfies
/// `value ~= result.u * diag(result.s) * transpose(result.v)` for finite inputs. The implementation
/// uses only `EmulatedDouble` arithmetic and does not call native `float`, native `double`, host
/// `math.h`, or Metal standard math functions for result computation.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline EmulatedDouble2x4SVD EmulatedDouble2x4JacobiSVD(EmulatedDouble2x4 value) {
    EmulatedDoubleJacobiSVDState state;
    EmulatedDoubleJacobiSVDInitialize(&state, 4u, 2u);
    state.a[0][0] = value.c0.x;
    state.a[0][1] = value.c0.y;
    state.a[0][2] = value.c0.z;
    state.a[0][3] = value.c0.w;
    state.a[1][0] = value.c1.x;
    state.a[1][1] = value.c1.y;
    state.a[1][2] = value.c1.z;
    state.a[1][3] = value.c1.w;
    EmulatedDoubleJacobiSVDRun(&state, 4u, 2u);

    EmulatedDouble4 u0 = EmulatedDouble4Make(
        state.a[0][0],
        state.a[0][1],
        state.a[0][2],
        state.a[0][3]
    );
    EmulatedDouble4 u1 = EmulatedDouble4Make(
        state.a[1][0],
        state.a[1][1],
        state.a[1][2],
        state.a[1][3]
    );

    EmulatedDouble2 v0 = EmulatedDouble2Make(
        state.v[0][0],
        state.v[0][1]
    );
    EmulatedDouble2 v1 = EmulatedDouble2Make(
        state.v[1][0],
        state.v[1][1]
    );

    EmulatedDouble2x4SVD result;
    result.u = EmulatedDouble2x4Make(
        u0,
        u1
    );
    result.s = EmulatedDouble2Make(
        state.s[0],
        state.s[1]
    );
    result.v = EmulatedDouble2x2Make(
        v0,
        v1
    );
    return result;
}

/// Computes a fixed-sweep one-sided Jacobi SVD of an `EmulatedDouble3x2` matrix.
///
/// The factorization returns singular values in descending order and satisfies
/// `value ~= result.u * diag(result.s) * transpose(result.v)` for finite inputs. The implementation
/// uses only `EmulatedDouble` arithmetic and does not call native `float`, native `double`, host
/// `math.h`, or Metal standard math functions for result computation.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline EmulatedDouble3x2SVD EmulatedDouble3x2JacobiSVD(EmulatedDouble3x2 value) {
    EmulatedDoubleJacobiSVDState state;
    EmulatedDoubleJacobiSVDInitialize(&state, 2u, 3u);
    state.a[0][0] = value.c0.x;
    state.a[0][1] = value.c0.y;
    state.a[1][0] = value.c1.x;
    state.a[1][1] = value.c1.y;
    state.a[2][0] = value.c2.x;
    state.a[2][1] = value.c2.y;
    EmulatedDoubleJacobiSVDRun(&state, 2u, 3u);

    EmulatedDouble2 u0 = EmulatedDouble2Make(
        state.a[0][0],
        state.a[0][1]
    );
    EmulatedDouble2 u1 = EmulatedDouble2Make(
        state.a[1][0],
        state.a[1][1]
    );
    EmulatedDouble2 u2 = EmulatedDouble2Make(
        state.a[2][0],
        state.a[2][1]
    );

    EmulatedDouble3 v0 = EmulatedDouble3Make(
        state.v[0][0],
        state.v[0][1],
        state.v[0][2]
    );
    EmulatedDouble3 v1 = EmulatedDouble3Make(
        state.v[1][0],
        state.v[1][1],
        state.v[1][2]
    );
    EmulatedDouble3 v2 = EmulatedDouble3Make(
        state.v[2][0],
        state.v[2][1],
        state.v[2][2]
    );

    EmulatedDouble3x2SVD result;
    result.u = EmulatedDouble3x2Make(
        u0,
        u1,
        u2
    );
    result.s = EmulatedDouble3Make(
        state.s[0],
        state.s[1],
        state.s[2]
    );
    result.v = EmulatedDouble3x3Make(
        v0,
        v1,
        v2
    );
    return result;
}

/// Computes a fixed-sweep one-sided Jacobi SVD of an `EmulatedDouble3x3` matrix.
///
/// The factorization returns singular values in descending order and satisfies
/// `value ~= result.u * diag(result.s) * transpose(result.v)` for finite inputs. The implementation
/// uses only `EmulatedDouble` arithmetic and does not call native `float`, native `double`, host
/// `math.h`, or Metal standard math functions for result computation.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline EmulatedDouble3x3SVD EmulatedDouble3x3JacobiSVD(EmulatedDouble3x3 value) {
    EmulatedDoubleJacobiSVDState state;
    EmulatedDoubleJacobiSVDInitialize(&state, 3u, 3u);
    state.a[0][0] = value.c0.x;
    state.a[0][1] = value.c0.y;
    state.a[0][2] = value.c0.z;
    state.a[1][0] = value.c1.x;
    state.a[1][1] = value.c1.y;
    state.a[1][2] = value.c1.z;
    state.a[2][0] = value.c2.x;
    state.a[2][1] = value.c2.y;
    state.a[2][2] = value.c2.z;
    EmulatedDoubleJacobiSVDRun(&state, 3u, 3u);

    EmulatedDouble3 u0 = EmulatedDouble3Make(
        state.a[0][0],
        state.a[0][1],
        state.a[0][2]
    );
    EmulatedDouble3 u1 = EmulatedDouble3Make(
        state.a[1][0],
        state.a[1][1],
        state.a[1][2]
    );
    EmulatedDouble3 u2 = EmulatedDouble3Make(
        state.a[2][0],
        state.a[2][1],
        state.a[2][2]
    );

    EmulatedDouble3 v0 = EmulatedDouble3Make(
        state.v[0][0],
        state.v[0][1],
        state.v[0][2]
    );
    EmulatedDouble3 v1 = EmulatedDouble3Make(
        state.v[1][0],
        state.v[1][1],
        state.v[1][2]
    );
    EmulatedDouble3 v2 = EmulatedDouble3Make(
        state.v[2][0],
        state.v[2][1],
        state.v[2][2]
    );

    EmulatedDouble3x3SVD result;
    result.u = EmulatedDouble3x3Make(
        u0,
        u1,
        u2
    );
    result.s = EmulatedDouble3Make(
        state.s[0],
        state.s[1],
        state.s[2]
    );
    result.v = EmulatedDouble3x3Make(
        v0,
        v1,
        v2
    );
    return result;
}

/// Computes a fixed-sweep one-sided Jacobi SVD of an `EmulatedDouble3x4` matrix.
///
/// The factorization returns singular values in descending order and satisfies
/// `value ~= result.u * diag(result.s) * transpose(result.v)` for finite inputs. The implementation
/// uses only `EmulatedDouble` arithmetic and does not call native `float`, native `double`, host
/// `math.h`, or Metal standard math functions for result computation.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline EmulatedDouble3x4SVD EmulatedDouble3x4JacobiSVD(EmulatedDouble3x4 value) {
    EmulatedDoubleJacobiSVDState state;
    EmulatedDoubleJacobiSVDInitialize(&state, 4u, 3u);
    state.a[0][0] = value.c0.x;
    state.a[0][1] = value.c0.y;
    state.a[0][2] = value.c0.z;
    state.a[0][3] = value.c0.w;
    state.a[1][0] = value.c1.x;
    state.a[1][1] = value.c1.y;
    state.a[1][2] = value.c1.z;
    state.a[1][3] = value.c1.w;
    state.a[2][0] = value.c2.x;
    state.a[2][1] = value.c2.y;
    state.a[2][2] = value.c2.z;
    state.a[2][3] = value.c2.w;
    EmulatedDoubleJacobiSVDRun(&state, 4u, 3u);

    EmulatedDouble4 u0 = EmulatedDouble4Make(
        state.a[0][0],
        state.a[0][1],
        state.a[0][2],
        state.a[0][3]
    );
    EmulatedDouble4 u1 = EmulatedDouble4Make(
        state.a[1][0],
        state.a[1][1],
        state.a[1][2],
        state.a[1][3]
    );
    EmulatedDouble4 u2 = EmulatedDouble4Make(
        state.a[2][0],
        state.a[2][1],
        state.a[2][2],
        state.a[2][3]
    );

    EmulatedDouble3 v0 = EmulatedDouble3Make(
        state.v[0][0],
        state.v[0][1],
        state.v[0][2]
    );
    EmulatedDouble3 v1 = EmulatedDouble3Make(
        state.v[1][0],
        state.v[1][1],
        state.v[1][2]
    );
    EmulatedDouble3 v2 = EmulatedDouble3Make(
        state.v[2][0],
        state.v[2][1],
        state.v[2][2]
    );

    EmulatedDouble3x4SVD result;
    result.u = EmulatedDouble3x4Make(
        u0,
        u1,
        u2
    );
    result.s = EmulatedDouble3Make(
        state.s[0],
        state.s[1],
        state.s[2]
    );
    result.v = EmulatedDouble3x3Make(
        v0,
        v1,
        v2
    );
    return result;
}

/// Computes a fixed-sweep one-sided Jacobi SVD of an `EmulatedDouble4x2` matrix.
///
/// The factorization returns singular values in descending order and satisfies
/// `value ~= result.u * diag(result.s) * transpose(result.v)` for finite inputs. The implementation
/// uses only `EmulatedDouble` arithmetic and does not call native `float`, native `double`, host
/// `math.h`, or Metal standard math functions for result computation.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline EmulatedDouble4x2SVD EmulatedDouble4x2JacobiSVD(EmulatedDouble4x2 value) {
    EmulatedDoubleJacobiSVDState state;
    EmulatedDoubleJacobiSVDInitialize(&state, 2u, 4u);
    state.a[0][0] = value.c0.x;
    state.a[0][1] = value.c0.y;
    state.a[1][0] = value.c1.x;
    state.a[1][1] = value.c1.y;
    state.a[2][0] = value.c2.x;
    state.a[2][1] = value.c2.y;
    state.a[3][0] = value.c3.x;
    state.a[3][1] = value.c3.y;
    EmulatedDoubleJacobiSVDRun(&state, 2u, 4u);

    EmulatedDouble2 u0 = EmulatedDouble2Make(
        state.a[0][0],
        state.a[0][1]
    );
    EmulatedDouble2 u1 = EmulatedDouble2Make(
        state.a[1][0],
        state.a[1][1]
    );
    EmulatedDouble2 u2 = EmulatedDouble2Make(
        state.a[2][0],
        state.a[2][1]
    );
    EmulatedDouble2 u3 = EmulatedDouble2Make(
        state.a[3][0],
        state.a[3][1]
    );

    EmulatedDouble4 v0 = EmulatedDouble4Make(
        state.v[0][0],
        state.v[0][1],
        state.v[0][2],
        state.v[0][3]
    );
    EmulatedDouble4 v1 = EmulatedDouble4Make(
        state.v[1][0],
        state.v[1][1],
        state.v[1][2],
        state.v[1][3]
    );
    EmulatedDouble4 v2 = EmulatedDouble4Make(
        state.v[2][0],
        state.v[2][1],
        state.v[2][2],
        state.v[2][3]
    );
    EmulatedDouble4 v3 = EmulatedDouble4Make(
        state.v[3][0],
        state.v[3][1],
        state.v[3][2],
        state.v[3][3]
    );

    EmulatedDouble4x2SVD result;
    result.u = EmulatedDouble4x2Make(
        u0,
        u1,
        u2,
        u3
    );
    result.s = EmulatedDouble4Make(
        state.s[0],
        state.s[1],
        state.s[2],
        state.s[3]
    );
    result.v = EmulatedDouble4x4Make(
        v0,
        v1,
        v2,
        v3
    );
    return result;
}

/// Computes a fixed-sweep one-sided Jacobi SVD of an `EmulatedDouble4x3` matrix.
///
/// The factorization returns singular values in descending order and satisfies
/// `value ~= result.u * diag(result.s) * transpose(result.v)` for finite inputs. The implementation
/// uses only `EmulatedDouble` arithmetic and does not call native `float`, native `double`, host
/// `math.h`, or Metal standard math functions for result computation.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline EmulatedDouble4x3SVD EmulatedDouble4x3JacobiSVD(EmulatedDouble4x3 value) {
    EmulatedDoubleJacobiSVDState state;
    EmulatedDoubleJacobiSVDInitialize(&state, 3u, 4u);
    state.a[0][0] = value.c0.x;
    state.a[0][1] = value.c0.y;
    state.a[0][2] = value.c0.z;
    state.a[1][0] = value.c1.x;
    state.a[1][1] = value.c1.y;
    state.a[1][2] = value.c1.z;
    state.a[2][0] = value.c2.x;
    state.a[2][1] = value.c2.y;
    state.a[2][2] = value.c2.z;
    state.a[3][0] = value.c3.x;
    state.a[3][1] = value.c3.y;
    state.a[3][2] = value.c3.z;
    EmulatedDoubleJacobiSVDRun(&state, 3u, 4u);

    EmulatedDouble3 u0 = EmulatedDouble3Make(
        state.a[0][0],
        state.a[0][1],
        state.a[0][2]
    );
    EmulatedDouble3 u1 = EmulatedDouble3Make(
        state.a[1][0],
        state.a[1][1],
        state.a[1][2]
    );
    EmulatedDouble3 u2 = EmulatedDouble3Make(
        state.a[2][0],
        state.a[2][1],
        state.a[2][2]
    );
    EmulatedDouble3 u3 = EmulatedDouble3Make(
        state.a[3][0],
        state.a[3][1],
        state.a[3][2]
    );

    EmulatedDouble4 v0 = EmulatedDouble4Make(
        state.v[0][0],
        state.v[0][1],
        state.v[0][2],
        state.v[0][3]
    );
    EmulatedDouble4 v1 = EmulatedDouble4Make(
        state.v[1][0],
        state.v[1][1],
        state.v[1][2],
        state.v[1][3]
    );
    EmulatedDouble4 v2 = EmulatedDouble4Make(
        state.v[2][0],
        state.v[2][1],
        state.v[2][2],
        state.v[2][3]
    );
    EmulatedDouble4 v3 = EmulatedDouble4Make(
        state.v[3][0],
        state.v[3][1],
        state.v[3][2],
        state.v[3][3]
    );

    EmulatedDouble4x3SVD result;
    result.u = EmulatedDouble4x3Make(
        u0,
        u1,
        u2,
        u3
    );
    result.s = EmulatedDouble4Make(
        state.s[0],
        state.s[1],
        state.s[2],
        state.s[3]
    );
    result.v = EmulatedDouble4x4Make(
        v0,
        v1,
        v2,
        v3
    );
    return result;
}

/// Computes a fixed-sweep one-sided Jacobi SVD of an `EmulatedDouble4x4` matrix.
///
/// The factorization returns singular values in descending order and satisfies
/// `value ~= result.u * diag(result.s) * transpose(result.v)` for finite inputs. The implementation
/// uses only `EmulatedDouble` arithmetic and does not call native `float`, native `double`, host
/// `math.h`, or Metal standard math functions for result computation.
///
/// Quality: Covered by `emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes`,
/// `emulatedDoubleJacobiSVDSortsSingularValuesDescending`, and
/// `emulatedDoubleJacobiSVDHandlesZeroMatrix`.
static inline EmulatedDouble4x4SVD EmulatedDouble4x4JacobiSVD(EmulatedDouble4x4 value) {
    EmulatedDoubleJacobiSVDState state;
    EmulatedDoubleJacobiSVDInitialize(&state, 4u, 4u);
    state.a[0][0] = value.c0.x;
    state.a[0][1] = value.c0.y;
    state.a[0][2] = value.c0.z;
    state.a[0][3] = value.c0.w;
    state.a[1][0] = value.c1.x;
    state.a[1][1] = value.c1.y;
    state.a[1][2] = value.c1.z;
    state.a[1][3] = value.c1.w;
    state.a[2][0] = value.c2.x;
    state.a[2][1] = value.c2.y;
    state.a[2][2] = value.c2.z;
    state.a[2][3] = value.c2.w;
    state.a[3][0] = value.c3.x;
    state.a[3][1] = value.c3.y;
    state.a[3][2] = value.c3.z;
    state.a[3][3] = value.c3.w;
    EmulatedDoubleJacobiSVDRun(&state, 4u, 4u);

    EmulatedDouble4 u0 = EmulatedDouble4Make(
        state.a[0][0],
        state.a[0][1],
        state.a[0][2],
        state.a[0][3]
    );
    EmulatedDouble4 u1 = EmulatedDouble4Make(
        state.a[1][0],
        state.a[1][1],
        state.a[1][2],
        state.a[1][3]
    );
    EmulatedDouble4 u2 = EmulatedDouble4Make(
        state.a[2][0],
        state.a[2][1],
        state.a[2][2],
        state.a[2][3]
    );
    EmulatedDouble4 u3 = EmulatedDouble4Make(
        state.a[3][0],
        state.a[3][1],
        state.a[3][2],
        state.a[3][3]
    );

    EmulatedDouble4 v0 = EmulatedDouble4Make(
        state.v[0][0],
        state.v[0][1],
        state.v[0][2],
        state.v[0][3]
    );
    EmulatedDouble4 v1 = EmulatedDouble4Make(
        state.v[1][0],
        state.v[1][1],
        state.v[1][2],
        state.v[1][3]
    );
    EmulatedDouble4 v2 = EmulatedDouble4Make(
        state.v[2][0],
        state.v[2][1],
        state.v[2][2],
        state.v[2][3]
    );
    EmulatedDouble4 v3 = EmulatedDouble4Make(
        state.v[3][0],
        state.v[3][1],
        state.v[3][2],
        state.v[3][3]
    );

    EmulatedDouble4x4SVD result;
    result.u = EmulatedDouble4x4Make(
        u0,
        u1,
        u2,
        u3
    );
    result.s = EmulatedDouble4Make(
        state.s[0],
        state.s[1],
        state.s[2],
        state.s[3]
    );
    result.v = EmulatedDouble4x4Make(
        v0,
        v1,
        v2,
        v3
    );
    return result;
}
#endif
