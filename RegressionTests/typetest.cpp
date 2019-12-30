/*
  Copyright (c) 2019, Jonathan Noble
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <gtest/gtest.h>

#include "types.h"

using namespace media_handling;

TEST (TypeTest, RationalEquals)
{
  Rational a {1, 1};
  Rational b {2, 2};

  ASSERT_TRUE(a == b);
  ASSERT_TRUE(a == Rational(1, 1));

  ASSERT_EQ(Rational(100,100), Rational(1,1));
}

TEST (TypeTest, RationalGT)
{
  Rational a {1, 1};
  Rational b {1, 2};
  ASSERT_TRUE(a > b);

  Rational c (2, 2);
  ASSERT_FALSE(c > a);
}

TEST (TypeTest, RationalLT)
{
  Rational a {1, 1};
  Rational b {1, 2};
  ASSERT_TRUE(b < a);

  Rational c (2, 2);
  ASSERT_FALSE(c > a);
}

TEST (TypeTest, RationalMultiplied)
{
  Rational a {1, 3};
  Rational b {1, 2};
  auto c = a * b;
  ASSERT_EQ(c, Rational(1, 6));
  a *= b;
  ASSERT_EQ(a, c);

  a *= 2;
  ASSERT_EQ(a, Rational(1, 3));

  Rational d {2, 5};
  Rational e {4, 9};
  auto f = d * e;
  ASSERT_EQ(f, Rational(8, 45));

  Rational g {3, 4};
  auto h = d * g;
  ASSERT_EQ(h, Rational(3, 10)); // 6/20

  auto i = g * 2;
  ASSERT_EQ(i, Rational(3, 2)); // 6/4
  auto j = 2 * g;
  ASSERT_EQ(i, j);

}

TEST (TypeTest, RationalAddition)
{
  Rational a {1,1};
  auto b = a + a;
  ASSERT_EQ(b, Rational(2,1));
  Rational c {1, 2};
  auto d = c + 1;
  ASSERT_EQ(d, Rational(3,2));
  auto e = 1 + c;
  ASSERT_EQ(d, e);
}

TEST (TypeTest, RationalSubtraction)
{
  Rational a {1, 2};
  Rational b {1, 4};
  auto c = a - b;
  ASSERT_EQ(c, Rational(1, 4));
  auto d = b - a;
  ASSERT_EQ(d, Rational(-1, 4));

  Rational e {20, 3};
  auto f = e - 1;
  ASSERT_EQ(f, Rational(17, 3));
  auto g = 1 - e;
  ASSERT_EQ(g, Rational(-17, 3));
}

TEST (TypeTest, RationalDivision)
{
  Rational a {1, 2};
  Rational b {1, 4};
  auto c = a / b;
  ASSERT_EQ(c, Rational(2, 1));
  auto d = 2 / a;
  ASSERT_EQ(d, Rational(4, 1));
  auto e = a / 2;
  ASSERT_EQ(e, Rational(1, 4));

  a /= 2;
  ASSERT_EQ(a, Rational(1, 4));

}

//TEST (TypeTest, RationalGCD)
//{
//  Rational a {10,20};
//  ASSERT_EQ(a.gcd(), 10);

//  Rational b {123, 456};
//  ASSERT_EQ(b.gcd(), 3);

//  Rational c {7907, 7919};
//  ASSERT_EQ(c.gcd(), 1);
//}
