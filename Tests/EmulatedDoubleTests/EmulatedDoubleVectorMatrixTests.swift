import Testing
@testable import EmulatedDouble

@Test func emulatedDoubleVectorConstructorsPreserveComponents() {
    expectVector(EmulatedDouble2Make(ed(1), ed(2)), [1, 2])
    expectVector(EmulatedDouble3Make(ed(1), ed(2), ed(3)), [1, 2, 3])
    expectVector(EmulatedDouble4Make(ed(1), ed(2), ed(3), ed(4)), [1, 2, 3, 4])
    expectVector(EmulatedDouble2Splat(ed(-2)), [-2, -2])
    expectVector(EmulatedDouble3Splat(ed(-3)), [-3, -3, -3])
    expectVector(EmulatedDouble4Splat(ed(-4)), [-4, -4, -4, -4])
    expectVector(EmulatedDouble2Zero(), [0, 0])
    expectVector(EmulatedDouble3Zero(), [0, 0, 0])
    expectVector(EmulatedDouble4Zero(), [0, 0, 0, 0])
}

@Test func emulatedDoubleVectorArithmeticMatchesScalarArithmetic() {
    let lhs2 = vec2(8, -6)
    let rhs2 = vec2(2, 3)
    expectVector(EmulatedDouble2Add(lhs2, rhs2), [10, -3])
    expectVector(EmulatedDouble2Subtract(lhs2, rhs2), [6, -9])
    expectVector(EmulatedDouble2Multiply(lhs2, rhs2), [16, -18])
    expectVector(EmulatedDouble2Divide(lhs2, rhs2), [4, -2])
    expectVector(EmulatedDouble2Negated(lhs2), [-8, 6])

    let lhs3 = vec3(8, -6, 10)
    let rhs3 = vec3(2, 3, -5)
    expectVector(EmulatedDouble3Add(lhs3, rhs3), [10, -3, 5])
    expectVector(EmulatedDouble3Subtract(lhs3, rhs3), [6, -9, 15])
    expectVector(EmulatedDouble3Multiply(lhs3, rhs3), [16, -18, -50])
    expectVector(EmulatedDouble3Divide(lhs3, rhs3), [4, -2, -2])
    expectVector(EmulatedDouble3Negated(lhs3), [-8, 6, -10])

    let lhs4 = vec4(8, -6, 10, -12)
    let rhs4 = vec4(2, 3, -5, -4)
    expectVector(EmulatedDouble4Add(lhs4, rhs4), [10, -3, 5, -16])
    expectVector(EmulatedDouble4Subtract(lhs4, rhs4), [6, -9, 15, -8])
    expectVector(EmulatedDouble4Multiply(lhs4, rhs4), [16, -18, -50, 48])
    expectVector(EmulatedDouble4Divide(lhs4, rhs4), [4, -2, -2, 3])
    expectVector(EmulatedDouble4Negated(lhs4), [-8, 6, -10, 12])
}

@Test func emulatedDoubleVectorScalarArithmeticMatchesScalarArithmetic() {
    expectVector(EmulatedDouble2AddScalar(vec2(1, 2), ed(3)), [4, 5])
    expectVector(EmulatedDouble2SubtractScalar(vec2(1, 2), ed(3)), [-2, -1])
    expectVector(EmulatedDouble2ScalarSubtract(ed(10), vec2(1, 2)), [9, 8])
    expectVector(EmulatedDouble2MultiplyScalar(vec2(1, 2), ed(3)), [3, 6])
    expectVector(EmulatedDouble2DivideScalar(vec2(6, 9), ed(3)), [2, 3])
    expectVector(EmulatedDouble2ScalarDivide(ed(12), vec2(3, 4)), [4, 3])

    expectVector(EmulatedDouble3AddScalar(vec3(1, 2, 3), ed(4)), [5, 6, 7])
    expectVector(EmulatedDouble3SubtractScalar(vec3(1, 2, 3), ed(4)), [-3, -2, -1])
    expectVector(EmulatedDouble3ScalarSubtract(ed(10), vec3(1, 2, 3)), [9, 8, 7])
    expectVector(EmulatedDouble3MultiplyScalar(vec3(1, 2, 3), ed(4)), [4, 8, 12])
    expectVector(EmulatedDouble3DivideScalar(vec3(8, 12, 16), ed(4)), [2, 3, 4])
    expectVector(EmulatedDouble3ScalarDivide(ed(24), vec3(3, 4, 6)), [8, 6, 4])

    expectVector(EmulatedDouble4AddScalar(vec4(1, 2, 3, 4), ed(5)), [6, 7, 8, 9])
    expectVector(EmulatedDouble4SubtractScalar(vec4(1, 2, 3, 4), ed(5)), [-4, -3, -2, -1])
    expectVector(EmulatedDouble4ScalarSubtract(ed(10), vec4(1, 2, 3, 4)), [9, 8, 7, 6])
    expectVector(EmulatedDouble4MultiplyScalar(vec4(1, 2, 3, 4), ed(5)), [5, 10, 15, 20])
    expectVector(EmulatedDouble4DivideScalar(vec4(10, 15, 20, 25), ed(5)), [2, 3, 4, 5])
    expectVector(EmulatedDouble4ScalarDivide(ed(60), vec4(3, 4, 5, 6)), [20, 15, 12, 10])
}

