import Foundation
import Testing
@testable import EmulatedDouble

@Test func emulatedDoubleMathConstantsExposeExpectedBitPatterns() {
    expectBits(EmulatedDoubleHalf(), 0x3fe0_0000_0000_0000)
    expectBits(EmulatedDoubleOne(), 0x3ff0_0000_0000_0000)
    expectBits(EmulatedDoubleTwo(), 0x4000_0000_0000_0000)
    expectBits(EmulatedDoubleThree(), 0x4008_0000_0000_0000)
    expectBits(EmulatedDoublePi(), Double.pi.bitPattern)
    expectBits(EmulatedDoubleHalfPi(), (Double.pi / 2).bitPattern)
    expectBits(EmulatedDoubleQuarterPi(), (Double.pi / 4).bitPattern)
    expectBits(EmulatedDoubleTwoPi(), (Double.pi * 2).bitPattern)
}

@Test func emulatedDoubleRelationalPredicatesFollowIEEEClassification() {
    let finite = ed(1.5)
    let subnormal = emulatedDouble(bitPattern: 0x0000_0000_0000_0001)
    let infinity = ed(.infinity)
    let nan = ed(.nan)

    #expect(EmulatedDoubleIsFinite(finite) == 1)
    #expect(EmulatedDoubleIsFinite(subnormal) == 1)
    #expect(EmulatedDoubleIsFinite(infinity) == 0)
    #expect(EmulatedDoubleIsFinite(nan) == 0)
    #expect(EmulatedDoubleIsInf(infinity) == 1)
    #expect(EmulatedDoubleIsInf(finite) == 0)
    #expect(EmulatedDoubleIsNormal(finite) == 1)
    #expect(EmulatedDoubleIsNormal(subnormal) == 0)
    #expect(EmulatedDoubleIsOrdered(finite, infinity) == 1)
    #expect(EmulatedDoubleIsOrdered(finite, nan) == 0)
    #expect(EmulatedDoubleIsUnordered(finite, nan) == 1)
}

@Test func emulatedDoubleSelectAndLogicalHelpersFollowScalarRules() {
    expectBits(EmulatedDoubleSelect(ed(1), ed(2), 0), ed(1).bitPatternForTest)
    expectBits(EmulatedDoubleSelect(ed(1), ed(2), 1), ed(2).bitPatternForTest)
    #expect(EmulatedDoubleNot(0) == 1)
    #expect(EmulatedDoubleNot(4) == 0)
    #expect(EmulatedDoubleAll(1, 2) == 1)
    #expect(EmulatedDoubleAll(1, 0) == 0)
    #expect(EmulatedDoubleAny(0, 2) == 1)
    #expect(EmulatedDoubleAny(0, 0) == 0)
}

@Test func emulatedDoubleComparisonsFollowIEEEOrdering() {
    #expect(EmulatedDoubleIsEqual(ed(0.0), ed(-0.0)) == 1)
    #expect(EmulatedDoubleIsEqual(ed(.nan), ed(.nan)) == 0)
    #expect(EmulatedDoubleIsLessThan(ed(-2), ed(-1)) == 1)
    #expect(EmulatedDoubleIsLessThan(ed(-0.0), ed(0.0)) == 0)
    #expect(EmulatedDoubleIsLessThan(ed(1), ed(2)) == 1)
    #expect(EmulatedDoubleIsLessThanOrEqual(ed(2), ed(2)) == 1)
    #expect(EmulatedDoubleIsGreaterThan(ed(3), ed(2)) == 1)
    #expect(EmulatedDoubleIsGreaterThanOrEqual(ed(3), ed(3)) == 1)
}

@Test func emulatedDoubleIntegerConversionRoundTripsRepresentativeValues() {
    let cases: [Int32] = [-2_147_483_648, -123_456, -1, 0, 1, 123_456, 2_147_483_647]

    for value in cases {
        #expect(EmulatedDoubleToInt32Saturating(EmulatedDoubleFromInt32(value)) == value)
    }

    #expect(EmulatedDoubleToInt32Saturating(ed(1.75)) == 1)
    #expect(EmulatedDoubleToInt32Saturating(ed(-1.75)) == -1)
    #expect(EmulatedDoubleToInt32Saturating(ed(Double.greatestFiniteMagnitude)) == Int32.max)
    #expect(EmulatedDoubleToInt32Saturating(ed(-Double.greatestFiniteMagnitude)) == Int32.min)
}

