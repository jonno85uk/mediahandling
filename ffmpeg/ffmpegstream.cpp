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

#include "ffmpegstream.h"
#include <cassert>
#include <sstream>
#include <thread>
#include <fmt/core.h>

#include "mediahandling.h"
#include "ffmpegsource.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}


constexpr AVSampleFormat SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
constexpr auto ERR_LEN = 256;
constexpr auto SEEK_DIRECTION = AVSEEK_FLAG_BACKWARD;

using media_handling::ffmpeg::FFMpegStream;
using media_handling::MediaFramePtr;

namespace  {
  std::array<char, ERR_LEN> err;
}


FFMpegStream::FFMpegStream(FFMpegSource* parent, AVStream* const stream)
  : parent_(parent),
    stream_(stream)
{
  if ( (parent == nullptr) || (stream_ == nullptr) || (stream_->codecpar == nullptr) ) {
    throw std::runtime_error("Required parameter(s) is/are null");
  }
  source_index_ = stream->index;
  parent->queueStream(source_index_);
  codec_ = avcodec_find_decoder(stream_->codecpar->codec_id);
  codec_ctx_ = avcodec_alloc_context3(codec_);
  assert(codec_ctx_);
  int err_code = avcodec_parameters_to_context(codec_ctx_, stream_->codecpar);
  if (err_code < 0) {
    av_strerror(err_code, err.data(), ERR_LEN);
    const auto msg = fmt::format("Failed to populate codec context: {}", err.data());
    logMessage(LogType::CRITICAL, msg);
    throw std::runtime_error(msg);
  }

  codec_ctx_->thread_count = static_cast<int32_t>(std::thread::hardware_concurrency());
  setupDecoder(stream_->codecpar->codec_id, opts_);
  // Open codec
  err_code = avcodec_open2(codec_ctx_, codec_, &opts_);
  if (err_code < 0) {
    av_strerror(err_code, err.data(), ERR_LEN);
    const auto msg = fmt::format("Could not open codec:  {}", err.data());
    logMessage(LogType::CRITICAL, msg);
    throw std::runtime_error(msg);
  }

  if (stream_->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
    type_ = StreamType::AUDIO;
  } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    if (stream->avg_frame_rate.den == 0) {
      type_ = StreamType::IMAGE;
    } else {
      type_ = StreamType::VIDEO;
    }
  } else {
    throw std::exception(); // TODO: custom type
  }


  pkt_ = av_packet_alloc();
  assert(pkt_);
  // For properties that require samples, this has to happen last otherwise ffmpeg resources will be unavailable
  extractProperties(*stream, *codec_ctx_);
}

FFMpegStream::~FFMpegStream()
{
  assert(parent_);
  parent_->unqueueStream(source_index_);
  stream_ = nullptr; //TODO: check this
  av_packet_free(&pkt_);
  avcodec_close(codec_ctx_);
  avcodec_free_context(&codec_ctx_);
  av_dict_free(&opts_);
}


int64_t FFMpegStream::timestamp() const
{
  return last_timestamp_;
}

MediaFramePtr FFMpegStream::frame(const int64_t timestamp)
{
  assert(codec_ctx_);

  if ((timestamp >= 0) && (last_timestamp_ != timestamp)) {
    // TODO: more checks to prevent unneeded seek
    if (!seek(timestamp)) {
      logMessage(LogType::WARNING, fmt::format("Failed to seek:  {}", timestamp));
      return nullptr;
    }
  } // else read next frame

  return frame(*codec_ctx_, stream_->index);
}


bool FFMpegStream::setFrame(const int64_t timestamp, MediaFramePtr sample)
{
  // TODO:
  return false;
}


media_handling::StreamType FFMpegStream::type() const
{
  return type_;
}


int32_t FFMpegStream::sourceIndex() const noexcept
{
  return source_index_;
}