@Test func emulatedDoubleVectorDotLengthAndDistanceUseScalarArithmetic() {
    #expect(double(EmulatedDouble2Dot(vec2(1, 2), vec2(3, 4))) == 11)
    #expect(double(EmulatedDouble3Dot(vec3(1, 2, 3), vec3(4, 5, 6))) == 32)
    #expect(double(EmulatedDouble4Dot(vec4(1, 2, 3, 4), vec4(5, 6, 7, 8))) == 70)
    #expect(double(EmulatedDouble2LengthSquared(vec2(3, 4))) == 25)
    #expect(double(EmulatedDouble3LengthSquared(vec3(1, 2, 2))) == 9)
    #expect(double(EmulatedDouble4LengthSquared(vec4(1, 2, 3, 4))) == 30)
    #expect(double(EmulatedDouble2DistanceSquared(vec2(1, 2), vec2(4, 6))) == 25)
    #expect(double(EmulatedDouble3DistanceSquared(vec3(1, 2, 3), vec3(2, 4, 6))) == 14)
    #expect(double(EmulatedDouble4DistanceSquared(vec4(1, 2, 3, 4), vec4(2, 4, 6, 8))) == 30)
}

@Test func emulatedDoubleVectorLengthDistanceAndNormalizeUseSqrt() {
    #expect(double(EmulatedDouble2Length(vec2(3, 4))) == 5)
    #expect(double(EmulatedDouble3Length(vec3(2, 3, 6))) == 7)
    #expect(double(EmulatedDouble4Length(vec4(1, 2, 2, 4))) == 5)

    #expect(double(EmulatedDouble2Distance(vec2(1, 2), vec2(4, 6))) == 5)
    #expect(double(EmulatedDouble3Distance(vec3(1, 2, 3), vec3(3, 5, 9))) == 7)
    #expect(double(EmulatedDouble4Distance(vec4(1, 2, 3, 4), vec4(2, 4, 5, 8))) == 5)

    expectVector(EmulatedDouble2Normalize(vec2(3, 4)), [3.0 / 5.0, 4.0 / 5.0])
    expectVector(EmulatedDouble3Normalize(vec3(2, 3, 6)), [2.0 / 7.0, 3.0 / 7.0, 6.0 / 7.0])
    expectVector(EmulatedDouble4Normalize(vec4(1, 2, 2, 4)), [1.0 / 5.0, 2.0 / 5.0, 2.0 / 5.0, 4.0 / 5.0])
}

@Test func emulatedDoubleVectorFaceforwardMatchesMetalDefinition() {
    expectVector(EmulatedDouble2Faceforward(vec2(1, 2), vec2(1, 0), vec2(-1, 0)), [1, 2])
    expectVector(EmulatedDouble2Faceforward(vec2(1, 2), vec2(1, 0), vec2(1, 0)), [-1, -2])

    expectVector(EmulatedDouble3Faceforward(vec3(1, 2, 3), vec3(1, 0, 0), vec3(-1, 0, 0)), [1, 2, 3])
    expectVector(EmulatedDouble3Faceforward(vec3(1, 2, 3), vec3(1, 0, 0), vec3(1, 0, 0)), [-1, -2, -3])

    expectVector(EmulatedDouble4Faceforward(vec4(1, 2, 3, 4), vec4(1, 0, 0, 0), vec4(-1, 0, 0, 0)), [1, 2, 3, 4])
    expectVector(EmulatedDouble4Faceforward(vec4(1, 2, 3, 4), vec4(1, 0, 0, 0), vec4(1, 0, 0, 0)), [-1, -2, -3, -4])
}

@Test func emulatedDoubleVectorReflectMatchesMetalDefinition() {
    expectVector(EmulatedDouble2Reflect(vec2(1, -1), vec2(0, 2)), [1, 1])
    expectVector(EmulatedDouble3Reflect(vec3(1, -1, 0), vec3(0, 2, 0)), [1, 1, 0])
    expectVector(EmulatedDouble4Reflect(vec4(1, -1, 0, 0), vec4(0, 2, 0, 0)), [1, 1, 0, 0])
}

