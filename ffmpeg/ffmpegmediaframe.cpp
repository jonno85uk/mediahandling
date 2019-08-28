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

#include "ffmpegmediaframe.h"

#include <cassert>

using media_handling::FFMpegMediaFrame;
using media_handling::MediaProperty;

FFMpegMediaFrame::FFMpegMediaFrame(AVFrame* const frame) : ff_frame_(frame)
{
  assert(frame);
  data_ = std::make_shared<uint8_t**>(frame->data);
}

FFMpegMediaFrame::~FFMpegMediaFrame()
{
  av_frame_free(&ff_frame_);
}

std::optional<bool> FFMpegMediaFrame::isAudio() const
{
  return is_audio_;
}

std::optional<bool> FFMpegMediaFrame::isVisual() const
{
  return is_visual_;
}

int64_t FFMpegMediaFrame::size() const
{
  return data_size_;
}

std::shared_ptr<uint8_t**> FFMpegMediaFrame::data() const
{
  return data_;
}

void FFMpegMediaFrame::setData(std::shared_ptr<uint8_t**> frame_data, const int64_t size)
{
  data_ = frame_data;
  data_size_ = size;
}

void FFMpegMediaFrame::extractProperties()
{
  assert(ff_frame_);
  if (ff_frame_->interlaced_frame) {
    if (ff_frame_->top_field_first) {
      this->setProperty(MediaProperty::FIELD_ORDER, FieldOrder::TOP_FIRST);
    } else {
      this->setProperty(MediaProperty::FIELD_ORDER, FieldOrder::BOTTOM_FIRST);
    }
  } else {
    this->setProperty(MediaProperty::FIELD_ORDER, FieldOrder::PROGRESSIVE);
  }
}