@Test func emulatedDoubleAbsCopySignAndSignPreserveExpectedBits() {
    expectBits(EmulatedDoubleAbs(ed(-1.5)), ed(1.5).bitPatternForTest)
    expectBits(EmulatedDoubleFabs(ed(-2.25)), ed(2.25).bitPatternForTest)
    expectBits(EmulatedDoubleCopySign(ed(1.5), ed(-0.0)), ed(-1.5).bitPatternForTest)
    expectBits(EmulatedDoubleSign(ed(2)), ed(1).bitPatternForTest)
    expectBits(EmulatedDoubleSign(ed(-2)), ed(-1).bitPatternForTest)
    expectBits(EmulatedDoubleSign(ed(-0.0)), ed(-0.0).bitPatternForTest)
    expectBits(EmulatedDoubleSign(ed(.nan)), ed(0).bitPatternForTest)
}

@Test func emulatedDoubleMinMaxClampAndSaturateFollowMetalRules() {
    expectBits(EmulatedDoubleMin(ed(3), ed(2)), ed(2).bitPatternForTest)
    expectBits(EmulatedDoubleFmin(ed(3), ed(2)), ed(2).bitPatternForTest)
    expectBits(EmulatedDoubleMax(ed(3), ed(2)), ed(3).bitPatternForTest)
    expectBits(EmulatedDoubleFmax(ed(3), ed(2)), ed(3).bitPatternForTest)
    expectBits(EmulatedDoubleMin(ed(.nan), ed(2)), ed(2).bitPatternForTest)
    expectBits(EmulatedDoubleMax(ed(2), ed(.nan)), ed(2).bitPatternForTest)
    expectBits(EmulatedDoubleMin3(ed(3), ed(1), ed(2)), ed(1).bitPatternForTest)
    expectBits(EmulatedDoubleFmin3(ed(3), ed(1), ed(2)), ed(1).bitPatternForTest)
    expectBits(EmulatedDoubleMax3(ed(3), ed(1), ed(2)), ed(3).bitPatternForTest)
    expectBits(EmulatedDoubleFmax3(ed(3), ed(1), ed(2)), ed(3).bitPatternForTest)
    expectBits(EmulatedDoubleClamp(ed(5), ed(1), ed(4)), ed(4).bitPatternForTest)
    expectBits(EmulatedDoubleClamp(ed(-1), ed(0), ed(4)), ed(0).bitPatternForTest)
    expectBits(EmulatedDoubleSaturate(ed(1.5)), ed(1).bitPatternForTest)
    expectBits(EmulatedDoubleSaturate(ed(-0.5)), ed(0).bitPatternForTest)
}

@Test func emulatedDoubleMedian3TreatsNaNsAsMissingData() {
    expectBits(EmulatedDoubleMedian3(ed(3), ed(1), ed(2)), ed(2).bitPatternForTest)
    expectBits(EmulatedDoubleFMedian3(ed(.nan), ed(1), ed(2)), ed(1).bitPatternForTest)
    #expect(EmulatedDoubleIsNaN(EmulatedDoubleMedian3(ed(.nan), ed(.nan), ed(.nan))) == 1)
}

@Test func emulatedDoubleStepMixAndSmoothstepMatchMetalDefinitions() {
    expectBits(EmulatedDoubleStep(ed(2), ed(1.5)), ed(0).bitPatternForTest)
    expectBits(EmulatedDoubleStep(ed(2), ed(2)), ed(1).bitPatternForTest)
    expectApprox(EmulatedDoubleMix(ed(10), ed(20), ed(0.25)), 12.5)
    expectApprox(EmulatedDoubleSmoothstep(ed(0), ed(2), ed(1)), 0.5)
}

