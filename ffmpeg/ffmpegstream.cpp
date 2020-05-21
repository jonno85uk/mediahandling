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
#include <set>

#include "mediahandling.h"
#include "ffmpegsource.h"
#include "timecode.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
}


constexpr auto ERR_LEN = 256;
constexpr auto SEEK_DIRECTION = AVSEEK_FLAG_BACKWARD;
constexpr auto TAG_TIMECODE = "timecode";

using media_handling::ffmpeg::FFMpegStream;
using media_handling::MediaFramePtr;

namespace mh = media_handling;

namespace  {
  std::array<char, ERR_LEN> err;
  const std::set<AVCodecID> NOBITRATE_CODECS {AV_CODEC_ID_WAVPACK, AV_CODEC_ID_PCM_S16LE, AV_CODEC_ID_PCM_S32LE,
        AV_CODEC_ID_FLAC};
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


FFMpegStream::FFMpegStream(FFMpegSink* sink, const AVCodecID codec)
  : sink_(sink)
{
  if (AVCodec* av_codec = avcodec_find_encoder(codec)) {
    codec_ = av_codec;
    sink_codec_ctx_ = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(av_codec), types::avCodecContextDeleter);
    // ensure formats are set to none so user has to explicitly set value
    sink_codec_ctx_->pix_fmt = AV_PIX_FMT_NONE;
    sink_codec_ctx_->sample_fmt = AV_SAMPLE_FMT_NONE;
    stream_ = avformat_new_stream(&sink_->formatContext(), av_codec); // Freed by FormatContext
    pkt_ = av_packet_alloc();
  } else {
    throw std::runtime_error("Codec is not supported as encoder");
  }
}

FFMpegStream::~FFMpegStream()
{
  if (parent_!= nullptr) {
    parent_->unqueueStream(source_index_);
  }
  stream_ = nullptr; //TODO: check this
  av_packet_free(&pkt_);
  avcodec_close(codec_ctx_);
  avcodec_free_context(&codec_ctx_);
  av_dict_free(&opts_);
}

void FFMpegStream::setProperties(std::map<media_handling::MediaProperty, std::any> props)
{
  if (setup_) {
    logMessage(LogType::WARNING, "Setting/changing properties of a writing stream that is in use is prohibited");
  } else {
    media_handling::MediaPropertyObject::setProperties(std::move(props));
  }
}

void FFMpegStream::setProperty(const MediaProperty prop, const std::any& value)
{
  if (setup_) {
    logMessage(LogType::WARNING, "Setting/changing a property of a writing stream that is in use is prohibited");
  } else {
    media_handling::MediaPropertyObject::setProperty(prop, value);
  }
}


bool FFMpegStream::index()
{
  // Intention is to change already set properties so unset lock
  setup_ = false;
  MediaFramePtr fframe = this->frame(0);
  if (fframe == nullptr) {
    return false;
  }
  int64_t frame_count = 0;
  int64_t frames_size = 0;
  int64_t duration = 0;
  bool okay = true;
  while ((fframe != nullptr) && okay) {
    fframe->extractProperties();
    frame_count++;
    frames_size += fframe->property<int32_t>(MediaProperty::FRAME_PACKET_SIZE, okay);
    duration += fframe->property<int64_t>(MediaProperty::FRAME_DURATION, okay);
    fframe = this->frame();
  }

  if (okay) {
    this->setProperty(MediaProperty::FRAME_COUNT, frame_count);
    const auto scale = this->property<Rational>(MediaProperty::TIMESCALE, okay);
    if (!okay) {
      setup_ = true;
      return false;
    }
    const Rational dur = duration * scale;
    this->setProperty(MediaProperty::DURATION, dur);
    const auto rate = this->property<Rational>(MediaProperty::FRAME_RATE, okay);
    if (!okay) {
      setup_ = true;
      return false;
    }
    const BitRate bit_rate = frames_size / (frame_count/rate);
    this->setProperty(MediaProperty::BITRATE, bit_rate);

  }
  setup_ = true;
  return okay;
}

int64_t FFMpegStream::timestamp() const
{
  return last_timestamp_;
}

MediaFramePtr FFMpegStream::frame(const int64_t time_stamp)
{
  return frameByTimestamp(time_stamp);
}

