import Testing
@testable import EmulatedDouble

@Test func emulatedDoubleMakePreservesWordsAndAccessors() {
    let value = EmulatedDoubleMake(0xbff8_0001, 0x2345_6789)

    #expect(value.highBits == 0xbff8_0001)
    #expect(value.lowBits == 0x2345_6789)
    #expect(EmulatedDoubleSignBit(value) == 1)
    #expect(EmulatedDoubleExponentBits(value) == 0x3ff)
    #expect(EmulatedDoubleFractionHighBits(value) == 0x0008_0001)
    #expect(EmulatedDoubleFractionLowBits(value) == 0x2345_6789)
    #expect(EmulatedDoubleHasZeroFraction(value) == 0)
}

@Test func emulatedDoubleClassifiesBinary64SpecialValues() {
    let cases: [ClassificationCase] = [
        .init(high: 0x0000_0000, low: 0x0000_0000, sign: 0, exponent: 0x000, hasZeroFraction: 1, isZero: 1, isSubnormal: 0, isInfinity: 0, isNaN: 0),
        .init(high: 0x8000_0000, low: 0x0000_0000, sign: 1, exponent: 0x000, hasZeroFraction: 1, isZero: 1, isSubnormal: 0, isInfinity: 0, isNaN: 0),
        .init(high: 0x0000_0000, low: 0x0000_0001, sign: 0, exponent: 0x000, hasZeroFraction: 0, isZero: 0, isSubnormal: 1, isInfinity: 0, isNaN: 0),
        .init(high: 0x000f_ffff, low: 0xffff_ffff, sign: 0, exponent: 0x000, hasZeroFraction: 0, isZero: 0, isSubnormal: 1, isInfinity: 0, isNaN: 0),
        .init(high: 0x0010_0000, low: 0x0000_0000, sign: 0, exponent: 0x001, hasZeroFraction: 1, isZero: 0, isSubnormal: 0, isInfinity: 0, isNaN: 0),
        .init(high: 0x3ff0_0000, low: 0x0000_0000, sign: 0, exponent: 0x3ff, hasZeroFraction: 1, isZero: 0, isSubnormal: 0, isInfinity: 0, isNaN: 0),
        .init(high: 0x7ff0_0000, low: 0x0000_0000, sign: 0, exponent: 0x7ff, hasZeroFraction: 1, isZero: 0, isSubnormal: 0, isInfinity: 1, isNaN: 0),
        .init(high: 0xfff0_0000, low: 0x0000_0000, sign: 1, exponent: 0x7ff, hasZeroFraction: 1, isZero: 0, isSubnormal: 0, isInfinity: 1, isNaN: 0),
        .init(high: 0x7ff8_0000, low: 0x0000_0000, sign: 0, exponent: 0x7ff, hasZeroFraction: 0, isZero: 0, isSubnormal: 0, isInfinity: 0, isNaN: 1),
        .init(high: 0x7ff0_0000, low: 0x0000_0001, sign: 0, exponent: 0x7ff, hasZeroFraction: 0, isZero: 0, isSubnormal: 0, isInfinity: 0, isNaN: 1),
    ]

    for testCase in cases {
        let value = EmulatedDoubleMake(testCase.high, testCase.low)
        #expect(EmulatedDoubleSignBit(value) == testCase.sign)
        #expect(EmulatedDoubleExponentBits(value) == testCase.exponent)
        #expect(EmulatedDoubleHasZeroFraction(value) == testCase.hasZeroFraction)
        #expect(EmulatedDoubleIsZero(value) == testCase.isZero)
        #expect(EmulatedDoubleIsSubnormal(value) == testCase.isSubnormal)
        #expect(EmulatedDoubleIsInfinity(value) == testCase.isInfinity)
        #expect(EmulatedDoubleIsNaN(value) == testCase.isNaN)
    }
}

@Test func emulatedDoubleCountsLeadingZeros() {
    let cases: [(value: UInt32, expected: UInt32)] = [
        (0x0000_0000, 32),
        (0x0000_0001, 31),
        (0x0000_0002, 30),
        (0x0000_8000, 16),
        (0x00f0_0000, 8),
        (0x4000_0000, 1),
        (0x8000_0000, 0),
        (0xffff_ffff, 0),
    ]

    for testCase in cases {
        #expect(EmulatedDoubleCountLeadingZeros32(testCase.value) == testCase.expected)
    }
}

