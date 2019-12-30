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

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <cstdint>
#include <memory>
#include <cmath>
#include <ostream>
#include <cassert>


/**
 * Place for all bespoke or aliased types used within the library
 */
namespace media_handling
{

// TODO: maybe turn into a mapping
enum class MediaProperty
{
  PROFILE,              //TODO: for mpeg2/4, aac, dnxhd, etc encoded profile
  CODEC,                // Codec
  CODEC_NAME,           // std::string
  AUDIO_FORMAT,         // SampleFormat
  AUDIO_LAYOUT,         // ChannelLayout
  AUDIO_SAMPLING_RATE,  // int32_t
  AUDIO_SAMPLES,        // int32_t per channel
  AUDIO_CHANNELS,       // int32_t
  AUDIO_STREAMS,        // int32_t
  VIDEO_STREAMS,        // int32_t
  VIDEO_FORMAT,         // int32_t
  BITRATE,              // int64_t
  DURATION,             // int64_t
  TIMESCALE,            // Rational
  FILENAME,             // std::string
  FILE_FORMAT,          // std::string
  STREAMS,              // int32_t
  PIXEL_FORMAT,         // enum PixelFormat
  DIMENSIONS,           // struct Dimensions
  PIXEL_ASPECT_RATIO,   // Rational
  DISPLAY_ASPECT_RATIO, // Rational
  FRAME_COUNT,          // int64_t
  FIELD_ORDER,          // FieldOrder
  TIMESTAMP,            // int64_t
  FRAME_RATE,           // Rational
  SEQUENCE_PATTERN,     // std::string
  COLOUR_SPACE,                       // TODO:
  COLOUR_PRIMARIES,                   // ColorPrimaries
  COLOUR_TRANSFER_CHARACTERISTICS,    // TODO:
  COLOR_MATRIX_COEFFICIENTS           // TODO:
};

enum class Codec
{
  //TODO: use fourccs
  UNKNOWN,
  AAC,
  DNXHD,
  DPX,
  H264,
  JPEG,
  JPEG2000,
  MPEG2_VIDEO,
  MPEG4,
  PNG,
  RAW,
  TIFF
};

enum class StreamType
{
  AUDIO,
  VIDEO,
  IMAGE,
  UNKNOWN
};

enum class FieldOrder
{
  PROGRESSIVE,
  TOP_FIRST,
  BOTTOM_FIRST
};

/**
 * @brief The SampleFormat enum
 * @note numbers indicate bits
 */
enum class SampleFormat
{
  NONE,
  UNSIGNED_8,
  SIGNED_16,
  SIGNED_32,
  SIGNED_64,
  FLOAT,
  DOUBLE,
  // planars
  UNSIGNED_8P,
  SIGNED_16P,
  SIGNED_32P,
  FLOAT_P,
  DOUBLE_P,
  SIGNED_64P
};

struct Dimensions
{
  int width {-1};
  int height {-1};
};

enum class InterpolationMethod
{
  NEAREST,
  BILINEAR,
  BICUBLIN,
  BICUBIC,
  LANCZOS
};

/**
 * @brief The PixelFormat enum
 * @note only 8bit per channel
 */
enum class PixelFormat
{
  // TODO: use fourccs
  RGB24,
  RGBA,
  YUV420,
  YUVJ420,  // Full-range YUV420
  YUV422,
  YUV444,
  YUV422_P_10_LE,
  YUV444_P_12_LE,
  UNKNOWN
};

enum class ChannelLayout
{
  MONO,
  STEREO,               // 2.0
  STEREO_LFE,           // 2.0 + LFE
  THREE_STEREO,         // 2.0 + front-center
  THREE_SURROUND,       // 2.0 + back-center
  THREE_STEREO_LFE,     // 3.1
  THREE_SURROUND_LFE,
  FOUR_STEREO,          // 2.0 + rear-left + rear-back       i.e. quadraphonic
  FOUR_SURROUND,        // 2.0 + front-center + back-center  i.e. 4.0
  FOUR_SURROUND_LFE,    // 4.1
  FIVE,                 // 5.0
  FIVE_STEREO,          // 5.0 but L+R front+back & center, no surrounds
  FIVE_LFE,             // 5.1
  FIVE_STEREO_LFE,      // oggs 5.1 https://www.xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-810004.3.9
  SIX,                  // 6.0
  SIX_LFE,              // 6.1
  SEVEN,                // 7.0
  SEVEN_LFE,            // 7.1
  UNKNOWN
};

enum class ColourPrimaries
{
  UNKNOWN,
  BT_470M,
  BT_601,
  BT_709,
  BT_2020,
  BT_2100,
  SMPTE_240M,
  SMPTE_428
};

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



#endif // PROPERTIES_H
