/* 
 * CS:APP Data Lab 
 * 
 * Name: Laura (Kai Sze) Luk
 * AndrewID: kluk
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  long Funct(long arg1, long arg2, ...) {
      /* brief description of how your implementation works */
      long var1 = Expr1;
      ...
      long varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. (Long) integer constants 0 through 255 (0xFFL), inclusive. You are
      not allowed to use big constants such as 0xffffffffL.
  3. Function arguments and local variables (no global variables).
  4. Local variables of type int and long
  5. Unary integer operations ! ~
     - Their arguments can have types int or long
     - Note that ! always returns int, even if the argument is long
  6. Binary integer operations & ^ | + << >>
     - Their arguments can have types int or long
  7. Casting from int to long and from long to int
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting other than between int and long.
  7. Use any data type other than int or long.  This implies that you
     cannot use arrays, structs, or unions.
 
  You may assume that your machine:
  1. Uses 2s complement representations for int and long.
  2. Data type int is 32 bits, long is 64.
  3. Performs right shifts arithmetically.
  4. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31 (int) or 63 (long)

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 63
   */
  long pow2plus1(long x) {
     /* exploit ability of shifts to compute powers of 2 */
     /* Note that the 'L' indicates a long constant */
     return (1L << x) + 1L;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 63
   */
  long pow2plus4(long x) {
     /* exploit ability of shifts to compute powers of 2 */
     long result = (1L << x);
     result += 4L;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2012 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* We do support the IEC 559 math functionality, real and complex.  */
/* wchar_t uses ISO/IEC 10646 (2nd ed., published 2011-03-15) /
   Unicode 6.0.  */
/* We do not support C11 <threads.h>.  */


// 1
/*
 * bitMatch - Create mask indicating which bits in x match those in y
 *            using only ~ and &
 *   Example: bitMatch(0x7L, 0xEL) = 0xFFFFFFFFFFFFFFF6L
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
long bitMatch(long x, long y) {
  // Using the NAND operation to create a negated XOR operation.
  // In expression form:
  // (A NAND (A NAND B)) NAND (B NAND (A NAND B)).

  long nand = ~(x & y);
  long first = ~(x & nand);
  long second = ~(y & nand);
  return first & second;
}


// 2
/* 
 * copyLSB - set all bits of result to least significant bit of x
 *   Examples:
 *     copyLSB(5L) = 0xFFFFFFFFFFFFFFFFL,
 *     copyLSB(6L) = 0x0000000000000000L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
long copyLSB(long x) {
  // Utilize arithmetic right shift to copy over LSB.

  return (x << 63) >> 63;
}


// 3
/*
 * distinctNegation - returns 1 if x != -x.
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 5
 *   Rating: 2
 */
long distinctNegation(long x) {
  // Using XOR operation to determine if negation of x has same bits as x.
  // Uses ! to transform any non-zero value to all 0's, to negate again.

  return !!(x ^ (~x + 0x1L));
}


// 4
/* 
 * leastBitPos - return a mask that marks the position of the
 *               least significant 1 bit. If x == 0, return 0
 *   Example: leastBitPos(96L) = 0x20L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2 
 */
long leastBitPos(long x) {
  // A 2's complement representation for negation conceptually moves the 
  // least significant 1 bit to the right by carries/overflow.

  return x & (~x + 0x1L);
}


// 5
/* 
 * dividePower2 - Compute x/(2^n), for 0 <= n <= 62
 *  Round toward zero
 *   Examples: dividePower2(15L,1L) = 7L, dividePower2(-33L,4L) = -2L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
long dividePower2(long x, long n) {
  // Inspiration from Lecture 3: to round right shift to 0 for negative  umbers.
  // Adds bias (non-zero if x is negative) before shift.

  long sign = !!((x >> 63) << 63);
  long bias = ~(~(sign << n) + sign);
  return (x + bias) >> n;
}


// 6
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4L,5L) = 4L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
long conditional(long x, long y, long z) {
  // Obtain truth value of x, represented as a long int.
  // Either side of the OR statement will be 0x00, for respective result.

  long truth = ((long)(!!x) << 63) >> 63;
  return (truth & y) | (~truth & z);
}


// 7
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4L,5L) = 1L.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
long isLessOrEqual(long x, long y) {
  // Checks if x is negative and they are different signs.
  // Checks sign of difference if x and y are same signs.

  long diff = !((y + (~x + 1)) >> 63); 
  long sign = (!!(x >> 63)) ^ (!!(y >> 63));

  return (sign & (x >> 63)) | ((!sign) & diff);
}


// 8
/*
 * trueThreeFourths - multiplies by 3/4 rounding toward 0,
 *   avoiding errors due to overflow
 *   Examples:
 *    trueThreeFourths(11L) = 8
 *    trueThreeFourths(-9L) = -6
 *    trueThreeFourths(4611686018427387904L) = 3458764513820540928L (no overflow)
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
long trueThreeFourths(long x) {
  // Separately calculate 3/4 of (x/4) and (x % 4). 
  // Ceiling of (3/4)*remainder if x < 0 (Lecture 3).

  long mod = x & 0x03L;
  long sign = !!((x >> 63) << 63);
  long bias = ~(~(sign << 2) + sign);

  mod = (mod + mod + mod + bias) >> 2;

  x = x >> 2;
  return x + x + x + mod;
}


// 9
/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5L) = 2, bitCount(7L) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 50
 *   Rating: 4
 */
long bitCount(long x) {
  // Divide and conquer method (Recitation 1).

  long FFFF = (0xFFL << 8) | 0xFFL;
  long mask6 = (FFFF << 16) | FFFF;
  long mask5 = (mask6 << 16) ^ mask6;
  long mask4 = (mask5 << 8) ^ mask5;
  long mask3 = (mask4 << 4) ^ mask4;
  long mask2 = (mask3 << 2) ^ mask3;
  long mask1 = (mask2 << 1) ^ mask2;
  
  long new = (x & mask1) + ((x >> 1) & mask1);
  new = (new & mask2) + ((new >> 2) & mask2);
  new = (new & mask3) + ((new >> 4) & mask3);
  new = (new & mask4) + ((new >> 8) & mask4);
  new = (new & mask5) + ((new >> 16) & mask5);
  
  return (new & mask6) + ((new >> 32) & mask6);
}


// 10
/*
 * isPalindrome - Return 1 if bit pattern in x is equal to its mirror image
 *   Example: isPalindrome(0x6F0F0123c480F0F6L) = 1L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 70
 *   Rating: 4
 */
long isPalindrome(long x) {
  // Swap halves of given bit value.
  // Repeatedly half bit group being swapped until 1 bit.

  long FFFF = (0xFFL << 8) | 0xFFL;
  long mask6 = (FFFF << 16) | FFFF;
  long mask5 = (mask6 << 16) ^ mask6;
  long mask4 = (mask5 << 8) ^ mask5;
  long mask3 = (mask4 << 4) ^ mask4;
  long mask2 = (mask3 << 2) ^ mask3;
  long mask1 = (mask2 << 1) ^ mask2;

  long rev = x;
  rev = ((rev & mask6) << 32) | (((rev & (mask6 << 32)) >> 32) & mask6);
  rev = ((rev & mask5) << 16) | (((rev & (mask5 << 16)) >> 16) & mask5);
  rev = ((rev & mask4) << 8) | (((rev & (mask4 << 8)) >> 8) & mask4);
  rev = ((rev & mask3) << 4) | (((rev & (mask3 << 4)) >> 4) & mask3);
  rev = ((rev & mask2) << 2) | (((rev & (mask2 << 2)) >> 2) & mask2);
  rev = ((rev & mask1) << 1) | (((rev & (mask1 << 1)) >> 1) & mask1);

  return !(rev ^ x);
}


// 11
/*
 * integerLog2 - return floor(log base 2 of x), where x > 0
 *   Example: integerLog2(16L) = 4L, integerLog2(31L) = 4L
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 60
 *   Rating: 4
 */
long integerLog2(long x) {
  // Repeatedly shift by multiples of 2 to hone in on MSB 1-bit.

  long half = (!!(x >> 32)) << 5;
  half += (!!(x >> (half + 16))) << 4;
  half += (!!(x >> (half + 8))) << 3;
  half += (!!(x >> (half + 4))) << 2;
  half += (!!(x >> (half + 2))) << 1;
  half += (!!(x >> (half + 1)));
  return half;
}


//float

// 12
/* 
 * floatIsEqual - Compute f == g for floating point arguments f and g.
 *   Both the arguments are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   If either argument is NaN, return 0.
 *   +0 and -0 are considered equal.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 25
 *   Rating: 2
 */
int floatIsEqual(unsigned uf, unsigned ug) {
  // Edge cases of NaN should return 0.
  // Both +/- 0 should be equal. 

  if ((((uf >> 23) & 0xFF) == 0xFF) && ((uf & 0x007FFFFF) != 0)) return 0;
  if ((((ug >> 23) & 0xFF) == 0xFF) && ((ug & 0x007FFFFF) != 0)) return 0;
  
  if ((uf << 1 == 0) && (ug << 1 == 0)) return 1;

  return uf == ug;
}


// 13
/* 
 * floatScale4 - Return bit-level equivalent of expression 4*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale4(unsigned uf) {
  // Multiply float by 4 (left shift by 2), casing on normalized
  // and denormalized values.

  unsigned sign = uf & 0x80000000u;
  unsigned exp = (uf >> 23) & 0xFFu;
  unsigned frac = uf & 0x007FFFFFu;

  // Argument is NaN/infinity, or 0.
  if (exp == 0xFFu || (exp == 0u && frac == 0u)) return uf;

  // Argument is a normalized value.
  if (exp != 0u) {
    exp += 2u;
    if (exp >= 0xFFu) {
      exp = 0xFFu;
      frac = 0;
    }

  // Argument is denormalized.
  } else {
    if ((frac & 0x400000u) != 0u) {
      frac = (frac << 1) & 0x7FFFFF;
      exp += 2;
    } else {
      return sign | (frac << 2);
    }
  }
  return sign | (exp << 23) | frac;
}


// 14
/* 
 * floatUnsigned2Float - Return bit-level equivalent of expression (float) u
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatUnsigned2Float(unsigned u) {
  // Obtain exponent bit value.
  // Obtain fraction bit value, rounding if too many bits.

  unsigned int exp = 0u;
  unsigned int frac = 0u;
  unsigned int x = u;
  unsigned int g;
  unsigned int r;
  unsigned int s;
  
  // Obtain exponent value.
  while (x > 0x1u) {
    exp += 1u;
    x = x >> 1;
  }

  // Mask first 23 bits for fractional value.
  if (exp > 23u) {
    frac = (u >> (exp - 23u)) & 0x7FFFFFu;

    // Masks for Guard, Round, and Sticky values.
    g = (0x100u >> (31u - exp)) & u;
    r = (0x80u >> (31u - exp)) & u;
    s = (0xFFu >> (32u - exp)) & u;

    // Round when appropriate.
    if ((r && s) || (r && g)) {
      frac += 1u;
      
      if (frac > 0x7FFFFF) {
        exp += 1u;
        frac = 0u;
      }
    }

  } else {
    frac = (u << (23u - exp)) & 0x7FFFFFu;
  }
  
  if (u > 0u) exp += 127u;

  return (exp << 23) | frac;
}