@Test func emulatedDoubleFromFloatBitsPreservesRepresentativeValues() {
    let cases: [WideningCase] = [
        .init(floatBits: 0x0000_0000, high: 0x0000_0000, low: 0x0000_0000),
        .init(floatBits: 0x8000_0000, high: 0x8000_0000, low: 0x0000_0000),
        .init(floatBits: 0x3f80_0000, high: 0x3ff0_0000, low: 0x0000_0000),
        .init(floatBits: 0xbf80_0000, high: 0xbff0_0000, low: 0x0000_0000),
        .init(floatBits: 0x3fc0_0000, high: 0x3ff8_0000, low: 0x0000_0000),
        .init(floatBits: 0xc020_0000, high: 0xc004_0000, low: 0x0000_0000),
        .init(floatBits: 0x0080_0000, high: 0x3810_0000, low: 0x0000_0000),
        .init(floatBits: 0x007f_ffff, high: 0x380f_ffff, low: 0xc000_0000),
        .init(floatBits: 0x0000_0001, high: 0x36a0_0000, low: 0x0000_0000),
        .init(floatBits: 0x7f7f_ffff, high: 0x47ef_ffff, low: 0xe000_0000),
        .init(floatBits: 0xff7f_ffff, high: 0xc7ef_ffff, low: 0xe000_0000),
        .init(floatBits: 0x7f80_0000, high: 0x7ff0_0000, low: 0x0000_0000),
        .init(floatBits: 0xff80_0000, high: 0xfff0_0000, low: 0x0000_0000),
        .init(floatBits: 0x7fc0_0000, high: 0x7ff8_0000, low: 0x0000_0000),
        .init(floatBits: 0x7fa1_2345, high: 0x7ff4_2468, low: 0xa000_0000),
        .init(floatBits: 0xffc1_2345, high: 0xfff8_2468, low: 0xa000_0000),
    ]

    for testCase in cases {
        expectEmulatedDouble(
            EmulatedDoubleFromFloatBits(testCase.floatBits),
            high: testCase.high,
            low: testCase.low
        )
    }
}

@Test func emulatedDoubleToFloatBitsRoundsToNearestEven() {
    #expect(EmulatedDoubleToFloatBits(EmulatedDoubleMake(0x3ff0_0000, 0x1000_0000)) == 0x3f80_0000)
    #expect(EmulatedDoubleToFloatBits(EmulatedDoubleMake(0x3ff0_0000, 0x1000_0001)) == 0x3f80_0001)
    #expect(EmulatedDoubleToFloatBits(EmulatedDoubleMake(0x3ff0_0000, 0x3000_0000)) == 0x3f80_0002)
    #expect(EmulatedDoubleToFloatBits(EmulatedDoubleMake(0x36a0_0000, 0x0000_0000)) == 0x0000_0001)
    #expect(EmulatedDoubleToFloatBits(EmulatedDoubleMake(0x3690_0000, 0x0000_0000)) == 0x0000_0000)
    #expect(EmulatedDoubleToFloatBits(EmulatedDoubleMake(0x3690_0000, 0x0000_0001)) == 0x0000_0001)
}

@Test func emulatedDoubleRoundShiftRight53ToUint32RoundsAcrossShiftRanges() {
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0000_0000, 0x0000_0001, 1) == 0)
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0000_0000, 0x0000_0002, 1) == 1)
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0000_0000, 0x0000_0003, 1) == 2)
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0000_0004, 0x8000_0000, 32) == 4)
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0000_0005, 0x8000_0000, 32) == 6)
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0000_0004, 0x8000_0001, 32) == 5)
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0000_0003, 0x0000_0000, 33) == 2)
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0000_0180, 0x0000_0000, 40) == 2)
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0010_0000, 0x0000_0000, 53) == 0)
    #expect(EmulatedDoubleRoundShiftRight53ToUint32(0x0010_0000, 0x0000_0000, 54) == 0)
}

@Test func emulatedDoubleToFloatBitsMatchesSwiftForFiniteDoubleValues() {
    let cases: [UInt64] = [
        0x0000_0000_0000_0000,
        0x8000_0000_0000_0000,
        0x0000_0000_0000_0001,
        0x8000_0000_0000_0001,
        0x0010_0000_0000_0000,
        0x3ff0_0000_0000_0000,
        0xbff0_0000_0000_0000,
        0x3ff0_0000_1000_0000,
        0x3ff0_0000_1000_0001,
        0x3ff0_0000_3000_0000,
        0x4009_21fb_5444_2d18,
        0xc009_21fb_5444_2d18,
        0x36a0_0000_0000_0000,
        0x3690_0000_0000_0000,
        0x3690_0000_0000_0001,
        0x3810_0000_0000_0000,
        0x47ef_ffff_e000_0000,
        0x7fe0_0000_0000_0000,
        0xffe0_0000_0000_0000,
    ]

    for bits in cases {
        let expected = Float(Double(bitPattern: bits)).bitPattern
        let actual = EmulatedDoubleToFloatBits(emulatedDouble(bitPattern: bits))
        #expect(actual == expected)
    }
}