bool FFMpegStream::setOutputFormat(const PixelFormat format,
                                   const media_handling::Dimensions& dims,
                                   media_handling::InterpolationMethod interp)
{
  assert(codec_);
  const AVPixelFormat output_av_fmt = types::convertPixelFormat(format);
  if (output_av_fmt == AV_PIX_FMT_NONE) {
    logMessage(LogType::CRITICAL, "FFMpegStream::setOutputFormat() -- Unknown AV pixel format");
    return false;
  }

  bool is_valid = false;
  const PixelFormat src_fmt = this->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, is_valid);
  if (!is_valid) {
    throw std::runtime_error("Do not know the pixel format of this stream");
  }

  const AVPixelFormat src_av_fmt = types::convertPixelFormat(src_fmt);
  if (src_av_fmt == AV_PIX_FMT_NONE) {
    logMessage(LogType::CRITICAL, "FFMpegStream::setOutputFormat() -- Unknown AV pixel format");
    return false;
  }


  auto src_dims = this->property<media_handling::Dimensions>(MediaProperty::DIMENSIONS, is_valid);
  if (!is_valid) {
    logMessage(LogType::CRITICAL, "FFMpegStream::setOutputFormat() -- Unknown dimensions of the stream");
    return false;
  }

  const auto [out_dims, out_interp] = [&] {
    if ( (dims.width) <= 0 || (dims.height <= 0)) {
      logMessage(LogType::INFO, "FFMpegStream::setOutputFormat() -- Output dimensions invalid");
      return std::make_tuple(src_dims, 0);
    }
    return std::make_tuple(dims, media_handling::types::convertInterpolationMethod(interp));
  }();

  SwsContext* ctx = sws_getContext(src_dims.width, src_dims.height, src_av_fmt,
                                   out_dims.width, out_dims.height, output_av_fmt,
                                   out_interp,
                                   nullptr, nullptr, nullptr);
  assert(ctx);
  output_format_.sws_context_ = std::shared_ptr<SwsContext>(ctx, types::swsContextDeleter);
  output_format_.pix_fmt_ = format;
  output_format_.dims_ = out_dims;

  return output_format_.sws_context_ != nullptr;
}

bool FFMpegStream::setOutputFormat(const SampleFormat format, std::optional<int32_t> rate)
{
  bool okay = false;
  const auto layout = property<ChannelLayout>(MediaProperty::AUDIO_LAYOUT, okay);
  assert(okay);
  const auto sample_rate = property<int32_t>(MediaProperty::AUDIO_SAMPLING_RATE, okay);
  assert(okay);
  const auto src_fmt = property<SampleFormat>(MediaProperty::AUDIO_FORMAT, okay);
  assert(okay);
  const auto av_layout = types::convertChannelLayout(layout);
  const auto av_src_fmt = types::convertSampleFormat(src_fmt);
  const auto av_format = types::convertSampleFormat(format);
  SwrContext* ctx = swr_alloc_set_opts(nullptr,
                                       static_cast<int64_t>(av_layout),
                                       av_format,
                                       rate.has_value() ? rate.value() : sample_rate,
                                       static_cast<int64_t>(av_layout),
                                       av_src_fmt,
                                       sample_rate,
                                       0,
                                       nullptr);

  const auto ret = swr_init(ctx);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage(LogType::CRITICAL, fmt::format("Could not init resample context: {}", err.data()));
    return false;
  }

  assert(ctx);
  output_format_.sample_fmt_ = format;
  output_format_.layout_ = layout;
  output_format_.sample_rate_ = rate.has_value() ? rate.value() : sample_rate;
  output_format_.swr_context_ = std::shared_ptr<SwrContext>(ctx, types::swrContextDeleter);
  return true;
}

void FFMpegStream::extractProperties(const AVStream& stream, const AVCodecContext& context)
{
  assert(context.codec);
  this->setProperty(MediaProperty::CODEC_NAME, std::string(context.codec->name));
  const media_handling::Codec cdc = types::convertCodecID(context.codec_id);
  this->setProperty(MediaProperty::CODEC, cdc);
  // TODO: if we use this things could go wrong on container != essence
  auto base = stream.time_base;
  if (base.den > 0) {
    Rational timescale(base.num, base.den);
    this->setProperty(MediaProperty::TIMESCALE, timescale);
  }
  this->setProperty(MediaProperty::BITRATE, context.bit_rate);

  // TODO: stream durations
  if ( (type_ == StreamType::VIDEO) || (type_ == StreamType::IMAGE) ) {
    extractVisualProperties(stream, context);
  } else if (type_ == StreamType::AUDIO) {
    extractAudioProperties(stream, context);
  } else {
    assert("Cannot get properties of unknown stream");
  }
}

void FFMpegStream::extractVisualProperties(const AVStream& stream, const AVCodecContext& context)
{
  this->setProperty(MediaProperty::FRAME_COUNT, static_cast<int64_t>(stream.nb_frames));
  const PixelFormat p_format = types::convertPixelFormat(context.pix_fmt);
  this->setProperty(MediaProperty::PIXEL_FORMAT, p_format);
  Dimensions dims {context.width, context.height};
  this->setProperty(MediaProperty::DIMENSIONS, dims);

  if (stream.sample_aspect_ratio.den > 0) {
    Rational par(stream.sample_aspect_ratio.num, stream.sample_aspect_ratio.den);
    this->setProperty(MediaProperty::PIXEL_ASPECT_RATIO, par);
  }

  if (stream.display_aspect_ratio.den > 0) {
    Rational dar(stream.display_aspect_ratio.num, stream.display_aspect_ratio.den);
    this->setProperty(MediaProperty::DISPLAY_ASPECT_RATIO, dar);
  }

  extractFrameProperties();
}