@Test func emulatedDoubleRoundingFunctionsMatchSwiftForRepresentativeValues() {
    let cases: [Double] = [-2.7, -2.5, -1.5, -0.5, -0.2, 0.2, 0.5, 1.5, 2.5, 2.7]

    for value in cases {
        expectBits(EmulatedDoubleFloor(ed(value)), value.rounded(.down).bitPattern)
        expectBits(EmulatedDoubleCeil(ed(value)), value.rounded(.up).bitPattern)
        expectBits(EmulatedDoubleTrunc(ed(value)), value.rounded(.towardZero).bitPattern)
        expectBits(EmulatedDoubleRound(ed(value)), value.rounded(.toNearestOrAwayFromZero).bitPattern)
        expectBits(EmulatedDoubleRint(ed(value)), value.rounded(.toNearestOrEven).bitPattern)
        #expect(EmulatedDoubleHasFractionalPart(ed(value)) == 1)
    }

    #expect(EmulatedDoubleHasFractionalPart(ed(4)) == 0)
}

@Test func emulatedDoubleIntegralPredicatesDetectIntegralAndOddValues() {
    #expect(EmulatedDoubleIsIntegral(ed(4)) == 1)
    #expect(EmulatedDoubleIsIntegral(ed(4.25)) == 0)
    #expect(EmulatedDoubleIsOddInteger(ed(3)) == 1)
    #expect(EmulatedDoubleIsOddInteger(ed(4)) == 0)
    #expect(EmulatedDoubleIsOddInteger(ed(3.5)) == 0)
}

@Test func emulatedDoubleFmodFractAndModfFollowDefinitions() {
    expectApprox(EmulatedDoubleFmod(ed(5.5), ed(2)), 1.5)
    expectApprox(EmulatedDoubleFmod(ed(-5.5), ed(2)), -1.5)
    expectApprox(EmulatedDoubleFract(ed(1.25)), 0.25)
    expectApprox(EmulatedDoubleFract(ed(-1.25)), 0.75)

    var integerPart = ed(0)
    let fraction = EmulatedDoubleModf(ed(-3.75), &integerPart)
    expectApprox(integerPart, -3)
    expectApprox(fraction, -0.75)
}

@Test func emulatedDoubleFrexpLdexpAndIlogbDecomposeFiniteValues() {
    var exponent: Int32 = 0
    let mantissa = EmulatedDoubleFrexp(ed(12), &exponent)
    #expect(exponent == 4)
    expectApprox(mantissa, 0.75)
    expectBits(EmulatedDoubleLdexp(mantissa, exponent), ed(12).bitPatternForTest)
    #expect(EmulatedDoubleIlogb(ed(12)) == 3)
    #expect(EmulatedDoubleIlogb(ed(0)) == Int32.min)
    #expect(EmulatedDoubleIlogb(ed(.infinity)) == Int32.max)
}

@Test func emulatedDoubleNextafterMovesOneRepresentableStep() {
    expectBits(EmulatedDoubleNextafter(ed(1), ed(2)), Double(1).nextUp.bitPattern)
    expectBits(EmulatedDoubleNextafter(ed(1), ed(0)), Double(1).nextDown.bitPattern)
    expectBits(EmulatedDoubleNextafter(ed(0), ed(-1)), 0x8000_0000_0000_0001)
    expectBits(EmulatedDoubleNextafter(ed(1), ed(1)), ed(1).bitPatternForTest)
}

@Test func emulatedDoubleFdimAndFmaFollowDefinitions() {
    expectApprox(EmulatedDoubleFdim(ed(5), ed(2)), 3)
    expectBits(EmulatedDoubleFdim(ed(2), ed(5)), ed(0).bitPatternForTest)
    expectApprox(EmulatedDoubleFma(ed(2), ed(3), ed(4)), 10)
}

@Test func emulatedDoubleRsqrtMatchesReciprocalSqrtDefinition() {
    expectApprox(EmulatedDoubleRsqrt(ed(4)), 0.5)
    expectApprox(EmulatedDoubleRsqrt(ed(0.25)), 2)
}

