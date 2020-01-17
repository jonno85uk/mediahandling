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
  std::set<AVCodecID> NOBITRATE_CODECS {AV_CODEC_ID_WAVPACK};
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
  assert(sink_);
  bool okay = true;
  std::call_once(setup_encoder_, [&] { okay = setupEncoder(); });
  if (!okay) {
    logMessage(LogType::CRITICAL, "Failed to setup encoder");
    return false;
  }
  // TODO:
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

bool FFMpegStream::setOutputFormat(const SampleFormat format, std::optional<int32_t> rate)
{
  if ((parent_ == nullptr) && (sink_ != nullptr) ) {
    logMessage(LogType::WARNING, "Stream is setup for encoding");
    return false;
  }
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


bool FFMpegStream::setInputFormat(const PixelFormat format)
{
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
    std::string msg = "Invalid pixel format set as input. Valid types are:\n";
    for (const auto& f : formats) {
      if (f == AV_PIX_FMT_NONE) {
        continue;
      }
      msg.append(fmt::format("\t {}\n", types::convertPixelFormat(f))); //TODO: maybe show as string repr
    }
    logMessage(LogType::WARNING, msg);
  }
  return okay;
}

bool FFMpegStream::setInputFormat(const SampleFormat format)
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
    std::string msg = "Invalid sample format set as input. Valid types are:\n";
    for (const auto& f : formats) {
      if (f == AV_SAMPLE_FMT_NONE) {
        continue;
      }
      msg.append(fmt::format("\t {}\n", types::convertSampleFormat(f))); //TODO: maybe show as string repr
    }
    logMessage(LogType::WARNING, msg);
  }
  return okay;
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
  this->setProperty(MediaProperty::BITRATE, static_cast<int32_t>(context.bit_rate));

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


bool FFMpegStream::setupEncoder()
{
  assert(stream_);
  assert(sink_codec_ctx_);
  assert(codec_);

  switch (sink_codec_ctx_->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
    {
      bool okay = setupAudioEncoder(*stream_, *sink_codec_ctx_, *codec_);
      if (!okay) {
        logMessage(LogType::CRITICAL, "Failed to setup audio encoder");
      }
      return okay;
    }
    case AVMEDIA_TYPE_VIDEO:
    {
      bool okay = setupVideoEncoder(*stream_, *sink_codec_ctx_, *codec_);
      if (!okay) {
        logMessage(LogType::CRITICAL, "Failed to setup video encoder");
      }
      return okay;
    }
    default:
      break;
  }

  logMessage(LogType::CRITICAL, "Unable to setup encoder for this codec type");
  return false;
}


bool FFMpegStream::setupAudioEncoder(AVStream& stream, AVCodecContext& context, AVCodec& codec) const
{
  bool okay;
  auto sample_rate = this->property<int32_t>(MediaProperty::AUDIO_SAMPLING_RATE, okay);
  if (!okay) {
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
    bitrate = this->property<int32_t>(MediaProperty::BITRATE, okay);
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
  return true;
}

bool FFMpegStream::setupVideoEncoder(AVStream& stream, AVCodecContext& context, AVCodec& codec) const
{
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
    logMessage(LogType::CRITICAL, "Video compression method propert not set");
    return false;
  }
  if (compression == CompressionStrategy::CBR) {
    const auto min_bitrate = this->property<int32_t>(MediaProperty::MIN_BITRATE, okay);
    if (!okay) {
      logMessage(LogType::CRITICAL, "Video min-bitrate property not set");
      return false;
    }
    const auto max_bitrate = this->property<int32_t>(MediaProperty::MAX_BITRATE, okay);
    if (!okay) {
      logMessage(LogType::CRITICAL, "Video max-bitrate property not set");
      return false;
    }
    const auto bitrate = this->property<int32_t>(MediaProperty::BITRATE, okay);
    if (!okay) {
      logMessage(LogType::CRITICAL, "Video bitrate property not set");
      return false;
    }
    context.bit_rate = bitrate;
    context.rc_max_rate = max_bitrate;
    context.rc_min_rate = min_bitrate;
  } else if (compression == CompressionStrategy::TARGETBITRATE) {
    const auto bitrate = this->property<int32_t>(MediaProperty::BITRATE, okay);
    if (!okay) {
      logMessage(LogType::CRITICAL, "Video bitrate property not set");
      return false;
    }
    context.bit_rate = bitrate;
  } else {
    // TODO:
  }

  context.width = dimensions.width;
  context.height = dimensions.height;
  context.framerate.den = static_cast<int>(frame_rate.denominator());
  context.framerate.num = static_cast<int>(frame_rate.numerator());
  context.time_base = av_inv_q(context.framerate);

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

  auto ret = avcodec_open2(&context, &codec, nullptr);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage(LogType::CRITICAL, fmt::format("Could not open output video encoder. {}", err.data()));
    return false;
  }

  return okay;
}

bool FFMpegStream::setupH264Encoder(AVCodecContext& ctx) const
{
  bool okay;
  const auto profile = this->property<Profile>(MediaProperty::PROFILE, okay);
  if (okay) {
    const std::vector<Profile> valid = {Profile::H264_BASELINE, Profile::H264_MAIN, Profile::H264_HIGH,
                                        Profile::H264_HIGH10, Profile::H264_HIGH422, Profile::H264_HIGH444};
    if (std::find(valid.begin(), valid.end(), profile) != valid.end()) {
      ctx.profile = types::convertProfile(profile);
    } else {
      logMessage(LogType::WARNING, "Incompatibile profile chosen for X264 encoder");
    }
  }
  const auto preset = this->property<Preset>(MediaProperty::PRESET, okay);
  if (okay) {
    const std::vector<Preset> valid = { Preset::X264_VERYSLOW, Preset::X264_SLOWER, Preset::X264_SLOW, Preset::X264_MEDIUM,
                                        Preset::X264_FAST, Preset::X264_FASTER, Preset::X264_VERYFAST,Preset::X264_SUPERFAST,
                                        Preset::X264_ULTRAFAST};
    if (std::find(valid.begin(), valid.end(), preset) != valid.end()) {
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
  return true;
}
bool FFMpegStream::setupMPEG4Encoder(AVCodecContext& ctx) const
{
  return true;
}
bool FFMpegStream::setupDNXHDEncoder(AVCodecContext& ctx) const
{
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
