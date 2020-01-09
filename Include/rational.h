
/*
  Copyright (c) 2020, Jonathan Noble
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

#ifndef RATIONAL_H
#define RATIONAL_H

#include <string>
#include <stdint.h>
#include <cassert>
#include <stdexcept>
#include <ostream>


namespace media_handling
{
  //TODO: perhaps template the types of numerator + denominator
  class Rational
  {
    public:
      Rational() = default;

      Rational& operator=(const Rational& rhs) noexcept
      {
        if (this != &rhs) {
          numerator_ = rhs.numerator_;
          denominator_ = rhs.denominator_;
        }
        return *this;
      }

      Rational& operator*=(const Rational& rhs) noexcept
      {
        *this = *this * rhs;
        const auto div = gcd();
        if (div > 1) {
          numerator_ /= div;
          denominator_ /= div;
        }
        return *this;
      }

      Rational& operator*=(const int32_t rhs) noexcept
      {
        return operator*=(Rational(rhs, 1));
      }

      Rational& operator/=(const Rational& rhs) noexcept
      {
        *this = *this / rhs;
        const auto div = gcd();
        if (div > 1) {
          numerator_ /= div;
          denominator_ /= div;
        }
        return *this;
      }

      Rational& operator/=(const int32_t rhs) noexcept
      {
        return operator/=(Rational(rhs, 1));
      }

      bool operator>(const Rational& rhs) const noexcept
      {
        const auto a = numerator_ * rhs.denominator_;
        const auto b = rhs.numerator_ * denominator_;
        return a > b;
      }
      bool operator<(const Rational& rhs) const noexcept
      {
        const auto a = numerator_ * rhs.denominator_;
        const auto b = rhs.numerator_ * denominator_;
        return a < b;
      }
      bool operator==(const Rational& rhs) const noexcept
      {
        return (numerator_ == rhs.numerator_) && (denominator_ == rhs.denominator_);
      }
      bool operator!=(const Rational& rhs) const noexcept
      {
        return !operator==(rhs);
      }
      Rational(const int64_t num, const int64_t denom)
        : numerator_(num), denominator_(denom)
      {
        if (denom == 0) {
          throw std::runtime_error("Denominator of Rational is zero");
        }
        const auto div = gcd();
        if (div > 1) {
          numerator_ /= div;
          denominator_ /= div;
        }
      }
      Rational invert() const noexcept
      {
        if (numerator_ == 0) {
          return {0, 1};
        }
        return {denominator_, numerator_};
      }
      constexpr int64_t numerator() const noexcept {return numerator_;}
      constexpr int64_t denominator() const noexcept {return denominator_;}
      constexpr double toDouble() const noexcept
      {
        assert(denominator_ != 0);
        return static_cast<double>(numerator_) / static_cast<double>(denominator_);
      }

      friend std::ostream& operator<<(std::ostream& os, const Rational& rhs)
      {
        os << '(' << rhs.numerator() << '/' << rhs.denominator() << ')';
        return os;
      }
      friend Rational operator*(const Rational& lhs, const Rational& rhs) noexcept
      {
        return {lhs.numerator_ * rhs.numerator_, lhs.denominator_ * rhs.denominator_};
      }
      friend Rational operator*(const Rational& lhs, const int32_t value) noexcept
      {
        return {lhs.numerator_ * value, lhs.denominator_};
      }
      friend Rational operator*(const int32_t value, const Rational& rhs) noexcept
      {
        return rhs * value;
      }
      friend Rational operator/(const Rational& lhs, const Rational& rhs) noexcept
      {
        return lhs * rhs.invert();
      }
      friend Rational operator/(const Rational& lhs, const int32_t value) noexcept
      {
        return lhs * Rational(1, value);
      }
      friend Rational operator/(const int32_t value, const Rational& rhs) noexcept
      {
        return Rational(value, 1) * rhs.invert();
      }
      friend Rational operator+(const Rational& lhs, const Rational& rhs) noexcept
      {
        const auto n = (lhs.numerator_ * rhs.denominator_) + (rhs.numerator_ * lhs.denominator_);
        const auto d = lhs.denominator_ * rhs.denominator_;
        return {n, d};
      }
      friend Rational operator+(const Rational& lhs, const int32_t value) noexcept
      {
        return lhs + Rational(value, 1);
      }
      friend Rational operator+(const int32_t value, const Rational& rhs) noexcept
      {
        return Rational(value, 1) + rhs;
      }
      friend Rational operator-(const Rational& lhs, const Rational& rhs) noexcept
      {
        const auto n = (lhs.numerator_ * rhs.denominator_) - (rhs.numerator_ * lhs.denominator_);
        const auto d = lhs.denominator_ * rhs.denominator_;
        return {n, d};
      }
      friend Rational operator-(const Rational& lhs, const int32_t value) noexcept
      {
        return lhs - Rational(value, 1);
      }
      friend Rational operator-(const int32_t value, const Rational& rhs) noexcept
      {
        return Rational(value, 1) - rhs;
      }

    private:
      int64_t numerator_ {0};
      int64_t denominator_ {0};
      int64_t gcd ()
      {
        // FIXME: brute forced
        for (auto i = denominator_; i > 0; --i) {
          if ( ((numerator_ % i) == 0) && ((denominator_ % i) == 0) ) {
            return i;
          }
        }
        return 1;
      }
  };
}
#endif
