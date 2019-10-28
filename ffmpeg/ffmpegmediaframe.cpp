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

#include "mediahandling.h"

using media_handling::FFMpegMediaFrame;
using media_handling::MediaProperty;


FFMpegMediaFrame::FFMpegMediaFrame(types::AVFrameUPtr frame, const bool visual)
  : ff_frame_(std::move(frame)),
    is_visual_(visual)
{
  assert(ff_frame_);
  assert(ff_frame_->pts >= 0);
  timestamp_ = ff_frame_->pts;
}


FFMpegMediaFrame::FFMpegMediaFrame(types::AVFrameUPtr frame, const bool visual, types::SWSContextPtr converter)
  : FFMpegMediaFrame(std::move(frame), visual)
{
  sws_context_ = converter;
}

FFMpegMediaFrame::FFMpegMediaFrame(types::AVFrameUPtr frame, const bool visual, types::SWRContextPtr converter)
  : FFMpegMediaFrame(std::move(frame), visual)
{
  swr_context_ = converter;
}

FFMpegMediaFrame::~FFMpegMediaFrame()
{
}

std::optional<bool> FFMpegMediaFrame::isAudio() const
{
  return is_audio_;
}

std::optional<bool> FFMpegMediaFrame::isVisual() const
{
  return is_visual_;
}

std::optional<int64_t> FFMpegMediaFrame::lineSize(const int index) const
{
  assert(ff_frame_);
  if (index > AV_NUM_DATA_POINTERS) {
    media_handling::logMessage("FFMpegMediaFrame::lineSize() -- index out of range");
    return {};
  }
  return ff_frame_->linesize[index];
}

uint8_t** FFMpegMediaFrame::data() noexcept
{
  assert(ff_frame_);
  if (is_visual_ && sws_context_) {
    // TODO: convert
    if (sws_frame_ == nullptr) {
      sws_frame_.reset(av_frame_alloc());
      sws_frame_->format = AV_PIX_FMT_RGB24; //TODO: get this programatically
      sws_frame_->width = ff_frame_->width;
      sws_frame_->height = ff_frame_->height;
      av_frame_get_buffer(sws_frame_.get(), 0);
    }
    // change the pixel format
    assert(sws_frame_);
    const auto out_slice_height = sws_scale(sws_context_.get(), (const uint8_t* const*)ff_frame_->data, ff_frame_->linesize, 0,
                                            ff_frame_->height, sws_frame_->data, sws_frame_->linesize);
    return sws_frame_->data;
  } else if (!is_visual_ && swr_context_) {
    // TODO:
    // change the sample format
  }
  return ff_frame_->data;

}

void FFMpegMediaFrame::extractProperties()
{
  assert(ff_frame_);
  if (is_visual_) {
    extractVisualProperties();
  } else {
    extractAudioProperties();
  }
}

int64_t FFMpegMediaFrame::timestamp() const
{
  return timestamp_;
}


void FFMpegMediaFrame::extractVisualProperties()
{
  assert(ff_frame_);

  // field-order
  if (ff_frame_->interlaced_frame) {
    if (ff_frame_->top_field_first) {
      this->setProperty(MediaProperty::FIELD_ORDER, FieldOrder::TOP_FIRST);
    } else {
      this->setProperty(MediaProperty::FIELD_ORDER, FieldOrder::BOTTOM_FIRST);
    }
  } else {
    this->setProperty(MediaProperty::FIELD_ORDER, FieldOrder::PROGRESSIVE);
  }

  // PAR
  Rational par {ff_frame_->sample_aspect_ratio.num, ff_frame_->sample_aspect_ratio.den};
  if (par != Rational{0,1}) {
    this->setProperty(MediaProperty::PIXEL_ASPECT_RATIO, par);
  }
}


void FFMpegMediaFrame::extractAudioProperties()
{
  assert(ff_frame_);
  this->setProperty(MediaProperty::AUDIO_SAMPLES, static_cast<int32_t>(ff_frame_->nb_samples));

  const SampleFormat format = types::convert(static_cast<AVSampleFormat>(ff_frame_->format));
  this->setProperty(MediaProperty::AUDIO_FORMAT, format);
}



