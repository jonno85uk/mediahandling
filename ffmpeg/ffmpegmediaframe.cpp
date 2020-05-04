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
#include <fmt/core.h>
extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#include "mediahandling.h"
#include "ffmpegtypes.h"

using media_handling::ffmpeg::FFMpegMediaFrame;
using media_handling::MediaProperty;

namespace mh = media_handling;

constexpr auto ERR_LEN = 1024;

namespace
{
  std::string err(ERR_LEN, '\0');
}


FFMpegMediaFrame::FFMpegMediaFrame(types::AVFrameUPtr frame, const bool visual)
  : ff_frame_(std::move(frame)),
    is_audio_(!visual),
    is_visual_(visual)
{
  assert(ff_frame_);
  timestamp_ = ff_frame_->pts;
}


FFMpegMediaFrame::FFMpegMediaFrame(types::AVFrameUPtr frame, const bool visual, InOutFormat format)
  : ff_frame_(std::move(frame)),
    is_audio_(!visual),
    is_visual_(visual),
    output_fmt_(std::move(format))
{
  assert(ff_frame_);
  timestamp_ = ff_frame_->pts;
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
    logMessage(LogType::WARNING, "FFMpegMediaFrame::lineSize() -- index out of range");
    return {};
  }
  return ff_frame_->linesize[index];
}

media_handling::IMediaFrame::FrameData FFMpegMediaFrame::data() noexcept
{
  if (frame_data_)
  {
    return frame_data_.value();
  }
  assert(ff_frame_);
  media_handling::IMediaFrame::FrameData f_d;
  f_d.timestamp_ = ff_frame_->best_effort_timestamp; // value lost in resampled frame
  int ret = 0;
  if (is_visual_ && (is_visual_ == true) && output_fmt_.sws_context_) {
    if (conv_frame_ == nullptr) {
      // FIXME: this doesn't allow for changes to putput format after the first set
      conv_frame_.reset(av_frame_alloc());
      conv_frame_->format = media_handling::types::convertPixelFormat(output_fmt_.pix_fmt_);
      conv_frame_->width = output_fmt_.dims_.width;
      conv_frame_->height = output_fmt_.dims_.height;
      av_frame_get_buffer(conv_frame_.get(), 0);
    }
    // change the pixel format
    assert(conv_frame_);
    ret = sws_scale(output_fmt_.sws_context_.get(),
                    static_cast<const uint8_t* const*>(ff_frame_->data),
                    ff_frame_->linesize,
                    0,
                    ff_frame_->height,
                    conv_frame_->data,
                    conv_frame_->linesize);
    f_d.data_ = conv_frame_->data;
    f_d.dims_ = {conv_frame_->width, conv_frame_->height};
    f_d.line_size_ = conv_frame_->linesize[0];
    f_d.pix_fmt_ = output_fmt_.pix_fmt_;
    f_d.data_size_ = static_cast<size_t>(av_image_get_buffer_size(static_cast<AVPixelFormat>(conv_frame_->format),
                                                                  conv_frame_->width,
                                                                  conv_frame_->height,
                                                                  1));
  } else if (is_audio_ && (is_audio_ == true) && output_fmt_.swr_context_) {
    // change the sample format
    if (conv_frame_ == nullptr) {
      conv_frame_.reset(av_frame_alloc());
      conv_frame_->channel_layout = types::convertChannelLayout(output_fmt_.layout_);
      conv_frame_->sample_rate = output_fmt_.sample_rate_;
      conv_frame_->format = types::convertSampleFormat(output_fmt_.sample_fmt_);
      conv_frame_->nb_samples = 1000;
      ret = av_frame_get_buffer(conv_frame_.get(), 0);
      if (ret < 0) {
          av_strerror(ret, err.data(), ERR_LEN);
          logMessage(LogType::CRITICAL, fmt::format("Could not allocate frame buffer: {}", err.data()));
          return {};
      }
      ret = av_frame_make_writable(conv_frame_.get());
      if (ret < 0) {
          av_strerror(ret, err.data(), ERR_LEN);
          logMessage(LogType::CRITICAL, fmt::format("Could not ensure frame data is writable: {}", err.data()));
          return {};
      }
    }
    ret = swr_convert_frame(output_fmt_.swr_context_.get(), conv_frame_.get(), ff_frame_.get());
    if (ret < 0) {
        av_strerror(ret, err.data(), ERR_LEN);
        logMessage(LogType::CRITICAL, fmt::format("Could not resample audio frame: {}", err.data()));
        return {};
    }
    f_d.data_ = conv_frame_->data;
    f_d.data_size_ = static_cast<size_t>(conv_frame_->nb_samples
                                         * av_get_bytes_per_sample(static_cast<AVSampleFormat>(conv_frame_->format))
                                         * conv_frame_->channels);
    f_d.samp_fmt_ = output_fmt_.sample_fmt_;
    f_d.sample_count_ = conv_frame_->nb_samples;
    f_d.line_size_ = conv_frame_->linesize[0];
  } else {
    // No conversion
    f_d.data_ = ff_frame_->data;
    f_d.line_size_ = ff_frame_->linesize[0];
    if (is_visual_ && (is_visual_ == true)) {
      f_d.data_size_ = static_cast<size_t>(av_image_get_buffer_size(static_cast<AVPixelFormat>(ff_frame_->format),
                                                                    ff_frame_->width,
                                                                    ff_frame_->height,
                                                                    1));
    } else if (is_audio_ && (is_audio_ == true)) {
      f_d.data_size_ = static_cast<size_t>(ff_frame_->nb_samples
                                           * av_get_bytes_per_sample(static_cast<AVSampleFormat>(ff_frame_->format))
                                           * ff_frame_->channels);
      f_d.sample_count_ = ff_frame_->nb_samples;
    }
  }
  return f_d;
}


void FFMpegMediaFrame::setData(FrameData frame_data)
{
  frame_data_ = std::move(frame_data);
}

void FFMpegMediaFrame::extractProperties()
{
  assert(ff_frame_);
  this->setProperty(MediaProperty::FRAME_PACKET_SIZE, static_cast<int32_t>(ff_frame_->pkt_size));
  this->setProperty(MediaProperty::FRAME_DURATION, ff_frame_->pkt_duration);
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
  if (par != Rational{0, 1}) {
    this->setProperty(MediaProperty::PIXEL_ASPECT_RATIO, par);
  }

  // Colour info
  const mh::ColourPrimaries primary = mh::types::convertColourPrimary(ff_frame_->color_primaries);
  const mh::TransferCharacteristics transfer = mh::types::convertTransferCharacteristics(ff_frame_->color_trc);
  const mh::MatrixCoefficients matrix = mh::types::convertMatrixCoefficients(ff_frame_->colorspace);
  const mh::ColourRange range = mh::types::convertColourRange(ff_frame_->color_range);

  const ColourSpace space {primary, transfer, matrix, range};
  this->setProperty(MediaProperty::COLOUR_SPACE, space);
}


void FFMpegMediaFrame::extractAudioProperties()
{
  assert(ff_frame_);
  this->setProperty(MediaProperty::AUDIO_SAMPLES, static_cast<int32_t>(ff_frame_->nb_samples));
  const SampleFormat format = types::convert(static_cast<AVSampleFormat>(ff_frame_->format));
  this->setProperty(MediaProperty::AUDIO_FORMAT, format);
}