@Test func emulatedDoubleTrigonometricFunctionsMatchReferenceValues() {
    let trigCases: [Double] = [-Double.pi / 3, -Double.pi / 6, 0, Double.pi / 6, Double.pi / 4, Double.pi / 3]

    for value in trigCases {
        expectApprox(EmulatedDoubleSin(ed(value)), sin(value), tolerance: 1.0e-10)
        expectApprox(EmulatedDoubleCos(ed(value)), cos(value), tolerance: 1.0e-10)
        expectApprox(EmulatedDoubleTan(ed(value)), tan(value), tolerance: 1.0e-9)
    }

    expectApprox(EmulatedDoubleSinPi(ed(0.5)), 1, tolerance: 1.0e-10)
    expectApprox(EmulatedDoubleCosPi(ed(1)), -1, tolerance: 1.0e-10)
    expectApprox(EmulatedDoubleTanPi(ed(0.25)), 1, tolerance: 1.0e-9)

    var cosine = ed(0)
    let sine = EmulatedDoubleSincos(ed(Double.pi / 6), &cosine)
    expectApprox(sine, 0.5, tolerance: 1.0e-10)
    expectApprox(cosine, cos(Double.pi / 6), tolerance: 1.0e-10)
}

@Test func emulatedDoubleExpLogAndPowMatchReferenceValues() {
    let expCases: [Double] = [-1, -0.5, 0, 0.5, 1, 2]
    for value in expCases {
        expectApprox(EmulatedDoubleExp(ed(value)), exp(value), tolerance: 1.0e-8)
        expectApprox(EmulatedDoubleExp2(ed(value)), exp2(value), tolerance: 1.0e-8)
        expectApprox(EmulatedDoubleExp10(ed(value)), pow(10, value), tolerance: 1.0e-7)
    }

    let logCases: [Double] = [0.25, 0.5, 1, 2, 10]
    for value in logCases {
        expectApprox(EmulatedDoubleLog(ed(value)), log(value), tolerance: 1.0e-8)
        expectApprox(EmulatedDoubleLog2(ed(value)), log2(value), tolerance: 1.0e-8)
        expectApprox(EmulatedDoubleLog10(ed(value)), log10(value), tolerance: 1.0e-8)
    }

    expectApprox(EmulatedDoublePow(ed(2), ed(3)), 8, tolerance: 1.0e-8)
    expectApprox(EmulatedDoublePow(ed(4), ed(0.5)), 2, tolerance: 1.0e-8)
    expectApprox(EmulatedDoublePow(ed(-2), ed(3)), -8, tolerance: 1.0e-8)
    expectApprox(EmulatedDoublePowr(ed(4), ed(0.5)), 2, tolerance: 1.0e-8)
}

@Test func emulatedDoubleInverseTrigonometricFunctionsMatchReferenceValues() {
    let cases: [Double] = [-0.75, -0.25, 0, 0.25, 0.75]
    for value in cases {
        expectApprox(EmulatedDoubleAtan(ed(value)), atan(value), tolerance: 1.0e-8)
        expectApprox(EmulatedDoubleAsin(ed(value)), asin(value), tolerance: 1.0e-8)
        expectApprox(EmulatedDoubleAcos(ed(value)), acos(value), tolerance: 1.0e-8)
    }

    expectApprox(EmulatedDoubleAtan2(ed(1), ed(1)), atan2(1, 1), tolerance: 1.0e-8)
    expectApprox(EmulatedDoubleAtan2(ed(1), ed(-1)), atan2(1, -1), tolerance: 1.0e-8)
}

@Test func emulatedDoubleHyperbolicFunctionsMatchDefinitions() {
    let cases: [Double] = [-0.75, -0.25, 0, 0.25, 0.75]
    for value in cases {
        expectApprox(EmulatedDoubleSinh(ed(value)), sinh(value), tolerance: 1.0e-8)
        expectApprox(EmulatedDoubleCosh(ed(value)), cosh(value), tolerance: 1.0e-8)
        expectApprox(EmulatedDoubleTanh(ed(value)), tanh(value), tolerance: 1.0e-8)
        expectApprox(EmulatedDoubleAsinh(ed(value)), asinh(value), tolerance: 1.0e-8)
    }

    expectApprox(EmulatedDoubleAcosh(ed(2)), acosh(2), tolerance: 1.0e-8)
    expectApprox(EmulatedDoubleAtanh(ed(0.25)), atanh(0.25), tolerance: 1.0e-8)
}

