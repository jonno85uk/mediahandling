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

#include <stdint.h>
#include <memory>
#include <cmath>
#include <boost/rational.hpp>


/**
 * Place for all bespoke or aliased types used within the library
 */
namespace media_handling
{

enum class MediaProperty
{
    PROFILE,              //TODO: for mpeg2/4, aac, dnxhd, etc encoded profile
    CODEC,                // std::string
    AUDIO_FORMAT,         // SampleFormat
    AUDIO_LAYOUT,
    AUDIO_SAMPLING_RATE,  // int32_t
    AUDIO_SAMPLES,        // per channel
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
    FIELD_ORDER           // FieldOrder
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
    int width {};
    int height {};
};

enum class PixelFormat
{
    RGB24,
    YUV420,
    YUV422,
    YUV444,
    UNKNOWN
};

struct MediaFrame
{
    std::shared_ptr<void> data_{nullptr};
    uint32_t line_size_{0};
    uint32_t line_count_{0};
    int32_t format_{0};
    std::optional<FieldOrder> field_order_;
};

using MediaFramePtr = std::shared_ptr<MediaFrame>;
using Rational = boost::rational<int64_t>;
}

#endif // PROPERTIES_H