@Test func emulatedDoubleToFloatBitsHandlesSpecialValues() {
    let cases: [(value: EmulatedDouble, expected: UInt32)] = [
        (EmulatedDoubleMake(0x7ff0_0000, 0x0000_0000), 0x7f80_0000),
        (EmulatedDoubleMake(0xfff0_0000, 0x0000_0000), 0xff80_0000),
        (EmulatedDoubleMake(0x7ff8_0000, 0x0000_0000), 0x7fc0_0000),
        (EmulatedDoubleMake(0xfff8_0000, 0x0000_0000), 0xffc0_0000),
        (EmulatedDoubleMake(0x7ff0_0000, 0x0000_0001), 0x7fc0_0000),
        (EmulatedDoubleMake(0x7ff0_0001, 0x0000_0000), 0x7f80_0008),
    ]

    for testCase in cases {
        #expect(EmulatedDoubleToFloatBits(testCase.value) == testCase.expected)
    }
}

@Test func emulatedDoubleRoundTripsFloatBits() {
    let cases: [UInt32] = [
        0x0000_0000,
        0x8000_0000,
        0x0000_0001,
        0x0000_0002,
        0x003f_ffff,
        0x007f_ffff,
        0x0080_0000,
        0x0080_0001,
        0x3eaa_aaab,
        0x3f80_0000,
        0x3f80_0001,
        0xbf80_0000,
        0x4020_0000,
        0xc020_0000,
        0x7f7f_ffff,
        0xff7f_ffff,
        0x7f80_0000,
        0xff80_0000,
        0x7fa1_2345,
        0x7fc0_0000,
        0xffc1_2345,
    ]

    for bits in cases {
        #expect(EmulatedDoubleToFloatBits(EmulatedDoubleFromFloatBits(bits)) == bits)
    }
}

@Test func emulatedDoubleFloatValueConversionRoundTripsBitPatterns() {
    let cases: [UInt32] = [
        0x0000_0000,
        0x8000_0000,
        0x0000_0001,
        0x007f_ffff,
        0x0080_0000,
        0x3f80_0000,
        0xbf80_0000,
        0x4020_0000,
        0xc020_0000,
        0x7f7f_ffff,
        0xff7f_ffff,
        0x7f80_0000,
        0xff80_0000,
        0x7fa1_2345,
        0x7fc0_0000,
        0xffc1_2345,
    ]

    for bits in cases {
        let value = Float(bitPattern: bits)
        let widened = EmulatedDoubleFromFloat(value)
        let narrowed = EmulatedDoubleToFloat(widened)

        #expect(EmulatedDoubleFloatToBits(value) == bits)
        #expect(EmulatedDoubleBitsToFloat(bits).bitPattern == bits)
        #expect(narrowed.bitPattern == bits)
    }
}

@Test func emulatedDoublePackFromFloatFractionPlacesHighAndLowFractionBits() {
    expectEmulatedDouble(
        EmulatedDoublePackFromFloatFraction(0x8000_0000, 0x3ff, 0x007f_ffff, 29),
        high: 0xbfff_ffff,
        low: 0xe000_0000
    )

    expectEmulatedDouble(
        EmulatedDoublePackFromFloatFraction(0x0000_0000, 0x380, 0x003f_ffff, 30),
        high: 0x380f_ffff,
        low: 0xc000_0000
    )
}

@Test func doubleToEmulatedDoublePreservesBinary64Bits() {
    let cases: [UInt64] = [
        0x0000_0000_0000_0000,
        0x8000_0000_0000_0000,
        0x0000_0000_0000_0001,
        0x8000_0000_0000_0001,
        0x000f_ffff_ffff_ffff,
        0x0010_0000_0000_0000,
        0x3ff0_0000_0000_0000,
        0xbff0_0000_0000_0000,
        0x4009_21fb_5444_2d18,
        0xc009_21fb_5444_2d18,
        0x7fe0_0000_0000_0000,
        0xffe0_0000_0000_0000,
        0x7ff0_0000_0000_0000,
        0xfff0_0000_0000_0000,
        0x7ff8_0000_0000_0000,
        0xfff8_0000_0000_0000,
        0x7ff0_0000_0000_0001,
        0xfff0_0000_0000_0001,
        0x7ff8_1234_5678_9abc,
    ]

    for bits in cases {
        let value = Double(bitPattern: bits).toEmulatedDouble()
        expectEmulatedDouble(
            value,
            high: UInt32((bits >> 32) & 0xffff_ffff),
            low: UInt32(bits & 0xffff_ffff)
        )
    }
}