@Test func emulatedDoubleTranscendentalFunctionsHandleSpecialValues() {
    #expect(EmulatedDoubleIsNaN(EmulatedDoubleSin(ed(.infinity))) == 1)
    #expect(EmulatedDoubleIsNaN(EmulatedDoubleCos(ed(.infinity))) == 1)
    #expect(EmulatedDoubleIsNaN(EmulatedDoubleLog(ed(-1))) == 1)
    expectBits(EmulatedDoubleLog(ed(0)), ed(-Double.infinity).bitPatternForTest)
    expectBits(EmulatedDoubleExp(ed(.infinity)), ed(.infinity).bitPatternForTest)
    expectBits(EmulatedDoubleExp(ed(-Double.infinity)), ed(0).bitPatternForTest)
    #expect(EmulatedDoubleIsNaN(EmulatedDoublePow(ed(-2), ed(0.5))) == 1)
}

@Test func emulatedDoubleVectorMathWrappersApplyScalarFunctionsComponentwise() {
    expectVector(EmulatedDouble2Abs(vec2(-1, 2)), [1, 2])
    expectVector(EmulatedDouble3Floor(vec3(1.2, -1.2, 2.8)), [1, -2, 2])
    expectVector(EmulatedDouble4Ceil(vec4(1.2, -1.2, 2.8, -2.8)), [2, -1, 3, -2])
    expectVectorApprox(EmulatedDouble2Sin(vec2(0, Double.pi / 6)), [0, 0.5], tolerance: 1.0e-10)
    expectVectorApprox(EmulatedDouble3Exp(vec3(0, 1, 2)), [1, exp(1), exp(2)], tolerance: 1.0e-8)
    expectVectorApprox(EmulatedDouble4Log(vec4(1, 2, 4, 8)), [0, log(2), log(4), log(8)], tolerance: 1.0e-8)
    expectVector(EmulatedDouble2Min(vec2(1, 4), vec2(2, 3)), [1, 3])
    expectVector(EmulatedDouble2Fmin(vec2(1, 4), vec2(2, 3)), [1, 3])
    expectVector(EmulatedDouble3Max(vec3(1, 4, 2), vec3(2, 3, 5)), [2, 4, 5])
    expectVector(EmulatedDouble3Fmax(vec3(1, 4, 2), vec3(2, 3, 5)), [2, 4, 5])
    expectVector(EmulatedDouble2CopySign(vec2(1, -2), vec2(-3, 4)), [-1, 2])
    expectBits(EmulatedDouble2Nextafter(vec2(1, 1), vec2(2, 0)).x, Double(1).nextUp.bitPattern)
    expectBits(EmulatedDouble2Nextafter(vec2(1, 1), vec2(2, 0)).y, Double(1).nextDown.bitPattern)
    expectVector(EmulatedDouble2Min3(vec2(3, 2), vec2(1, 4), vec2(2, 1)), [1, 1])
    expectVector(EmulatedDouble3Max3(vec3(3, 2, 5), vec3(1, 4, 4), vec3(2, 1, 6)), [3, 4, 6])
    expectVector(EmulatedDouble4Median3(vec4(3, 2, 5, 9), vec4(1, 4, 4, 8), vec4(2, 1, 6, 7)), [2, 2, 5, 8])
    expectVector(EmulatedDouble2Ldexp(vec2(0.5, 1.5), 2), [2, 6])
    expectVector(EmulatedDouble4Clamp(vec4(-1, 0.5, 2, 5), vec4(0, 0, 0, 0), vec4(1, 1, 1, 1)), [0, 0.5, 1, 1])
    expectVector(EmulatedDouble2Saturate(vec2(-1, 2)), [0, 1])
    expectVectorApprox(EmulatedDouble3Mix(vec3(0, 10, 20), vec3(10, 20, 30), vec3(0.5, 0.25, 0.75)), [5, 12.5, 27.5])
    expectVectorApprox(EmulatedDouble2Smoothstep(vec2(0, 0), vec2(2, 2), vec2(1, 2)), [0.5, 1])
    expectVector(EmulatedDouble2Fma(vec2(2, 3), vec2(4, 5), vec2(6, 7)), [14, 22])
    expectVectorApprox(EmulatedDouble2Pow(vec2(2, 4), vec2(3, 0.5)), [8, 2])
    expectVectorApprox(EmulatedDouble2Atan2(vec2(1, 1), vec2(1, -1)), [atan2(1, 1), atan2(1, -1)], tolerance: 1.0e-8)
}