MediaFramePtr FFMpegStream::frameByTimestamp(const int64_t time_stamp)
{
  assert(codec_ctx_);

  if ((time_stamp >= 0) && (last_timestamp_ != time_stamp)) {
    const int diff = abs(last_timestamp_ - time_stamp);
    if ( (diff > pts_intvl_) || (time_stamp < last_timestamp_)) {
      // Only seek if change or required time_stamp is not next frame or reversed
      if (!seek(time_stamp)) {
        logMessage(LogType::WARNING, fmt::format("Failed to seek:  {}", time_stamp));
        return nullptr;
      }
    }
  } // else read next frame

  if (time_stamp == -1) {
    // Get next frame
    return frame(*codec_ctx_, stream_->index);
  }

  constexpr auto RETRY_LIMIT = 100000;
  MediaFramePtr result;
  auto cnt = 0;
  do {
    result = frame(*codec_ctx_, stream_->index);
  } while (result && (result->timestamp() != time_stamp) && (cnt++ < RETRY_LIMIT));
  if (!result && cnt >= RETRY_LIMIT) {
    logMessage(LogType::WARNING, fmt::format("Failed to retrieve frame. ts={}", time_stamp));
  }
  return result;
}

MediaFramePtr FFMpegStream::frameBySecond(const double second)
{
  bool okay;
  const auto scale = this->property<Rational>(MediaProperty::TIMESCALE, okay);
  assert(okay);
  const auto rate = this->property<Rational>(MediaProperty::FRAME_RATE, okay);
  assert(okay);
  const int64_t ts = (second * rate) / scale;
  return this->frameByTimestamp(ts);
}

MediaFramePtr FFMpegStream::frameByFrameNumber(const int64_t frame_number)
{
  bool okay;
  const auto scale = this->property<Rational>(MediaProperty::TIMESCALE, okay);
  assert(okay);
  const auto rate = this->property<Rational>(MediaProperty::FRAME_RATE, okay);
  assert(okay);
  const int64_t ts = (frame_number / rate) / scale;
  return this->frameByTimestamp(ts);
}