@Test func emulatedDoubleVectorRefractMatchesMetalDefinition() {
    expectVector(EmulatedDouble2Refract(vec2(0, -1), vec2(0, 1), ed(1)), [0, -1])
    expectVector(EmulatedDouble3Refract(vec3(0, -1, 0), vec3(0, 1, 0), ed(1)), [0, -1, 0])
    expectVector(EmulatedDouble4Refract(vec4(0, -1, 0, 0), vec4(0, 1, 0, 0), ed(1)), [0, -1, 0, 0])
    expectVector(EmulatedDouble2Refract(vec2(1, 0), vec2(0, 1), ed(2)), [0, 0])
}

@Test func emulatedDoubleVectorCrossProductMatchesRightHandRule() {
    expectVector(EmulatedDouble3Cross(vec3(1, 0, 0), vec3(0, 1, 0)), [0, 0, 1])
    expectVector(EmulatedDouble3Cross(vec3(2, 3, 4), vec3(5, 6, 7)), [-3, 6, -3])
}

@Test func emulatedDoubleMatrixConstructorsUseColumnMajorLayout() {
    expectMatrix(EmulatedDouble2x2Make(vec2(1, 2), vec2(3, 4)), [1, 2, 3, 4])
    expectMatrix(EmulatedDouble2x3Make(vec3(1, 2, 3), vec3(4, 5, 6)), [1, 2, 3, 4, 5, 6])
    expectMatrix(EmulatedDouble2x4Make(vec4(1, 2, 3, 4), vec4(5, 6, 7, 8)), [1, 2, 3, 4, 5, 6, 7, 8])
    expectMatrix(EmulatedDouble3x2Make(vec2(1, 2), vec2(3, 4), vec2(5, 6)), [1, 2, 3, 4, 5, 6])
    expectMatrix(EmulatedDouble3x3Make(vec3(1, 2, 3), vec3(4, 5, 6), vec3(7, 8, 9)), [1, 2, 3, 4, 5, 6, 7, 8, 9])
    expectMatrix(EmulatedDouble3x4Make(vec4(1, 2, 3, 4), vec4(5, 6, 7, 8), vec4(9, 10, 11, 12)), [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12])
    expectMatrix(EmulatedDouble4x2Make(vec2(1, 2), vec2(3, 4), vec2(5, 6), vec2(7, 8)), [1, 2, 3, 4, 5, 6, 7, 8])
    expectMatrix(EmulatedDouble4x3Make(vec3(1, 2, 3), vec3(4, 5, 6), vec3(7, 8, 9), vec3(10, 11, 12)), [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12])
    expectMatrix(EmulatedDouble4x4Make(vec4(1, 2, 3, 4), vec4(5, 6, 7, 8), vec4(9, 10, 11, 12), vec4(13, 14, 15, 16)), [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16])
    expectMatrix(EmulatedDouble4x3Filled(ed(-2)), Array(repeating: -2, count: 12))
}

@Test func emulatedDoubleMatrixDiagonalAndIdentityConstructors() {
    expectMatrix(EmulatedDouble2x2Diagonal(ed(5)), [5, 0, 0, 5])
    expectMatrix(EmulatedDouble2x3Diagonal(ed(5)), [5, 0, 0, 0, 5, 0])
    expectMatrix(EmulatedDouble2x4Diagonal(ed(5)), [5, 0, 0, 0, 0, 5, 0, 0])
    expectMatrix(EmulatedDouble3x2Diagonal(ed(5)), [5, 0, 0, 5, 0, 0])
    expectMatrix(EmulatedDouble3x3Diagonal(ed(5)), [5, 0, 0, 0, 5, 0, 0, 0, 5])
    expectMatrix(EmulatedDouble3x4Diagonal(ed(5)), [5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5, 0])
    expectMatrix(EmulatedDouble4x2Diagonal(ed(5)), [5, 0, 0, 5, 0, 0, 0, 0])
    expectMatrix(EmulatedDouble4x3Diagonal(ed(5)), [5, 0, 0, 0, 5, 0, 0, 0, 5, 0, 0, 0])
    expectMatrix(EmulatedDouble4x4Diagonal(ed(5)), [5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5])
    expectMatrix(EmulatedDouble2x2Identity(), [1, 0, 0, 1])
    expectMatrix(EmulatedDouble3x3Identity(), [1, 0, 0, 0, 1, 0, 0, 0, 1])
    expectMatrix(EmulatedDouble4x4Identity(), [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1])
}

