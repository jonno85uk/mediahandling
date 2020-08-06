
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
#include <fmt/core.h>
#include <math.h>

constexpr auto DOUBLE_FUDGER = 1'000'000;

namespace media_handling
{
  //TODO: perhaps template the types of numerator + denominator
  class EXPORT Rational
  {
    public:
      Rational() = default;
      explicit Rational(const int64_t num)
        : numerator_(num),
          denominator_(1LL)
      {

      }
      explicit Rational(const int32_t num)
        : numerator_(num),
          denominator_(1LL)
      {

      }
      explicit Rational(const double num)
        : Rational((llround(num * DOUBLE_FUDGER)),DOUBLE_FUDGER)
      {
        // FIXME: this is poor. do this properly
      }

      Rational(const int64_t num, const int64_t denom)
        : numerator_(num), denominator_(denom)
      {
        if (denom == 0LL) {
          throw std::runtime_error("Denominator of Rational is zero");
        }
        const auto div = gcd();
        if (div > 1LL) {
          numerator_ /= div;
          denominator_ /= div;
        }
      }

      Rational& operator=(const Rational& rhs) noexcept
      {
        if (this != &rhs) {
          numerator_ = rhs.numerator_;
          denominator_ = rhs.denominator_;
        }
        return *this;
      }
      template<typename T>
      Rational& operator=(const T value) noexcept
      {
        const auto tmp = Rational(value);
        numerator_ = tmp.numerator_;
        denominator_ = tmp.denominator_;
        return *this;
      }

    public:
      operator double() const noexcept
      {
        return toDouble();
      }

      operator int32_t() const noexcept
      {
        return static_cast<int32_t>(lround(toDouble()));
      }

      operator int64_t() const noexcept
      {
        return static_cast<int64_t>(llround(toDouble()));
      }

      Rational& operator*=(const Rational& rhs) noexcept
      {
        *this = *this * rhs;
        const auto div = gcd();
        if (div > 1LL) {
          numerator_ /= div;
          denominator_ /= div;
        }
        return *this;
      }

      template<typename T>
      Rational& operator*=(const T rhs) noexcept
      {
        return operator*=(Rational(rhs));
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

      template<typename T>
      Rational& operator/=(const T rhs) noexcept
      {
        return operator/=(Rational(rhs));
      }
      template<typename T>
      constexpr bool operator>=(const T& val) const noexcept
      {
        return toDouble() >= val;
      }
      template<typename T>
      constexpr bool operator>(const T& val) const noexcept
      {
        return toDouble() > val;
      }
      template<typename T>
      constexpr bool operator<=(const T& val) const noexcept
      {
        return toDouble() <= val;
      }
      template<typename T>
      constexpr bool operator<(const T& val) const noexcept
      {
        return toDouble() < val;
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

      Rational invert() const noexcept
      {
        if (numerator_ == 0LL) {
          return Rational(0);
        }
        return {denominator_, numerator_};
      }
      std::string toString()
      {
        if (denominator_ == 1) {
          return std::to_string(numerator_);
        }
        return fmt::format("{}/{}", numerator_, denominator_);
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

      template<typename T>
      friend Rational operator*(const Rational& lhs, const T value) noexcept
      {
        return lhs * Rational(value);
      }
      template<typename T>
      friend Rational operator*(const T value, const Rational& rhs) noexcept
      {
        return rhs * Rational(value);
      }
      friend Rational operator/(const Rational& lhs, const Rational& rhs) noexcept
      {
        return lhs * rhs.invert();
      }
      template<typename T>
      friend Rational operator/(const Rational& lhs, const T value) noexcept
      {
        return lhs / Rational(value);
      }
      template<typename T>
      friend Rational operator/(const T value, const Rational& rhs) noexcept
      {
        return Rational(value) / rhs;
      }
      friend Rational operator+(const Rational& lhs, const Rational& rhs) noexcept
      {
        const auto n = (lhs.numerator_ * rhs.denominator_) + (rhs.numerator_ * lhs.denominator_);
        const auto d = lhs.denominator_ * rhs.denominator_;
        return {n, d};
      }
      template<typename T>
      friend Rational operator+(const Rational& lhs, const T value) noexcept
      {
        return lhs + Rational(value);
      }
      template<typename T>
      friend Rational operator+(const T value, const Rational& rhs) noexcept
      {
        return Rational(value) + rhs;
      }
      friend Rational operator-(const Rational& lhs, const Rational& rhs) noexcept
      {
        const auto n = (lhs.numerator_ * rhs.denominator_) - (rhs.numerator_ * lhs.denominator_);
        const auto d = lhs.denominator_ * rhs.denominator_;
        return {n, d};
      }
      template<typename T>
      friend Rational operator-(const Rational& lhs, const T value) noexcept
      {
        return lhs - Rational(value);
      }
      template<typename T>
      friend Rational operator-(const T value, const Rational& rhs) noexcept
      {
        return Rational(value) - rhs;
      }
      template<typename T>
      friend bool operator>(const Rational& lhs, const T value) noexcept
      {
        return lhs > Rational(value);
      }
      template<typename T>
      friend bool operator<(const Rational& lhs, const T value) noexcept
      {
        return lhs < Rational(value);
      }
      template<typename T>
      friend bool operator==(const Rational& lhs, const T value) noexcept
      {
        return lhs == Rational(value);
      }
      template<typename T>
      friend bool operator!=(const Rational& lhs, const T value) noexcept
      {
        return lhs != Rational(value);
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