@Test func emulatedDoubleVectorOutParameterMathWrappersFillOutputsComponentwise() {
    var exponents = [Int32](repeating: 0, count: 4)
    let mantissas = exponents.withUnsafeMutableBufferPointer {
        EmulatedDouble4Frexp(vec4(1, 2, 4, 8), $0.baseAddress!)
    }
    expectVector(mantissas, [0.5, 0.5, 0.5, 0.5])
    #expect(exponents == [1, 2, 3, 4])

    var integerPart = EmulatedDouble3Zero()
    let fraction = EmulatedDouble3Modf(vec3(1.25, -2.5, 3.75), &integerPart)
    expectVector(integerPart, [1, -2, 3])
    expectVector(fraction, [0.25, -0.5, 0.75])

    var cosine = EmulatedDouble2Zero()
    let sine = EmulatedDouble2Sincos(vec2(0, Double.pi / 6), &cosine)
    expectVectorApprox(sine, [0, 0.5], tolerance: 1.0e-10)
    expectVectorApprox(cosine, [1, cos(Double.pi / 6)], tolerance: 1.0e-10)
}

private func ed(_ value: Double) -> EmulatedDouble {
    value.toEmulatedDouble()
}

private func emulatedDouble(bitPattern: UInt64) -> EmulatedDouble {
    EmulatedDoubleMake(
        UInt32((bitPattern >> 32) & 0xffff_ffff),
        UInt32(bitPattern & 0xffff_ffff)
    )
}

private func bitPattern(_ value: EmulatedDouble) -> UInt64 {
    (UInt64(value.highBits) << 32) | UInt64(value.lowBits)
}

private func double(_ value: EmulatedDouble) -> Double {
    Double(bitPattern: bitPattern(value))
}

private func expectBits(_ value: EmulatedDouble, _ expected: UInt64) {
    #expect(bitPattern(value) == expected)
}

private func expectApprox(_ value: EmulatedDouble, _ expected: Double, tolerance: Double = 1.0e-12) {
    let actual = double(value)
    if expected.isNaN {
        #expect(actual.isNaN)
    } else if expected.isInfinite {
        #expect(actual == expected)
    } else {
        #expect(abs(actual - expected) <= tolerance)
    }
}

private func vec2(_ x: Double, _ y: Double) -> EmulatedDouble2 {
    EmulatedDouble2Make(ed(x), ed(y))
}

private func vec3(_ x: Double, _ y: Double, _ z: Double) -> EmulatedDouble3 {
    EmulatedDouble3Make(ed(x), ed(y), ed(z))
}

private func vec4(_ x: Double, _ y: Double, _ z: Double, _ w: Double) -> EmulatedDouble4 {
    EmulatedDouble4Make(ed(x), ed(y), ed(z), ed(w))
}

private func expectVector(_ actual: EmulatedDouble2, _ expected: [Double]) {
    #expect([double(actual.x), double(actual.y)] == expected)
}

private func expectVector(_ actual: EmulatedDouble3, _ expected: [Double]) {
    #expect([double(actual.x), double(actual.y), double(actual.z)] == expected)
}

private func expectVector(_ actual: EmulatedDouble4, _ expected: [Double]) {
    #expect([double(actual.x), double(actual.y), double(actual.z), double(actual.w)] == expected)
}

private func expectVectorApprox(_ actual: EmulatedDouble2, _ expected: [Double], tolerance: Double = 1.0e-12) {
    expectApprox(actual.x, expected[0], tolerance: tolerance)
    expectApprox(actual.y, expected[1], tolerance: tolerance)
}

private func expectVectorApprox(_ actual: EmulatedDouble3, _ expected: [Double], tolerance: Double = 1.0e-12) {
    expectApprox(actual.x, expected[0], tolerance: tolerance)
    expectApprox(actual.y, expected[1], tolerance: tolerance)
    expectApprox(actual.z, expected[2], tolerance: tolerance)
}

private func expectVectorApprox(_ actual: EmulatedDouble4, _ expected: [Double], tolerance: Double = 1.0e-12) {
    expectApprox(actual.x, expected[0], tolerance: tolerance)
    expectApprox(actual.y, expected[1], tolerance: tolerance)
    expectApprox(actual.z, expected[2], tolerance: tolerance)
    expectApprox(actual.w, expected[3], tolerance: tolerance)
}

private extension EmulatedDouble {
    var bitPatternForTest: UInt64 {
        bitPattern(self)
    }
}
