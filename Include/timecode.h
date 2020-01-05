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

#ifndef TIMECODE_H
#define TIMECODE_H

#include <string>

#include "types.h"

namespace media_handling
{
  class TimeCode
  {
    public:
      /**
       * @brief Constructor
       * @param time_scale
       * @param frame_rate
       * @param time_stamp
       */
      TimeCode(Rational time_scale, Rational frame_rate, const int64_t time_stamp=0);
      /**
       * @brief Convert time to millis
       * @return
       */
      int64_t toMillis() const;
      /**
       * @brief Convert to SMPTE timecode format
       * @param drop  Use drop-frame timecode if possible
       * @return  SMPTE timecode
       */
      std::string toString(const bool drop=true) const;
      /**
       * @brief Convert time to number of frames
       * @return
       */
      int64_t toFrames() const;
      /**
       * @brief             Set the time in units of time-scale
       * @param time_stamp  Value to set
       */
      void setTimestamp(const int64_t time_stamp) noexcept;
      /**
       * @brief Retrieve this time's scale
       * @return
       */
      Rational timeScale() const noexcept;
      /**
       * @brief   Retrieve this time's frame rate
       * @return
       */
      Rational frameRate() const noexcept;
      /**
       * @brief Retrieve this time's time-stamp
       * @return
       */
      int64_t timestamp() const noexcept;
    private:
      friend class TimeCodeTest;
      Rational time_scale_;
      Rational frame_rate_;
      int64_t time_stamp_ {0};
      struct {
          bool drop_ {false};
          int32_t second_ {0};
          int32_t minute_ {0};
          int32_t drop_minute_ {0};
          int32_t ten_minute_ {0};
          int32_t drop_ten_minute_ {0};
          int32_t hour_ {0};
          int32_t drop_count_ {0};
      } frames_;
      /**
       * @brief         Convert frames to smpte timecode
       * @param frames
       * @param drop    true==use drop-frame format
       * @return        hh:mm:ss:;ff
       */
      std::string framesToSMPTE(int64_t frames, const bool drop=true) const;

  };
}



#endif // TIMECODE_H