@Test func floatToEmulatedDoubleMatchesCoreFloatConversion() {
    let cases: [UInt32] = [
        0x0000_0000,
        0x8000_0000,
        0x0000_0001,
        0x0000_0002,
        0x003f_ffff,
        0x007f_ffff,
        0x0080_0000,
        0x0080_0001,
        0x3eaa_aaab,
        0x3f80_0000,
        0x3f80_0001,
        0xbf80_0000,
        0x4020_0000,
        0xc020_0000,
        0x7f7f_ffff,
        0xff7f_ffff,
        0x7f80_0000,
        0xff80_0000,
        0x7fa1_2345,
        0x7fc0_0000,
        0xffc1_2345,
    ]

    for bits in cases {
        let actual = Float(bitPattern: bits).toEmulatedDouble()
        let expected = EmulatedDoubleFromFloatBits(bits)

        expectEmulatedDouble(actual, high: expected.highBits, low: expected.lowBits)
    }
}

@Test func emulatedDoubleArithmeticHelpersExposeExpectedConstants() {
    #expect(EmulatedDoubleHiddenBit() == 0x0010_0000_0000_0000)
    #expect(EmulatedDoubleGuardedHiddenBit() == 0x0080_0000_0000_0000)
}

@Test func emulatedDoubleFinitePartsMatchExpectedSignificands() {
    let cases: [(bits: UInt64, exponent: Int32, significand: UInt64)] = [
        (0x0000_0000_0000_0000, -1022, 0x0000_0000_0000_0000),
        (0x3ff0_0000_0000_0000, 0, 0x0010_0000_0000_0000),
        (0x3ff8_0000_0000_0000, 0, 0x0018_0000_0000_0000),
        (0x0010_0000_0000_0000, -1022, 0x0010_0000_0000_0000),
        (0x0000_0000_0000_0001, -1022, 0x0000_0000_0000_0001),
    ]

    for testCase in cases {
        var exponent: Int32 = 0
        let significand = EmulatedDoubleFiniteSignificand(
            emulatedDouble(bitPattern: testCase.bits),
            &exponent
        )

        #expect(exponent == testCase.exponent)
        #expect(significand == testCase.significand)
    }
}

@Test func emulatedDoubleNormalizeFiniteSignificandRestoresHiddenBit() {
    var smallestSignificand: UInt64 = 1
    var smallestExponent: Int32 = -1022
    EmulatedDoubleNormalizeFiniteSignificand(&smallestSignificand, &smallestExponent)
    #expect(smallestSignificand == EmulatedDoubleHiddenBit())
    #expect(smallestExponent == -1074)

    var halfHiddenSignificand: UInt64 = EmulatedDoubleHiddenBit() >> 1
    var halfHiddenExponent: Int32 = 0
    EmulatedDoubleNormalizeFiniteSignificand(&halfHiddenSignificand, &halfHiddenExponent)
    #expect(halfHiddenSignificand == EmulatedDoubleHiddenBit())
    #expect(halfHiddenExponent == -1)

    var zeroSignificand: UInt64 = 0
    var zeroExponent: Int32 = 7
    EmulatedDoubleNormalizeFiniteSignificand(&zeroSignificand, &zeroExponent)
    #expect(zeroSignificand == 0)
    #expect(zeroExponent == 7)
}

@Test func emulatedDoubleCompareMagnitudeOrdersFiniteValues() {
    #expect(
        EmulatedDoubleCompareMagnitude(
            emulatedDouble(bitPattern: 0xc000_0000_0000_0000),
            emulatedDouble(bitPattern: 0x3ff0_0000_0000_0000)
        ) == 1
    )
    #expect(
        EmulatedDoubleCompareMagnitude(
            emulatedDouble(bitPattern: 0xbff0_0000_0000_0000),
            emulatedDouble(bitPattern: 0x4000_0000_0000_0000)
        ) == -1
    )
    #expect(
        EmulatedDoubleCompareMagnitude(
            emulatedDouble(bitPattern: 0xbff8_0000_0000_0000),
            emulatedDouble(bitPattern: 0x3ff8_0000_0000_0000)
        ) == 0
    )
    #expect(
        EmulatedDoubleCompareMagnitude(
            emulatedDouble(bitPattern: 0x0000_0000_0000_0001),
            emulatedDouble(bitPattern: 0x0000_0000_0000_0000)
        ) == 1
    )
}

@Test func emulatedDoubleShiftRightJam64PreservesStickyBit() {
    #expect(EmulatedDoubleShiftRightJam64(0b1000, 0) == 0b1000)
    #expect(EmulatedDoubleShiftRightJam64(0b1000, 2) == 0b0010)
    #expect(EmulatedDoubleShiftRightJam64(0b1011, 2) == 0b0011)
    #expect(EmulatedDoubleShiftRightJam64(0, 64) == 0)
    #expect(EmulatedDoubleShiftRightJam64(0x8000_0000_0000_0000, 64) == 1)
    #expect(EmulatedDoubleShiftRightJam64(0x8000_0000_0000_0000, 127) == 1)
}