bool FFMpegStream::writeFrame(MediaFramePtr sample)
{
  bool okay = true;
  std::call_once(setup_encoder_, [&] { okay = setupEncoder(); });
  if (!okay) {
    logMessage(LogType::CRITICAL, "Failed to setup encoder");
    return false;
  }
  if ( (sink_codec_ctx_ == nullptr) || (sink_frame_ == nullptr) ) {
    logMessage(LogType::CRITICAL, "Stream has not been configured correctly for writing");
    return false;
  }

  // send frame to encoder
  if (sample) {
    const auto data = sample->data();
    assert(data.data_);
    if (input_format_.swr_context_ != nullptr) {
      // Convert audio
      assert(data.line_size_ > 0);
      const auto ret = swr_convert(input_format_.swr_context_.get(),
                                   sink_frame_->data, sink_frame_->linesize[0],
                                   const_cast<const uint8_t**>(data.data_), data.line_size_);
      if (ret < 0) {
        av_strerror(ret, err.data(), ERR_LEN);
        const auto msg = fmt::format("Failed to convert audio sample, msg={}", err.data());
        logMessage(LogType::CRITICAL, msg);
        return false;
      }
    } else if (input_format_.sws_context_ != nullptr) {
      // TODO: convert video
    } else {
      // Copy
      for (auto ix = 0; ix < AV_NUM_DATA_POINTERS; ++ix) {
        sink_frame_->data[ix] = data.data_[ix];
      }
    }

    if (data.sample_count_ >= 0) {
      sink_frame_->pts = audio_samples_;
      audio_samples_ += data.sample_count_;
    } else {
      sink_frame_->pts++;
    }
    auto ret = avcodec_send_frame(sink_codec_ctx_.get(), sink_frame_.get());
    if (ret < 0) {
      av_strerror(ret, err.data(), ERR_LEN);
      const auto msg = fmt::format("Failed to send frame to encoder: {}", err.data());
      logMessage(LogType::CRITICAL, msg);
      return false;
    }
  } else {
    const auto ret = avcodec_send_frame(sink_codec_ctx_.get(), nullptr);
    if (ret < 0) {
      av_strerror(ret, err.data(), ERR_LEN);
      const auto msg = fmt::format("Failed to send frame to encoder: {}", err.data());
      logMessage(LogType::CRITICAL, msg);
      return false;
    }
  }
  // Retrieve packet from encoder
  int ret = 0;
  while (ret >= 0) {
    ret = avcodec_receive_packet(sink_codec_ctx_.get(), pkt_);
    if (ret == AVERROR(EAGAIN)) {
      return true;
    } else if (ret < 0) {
      if (ret != AVERROR_EOF) {
        av_strerror(ret, err.data(), ERR_LEN);
        const auto msg = fmt::format("Failed to receive packet from encoder, msg={}", err.data());
        logMessage(LogType::CRITICAL, msg);
        return false;
      }
      return true;
    }

    pkt_->stream_index = stream_->index;
    av_packet_rescale_ts(pkt_, sink_codec_ctx_->time_base, stream_->time_base);
    // Send packet to container writer
    ret = av_interleaved_write_frame(&sink_->formatContext(), pkt_);
    av_packet_unref(pkt_);
    if (ret < 0 ){
      av_strerror(ret, err.data(), ERR_LEN);
      const auto msg = fmt::format("Failed to write frame to container, msg={}", err.data());
      logMessage(LogType::CRITICAL, msg);
      return false;
    }
  } //while
  return true;
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
  if ((parent_ == nullptr) && (sink_ != nullptr) ) {
    logMessage(LogType::WARNING, "Stream is setup for encoding");
    return false;
  }
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

bool FFMpegStream::setOutputFormat(const SampleFormat format, std::optional<SampleRate> rate)
{
  if ((parent_ == nullptr) && (sink_ != nullptr) ) {
    logMessage(LogType::WARNING, "Stream is setup for encoding");
    return false;
  }
  bool okay = false;
  const auto layout = property<ChannelLayout>(MediaProperty::AUDIO_LAYOUT, okay);
  assert(okay);
  const auto sample_rate = property<SampleRate>(MediaProperty::AUDIO_SAMPLING_RATE, okay);
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


bool FFMpegStream::setInputFormat(const PixelFormat format)
{
  if (codec_->pix_fmts == nullptr) {
    logMessage(LogType::CRITICAL, "Encoder has no known supported pixel formats");
    return false;
  }
  const auto ff_format = types::convertPixelFormat(format);
  std::set<AVPixelFormat> formats;
  AVPixelFormat fmt;
  auto ix = 0;
  bool okay = false;
  do {
    fmt = codec_->pix_fmts[ix++];
    formats.insert(fmt);
    if (fmt == ff_format) {
      okay = true;
      sink_codec_ctx_->pix_fmt = fmt;
    }
  } while ((!okay) && (fmt != AV_PIX_FMT_NONE));

  if (!okay) {
    const auto dims = this->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
    if (okay) {
      SwsContext* ctx = sws_getContext(dims.width, dims.height, ff_format,
                                       dims.width, dims.height, codec_->pix_fmts[0],
                                       0,
                                       nullptr, nullptr, nullptr);
      assert(ctx);
      output_format_.sws_context_ = std::shared_ptr<SwsContext>(ctx, types::swsContextDeleter);
      output_format_.pix_fmt_ = format;
      output_format_.dims_ = dims;
      sink_codec_ctx_->pix_fmt = codec_->pix_fmts[0];
      logMessage(LogType::WARNING, fmt::format("Auto converting input format to {}", codec_->pix_fmts[0]));
      return true;
    }
    std::string msg = "Invalid pixel format set as input. Valid types are:\n";
    for (const auto& f : formats) {
      if (f == AV_PIX_FMT_NONE) {
        continue;
      }
      msg.append(fmt::format("\t {}\n", static_cast<int>(types::convertPixelFormat(f)))); //TODO: maybe show as string repr
    }
    logMessage(LogType::WARNING, msg);
  }
  return okay;
}

bool FFMpegStream::setInputFormat(const SampleFormat format, std::optional<SampleRate> rate)
{
  assert(codec_);
  assert(sink_codec_ctx_);
  const auto ff_format = types::convertSampleFormat(format);
  std::set<AVSampleFormat> formats;
  AVSampleFormat fmt;
  auto ix = 0;
  bool okay = false;
  do {
    fmt = codec_->sample_fmts[ix++];
    formats.insert(fmt);
    if (fmt == ff_format) {
      okay = true;
      sink_codec_ctx_->sample_fmt = fmt;
    }
  } while ((!okay) && (fmt != AV_SAMPLE_FMT_NONE));

  if (!okay) {

    const auto dst_fmt = types::convertSampleFormat(codec_->sample_fmts[0]);
    const auto dst_rate = this->property<SampleRate>(MediaProperty::AUDIO_SAMPLING_RATE, okay);
    if (!okay)
    {
      logMessage(LogType::CRITICAL, "Stream sampling rate has not been set");
      return okay;
    }
    const auto layout = this->property<ChannelLayout>(MediaProperty::AUDIO_LAYOUT, okay);
    if (!okay)
    {
      logMessage(LogType::CRITICAL, "Stream channel layout has not been set");
      return okay;
    }
    auto src_rate = rate.has_value() ? rate.value() : dst_rate;

    okay = setupSWR(input_format_, layout, format, dst_fmt, src_rate, dst_rate);
    if (okay)
    {
      sink_codec_ctx_->sample_fmt = codec_->sample_fmts[0];
      logMessage(LogType::INFO, "Setup an auto audio-converter");
    }
    else
    {
      logMessage(LogType::CRITICAL, "Failed to setup auto audio-converter");
    }

  }
  return okay;
}


void FFMpegStream::initialise() noexcept
{
  setup_ = true;
}

void FFMpegStream::extractProperties(const AVStream& stream, const AVCodecContext& context)
{
  assert(context.codec);
  assert(context.codec->name);
  this->setProperty(MediaProperty::CODEC_NAME, std::string(context.codec->name));
  const media_handling::Codec cdc = types::convertCodecID(context.codec_id);
  this->setProperty(MediaProperty::CODEC, cdc);
  if (stream.avg_frame_rate.den != 0)
  {
    const Rational frame_rate(stream.avg_frame_rate.num, stream.avg_frame_rate.den);
    this->setProperty(MediaProperty::FRAME_RATE, frame_rate);
  }
  // TODO: if we use this things could go wrong on container != essence
  const auto base = stream.time_base;
  if (base.den > 0) {
    const Rational timescale(base.num, base.den);
    this->setProperty(MediaProperty::TIMESCALE, timescale);
    const Rational duration = stream.duration * timescale;
    this->setProperty(MediaProperty::DURATION, duration);
  }
  this->setProperty(MediaProperty::BITRATE, static_cast<BitRate>(context.bit_rate));

  if ( (type_ == StreamType::VIDEO) || (type_ == StreamType::IMAGE) ) {
    extractVisualProperties(stream, context);
  } else if (type_ == StreamType::AUDIO) {
    extractAudioProperties(stream, context);
  } else {
    assert("Cannot get properties of unknown stream");
  }

  if (stream.r_frame_rate.den > 0) {
    const Rational tb(stream.time_base.num, stream.time_base.den);
    const Rational fr(stream.r_frame_rate.num, stream.r_frame_rate.den);
    pts_intvl_ = (1 / fr) / tb;
  }

  if (stream.metadata)
  {
    extractMetadata(*stream.metadata);
  }
}

void FFMpegStream::extractMetadata(const AVDictionary& metadata)
{
  auto count = av_dict_count(&metadata);
  if (count < 1)
  {
    return;
  }
  bool okay;
  if (const auto timescale = this->property<Rational>(MediaProperty::TIMESCALE, okay); okay)
  {
    if (const auto frame_rate = this->property<Rational>(MediaProperty::FRAME_RATE, okay); okay)
    {
      TimeCode tc(timescale, frame_rate);
      if (AVDictionaryEntry* entry = av_dict_get(&metadata, TAG_TIMECODE, nullptr, 0))
      {
        std::string tc_str(entry->value);
        if (!tc.setTimeCode(tc_str))
        {
          logMessage(LogType::WARNING, "Failed to configure start timecode");
        }
      }
      this->setProperty(MediaProperty::START_TIMECODE, tc);
    }
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
  } else {
    this->setProperty(MediaProperty::DISPLAY_ASPECT_RATIO, Rational(context.width, context.height));
  }

  const Profile prof = types::convertProfile(context.profile);
  this->setProperty(MediaProperty::PROFILE, prof);

  extractFrameProperties();
}

void FFMpegStream::extractAudioProperties(const AVStream& stream, const AVCodecContext& context)
{
  this->setProperty(MediaProperty::AUDIO_CHANNELS, static_cast<int32_t>(context.channels));
  this->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, static_cast<SampleRate>(context.sample_rate));
  const SampleFormat s_format = types::convertSampleFormat(context.sample_fmt);
  this->setProperty(MediaProperty::AUDIO_FORMAT, s_format);

#ifdef DEBUG
  av_get_channel_layout_string(err.data(), ERR_LEN, context.channels, context.channel_layout);
  std::cout << err.data() << std::endl;
#endif
  const ChannelLayout layout = types::convertChannelLayout(context.channel_layout);
  this->setProperty(MediaProperty::AUDIO_LAYOUT, layout);
}

bool FFMpegStream::seek(const int64_t time_stamp)
{
  assert(parent_);
  assert(stream_);
  assert(codec_ctx_);
  parent_->resetPacketQueue();
  avcodec_flush_buffers(codec_ctx_);
  const int ret = av_seek_frame(parent_->context(), stream_->index, time_stamp, SEEK_DIRECTION);
  logMessage(LogType::DEBUG, fmt::format("Seeking. ts={}, idx={}", time_stamp, stream_->index));
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


bool FFMpegStream::setupEncoder()
{
  assert(stream_);
  assert(sink_codec_ctx_);
  assert(codec_);

  sink_frame_.reset(av_frame_alloc());
  sink_frame_->pts = 0;

  switch (sink_codec_ctx_->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
    {
      bool okay = setupAudioEncoder(*stream_, *sink_codec_ctx_, *codec_);
      if (!okay) {
        logMessage(LogType::CRITICAL, "Failed to setup audio encoder");
      }
      setup_ = okay;
      return sink_->writeHeader();
    }
    case AVMEDIA_TYPE_VIDEO:
    {
      bool okay = setupVideoEncoder(*stream_, *sink_codec_ctx_, *codec_);
      if (!okay) {
        logMessage(LogType::CRITICAL, "Failed to setup video encoder");
      }
      setup_ = okay;
      return sink_->writeHeader();
    }
    default:
      break;
  }

  logMessage(LogType::CRITICAL, "Unable to setup encoder for this codec type");
  return false;
}

bool checkSupportedSampleRates(const int* rates, const int32_t sample_rate)
{
  // check supported sample rates
  if (rates == nullptr) {
    logMessage(media_handling::LogType::WARNING, "Unable to validify set sample-rate with codec supported rates");
    return true;
  }
  int rate = -1;
  auto ix = 0;
  bool match = false;
  do {
    rate = rates[ix++];
    match = (rate == sample_rate);
  } while ((rate != 0) && (!match));

  if (!match) {
    logMessage(media_handling::LogType::CRITICAL, "Invalid sample rate set for audio encoder");
    return false;
  }
  return true;
}

bool FFMpegStream::setupAudioEncoder(AVStream& stream, AVCodecContext& context, AVCodec& codec) const
{
  AVFormatContext fmt = sink_->formatContext();
  assert(fmt.oformat);
  auto ret = avformat_query_codec(fmt.oformat, codec.id, FF_COMPLIANCE_NORMAL);
  if (ret != 1) {
    logMessage(LogType::CRITICAL, fmt::format("The audio codec '{}' is not supported in the container '{}'",
                                              codec.name,
                                              fmt.oformat->name));
    return false;
  }
  bool okay;
  auto sample_rate = this->property<SampleRate>(MediaProperty::AUDIO_SAMPLING_RATE, okay);
  if (okay) {
    if (!checkSupportedSampleRates(codec.supported_samplerates, sample_rate)) {
      return false;
    }
  } else {
    logMessage(LogType::CRITICAL, "Audio sample rate property not set");
    return false;
  }
  auto layout = this->property<ChannelLayout>(MediaProperty::AUDIO_LAYOUT, okay);
  if (!okay) {
    logMessage(LogType::CRITICAL, "Audio Layout property not set");
    return false;
  }
  int64_t bitrate = 0;
  if (NOBITRATE_CODECS.find(context.codec_id) == NOBITRATE_CODECS.end()) {
    // A bitrate is required for lossless codecs
    bitrate = this->property<BitRate>(MediaProperty::BITRATE, okay);
    if (!okay) {
      logMessage(LogType::CRITICAL, "Audio Bitrate property not set");
      return false;
    }
    context.bit_rate = bitrate;
  }
  context.sample_rate = sample_rate;
  context.channel_layout = types::convertChannelLayout(layout);
  context.channels = av_get_channel_layout_nb_channels(context.channel_layout);
  context.time_base.num = 1;
  context.time_base.den = sample_rate;
  stream.time_base = context.time_base;
  if (context.sample_fmt == AV_SAMPLE_FMT_NONE) {
    logMessage(LogType::CRITICAL, "Input sample format has not been specified");
    return false;
  }

  if (fmt.oformat->flags & AVFMT_GLOBALHEADER) {
    context.flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  ret = avcodec_open2(&context, &codec, nullptr);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage(LogType::CRITICAL, fmt::format("Could not open output audio encoder. {}", err.data()));
    return false;
  }

  assert(sink_frame_);
  sink_frame_->sample_rate = sample_rate;
  sink_frame_->nb_samples = context.codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE ? 10000 : context.frame_size;
  sink_frame_->format = context.sample_fmt;
  sink_frame_->channel_layout = context.channel_layout;
  sink_frame_->channels = context.channels;
  ret = av_frame_get_buffer(sink_frame_.get(), 0);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    const auto msg = fmt::format("Failed to allocate frame buffers, msg={}", err.data());
    logMessage(LogType::CRITICAL, msg);
    return false;
  }
  ret = av_frame_make_writable(sink_frame_.get());
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    const auto msg = fmt::format("Failed to make frame writable, msg={}", err.data());
    logMessage(LogType::CRITICAL, msg);
    return false;
  }
  assert(pkt_);
  av_init_packet(pkt_);

  // Fill the AVCodecParameters based on the values from the codec context.
  ret = avcodec_parameters_from_context(stream.codecpar, &context);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    const auto msg = fmt::format("Could not copy audio encoder parameters to output stream, msg={}", err.data());
    logMessage(LogType::CRITICAL, msg);
    return false;
  }
  return true;
}