@Test func emulatedDoubleMatrixComponentwiseArithmeticMatchesVectorArithmetic() {
    let lhs = EmulatedDouble3x2Make(vec2(8, -6), vec2(10, -12), vec2(14, -16))
    let rhs = EmulatedDouble3x2Make(vec2(2, 3), vec2(-5, -4), vec2(7, -8))
    expectMatrix(EmulatedDouble3x2Add(lhs, rhs), [10, -3, 5, -16, 21, -24])
    expectMatrix(EmulatedDouble3x2Subtract(lhs, rhs), [6, -9, 15, -8, 7, -8])
    expectMatrix(EmulatedDouble3x2MultiplyComponents(lhs, rhs), [16, -18, -50, 48, 98, 128])
    expectMatrix(EmulatedDouble3x2DivideComponents(lhs, rhs), [4, -2, -2, 3, 2, 2])
    expectMatrix(EmulatedDouble3x2MultiplyScalar(lhs, ed(2)), [16, -12, 20, -24, 28, -32])
    expectMatrix(EmulatedDouble3x2DivideScalar(lhs, ed(2)), [4, -3, 5, -6, 7, -8])
    expectMatrix(EmulatedDouble3x2Negated(lhs), [-8, 6, -10, 12, -14, 16])
}

@Test func emulatedDoubleMatrixTransposeSwapsColumnsAndRows() {
    expectMatrix(
        EmulatedDouble2x3Transpose(EmulatedDouble2x3Make(vec3(1, 2, 3), vec3(4, 5, 6))),
        [1, 4, 2, 5, 3, 6]
    )
    expectMatrix(
        EmulatedDouble4x3Transpose(EmulatedDouble4x3Make(vec3(1, 2, 3), vec3(4, 5, 6), vec3(7, 8, 9), vec3(10, 11, 12))),
        [1, 4, 7, 10, 2, 5, 8, 11, 3, 6, 9, 12]
    )
    expectMatrix(
        EmulatedDouble4x4Transpose(EmulatedDouble4x4Make(vec4(1, 2, 3, 4), vec4(5, 6, 7, 8), vec4(9, 10, 11, 12), vec4(13, 14, 15, 16))),
        [1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16]
    )
}

@Test func emulatedDoubleMatrixVectorMultiplicationMatchesColumnMajorRules() {
    let matrix = EmulatedDouble2x3Make(vec3(1, 2, 3), vec3(4, 5, 6))
    expectVector(EmulatedDouble2x3MultiplyVector(matrix, vec2(7, 8)), [39, 54, 69])
    expectVector(EmulatedDouble2x3LeftMultiplyVector(vec3(7, 8, 9), matrix), [50, 122])

    let matrix4 = EmulatedDouble4x2Make(vec2(1, 2), vec2(3, 4), vec2(5, 6), vec2(7, 8))
    expectVector(EmulatedDouble4x2MultiplyVector(matrix4, vec4(1, 2, 3, 4)), [50, 60])
    expectVector(EmulatedDouble4x2LeftMultiplyVector(vec2(9, 10), matrix4), [29, 67, 105, 143])
}