@Test func emulatedDoubleShiftRightJam128PreservesStickyBit() {
    #expect(EmulatedDoubleShiftRightJam128(0x1234, 0x5678, 0) == 0x5678)
    #expect(EmulatedDoubleShiftRightJam128(0, 0b1000, 2) == 0b0010)
    #expect(EmulatedDoubleShiftRightJam128(0, 0b1011, 2) == 0b0011)
    #expect(EmulatedDoubleShiftRightJam128(1, 0, 64) == 1)
    #expect(EmulatedDoubleShiftRightJam128(1, 1, 64) == 1)
    #expect(EmulatedDoubleShiftRightJam128(0x8000_0000_0000_0000, 0, 127) == 1)
    #expect(EmulatedDoubleShiftRightJam128(0, 1, 128) == 1)
}

@Test func emulatedDoubleMultiply64To128MatchesKnownProducts() {
    var high: UInt64 = 0
    var low: UInt64 = 0

    EmulatedDoubleMultiply64To128(0xffff_ffff, 0xffff_ffff, &high, &low)
    #expect(high == 0x0000_0000_0000_0000)
    #expect(low == 0xffff_fffe_0000_0001)

    EmulatedDoubleMultiply64To128(0x0010_0000_0000_0000, 0x0010_0000_0000_0000, &high, &low)
    #expect(high == 0x0000_0100_0000_0000)
    #expect(low == 0x0000_0000_0000_0000)

    EmulatedDoubleMultiply64To128(UInt64.max, UInt64.max, &high, &low)
    #expect(high == 0xffff_ffff_ffff_fffe)
    #expect(low == 0x0000_0000_0000_0001)
}

@Test func emulatedDoubleRoundAndPackHandlesNormalSubnormalOverflowAndUnderflow() {
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleRoundAndPack(0, 0, EmulatedDoubleGuardedHiddenBit()),
        0x3ff0_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleRoundAndPack(1, 0, EmulatedDoubleGuardedHiddenBit()),
        0xbff0_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleRoundAndPack(0, 0, ((EmulatedDoubleHiddenBit() + 1) << 3) | 0x4),
        0x3ff0_0000_0000_0002
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleRoundAndPack(0, -1022, 1 << 3),
        0x0000_0000_0000_0001
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleRoundAndPack(0, -1074, EmulatedDoubleGuardedHiddenBit()),
        0x0000_0000_0000_0001
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleRoundAndPack(1, -1075, EmulatedDoubleGuardedHiddenBit()),
        0x8000_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleRoundAndPack(0, 1024, EmulatedDoubleGuardedHiddenBit()),
        0x7ff0_0000_0000_0000
    )
}

@Test func emulatedDoubleArithmeticHandlesSignedZeroes() {
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleAdd(emulatedDouble(bitPattern: 0x0000_0000_0000_0000), emulatedDouble(bitPattern: 0x8000_0000_0000_0000)),
        0x0000_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleAdd(emulatedDouble(bitPattern: 0x8000_0000_0000_0000), emulatedDouble(bitPattern: 0x8000_0000_0000_0000)),
        0x8000_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleSubtract(emulatedDouble(bitPattern: 0x8000_0000_0000_0000), emulatedDouble(bitPattern: 0x0000_0000_0000_0000)),
        0x8000_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleMultiply(emulatedDouble(bitPattern: 0x8000_0000_0000_0000), emulatedDouble(bitPattern: 0x3ff0_0000_0000_0000)),
        0x8000_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleDivide(emulatedDouble(bitPattern: 0x8000_0000_0000_0000), emulatedDouble(bitPattern: 0x3ff0_0000_0000_0000)),
        0x8000_0000_0000_0000
    )
}

@Test func emulatedDoubleNegatedTogglesOnlySignBit() {
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleNegated(emulatedDouble(bitPattern: 0x3ff8_0000_0000_0001)),
        0xbff8_0000_0000_0001
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleNegated(emulatedDouble(bitPattern: 0x8000_0000_0000_0000)),
        0x0000_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleNegated(emulatedDouble(bitPattern: 0x7ff8_1234_5678_9abc)),
        0xfff8_1234_5678_9abc
    )
}

