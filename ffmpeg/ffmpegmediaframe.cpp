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


void media_handling::avframeDeleter(AVFrame* frame)
{
  av_frame_free(&frame);
}

void media_handling::swsContextDeleter(SwsContext* context)
{
  sws_freeContext(context);
}


void media_handling::swrContextDeleter(SwrContext* context)
{
  swr_free(&context);
}

FFMpegMediaFrame::FFMpegMediaFrame(media_handling::AVFrameUPtr frame, const bool visual)
  : ff_frame_(std::move(frame)),
    visual_(visual)
{
  assert(ff_frame_);
  assert(ff_frame_->pts >= 0);
  timestamp_ = ff_frame_->pts;
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
  return ff_frame_->data;

}

uint8_t** FFMpegMediaFrame::convertedData() noexcept
{
  assert(ff_frame_);
  if (visual_ && output_format_.pixel_.has_value()) {
    if (!sws_context_) {
      sws_context_.reset(createSwsContext());
    }
    if (sws_context_) {
      // TODO: convert
      return nullptr;
    } else {
      media_handling::logMessage("FFMpegMediaFrame::data() -- failed to create format converter");
    }
  } else if (!visual_ && output_format_.sample_.has_value()) {
    if (!swr_context_) {
      swr_context_.reset(createSwrContext());
    }
    if (swr_context_) {
      // TODO: convert
      return nullptr;
    } else {
      media_handling::logMessage("FFMpegMediaFrame::data() -- failed to create format converter");
    }
  }
  return nullptr;
}

void FFMpegMediaFrame::extractProperties()
{
  assert(ff_frame_);
  if (visual_) {
    extractVisualProperties();
  } else {
    extractAudioProperties();
  }
}

int64_t FFMpegMediaFrame::timestamp() const
{
  return timestamp_;
}


bool FFMpegMediaFrame::setOutputFormat(const AVPixelFormat fmt)
{
  if (!visual_) {
    media_handling::logMessage("FFMpegMediaFrame::setOutputFormat() -- not able to set pixel format of audio frame");
    return false;
  }

  output_format_.pixel_ = fmt;
  return true;
}

bool FFMpegMediaFrame::setOutputFormat(const AVSampleFormat fmt)
{
  if (visual_) {
    media_handling::logMessage("FFMpegMediaFrame::setOutputFormat() -- not able to set pixel format of audio frame");
    return false;
  }

  output_format_.sample_ = fmt;
  return true;
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

  const SampleFormat format = convert(static_cast<AVSampleFormat>(ff_frame_->format));
  this->setProperty(MediaProperty::AUDIO_FORMAT, format);
}

constexpr media_handling::SampleFormat FFMpegMediaFrame::convert(enum AVSampleFormat av_format) const noexcept
{
  SampleFormat format = SampleFormat::NONE;
  switch (av_format) {
    case AV_SAMPLE_FMT_NONE:
      [[fallthrough]];
    case AV_SAMPLE_FMT_NB:
      [[fallthrough]];
    default:
      format = SampleFormat::NONE;
      break;
    case AV_SAMPLE_FMT_U8:
      format = SampleFormat::UNSIGNED_8;
      break;
    case AV_SAMPLE_FMT_S16:
      format = SampleFormat::SIGNED_16;
      break;
    case AV_SAMPLE_FMT_S32:
      format = SampleFormat::SIGNED_32;
      break;
    case AV_SAMPLE_FMT_FLT:
      format = SampleFormat::FLOAT;
      break;
    case AV_SAMPLE_FMT_DBL:
      format = SampleFormat::DOUBLE;
      break;
    case AV_SAMPLE_FMT_U8P:
      format = SampleFormat::UNSIGNED_8P;
      break;
    case AV_SAMPLE_FMT_S16P:
      format = SampleFormat::SIGNED_16P;
      break;
    case AV_SAMPLE_FMT_S32P:
      format = SampleFormat::SIGNED_32P;
      break;
    case AV_SAMPLE_FMT_FLTP:
      format = SampleFormat::FLOAT_P;
      break;
    case AV_SAMPLE_FMT_DBLP:
      format = SampleFormat::DOUBLE_P;
      break;
    case AV_SAMPLE_FMT_S64:
      format = SampleFormat::SIGNED_64;
      break;
    case AV_SAMPLE_FMT_S64P:
      format = SampleFormat::SIGNED_64P;
      break;
  }
  return format;
}

SwsContext* FFMpegMediaFrame::createSwsContext()
{

}

SwrContext* FFMpegMediaFrame::createSwrContext()
{

}