@Test func emulatedDoubleMatrixMatrixMultiplicationCoversCompatibleShapes() {
    expectMatrix(EmulatedDouble2x2MultiplyEmulatedDouble2x2(EmulatedDouble2x2Filled(ed(1)), EmulatedDouble2x2Filled(ed(1))), Array(repeating: 2, count: 4))
    expectMatrix(EmulatedDouble2x2MultiplyEmulatedDouble3x2(EmulatedDouble2x2Filled(ed(1)), EmulatedDouble3x2Filled(ed(1))), Array(repeating: 2, count: 6))
    expectMatrix(EmulatedDouble2x2MultiplyEmulatedDouble4x2(EmulatedDouble2x2Filled(ed(1)), EmulatedDouble4x2Filled(ed(1))), Array(repeating: 2, count: 8))
    expectMatrix(EmulatedDouble2x3MultiplyEmulatedDouble2x2(EmulatedDouble2x3Filled(ed(1)), EmulatedDouble2x2Filled(ed(1))), Array(repeating: 2, count: 6))
    expectMatrix(EmulatedDouble2x3MultiplyEmulatedDouble3x2(EmulatedDouble2x3Filled(ed(1)), EmulatedDouble3x2Filled(ed(1))), Array(repeating: 2, count: 9))
    expectMatrix(EmulatedDouble2x3MultiplyEmulatedDouble4x2(EmulatedDouble2x3Filled(ed(1)), EmulatedDouble4x2Filled(ed(1))), Array(repeating: 2, count: 12))
    expectMatrix(EmulatedDouble2x4MultiplyEmulatedDouble2x2(EmulatedDouble2x4Filled(ed(1)), EmulatedDouble2x2Filled(ed(1))), Array(repeating: 2, count: 8))
    expectMatrix(EmulatedDouble2x4MultiplyEmulatedDouble3x2(EmulatedDouble2x4Filled(ed(1)), EmulatedDouble3x2Filled(ed(1))), Array(repeating: 2, count: 12))
    expectMatrix(EmulatedDouble2x4MultiplyEmulatedDouble4x2(EmulatedDouble2x4Filled(ed(1)), EmulatedDouble4x2Filled(ed(1))), Array(repeating: 2, count: 16))
    expectMatrix(EmulatedDouble3x2MultiplyEmulatedDouble2x3(EmulatedDouble3x2Filled(ed(1)), EmulatedDouble2x3Filled(ed(1))), Array(repeating: 3, count: 4))
    expectMatrix(EmulatedDouble3x2MultiplyEmulatedDouble3x3(EmulatedDouble3x2Filled(ed(1)), EmulatedDouble3x3Filled(ed(1))), Array(repeating: 3, count: 6))
    expectMatrix(EmulatedDouble3x2MultiplyEmulatedDouble4x3(EmulatedDouble3x2Filled(ed(1)), EmulatedDouble4x3Filled(ed(1))), Array(repeating: 3, count: 8))
    expectMatrix(EmulatedDouble3x3MultiplyEmulatedDouble2x3(EmulatedDouble3x3Filled(ed(1)), EmulatedDouble2x3Filled(ed(1))), Array(repeating: 3, count: 6))
    expectMatrix(EmulatedDouble3x3MultiplyEmulatedDouble3x3(EmulatedDouble3x3Filled(ed(1)), EmulatedDouble3x3Filled(ed(1))), Array(repeating: 3, count: 9))
    expectMatrix(EmulatedDouble3x3MultiplyEmulatedDouble4x3(EmulatedDouble3x3Filled(ed(1)), EmulatedDouble4x3Filled(ed(1))), Array(repeating: 3, count: 12))
    expectMatrix(EmulatedDouble3x4MultiplyEmulatedDouble2x3(EmulatedDouble3x4Filled(ed(1)), EmulatedDouble2x3Filled(ed(1))), Array(repeating: 3, count: 8))
    expectMatrix(EmulatedDouble3x4MultiplyEmulatedDouble3x3(EmulatedDouble3x4Filled(ed(1)), EmulatedDouble3x3Filled(ed(1))), Array(repeating: 3, count: 12))
    expectMatrix(EmulatedDouble3x4MultiplyEmulatedDouble4x3(EmulatedDouble3x4Filled(ed(1)), EmulatedDouble4x3Filled(ed(1))), Array(repeating: 3, count: 16))
    expectMatrix(EmulatedDouble4x2MultiplyEmulatedDouble2x4(EmulatedDouble4x2Filled(ed(1)), EmulatedDouble2x4Filled(ed(1))), Array(repeating: 4, count: 4))
    expectMatrix(EmulatedDouble4x2MultiplyEmulatedDouble3x4(EmulatedDouble4x2Filled(ed(1)), EmulatedDouble3x4Filled(ed(1))), Array(repeating: 4, count: 6))
    expectMatrix(EmulatedDouble4x2MultiplyEmulatedDouble4x4(EmulatedDouble4x2Filled(ed(1)), EmulatedDouble4x4Filled(ed(1))), Array(repeating: 4, count: 8))
    expectMatrix(EmulatedDouble4x3MultiplyEmulatedDouble2x4(EmulatedDouble4x3Filled(ed(1)), EmulatedDouble2x4Filled(ed(1))), Array(repeating: 4, count: 6))
    expectMatrix(EmulatedDouble4x3MultiplyEmulatedDouble3x4(EmulatedDouble4x3Filled(ed(1)), EmulatedDouble3x4Filled(ed(1))), Array(repeating: 4, count: 9))
    expectMatrix(EmulatedDouble4x3MultiplyEmulatedDouble4x4(EmulatedDouble4x3Filled(ed(1)), EmulatedDouble4x4Filled(ed(1))), Array(repeating: 4, count: 12))
    expectMatrix(EmulatedDouble4x4MultiplyEmulatedDouble2x4(EmulatedDouble4x4Filled(ed(1)), EmulatedDouble2x4Filled(ed(1))), Array(repeating: 4, count: 8))
    expectMatrix(EmulatedDouble4x4MultiplyEmulatedDouble3x4(EmulatedDouble4x4Filled(ed(1)), EmulatedDouble3x4Filled(ed(1))), Array(repeating: 4, count: 12))
    expectMatrix(EmulatedDouble4x4MultiplyEmulatedDouble4x4(EmulatedDouble4x4Filled(ed(1)), EmulatedDouble4x4Filled(ed(1))), Array(repeating: 4, count: 16))
}