@Test func emulatedDoubleArithmeticHandlesSpecialValues() {
    expectNaN(
        EmulatedDoubleAdd(
            emulatedDouble(bitPattern: 0x7ff0_0000_0000_0000),
            emulatedDouble(bitPattern: 0xfff0_0000_0000_0000)
        )
    )
    expectNaN(
        EmulatedDoubleSubtract(
            emulatedDouble(bitPattern: 0x7ff0_0000_0000_0000),
            emulatedDouble(bitPattern: 0x7ff0_0000_0000_0000)
        )
    )
    expectNaN(
        EmulatedDoubleMultiply(
            emulatedDouble(bitPattern: 0x0000_0000_0000_0000),
            emulatedDouble(bitPattern: 0x7ff0_0000_0000_0000)
        )
    )
    expectNaN(
        EmulatedDoubleDivide(
            emulatedDouble(bitPattern: 0x0000_0000_0000_0000),
            emulatedDouble(bitPattern: 0x0000_0000_0000_0000)
        )
    )
    expectNaN(
        EmulatedDoubleDivide(
            emulatedDouble(bitPattern: 0x7ff0_0000_0000_0000),
            emulatedDouble(bitPattern: 0x7ff0_0000_0000_0000)
        )
    )

    expectEmulatedDoubleBitPattern(EmulatedDoubleInfinity(0), 0x7ff0_0000_0000_0000)
    expectEmulatedDoubleBitPattern(EmulatedDoubleInfinity(1), 0xfff0_0000_0000_0000)
    expectEmulatedDoubleBitPattern(EmulatedDoubleZero(0), 0x0000_0000_0000_0000)
    expectEmulatedDoubleBitPattern(EmulatedDoubleZero(1), 0x8000_0000_0000_0000)
    expectEmulatedDoubleBitPattern(EmulatedDoubleDefaultNaN(), 0x7ff8_0000_0000_0000)
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleQuietNaN(emulatedDouble(bitPattern: 0x7ff0_0000_0000_0001)),
        0x7ff8_0000_0000_0001
    )
    expectNaN(EmulatedDoubleQuietNaN(emulatedDouble(bitPattern: 0x3ff0_0000_0000_0000)))

    expectEmulatedDoubleBitPattern(
        EmulatedDoubleAdd(emulatedDouble(bitPattern: 0x7ff0_0000_0000_0000), emulatedDouble(bitPattern: 0x3ff0_0000_0000_0000)),
        0x7ff0_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleMultiply(emulatedDouble(bitPattern: 0xfff0_0000_0000_0000), emulatedDouble(bitPattern: 0x4000_0000_0000_0000)),
        0xfff0_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleDivide(emulatedDouble(bitPattern: 0xbff0_0000_0000_0000), emulatedDouble(bitPattern: 0x0000_0000_0000_0000)),
        0xfff0_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleDivide(emulatedDouble(bitPattern: 0xbff0_0000_0000_0000), emulatedDouble(bitPattern: 0x7ff0_0000_0000_0000)),
        0x8000_0000_0000_0000
    )
}

@Test func emulatedDoubleAddMatchesSwiftForFiniteValues() {
    let cases: [(UInt64, UInt64)] = [
        (0x3ff0_0000_0000_0000, 0x4000_0000_0000_0000),
        (0xbff0_0000_0000_0000, 0x3fd0_0000_0000_0000),
        (0x3ff0_0000_0000_0000, 0xbff0_0000_0000_0000),
        (0x0000_0000_0000_0001, 0x0000_0000_0000_0001),
        (0x0010_0000_0000_0000, 0x8000_0000_0000_0001),
        (0x3ff0_0000_0000_0000, 0x3ca0_0000_0000_0000),
        (0x3ff0_0000_0000_0000, 0x3cb0_0000_0000_0000),
        (0x7fe0_0000_0000_0000, 0x7fe0_0000_0000_0000),
    ]

    for (lhs, rhs) in cases {
        expectBinaryOperationMatchesSwift(lhs, rhs, EmulatedDoubleAdd) { $0 + $1 }
    }
}

@Test func emulatedDoubleSubtractMatchesSwiftForFiniteValues() {
    let cases: [(UInt64, UInt64)] = [
        (0x4008_0000_0000_0000, 0x3ff0_0000_0000_0000),
        (0x3ff0_0000_0000_0000, 0x4000_0000_0000_0000),
        (0x3ff0_0000_0000_0000, 0x3ff0_0000_0000_0000),
        (0x0000_0000_0000_0001, 0x0000_0000_0000_0001),
        (0x0010_0000_0000_0000, 0x0000_0000_0000_0001),
        (0x3ff0_0000_0000_0000, 0x3ca0_0000_0000_0000),
        (0x3ff0_0000_0000_0000, 0x3cb0_0000_0000_0000),
    ]

    for (lhs, rhs) in cases {
        expectBinaryOperationMatchesSwift(lhs, rhs, EmulatedDoubleSubtract) { $0 - $1 }
    }
}