bool FFMpegStream::setupVideoEncoder(AVStream& stream, AVCodecContext& context, AVCodec& codec) const
{
  AVFormatContext fmt = sink_->formatContext();
  assert(fmt.oformat);
  auto ret = avformat_query_codec(fmt.oformat, codec.id, FF_COMPLIANCE_NORMAL);
  if (ret != 1) {
    logMessage(LogType::CRITICAL, fmt::format("The video codec '{}' is not supported in the container '{}'",
                                              codec.name,
                                              fmt.oformat->name));
    return false;
  }
  auto okay = false;
  const auto dimensions = this->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
  if (!okay) {
    logMessage(LogType::CRITICAL, "Video dimensions property not set");
    return false;
  }
  const auto frame_rate = this->property<Rational>(MediaProperty::FRAME_RATE, okay);
  if (!okay) {
    logMessage(LogType::CRITICAL, "Video frame-rate property not set");
    return false;
  }
  const auto compression = this->property<CompressionStrategy>(MediaProperty::COMPRESSION, okay);
  if (!okay) {
    logMessage(LogType::CRITICAL, "Video compression method property not set");
    return false;
  }
  if (compression == CompressionStrategy::CBR) {

    const auto bitrate = this->property<BitRate>(MediaProperty::BITRATE, okay);
    if (!okay) {
      logMessage(LogType::CRITICAL, "Video bitrate property not set");
      return false;
    }
    context.bit_rate    = bitrate;
    context.rc_max_rate = bitrate;
    context.rc_min_rate = bitrate;
#ifdef MPEG2_HEADACHES
    context.rc_buffer_size = bitrate / 4;
    context.rc_max_available_vbv_use = 1.0;
    context.rc_min_vbv_overflow_use = 1.0;
    auto props = reinterpret_cast<AVCPBProperties*>(av_stream_new_side_data(&stream, AV_PKT_DATA_CPB_PROPERTIES, NULL));
    props->avg_bitrate = bitrate;
    props->buffer_size = bitrate;
    props->max_bitrate = bitrate;
    props->min_bitrate = bitrate;
#endif
  } else if (compression == CompressionStrategy::TARGETBITRATE) {
    const auto bit_rate = this->property<BitRate>(MediaProperty::BITRATE, okay);
    if (!okay) {
      logMessage(LogType::CRITICAL, "Video bitrate property not set");
      return false;
    }
    context.bit_rate = bit_rate;
    const auto min_bitrate = this->property<BitRate>(MediaProperty::MIN_BITRATE, okay);
    if (okay) {
      context.rc_min_rate = min_bitrate;
    }
    const auto max_bitrate = this->property<BitRate>(MediaProperty::MAX_BITRATE, okay);
    if (okay) {
      context.rc_max_rate = max_bitrate;
    }
  } else {
    // TODO:
  }

  context.width = dimensions.width;
  context.height = dimensions.height;
  context.framerate.den = static_cast<int>(frame_rate.denominator());
  context.framerate.num = static_cast<int>(frame_rate.numerator());
  context.time_base = av_inv_q(context.framerate);
  stream.time_base = context.time_base;

  const auto gop_struct = this->property<GOP>(MediaProperty::GOP, okay);
  if (okay) {
    context.gop_size = gop_struct.n_;
    context.max_b_frames = gop_struct.m_;
  }

  const auto threads = this->property<int32_t>(MediaProperty::THREADS, okay);
  if (okay) {
    context.thread_count = threads;
  } else {
    context.thread_count = static_cast<int>(std::thread::hardware_concurrency());
    logMessage(LogType::INFO, fmt::format("Automatically setting thread count to {} threads", context.thread_count));
  }
  context.thread_type = FF_THREAD_SLICE;

  if (context.pix_fmt == AV_PIX_FMT_NONE) {
    logMessage(LogType::CRITICAL, "Input pixel format has not been specified");
    return false;
  }

  // Do bare minimum before avcodec_open2
  switch (context.codec_id) {
    case AV_CODEC_ID_H264:
      okay = setupH264Encoder(context);
      break;
    case AV_CODEC_ID_MPEG2VIDEO:
      okay = setupMPEG2Encoder(context);
      break;
    case AV_CODEC_ID_DNXHD:
      okay = setupDNXHDEncoder(context);
      break;
    case AV_CODEC_ID_MPEG4:
      okay = setupMPEG4Encoder(context);
      break;
    default:
      // Nothing defined for these codecs yet
      break;
  }

  if (!okay) {
    logMessage(LogType::CRITICAL, "Failed to setup encoder");
  }

  if (fmt.oformat->flags & AVFMT_GLOBALHEADER) {
    context.flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  ret = avcodec_open2(&context, &codec, nullptr);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage(LogType::CRITICAL, fmt::format("Could not open output video encoder. {}", err.data()));
    return false;
  }

  assert(sink_frame_);
  sink_frame_->width = dimensions.width;
  sink_frame_->height = dimensions.height;
  sink_frame_->format = context.pix_fmt;
  ret = av_frame_get_buffer(sink_frame_.get(), 0);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    const auto msg = fmt::format("Failed to initialise buffers for video frame, msg={}", err.data());
    logMessage(LogType::CRITICAL, msg);
    return false;
  }

  assert(pkt_);
  av_init_packet(pkt_);

  // Fill the AVCodecParameters based on the values from the codec context.
  ret = avcodec_parameters_from_context(stream.codecpar, &context);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    const auto msg = fmt::format("Could not copy video encoder parameters to output stream, msg={}", err.data());
    logMessage(LogType::CRITICAL, msg);
    return false;
  }
  return okay;
}