@Test func emulatedDoubleMatrixDeterminantsMatchKnownValues() {
    #expect(double(EmulatedDouble2x2Determinant(EmulatedDouble2x2Make(vec2(1, 3), vec2(2, 4)))) == -2)
    #expect(double(EmulatedDouble3x3Determinant(EmulatedDouble3x3Make(vec3(6, 4, 2), vec3(1, -2, 8), vec3(1, 5, 7)))) == -306)
    #expect(double(EmulatedDouble4x4Determinant(EmulatedDouble4x4Diagonal(ed(3)))) == 81)
}

@Test func emulatedDoubleJacobiSVDReconstructsAllFixedMatrixShapes() {
    let matrix2x2 = EmulatedDouble2x2Make(vec2(1, 2), vec2(3, 4))
    expectSVDReconstructs(EmulatedDouble2x2JacobiSVD(matrix2x2), original: matrixArray(matrix2x2))

    let matrix2x3 = EmulatedDouble2x3Make(vec3(1, -2, 3), vec3(4, 0.5, -1))
    expectSVDReconstructs(EmulatedDouble2x3JacobiSVD(matrix2x3), original: matrixArray(matrix2x3))

    let matrix2x4 = EmulatedDouble2x4Make(vec4(1, 2, 3, 4), vec4(-2, 1, 0.5, -1))
    expectSVDReconstructs(EmulatedDouble2x4JacobiSVD(matrix2x4), original: matrixArray(matrix2x4))

    let matrix3x2 = EmulatedDouble3x2Make(vec2(1, 2), vec2(3, -1), vec2(0.5, 4))
    expectSVDReconstructs(EmulatedDouble3x2JacobiSVD(matrix3x2), original: matrixArray(matrix3x2))

    let matrix3x3 = EmulatedDouble3x3Make(vec3(1, 2, 3), vec3(0, 4, 5), vec3(-1, 2, 6))
    expectSVDReconstructs(EmulatedDouble3x3JacobiSVD(matrix3x3), original: matrixArray(matrix3x3))

    let matrix3x4 = EmulatedDouble3x4Make(vec4(1, 2, 3, 4), vec4(-2, 0.5, 1, -1), vec4(3, -1, 2, 0))
    expectSVDReconstructs(EmulatedDouble3x4JacobiSVD(matrix3x4), original: matrixArray(matrix3x4))

    let matrix4x2 = EmulatedDouble4x2Make(vec2(1, 2), vec2(3, -1), vec2(0.5, 4), vec2(-2, 0.25))
    expectSVDReconstructs(EmulatedDouble4x2JacobiSVD(matrix4x2), original: matrixArray(matrix4x2))

    let matrix4x3 = EmulatedDouble4x3Make(vec3(1, 2, 3), vec3(0, 4, -1), vec3(5, -2, 2), vec3(1.5, 0.5, -3))
    expectSVDReconstructs(EmulatedDouble4x3JacobiSVD(matrix4x3), original: matrixArray(matrix4x3))

    let matrix4x4 = EmulatedDouble4x4Make(vec4(1, 2, 3, 4), vec4(0, 4, 5, 6), vec4(-1, 2, 6, 0.5), vec4(3, -2, 1, 2))
    expectSVDReconstructs(EmulatedDouble4x4JacobiSVD(matrix4x4), original: matrixArray(matrix4x4))
}

@Test func emulatedDoubleJacobiSVDSortsSingularValuesDescending() {
    let matrix = EmulatedDouble4x4Make(
        vec4(1, 0, 0, 0),
        vec4(0, 4, 0, 0),
        vec4(0, 0, 2, 0),
        vec4(0, 0, 0, 3)
    )
    let svd = EmulatedDouble4x4JacobiSVD(matrix)

    expectVectorApprox(svd.s, [4, 3, 2, 1], tolerance: 1.0e-10)
    expectSVDReconstructs(svd, original: matrixArray(matrix), tolerance: 1.0e-10)
}

@Test func emulatedDoubleJacobiSVDHandlesZeroMatrix() {
    let matrix = EmulatedDouble3x4Filled(ed(0))
    let svd = EmulatedDouble3x4JacobiSVD(matrix)

    expectVector(svd.s, [0, 0, 0])
    expectMatrix(svd.u, Array(repeating: 0, count: 12))
    expectSVDReconstructs(svd, original: matrixArray(matrix), tolerance: 0)
}