@Test func emulatedDoubleMultiplyMatchesSwiftForFiniteValues() {
    let cases: [(UInt64, UInt64)] = [
        (0x3ff8_0000_0000_0000, 0x4000_0000_0000_0000),
        (0xc000_0000_0000_0000, 0x3fe0_0000_0000_0000),
        (0x0010_0000_0000_0000, 0x3fe0_0000_0000_0000),
        (0x0000_0000_0000_0001, 0x3fe0_0000_0000_0000),
        (0x7fe0_0000_0000_0000, 0x4000_0000_0000_0000),
        (0x3ff0_0000_0000_0001, 0x3ff0_0000_0000_0001),
    ]

    for (lhs, rhs) in cases {
        expectBinaryOperationMatchesSwift(lhs, rhs, EmulatedDoubleMultiply) { $0 * $1 }
    }
}

@Test func emulatedDoubleDivideMatchesSwiftForFiniteValues() {
    let cases: [(UInt64, UInt64)] = [
        (0x4008_0000_0000_0000, 0x4000_0000_0000_0000),
        (0x3ff0_0000_0000_0000, 0x4008_0000_0000_0000),
        (0xbff0_0000_0000_0000, 0x3fe0_0000_0000_0000),
        (0x0010_0000_0000_0000, 0x4000_0000_0000_0000),
        (0x0000_0000_0000_0001, 0x4000_0000_0000_0000),
        (0x7fe0_0000_0000_0000, 0x3fe0_0000_0000_0000),
        (0x3ff0_0000_0000_0001, 0x3ff0_0000_0000_0001),
    ]

    for (lhs, rhs) in cases {
        expectBinaryOperationMatchesSwift(lhs, rhs, EmulatedDoubleDivide) { $0 / $1 }
    }
}

@Test func emulatedDoubleSqrtMatchesSwiftForRepresentativeValues() {
    let cases: [UInt64] = [
        0x0000_0000_0000_0000,
        0x0000_0000_0000_0001,
        0x000f_ffff_ffff_ffff,
        0x0010_0000_0000_0000,
        0x3ca0_0000_0000_0000,
        0x3ff0_0000_0000_0000,
        0x4000_0000_0000_0000,
        0x4009_21fb_5444_2d18,
        0x7fe0_0000_0000_0000,
        0x7fef_ffff_ffff_ffff,
    ]

    for bits in cases {
        expectUnaryOperationMatchesSwift(bits, EmulatedDoubleSqrt) { $0.squareRoot() }
    }

    for bits in generatedFiniteDoubleBitPatterns() where (bits & 0x8000_0000_0000_0000) == 0 {
        expectUnaryOperationMatchesSwift(bits, EmulatedDoubleSqrt) { $0.squareRoot() }
    }
}

@Test func emulatedDoubleSqrtHandlesSpecialValues() {
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleSqrt(emulatedDouble(bitPattern: 0x8000_0000_0000_0000)),
        0x8000_0000_0000_0000
    )
    expectEmulatedDoubleBitPattern(
        EmulatedDoubleSqrt(emulatedDouble(bitPattern: 0x7ff0_0000_0000_0000)),
        0x7ff0_0000_0000_0000
    )
    expectNaN(EmulatedDoubleSqrt(emulatedDouble(bitPattern: 0xbff0_0000_0000_0000)))
    expectNaN(EmulatedDoubleSqrt(emulatedDouble(bitPattern: 0xfff0_0000_0000_0000)))
    expectNaN(EmulatedDoubleSqrt(emulatedDouble(bitPattern: 0x7ff0_0000_0000_0001)))
}

@Test func emulatedDoubleIsLessThanZeroFollowsIEEEComparisonRules() {
    #expect(EmulatedDoubleIsLessThanZero(emulatedDouble(bitPattern: 0xbff0_0000_0000_0000)) == 1)
    #expect(EmulatedDoubleIsLessThanZero(emulatedDouble(bitPattern: 0xfff0_0000_0000_0000)) == 1)
    #expect(EmulatedDoubleIsLessThanZero(emulatedDouble(bitPattern: 0x8000_0000_0000_0000)) == 0)
    #expect(EmulatedDoubleIsLessThanZero(emulatedDouble(bitPattern: 0x0000_0000_0000_0000)) == 0)
    #expect(EmulatedDoubleIsLessThanZero(emulatedDouble(bitPattern: 0x3ff0_0000_0000_0000)) == 0)
    #expect(EmulatedDoubleIsLessThanZero(emulatedDouble(bitPattern: 0x7ff8_0000_0000_0000)) == 0)
}