bool FFMpegStream::setupH264Encoder(AVCodecContext& ctx) const
{
  bool okay;
  const auto profile = this->property<Profile>(MediaProperty::PROFILE, okay);
  if (okay) {
    const std::set<Profile> valid = {Profile::H264_BASELINE, Profile::H264_MAIN, Profile::H264_HIGH,
                                     Profile::H264_HIGH10, Profile::H264_HIGH422, Profile::H264_HIGH444};
    if (valid.count(profile) == 1) {
      ctx.profile = types::convertProfile(profile);
    } else {
      logMessage(LogType::WARNING, "Incompatibile profile chosen for X264 encoder");
    }
  }
  const auto preset = this->property<Preset>(MediaProperty::PRESET, okay);
  if (okay) {
    const std::set<Preset> valid = { Preset::X264_VERYSLOW, Preset::X264_SLOWER, Preset::X264_SLOW, Preset::X264_MEDIUM,
                                     Preset::X264_FAST, Preset::X264_FASTER, Preset::X264_VERYFAST,Preset::X264_SUPERFAST,
                                     Preset::X264_ULTRAFAST};
    if (valid.count(preset) == 1) {
      auto ret = av_opt_set(ctx.priv_data, "preset", types::convertPreset(preset).data(), 0);
      if (ret < 0) {
        av_strerror(ret, err.data(), ERR_LEN);
        logMessage(LogType::CRITICAL, fmt::format("Failed to set preset, msg={}", err.data()));
        return false;
      }
    } else {
      logMessage(LogType::WARNING, "Incompatibile preset chosen for X264 encoder");
    }
  }
  return true;
}
bool FFMpegStream::setupMPEG2Encoder(AVCodecContext& ctx) const
{
  bool okay;
  const auto profile = this->property<Profile>(MediaProperty::PROFILE, okay);
  if (okay) {
    const std::set<Profile> valid = {Profile::MPEG2_SIMPLE, Profile::MPEG2_MAIN, Profile::MPEG2_HIGH, Profile::MPEG2_422};
    if (valid.count(profile) == 1) {
      ctx.profile = types::convertProfile(profile);
    } else {
      logMessage(LogType::WARNING, "Incompatibile profile chosen for MPEG2 encoder");
    }
  }
  return true;
}
bool FFMpegStream::setupMPEG4Encoder(AVCodecContext& ctx) const
{
  // TODO: low priority
  return true;
}

