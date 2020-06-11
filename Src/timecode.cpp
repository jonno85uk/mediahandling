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

#include "timecode.h"

#include <fmt/core.h>
#include <cmath>
#include <string>

#include "mediahandling.h"

using media_handling::TimeCode;

constexpr auto DROP_FMT = "{:02d}:{:02d}:{:02d}{}{:02d}";
constexpr auto SECONDS_IN_MIN = 60;
constexpr auto SECONDS_IN_HOUR = SECONDS_IN_MIN * 60;
constexpr auto DROP_FACTOR = 0.06;

namespace
{
  const media_handling::Rational NTSC_30 {30000, 1001};
  const media_handling::Rational NTSC_60 {60000, 1001};
}


TimeCode::TimeCode(media_handling::Rational time_scale, media_handling::Rational frame_rate, const int64_t time_stamp)
  : time_scale_(std::move(time_scale)),
    frame_rate_(std::move(frame_rate)),
    time_stamp_(time_stamp)
{
  const auto rate = lround(frame_rate.toDouble());
  frames_.drop_ = frame_rate_.denominator() != 1;
  frames_.second_ = rate;
  frames_.minute_ = rate * SECONDS_IN_MIN;
  frames_.ten_minute_ = frames_.minute_ * 10;
  frames_.hour_ = rate * SECONDS_IN_HOUR;

  if (frames_.drop_) {
    frames_.drop_count_ = lround(frame_rate.toDouble() * DROP_FACTOR);
    frames_.drop_minute_ = floor((frame_rate * SECONDS_IN_MIN).toDouble());
    frames_.drop_ten_minute_ = lround((frame_rate * SECONDS_IN_MIN).toDouble() * 10);
  }
}

int64_t TimeCode::toMillis() const
{
  return llround((time_stamp_ * time_scale_).toDouble() * 1000);
}

std::string TimeCode::toString(const bool drop) const
{
  return framesToSMPTE(toFrames(), drop);
}

int64_t TimeCode::toFrames() const
{
  return floor(((time_stamp_ * time_scale_) * frame_rate_).toDouble());
}


bool TimeCode::setFrames(const int64_t count)
{
  if (count < 0) {
    return false;
  }
  // TODO: check for >23:59::59:ff
  time_stamp_ = static_cast<int64_t>(std::ceil((count / frame_rate_ / time_scale_).toDouble()));
  return true;
}

void TimeCode::setTimestamp(const int64_t time_stamp) noexcept
{
  time_stamp_ = time_stamp;
}

bool TimeCode::setTimeCode(const std::string& timecode)
{
  if (timecode.empty() || (timecode.length() != 11) ) {
    LCRITICAL("Timecode empty or of incorrect length");
    return false;
  }
  const auto hours = std::stoi(timecode.substr(0, 2));
  if (hours >= 24) {
    LCRITICAL("Timecode has greater than 23hours");
    return false;
  }
  if (timecode.at(2) != ':') {
    LCRITICAL("Timecode has incorrect hour/minute separator");
    return false;
  }
  const auto minutes = std::stoi(timecode.substr(3, 2));
  if (minutes >= 60) {
    LCRITICAL("Timecode has greater than 59 minutes");
    return false;
  }
  if (timecode.at(2) != ':') {
    LCRITICAL("Timecode has incorrect minute/second separator");
    return false;
  }
  const auto tc_seconds = std::stoi(timecode.substr(6, 2));
  if (tc_seconds >= 60) {
    LCRITICAL("Timecode has greater than 59 seconds");
    return false;
  }
  if ((timecode.at(8) != ':') && (timecode.at(8) != ';') ) {
    LCRITICAL("Timecode has incorrect second/frame separator");
    return false;
  }

  if ( ( (frame_rate_ != NTSC_30) && (frame_rate_ != NTSC_60)) && (timecode.at(8) == ';') ) {
    LCRITICAL("Timecode has incorrect second/frame separator for NTSC frame-rate");
    return false;
  }
  const auto tc_frames = std::stoi(timecode.substr(9, 2));
  if (tc_frames >= std::round(frame_rate_.toDouble())) {
    LCRITICAL(fmt::format("Timecode has greater than {} frames",
                                              static_cast<int>(std::round(frame_rate_.toDouble())) - 1));
    return false;
  }

  const auto seconds = tc_seconds + (hours * SECONDS_IN_HOUR) + (minutes * SECONDS_IN_MIN);
  if ( (frame_rate_ == NTSC_30) || (frame_rate_ == NTSC_60) )   {
    if (timecode.at(8) == ';') {
      time_stamp_ = static_cast<int64_t>(std::ceil(((seconds / time_scale_)
                                                    + ((tc_frames / frame_rate_) / time_scale_)).toDouble()));
    } else {
      const auto full_rate = static_cast<int32_t>(std::ceil(frame_rate_.toDouble()));
      const auto all_frames = (full_rate * seconds) + tc_frames;
      const auto scaled = all_frames / frame_rate_ / time_scale_;
      time_stamp_ = llround(scaled.toDouble());
    }
  } else {
    time_stamp_ = static_cast<int64_t>(std::ceil(((seconds / time_scale_)
                                                  + ((tc_frames / frame_rate_) / time_scale_)).toDouble()));
  }
  return true;
}

media_handling::Rational TimeCode::timeScale() const noexcept
{
  return time_scale_;
}

media_handling::Rational TimeCode::frameRate() const noexcept
{
  return frame_rate_;
}

int64_t TimeCode::timestamp() const noexcept
{
  return time_stamp_;
}

std::string TimeCode::framesToSMPTE(int64_t frames, const bool drop) const
{
  assert(frames_.second_ != 0);
  assert(frames_.minute_ != 0);
  assert(frames_.hour_ != 0);
  // Influenced by http://www.davidheidelberger.com/blog/?p=29
  char token = ':';
  if (drop && frames_.drop_ && ( (frame_rate_ == NTSC_30) || (frame_rate_ == NTSC_60) ) ) {
    assert(frames_.drop_minute_ != 0);
    assert(frames_.drop_ten_minute_ != 0);
    const auto d = frames / frames_.drop_ten_minute_;
    const auto m = frames % frames_.drop_ten_minute_;
    if (m > frames_.drop_count_) {
      frames += (frames_.drop_count_ * 9 * d) + frames_.drop_count_ * ((m - frames_.drop_count_) / frames_.drop_minute_);
    } else {
      frames += frames_.drop_count_ * 9 * d;
    }
    token = ';';
  }
  const auto f_rem = frames % frames_.second_;
  const auto s = (frames / frames_.second_) % 60;
  const auto m = (frames / frames_.minute_) % 60;
  const auto h = (frames / frames_.hour_) % 60;

  return fmt::format(DROP_FMT, h, m , s, token, f_rem);
}