private func ed(_ value: Double) -> EmulatedDouble {
    value.toEmulatedDouble()
}

private func bitPattern(_ value: EmulatedDouble) -> UInt64 {
    (UInt64(value.highBits) << 32) | UInt64(value.lowBits)
}

private func double(_ value: EmulatedDouble) -> Double {
    Double(bitPattern: bitPattern(value))
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

private func expectVectorApprox(_ actual: EmulatedDouble4, _ expected: [Double], tolerance: Double) {
    #expect(abs(double(actual.x) - expected[0]) <= tolerance)
    #expect(abs(double(actual.y) - expected[1]) <= tolerance)
    #expect(abs(double(actual.z) - expected[2]) <= tolerance)
    #expect(abs(double(actual.w) - expected[3]) <= tolerance)
}

private func expectMatrix(_ actual: EmulatedDouble2x2, _ expected: [Double]) {
    expectVector(actual.c0, Array(expected[0..<2]))
    expectVector(actual.c1, Array(expected[2..<4]))
}

private func expectMatrix(_ actual: EmulatedDouble2x3, _ expected: [Double]) {
    expectVector(actual.c0, Array(expected[0..<3]))
    expectVector(actual.c1, Array(expected[3..<6]))
}

private func expectMatrix(_ actual: EmulatedDouble2x4, _ expected: [Double]) {
    expectVector(actual.c0, Array(expected[0..<4]))
    expectVector(actual.c1, Array(expected[4..<8]))
}

private func expectMatrix(_ actual: EmulatedDouble3x2, _ expected: [Double]) {
    expectVector(actual.c0, Array(expected[0..<2]))
    expectVector(actual.c1, Array(expected[2..<4]))
    expectVector(actual.c2, Array(expected[4..<6]))
}

private func expectMatrix(_ actual: EmulatedDouble3x3, _ expected: [Double]) {
    expectVector(actual.c0, Array(expected[0..<3]))
    expectVector(actual.c1, Array(expected[3..<6]))
    expectVector(actual.c2, Array(expected[6..<9]))
}

private func expectMatrix(_ actual: EmulatedDouble3x4, _ expected: [Double]) {
    expectVector(actual.c0, Array(expected[0..<4]))
    expectVector(actual.c1, Array(expected[4..<8]))
    expectVector(actual.c2, Array(expected[8..<12]))
}

private func expectMatrix(_ actual: EmulatedDouble4x2, _ expected: [Double]) {
    expectVector(actual.c0, Array(expected[0..<2]))
    expectVector(actual.c1, Array(expected[2..<4]))
    expectVector(actual.c2, Array(expected[4..<6]))
    expectVector(actual.c3, Array(expected[6..<8]))
}

private func expectMatrix(_ actual: EmulatedDouble4x3, _ expected: [Double]) {
    expectVector(actual.c0, Array(expected[0..<3]))
    expectVector(actual.c1, Array(expected[3..<6]))
    expectVector(actual.c2, Array(expected[6..<9]))
    expectVector(actual.c3, Array(expected[9..<12]))
}

private func expectMatrix(_ actual: EmulatedDouble4x4, _ expected: [Double]) {
    expectVector(actual.c0, Array(expected[0..<4]))
    expectVector(actual.c1, Array(expected[4..<8]))
    expectVector(actual.c2, Array(expected[8..<12]))
    expectVector(actual.c3, Array(expected[12..<16]))
}

private func vectorArray(_ value: EmulatedDouble2) -> [Double] {
    [double(value.x), double(value.y)]
}

private func vectorArray(_ value: EmulatedDouble3) -> [Double] {
    [double(value.x), double(value.y), double(value.z)]
}

private func vectorArray(_ value: EmulatedDouble4) -> [Double] {
    [double(value.x), double(value.y), double(value.z), double(value.w)]
}

private func matrixArray(_ value: EmulatedDouble2x2) -> [Double] {
    vectorArray(value.c0) + vectorArray(value.c1)
}

private func matrixArray(_ value: EmulatedDouble2x3) -> [Double] {
    vectorArray(value.c0) + vectorArray(value.c1)
}

private func matrixArray(_ value: EmulatedDouble2x4) -> [Double] {
    vectorArray(value.c0) + vectorArray(value.c1)
}

private func matrixArray(_ value: EmulatedDouble3x2) -> [Double] {
    vectorArray(value.c0) + vectorArray(value.c1) + vectorArray(value.c2)
}

private func matrixArray(_ value: EmulatedDouble3x3) -> [Double] {
    vectorArray(value.c0) + vectorArray(value.c1) + vectorArray(value.c2)
}

private func matrixArray(_ value: EmulatedDouble3x4) -> [Double] {
    vectorArray(value.c0) + vectorArray(value.c1) + vectorArray(value.c2)
}

private func matrixArray(_ value: EmulatedDouble4x2) -> [Double] {
    vectorArray(value.c0) + vectorArray(value.c1) + vectorArray(value.c2) + vectorArray(value.c3)
}

private func matrixArray(_ value: EmulatedDouble4x3) -> [Double] {
    vectorArray(value.c0) + vectorArray(value.c1) + vectorArray(value.c2) + vectorArray(value.c3)
}

private func matrixArray(_ value: EmulatedDouble4x4) -> [Double] {
    vectorArray(value.c0) + vectorArray(value.c1) + vectorArray(value.c2) + vectorArray(value.c3)
}

private func expectSVDReconstructs(_ svd: EmulatedDouble2x2SVD, original: [Double], tolerance: Double = 1.0e-5) {
    expectSVDReconstructs(u: matrixArray(svd.u), s: vectorArray(svd.s), v: matrixArray(svd.v), original: original, rows: 2, columns: 2, tolerance: tolerance)
}

private func expectSVDReconstructs(_ svd: EmulatedDouble2x3SVD, original: [Double], tolerance: Double = 1.0e-5) {
    expectSVDReconstructs(u: matrixArray(svd.u), s: vectorArray(svd.s), v: matrixArray(svd.v), original: original, rows: 3, columns: 2, tolerance: tolerance)
}

private func expectSVDReconstructs(_ svd: EmulatedDouble2x4SVD, original: [Double], tolerance: Double = 1.0e-5) {
    expectSVDReconstructs(u: matrixArray(svd.u), s: vectorArray(svd.s), v: matrixArray(svd.v), original: original, rows: 4, columns: 2, tolerance: tolerance)
}

private func expectSVDReconstructs(_ svd: EmulatedDouble3x2SVD, original: [Double], tolerance: Double = 1.0e-5) {
    expectSVDReconstructs(u: matrixArray(svd.u), s: vectorArray(svd.s), v: matrixArray(svd.v), original: original, rows: 2, columns: 3, tolerance: tolerance)
}

private func expectSVDReconstructs(_ svd: EmulatedDouble3x3SVD, original: [Double], tolerance: Double = 1.0e-5) {
    expectSVDReconstructs(u: matrixArray(svd.u), s: vectorArray(svd.s), v: matrixArray(svd.v), original: original, rows: 3, columns: 3, tolerance: tolerance)
}

private func expectSVDReconstructs(_ svd: EmulatedDouble3x4SVD, original: [Double], tolerance: Double = 1.0e-5) {
    expectSVDReconstructs(u: matrixArray(svd.u), s: vectorArray(svd.s), v: matrixArray(svd.v), original: original, rows: 4, columns: 3, tolerance: tolerance)
}

private func expectSVDReconstructs(_ svd: EmulatedDouble4x2SVD, original: [Double], tolerance: Double = 1.0e-5) {
    expectSVDReconstructs(u: matrixArray(svd.u), s: vectorArray(svd.s), v: matrixArray(svd.v), original: original, rows: 2, columns: 4, tolerance: tolerance)
}

private func expectSVDReconstructs(_ svd: EmulatedDouble4x3SVD, original: [Double], tolerance: Double = 1.0e-5) {
    expectSVDReconstructs(u: matrixArray(svd.u), s: vectorArray(svd.s), v: matrixArray(svd.v), original: original, rows: 3, columns: 4, tolerance: tolerance)
}

private func expectSVDReconstructs(_ svd: EmulatedDouble4x4SVD, original: [Double], tolerance: Double = 1.0e-5) {
    expectSVDReconstructs(u: matrixArray(svd.u), s: vectorArray(svd.s), v: matrixArray(svd.v), original: original, rows: 4, columns: 4, tolerance: tolerance)
}

private func expectSVDReconstructs(
    u: [Double],
    s: [Double],
    v: [Double],
    original: [Double],
    rows: Int,
    columns: Int,
    tolerance: Double
) {
    for column in 0..<columns {
        for row in 0..<rows {
            var reconstructed = 0.0
            for singularIndex in 0..<columns {
                reconstructed += u[singularIndex * rows + row] * s[singularIndex] * v[singularIndex * columns + column]
            }
            let expected = original[column * rows + row]
            #expect(abs(reconstructed - expected) <= tolerance)
        }
    }
}