@Test func emulatedDoubleArithmeticMatchesSwiftAcrossGeneratedFiniteValues() {
    let values = generatedFiniteDoubleBitPatterns()

    for lhs in values {
        for rhs in values {
            expectBinaryOperationMatchesSwift(lhs, rhs, EmulatedDoubleAdd) { $0 + $1 }
            expectBinaryOperationMatchesSwift(lhs, rhs, EmulatedDoubleSubtract) { $0 - $1 }
            expectBinaryOperationMatchesSwift(lhs, rhs, EmulatedDoubleMultiply) { $0 * $1 }

            if isNonzeroDoubleBitPattern(rhs) {
                expectBinaryOperationMatchesSwift(lhs, rhs, EmulatedDoubleDivide) { $0 / $1 }
            }
        }
    }
}

private func expectEmulatedDouble(_ value: EmulatedDouble, high: UInt32, low: UInt32) {
    #expect(value.highBits == high)
    #expect(value.lowBits == low)
}

private func expectEmulatedDoubleBitPattern(_ value: EmulatedDouble, _ expected: UInt64) {
    #expect(bitPattern(value) == expected)
}

private func expectNaN(_ value: EmulatedDouble) {
    #expect(EmulatedDoubleIsNaN(value) == 1)
    #expect((value.highBits & 0x0008_0000) != 0)
}

private func expectBinaryOperationMatchesSwift(
    _ lhsBits: UInt64,
    _ rhsBits: UInt64,
    _ operation: (EmulatedDouble, EmulatedDouble) -> EmulatedDouble,
    _ swiftOperation: (Double, Double) -> Double
) {
    let actual = operation(emulatedDouble(bitPattern: lhsBits), emulatedDouble(bitPattern: rhsBits))
    let expected = swiftOperation(Double(bitPattern: lhsBits), Double(bitPattern: rhsBits)).bitPattern

    #expect(bitPattern(actual) == expected)
}

private func expectUnaryOperationMatchesSwift(
    _ bits: UInt64,
    _ operation: (EmulatedDouble) -> EmulatedDouble,
    _ swiftOperation: (Double) -> Double
) {
    let actual = operation(emulatedDouble(bitPattern: bits))
    let expected = swiftOperation(Double(bitPattern: bits)).bitPattern

    #expect(bitPattern(actual) == expected)
}

private func bitPattern(_ value: EmulatedDouble) -> UInt64 {
    (UInt64(value.highBits) << 32) | UInt64(value.lowBits)
}

private func generatedFiniteDoubleBitPatterns() -> [UInt64] {
    var values: [UInt64] = [
        0x0000_0000_0000_0000,
        0x8000_0000_0000_0000,
        0x0000_0000_0000_0001,
        0x8000_0000_0000_0001,
        0x000f_ffff_ffff_ffff,
        0x0010_0000_0000_0000,
        0x0010_0000_0000_0001,
        0x3ca0_0000_0000_0000,
        0x3cb0_0000_0000_0000,
        0x3fe0_0000_0000_0000,
        0x3ff0_0000_0000_0000,
        0x3ff0_0000_0000_0001,
        0x3ff8_0000_0000_0000,
        0x4000_0000_0000_0000,
        0x4008_0000_0000_0000,
        0x7fe0_0000_0000_0000,
        0x7fef_ffff_ffff_ffff,
        0xbfe0_0000_0000_0000,
        0xbff0_0000_0000_0000,
        0xc000_0000_0000_0000,
        0xffef_ffff_ffff_ffff,
    ]
    var state: UInt64 = 0x4d59_5df4_d0f3_3173

    while values.count < 96 {
        state = state &* 6_364_136_223_846_793_005 &+ 1_442_695_040_888_963_407
        if isFiniteDoubleBitPattern(state) {
            values.append(state)
        }
    }

    return values
}

private func isFiniteDoubleBitPattern(_ bits: UInt64) -> Bool {
    ((bits >> 52) & 0x7ff) != 0x7ff
}

private func isNonzeroDoubleBitPattern(_ bits: UInt64) -> Bool {
    (bits & 0x7fff_ffff_ffff_ffff) != 0
}

private func emulatedDouble(bitPattern: UInt64) -> EmulatedDouble {
    EmulatedDoubleMake(
        UInt32((bitPattern >> 32) & 0xffff_ffff),
        UInt32(bitPattern & 0xffff_ffff)
    )
}

private struct ClassificationCase {
    var high: UInt32
    var low: UInt32
    var sign: UInt32
    var exponent: UInt32
    var hasZeroFraction: UInt32
    var isZero: UInt32
    var isSubnormal: UInt32
    var isInfinity: UInt32
    var isNaN: UInt32
}

private struct WideningCase {
    var floatBits: UInt32
    var high: UInt32
    var low: UInt32
}
