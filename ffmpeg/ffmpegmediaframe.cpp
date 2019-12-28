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
#include <libswscale/swscale.h>

#include "mediahandling.h"
#include "ffmpegtypes.h"

using media_handling::FFMpegMediaFrame;
using media_handling::MediaProperty;


FFMpegMediaFrame::FFMpegMediaFrame(types::AVFrameUPtr frame, const bool visual)
  : ff_frame_(std::move(frame)),
    is_visual_(visual),
    is_audio_(!visual)
{
  assert(ff_frame_);
  assert(ff_frame_->pts >= 0);
  timestamp_ = ff_frame_->pts;
}


FFMpegMediaFrame::FFMpegMediaFrame(types::AVFrameUPtr frame, const bool visual, OutputFormat format)
  : ff_frame_(std::move(frame)),
    is_visual_(visual),
    is_audio_(!visual),
    output_fmt_(std::move(format))
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

media_handling::IMediaFrame::FrameData FFMpegMediaFrame::data() noexcept
{
  assert(ff_frame_);
  media_handling::IMediaFrame::FrameData f_d;
  if (is_visual_ && output_fmt_.sws_context_) {
    if (sws_frame_ == nullptr) {
      // FIXME: this doesn't allow for changes to putput format after the first set
      sws_frame_.reset(av_frame_alloc());
      sws_frame_->format = media_handling::types::convertPixelFormat(output_fmt_.pix_fmt_);
      sws_frame_->width = output_fmt_.dims_.width;
      sws_frame_->height = output_fmt_.dims_.height;
      av_frame_get_buffer(sws_frame_.get(), 0);
    }
    // change the pixel format
    assert(sws_frame_);
    sws_scale(output_fmt_.sws_context_.get(),
              static_cast<const uint8_t* const*>(ff_frame_->data),
              ff_frame_->linesize,
              0,
              ff_frame_->height,
              sws_frame_->data,
              sws_frame_->linesize);
    f_d.data_ = sws_frame_->data;
    f_d.line_size_ = sws_frame_->linesize[0];
    f_d.pix_fmt_ = output_fmt_.pix_fmt_;
    f_d.data_size_ = avpicture_get_size(static_cast<AVPixelFormat>(sws_frame_->format), sws_frame_->width, sws_frame_->height);
  } else if (!is_visual_ && output_fmt_.swr_context_) {
    // TODO:
    // change the sample format
  } else {
    // No conversion
    f_d.data_ = ff_frame_->data;
    f_d.line_size_ = ff_frame_->linesize[0];
    f_d.data_size_ = avpicture_get_size(static_cast<AVPixelFormat>(ff_frame_->format), ff_frame_->width, ff_frame_->height);
  }
  return f_d;

}

void FFMpegMediaFrame::extractProperties()
{
  assert(ff_frame_);
  if (is_visual_ && (is_visual_ == true) ) {
    extractVisualProperties();
  } else if (is_audio_ && (is_audio_ == true) ) {
    extractAudioProperties();
  } else {
    // TODO: log
  }
}

int64_t FFMpegMediaFrame::timestamp() const noexcept
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

  // Colour info
  const media_handling::ColourPrimaries primary = media_handling::types::convertColourPrimary(ff_frame_->color_primaries);
  this->setProperty(MediaProperty::COLOUR_PRIMARIES, primary);
}


void FFMpegMediaFrame::extractAudioProperties()
{
  assert(ff_frame_);
  this->setProperty(MediaProperty::AUDIO_SAMPLES, static_cast<int32_t>(ff_frame_->nb_samples));

  const SampleFormat format = types::convert(static_cast<AVSampleFormat>(ff_frame_->format));
  this->setProperty(MediaProperty::AUDIO_FORMAT, format);
}