bool FFMpegStream::setupDNXHDEncoder(AVCodecContext& ctx) const
{
  bool okay;
  const auto profile = this->property<Profile>(MediaProperty::PROFILE, okay);
  if (okay) {
    const std::set<Profile> valid = {Profile::DNXHD, Profile::DNXHR_LB, Profile::DNXHR_SQ, Profile::DNXHR_HQ,
                                     Profile::DNXHR_HQX, Profile::DNXHR_444};
    if (valid.count(profile) == 1) {
      ctx.profile = types::convertProfile(profile);
    } else {
      logMessage(LogType::WARNING, "Incompatibile profile chosen for MPEG2 encoder");
    }
  }
  return true;
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

    const auto dar = tmp_frame->property<Rational>(MediaProperty::DISPLAY_ASPECT_RATIO, is_valid);
    if (is_valid) {
      MediaPropertyObject::setProperty(MediaProperty::DISPLAY_ASPECT_RATIO, dar);
    }
  } else {
    logMessage(LogType::CRITICAL, "Failed to read a frame from stream");
  }
  // Ensure playhead is reset
  this->seek(0);
}



MediaFramePtr FFMpegStream::frame(AVCodecContext& codec_ctx, const int stream_idx) const
{
  int err_code = 0;

  types::AVFrameUPtr frame(av_frame_alloc());
  while (err_code >= 0)
  {
    const auto pkt = parent_->nextPacket(stream_idx);
    // Send nulls to flush decoder
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
          return std::make_shared<media_handling::ffmpeg::FFMpegMediaFrame>(std::move(frame),
                                                                    type_ != StreamType::AUDIO,
                                                                    output_format_);
        }
        return std::make_shared<media_handling::ffmpeg::FFMpegMediaFrame>(std::move(frame), type_ != StreamType::AUDIO);
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


bool FFMpegStream::setupSWR(FFMpegMediaFrame::InOutFormat& fmt,
                            const ChannelLayout layout,
                            const SampleFormat src_fmt,
                            const SampleFormat dst_fmt,
                            const int32_t src_rate,
                            const int32_t dst_rate)
{
  const auto av_layout = types::convertChannelLayout(layout);
  const auto av_src_fmt = types::convertSampleFormat(src_fmt);
  const auto av_dst_fmt = types::convertSampleFormat(dst_fmt);
  SwrContext* ctx = swr_alloc_set_opts(nullptr,
                                       static_cast<int64_t>(av_layout),
                                       av_dst_fmt,
                                       dst_rate,
                                       static_cast<int64_t>(av_layout),
                                       av_src_fmt,
                                       src_rate,
                                       0,
                                       nullptr);

  const auto ret = swr_init(ctx);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage(LogType::CRITICAL, fmt::format("Could not init resample context: {}", err.data()));
    return false;
  }

  assert(ctx);
  fmt.sample_fmt_ = dst_fmt;
  fmt.layout_ = layout;
  fmt.sample_rate_ = dst_rate;
  fmt.swr_context_ = std::shared_ptr<SwrContext>(ctx, types::swrContextDeleter);
  return true;
}
