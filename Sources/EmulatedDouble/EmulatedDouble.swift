//
//  EmulatedDouble.swift
//  EmulatedDouble
//
//  Created by Yuki Kuwashima on 2026/06/19.
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

@_exported import EmulatedDoubleCore

public extension Double {
    /// Converts this `Double` to an `EmulatedDouble` without changing its IEEE 754 binary64 bit pattern.
    ///
    /// The high 32 bits of `Double.bitPattern` become `EmulatedDouble.highBits`, and the low 32 bits
    /// become `EmulatedDouble.lowBits`. This preserves finite values, signed zeroes, infinities,
    /// subnormals, and NaN payload/sign bits exactly.
    ///
    /// Quality: Covered by `doubleToEmulatedDoublePreservesBinary64Bits`.
    func toEmulatedDouble() -> EmulatedDouble {
        EmulatedDoubleMake(
            UInt32((bitPattern >> 32) & 0xffff_ffff),
            UInt32(bitPattern & 0xffff_ffff)
        )
    }
}

public extension Float {
    /// Converts this `Float` to an exact `EmulatedDouble` representation.
    ///
    /// The IEEE 754 binary32 bit pattern is widened into the corresponding binary64 encoding.
    /// Finite values, signed zeroes, infinities, subnormals, and NaN payload/sign bits follow the
    /// shared `EmulatedDoubleFromFloat` conversion path.
    ///
    /// Quality: Covered by `floatToEmulatedDoubleMatchesCoreFloatConversion`.
    func toEmulatedDouble() -> EmulatedDouble {
        EmulatedDoubleFromFloat(self)
    }
}