void FFMpegStream::extractAudioProperties(const AVStream& stream, const AVCodecContext& context)
{
  this->setProperty(MediaProperty::AUDIO_CHANNELS, static_cast<int32_t>(context.channels));
  this->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, static_cast<int32_t>(context.sample_rate));
  const SampleFormat s_format = types::convertSampleFormat(context.sample_fmt);
  this->setProperty(MediaProperty::AUDIO_FORMAT, s_format);

#ifdef DEBUG
  av_get_channel_layout_string(err.data(), ERR_LEN, context.channels, context.channel_layout);
  std::cout << err.data() << std::endl;
#endif
  const ChannelLayout layout = types::convertChannelLayout(context.channel_layout);
  this->setProperty(MediaProperty::AUDIO_LAYOUT, layout);
}

bool FFMpegStream::seek(const int64_t timestamp)
{
  assert(parent_);
  assert(stream_);
  assert(codec_ctx_);
  parent_->resetPacketQueue();
  avcodec_flush_buffers(codec_ctx_);
  int ret = av_seek_frame(parent_->context(), stream_->index, timestamp, SEEK_DIRECTION);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage(LogType::WARNING, fmt::format("Could not seek frame: {}", err.data()));
    return false;
  }
  return true;
}


void FFMpegStream::setupDecoder(const AVCodecID codec_id, AVDictionary* dict) const
{
  if (codec_id == AV_CODEC_ID_H264) {
    // NOTE: av_dict_set leaks memory, nothing we can do
    av_dict_set(&dict, "tune", "fastdecode", 0);
    av_dict_set(&dict, "tune", "zerolatency", 0);
  }
}


void FFMpegStream::extractFrameProperties()
{
  if (auto tmp_frame = this->frame(0)) {
    tmp_frame->extractProperties();
    bool is_valid;
    if (type_ == StreamType::VIDEO) {
      auto val = tmp_frame->property<media_handling::FieldOrder>(MediaProperty::FIELD_ORDER, is_valid);
      if (is_valid) {
        MediaPropertyObject::setProperty(MediaProperty::FIELD_ORDER, val);
      }
    } else if (type_ == StreamType::IMAGE) {
      logMessage(LogType::DEBUG, "Setting image progressive");
      MediaPropertyObject::setProperty(MediaProperty::FIELD_ORDER, FieldOrder::PROGRESSIVE);
    }
    auto par = MediaPropertyObject::property<Rational>(MediaProperty::PIXEL_ASPECT_RATIO, is_valid);
    if (!is_valid || (par == Rational{0,1}) ) {
      // Couldn't find it or value doesn't make sense
      par = tmp_frame->property<Rational>(MediaProperty::PIXEL_ASPECT_RATIO, is_valid);
      if (is_valid) {
        MediaPropertyObject::setProperty(MediaProperty::PIXEL_ASPECT_RATIO, par);
      }
    }
    const auto space = tmp_frame->property<ColourSpace>(MediaProperty::COLOUR_SPACE, is_valid);
    if (is_valid) {
      MediaPropertyObject::setProperty(MediaProperty::COLOUR_SPACE, space);
    }
  } else {
    logMessage(LogType::CRITICAL, "Failed to read a frame from stream");
  }
}



MediaFramePtr FFMpegStream::frame(AVCodecContext& codec_ctx, const int stream_idx) const
{
  int err_code = 0;

  types::AVFrameUPtr frame(av_frame_alloc());
  while (err_code >= 0)
  {
    auto pkt = parent_->nextPacket(stream_idx);
    if (pkt == nullptr) {
      break;
    }

    err_code = avcodec_send_packet(&codec_ctx, pkt.get());
    if (err_code < 0) {
      av_strerror(err_code, err.data(), ERR_LEN);
      logMessage(LogType::WARNING, fmt::format("Failed sending a packet for decoding: {}", err.data()));
      break;
    }

    int dec_err_code = 0;
    while (dec_err_code >= 0) {
      dec_err_code = avcodec_receive_frame(&codec_ctx, frame.get());
      if (dec_err_code == 0) {
        last_timestamp_ = frame->best_effort_timestamp;
        // successful read
        assert(type_ != media_handling::StreamType::UNKNOWN);
        if ( (output_format_.swr_context_ != nullptr) || (output_format_.sws_context_ != nullptr) ) {
          return std::make_shared<media_handling::FFMpegMediaFrame>(std::move(frame),
                                                                    type_ != StreamType::AUDIO,
                                                                    output_format_);
        }
        return std::make_shared<media_handling::FFMpegMediaFrame>(std::move(frame), type_ != StreamType::AUDIO);
      }

      if (dec_err_code == AVERROR(EAGAIN)) {
        // Need to provide decoder more packets before a frame is available
        continue;
      }
      else if (dec_err_code == AVERROR_EOF) {
        // Most likely reached the end of the stream
        break;
      } else {
        av_strerror(dec_err_code, err.data(), ERR_LEN);
        logMessage(LogType::CRITICAL, fmt::format("Failed to decode: {}", err.data()));
        break;
      }
    }//while
  }//while
  return nullptr;
}
