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

#include "ffmpegmediaframe.h"


extern "C" {
//#include <libavformat/avformat.h>
//#include <libavfilter/avfilter.h>
//#include <libavfilter/buffersink.h>
//#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
}


constexpr AVSampleFormat SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
constexpr auto ERR_LEN = 256;
constexpr auto SEEK_DIRECTION = AVSEEK_FLAG_BACKWARD;

using media_handling::ffmpeg::FFMpegStream;
using media_handling::MediaFramePtr;

namespace  {
  std::array<char, ERR_LEN> err;
}


FFMpegStream::FFMpegStream(AVFormatContext* parent, AVStream* const stream)
  : parent_(parent),
    stream_(stream)
{
  if ( (parent == nullptr) || (stream_ == nullptr) || (stream_->codecpar == nullptr) ) {
    throw std::exception();
  }
  codec_ = avcodec_find_decoder(stream_->codecpar->codec_id);
  codec_ctx_ = avcodec_alloc_context3(codec_);
  assert(codec_ctx_);
  int err_code = avcodec_parameters_to_context(codec_ctx_, stream_->codecpar);
  if (err_code < 0) {
    av_strerror(err_code, err.data(), ERR_LEN);
    std::cerr << "Failed to populate codec context: " << err.data() << std::endl;
    throw std::exception();
  }

  codec_ctx_->thread_count = static_cast<int32_t>(std::thread::hardware_concurrency());
  setupDecoder(stream_->codecpar->codec_id, opts_);
  // Open codec
  err_code = avcodec_open2(codec_ctx_, codec_, &opts_);
  if (err_code < 0) {
    av_strerror(err_code, err.data(), ERR_LEN);
    std::cerr << "Could not open codec: " << err.data() << std::endl;
    throw std::exception();
  }

  if (stream_->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
    type_ = StreamType::AUDIO;
  } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    type_ = StreamType::VISUAL;
  } else {
    throw std::exception(); // TODO: custom type
  }

  // Filters
  // allocate filtergraph
  //  filter_graph_ = avfilter_graph_alloc();
  //  assert(filter_graph_);
  //  if (stream_->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
  //    setupForVideo(*stream_, buffer_ctx_, *filter_graph_, pixel_format_);
  //  } else if (stream_->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
  //    setupForAudio(*stream_, buffer_ctx_, *filter_graph_, *codec_ctx_);
  //  } else {
  //    // TODO:
  //    auto media_type = av_get_media_type_string(stream_->codecpar->codec_type);
  //    std::cerr << "Unsupported media type:" << media_type << std::endl;
  //    throw std::exception();
  //  }

  pkt_ = av_packet_alloc();
  assert(pkt_);
  // For properties that require samples, this has to happen last otherwise ffmpeg resources will be unavailable
  extractProperties(*stream, *codec_ctx_);
}

FFMpegStream::~FFMpegStream()
{
  stream_ = nullptr; //TODO: check this
  av_packet_free(&pkt_);
  avcodec_free_context(&codec_ctx_);
  av_dict_free(&opts_);
}


MediaFramePtr FFMpegStream::frame(const int64_t timestamp)
{
  assert(parent_);
  assert(codec_ctx_);
  assert(pkt_);

  if ((timestamp >= 0) && (last_timestamp_ != timestamp)) {
    // TODO: more checks to prevent unneeded seek
    if (!seek(timestamp)) {
      std::cerr << "Failed to seek: " << timestamp << std::endl;
      return nullptr;
    }
  } // else read next frame

  return frame(*parent_, *codec_ctx_, *pkt_, stream_->index);
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


void FFMpegStream::extractProperties(const AVStream& stream, const AVCodecContext& context)
{
  assert(context.codec);
  MediaPropertyObject::setProperty(MediaProperty::CODEC, std::string(context.codec->name));
  // TODO: if we use this things could go wrong on container != essence
  auto base = stream.time_base;
  if (base.den > 0) {
    Rational timescale(base.num, base.den);
    MediaPropertyObject::setProperty(MediaProperty::TIMESCALE, timescale);
  }

  // TODO: stream durations
  if (type_ == StreamType::VISUAL) {
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
  const PixelFormat p_format = convertPixelFormat(context.pix_fmt);
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
  const SampleFormat s_format = convertSampleFormat(context.sample_fmt);
  this->setProperty(MediaProperty::AUDIO_FORMAT, s_format);
  const ChannelLayout layout = convertChannelLayout(context.channel_layout);
  this->setProperty(MediaProperty::AUDIO_LAYOUT, layout);

  //stream.codecpar->color_space;
}

bool FFMpegStream::seek(const int64_t timestamp)
{
  assert(parent_);
  assert(stream_);
  assert(codec_ctx_);
  avcodec_flush_buffers(codec_ctx_);
  int ret = av_seek_frame(parent_, stream_->index, timestamp, SEEK_DIRECTION);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    std::cerr << "Could not seek frame: " << err.data() << std::endl;
    return false;
  }
  return true;
}


void FFMpegStream::setupForVideo(const AVStream& strm, Buffers& bufs, AVFilterGraph& graph, int& pix_fmt) const
{
  return;
  //TODO: this should be done after decode
  //TODO: un-c
  char filter_args[512];
  snprintf(filter_args, sizeof(filter_args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
           strm.codecpar->width,
           strm.codecpar->height,
           strm.codecpar->format,
           strm.time_base.num,
           strm.time_base.den,
           strm.codecpar->sample_aspect_ratio.num,
           strm.codecpar->sample_aspect_ratio.den
           );
  int ret = avfilter_graph_create_filter(&bufs.source_, avfilter_get_by_name("buffer"), "in", filter_args, nullptr, &graph);
  if (ret < 0) {
    av_strerror(ret, err.data(), 1024);
    std::cerr << "Could not create filter(source): " << err.data() << std::endl;
    return;
  }
  ret = avfilter_graph_create_filter(&bufs.sink_, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, &graph);
  if (ret < 0) {
    av_strerror(ret, err.data(), 1024);
    std::cerr << "Could not create filter(sink): " << err.data() << std::endl;
    return;
  }

  AVFilterContext* last_filter = bufs.source_;

  // FIXME: we should know if it's interlaced or not when reading a frame, then setup deinterlace filter, once.
  //  if (ms->video_interlacing != ScanMethod::PROGRESSIVE) {
  //    AVFilterContext* yadif_filter;
  //    char yadif_args[100];
  //    snprintf(yadif_args, sizeof(yadif_args), "mode=3:parity=%d", ((ms->video_interlacing == ScanMethod::TOP_FIRST) ? 0 : 1));
  //    avfilter_graph_create_filter(&yadif_filter, avfilter_get_by_name("yadif"), "yadif", yadif_args, nullptr, filter_graph);

  //    avfilter_link(last_filter, 0, yadif_filter, 0);
  //    last_filter = yadif_filter;
  //  }

  enum AVPixelFormat valid_pix_fmts[] = {
    AV_PIX_FMT_RGB24,
    AV_PIX_FMT_RGBA,
    AV_PIX_FMT_NONE
  };

  pix_fmt = avcodec_find_best_pix_fmt_of_list(valid_pix_fmts,
                                              static_cast<enum AVPixelFormat>(strm.codecpar->format),
                                              1,
                                              nullptr);
  char format_args[100];
  const char* chosen_format = av_get_pix_fmt_name(static_cast<enum AVPixelFormat>(pix_fmt));
  snprintf(format_args, sizeof(format_args), "pix_fmts=%s", chosen_format);

  AVFilterContext* format_conv;
  ret = avfilter_graph_create_filter(&format_conv, avfilter_get_by_name("format"), "fmt", format_args, nullptr, &graph);
  avfilter_link(last_filter, 0, format_conv, 0);
  avfilter_link(format_conv, 0, bufs.sink_, 0);

  avfilter_graph_config(&graph, nullptr);
}

void FFMpegStream::setupForAudio(const AVStream& strm, Buffers& bufs, AVFilterGraph& graph, AVCodecContext& codec_context) const
{
  if (codec_context.channel_layout == 0) {
    codec_context.channel_layout = av_get_default_channel_layout(strm.codecpar->channels);
  }

  //  // set up cache
  //  queue.append(av_frame_alloc());
  //  if (timeline_info.reverse) {
  //    AVFrame* reverse_frame = av_frame_alloc();

  //    reverse_frame->format = SAMPLE_FORMAT;
  //    reverse_frame->nb_samples = current_audio_freq()*2;
  //    reverse_frame->channel_layout = sequence->audioLayout();
  //    reverse_frame->channels = av_get_channel_layout_nb_channels(sequence->audioLayout());
  //    av_frame_get_buffer(reverse_frame, 0);

  //    queue.append(reverse_frame);
  //  }

  //TODO: un-c
  char filter_args[512];
  snprintf(filter_args, sizeof(filter_args), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
           strm.time_base.num,
           strm.time_base.den,
           strm.codecpar->sample_rate,
           av_get_sample_fmt_name(codec_context.sample_fmt),
           codec_context.channel_layout
           );

  avfilter_graph_create_filter(&bufs.source_, avfilter_get_by_name("abuffer"), "in", filter_args, nullptr, &graph);
  avfilter_graph_create_filter(&bufs.sink_, avfilter_get_by_name("abuffersink"), "out", nullptr, nullptr, &graph);

  enum AVSampleFormat sample_fmts[] = { SAMPLE_FORMAT,  static_cast<AVSampleFormat>(-1) };
  int err_code = av_opt_set_int_list(bufs.sink_, "sample_fmts", sample_fmts, -1, AV_OPT_SEARCH_CHILDREN);
  if (err_code < 0) {
    av_strerror(err_code, err.data(), ERR_LEN);
    std::cerr << "Could not set output sample format: " << err.data() << std::endl;
  }

  int64_t channel_layouts[] = { AV_CH_LAYOUT_STEREO, static_cast<AVSampleFormat>(-1) };
  err_code = av_opt_set_int_list(bufs.sink_, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN);
  if (err_code < 0) {
    av_strerror(err_code, err.data(), ERR_LEN);
    std::cerr << "Could not set output channel layout: " << err.data() << std::endl;
  }

  //  int target_sample_rate = current_audio_freq();

  //  double playback_speed = timeline_info.speed * ftg->speed;

  //  if (qFuzzyCompare(playback_speed, 1.0)) {
  //    avfilter_link(buffersrc_ctx, 0, buffersink_ctx, 0);
  //  } else if (timeline_info.maintain_audio_pitch) {
  //    AVFilterContext* previous_filter = buffersrc_ctx;
  //    AVFilterContext* last_filter = buffersrc_ctx;

  //    char speed_param[10];

  //    double base = (playback_speed > 1.0) ? 2.0 : 0.5;

  //    double speedlog = log(playback_speed) / log(base);
  //    int whole2 = qFloor(speedlog);
  //    speedlog -= whole2;

  //    if (whole2 > 0) {
  //      snprintf(speed_param, sizeof(speed_param), "%f", base);
  //      for (int i=0;i<whole2;i++) {
  //        AVFilterContext* tempo_filter = nullptr;
  //        avfilter_graph_create_filter(&tempo_filter, avfilter_get_by_name("atempo"), "atempo", speed_param, nullptr, filter_graph);
  //        avfilter_link(previous_filter, 0, tempo_filter, 0);
  //        previous_filter = tempo_filter;
  //      }
  //    }

  //    snprintf(speed_param, sizeof(speed_param), "%f", qPow(base, speedlog));
  //    last_filter = nullptr;
  //    avfilter_graph_create_filter(&last_filter, avfilter_get_by_name("atempo"), "atempo", speed_param, nullptr, filter_graph);
  //    avfilter_link(previous_filter, 0, last_filter, 0);
  //    //				}

  //    avfilter_link(last_filter, 0, buffersink_ctx, 0);
  //  } else {
  //    target_sample_rate = qRound64(target_sample_rate / playback_speed);
  //    avfilter_link(buffersrc_ctx, 0, buffersink_ctx, 0);
  //  }

  //  int sample_rates[] = { target_sample_rate, 0 };
  //  if (av_opt_set_int_list(buffersink_ctx, "sample_rates", sample_rates, 0, AV_OPT_SEARCH_CHILDREN) < 0) {
  //    qCritical() << "Could not set output sample rates";
  //  }

  avfilter_graph_config(&graph, nullptr);

  //  audio_playback.reset = true;
}

void FFMpegStream::setupDecoder(const AVCodecID codec_id, AVDictionary* dict) const
{
  if (codec_id == AV_CODEC_ID_H264) {
    av_dict_set(&dict, "tune", "fastdecode", 0);
    av_dict_set(&dict, "tune", "zerolatency", 0);
  }
}


constexpr media_handling::PixelFormat FFMpegStream::convertPixelFormat(const AVPixelFormat format) const
{
  PixelFormat converted {PixelFormat::UNKNOWN};
  switch (format) {
    case AV_PIX_FMT_RGB24:
      converted = PixelFormat::RGB24;
      break;
    case AV_PIX_FMT_YUV420P:
      converted = PixelFormat::YUV420;
      break;
    case AV_PIX_FMT_YUV422P:
      converted = PixelFormat::YUV422;
      break;
    case AV_PIX_FMT_YUV444P:
      converted = PixelFormat::YUV444;
      break;
    default:
      converted = PixelFormat::UNKNOWN;
      break;
  }
  return converted;
}


constexpr media_handling::SampleFormat FFMpegStream::convertSampleFormat(const AVSampleFormat format) const
{
  SampleFormat converted {SampleFormat::NONE};

  switch (format) {
    case AV_SAMPLE_FMT_NONE:
      [[fallthrough]];
    case AV_SAMPLE_FMT_NB:
      converted = SampleFormat::NONE;
      break;
    case AV_SAMPLE_FMT_U8:
      converted = SampleFormat::UNSIGNED_8;
      break;
    case AV_SAMPLE_FMT_S16:
      converted = SampleFormat::SIGNED_16;
      break;
    case AV_SAMPLE_FMT_S32:
      converted = SampleFormat::SIGNED_32;
      break;
    case AV_SAMPLE_FMT_S64:
      converted = SampleFormat::SIGNED_64;
      break;
    case AV_SAMPLE_FMT_FLT:
      converted = SampleFormat::FLOAT;
      break;
    case AV_SAMPLE_FMT_DBL:
      converted = SampleFormat::DOUBLE;
      break;
    case AV_SAMPLE_FMT_U8P:
      converted = SampleFormat::UNSIGNED_8P;
      break;
    case AV_SAMPLE_FMT_S16P:
      converted = SampleFormat::SIGNED_16P;
      break;
    case AV_SAMPLE_FMT_S32P:
      converted = SampleFormat::SIGNED_32P;
      break;
    case AV_SAMPLE_FMT_S64P:
      converted = SampleFormat::SIGNED_64P;
      break;
    case AV_SAMPLE_FMT_FLTP:
      converted = SampleFormat::FLOAT_P;
      break;
    case AV_SAMPLE_FMT_DBLP:
      converted = SampleFormat::DOUBLE_P;
      break;
  }

  return converted;
}


constexpr media_handling::ChannelLayout FFMpegStream::convertChannelLayout(const uint64_t layout) const
{
  media_handling::ChannelLayout conv_layout = ChannelLayout::UNKNOWN;

  switch (layout)
  {
    case AV_CH_LAYOUT_MONO:
      conv_layout = ChannelLayout::MONO;
      break;
    case AV_CH_LAYOUT_STEREO:
      conv_layout = ChannelLayout::STEREO;
      break;
    case AV_CH_LAYOUT_2POINT1:
      conv_layout = ChannelLayout::STEREO_LFE;
      break;
    case AV_CH_LAYOUT_2_1:
      conv_layout = ChannelLayout::THREE_SURROUND;
      break;
    case AV_CH_LAYOUT_SURROUND:
      conv_layout = ChannelLayout::THREE_STEREO;
      break;
    case AV_CH_LAYOUT_3POINT1:
      conv_layout = ChannelLayout::THREE_SURROUND_LFE;
      break;
    case AV_CH_LAYOUT_4POINT0:
      conv_layout = ChannelLayout::FOUR_SURROUND;
      break;
    case AV_CH_LAYOUT_QUAD:
      conv_layout = ChannelLayout::FOUR_STEREO;
      break;
    case AV_CH_LAYOUT_4POINT1:
      conv_layout = ChannelLayout::FOUR_SURROUND_LFE;
      break;
    case AV_CH_LAYOUT_5POINT0:
      conv_layout = ChannelLayout::FIVE;
      break;
    case AV_CH_LAYOUT_5POINT1:
      conv_layout = ChannelLayout::FIVE_LFE;
      break;
    case AV_CH_LAYOUT_6POINT0:
      conv_layout = ChannelLayout::SIX;
      break;
    case AV_CH_LAYOUT_6POINT1:
      conv_layout = ChannelLayout::SIX_LFE;
      break;
    case AV_CH_LAYOUT_7POINT0:
      conv_layout = ChannelLayout::SEVEN;
      break;
    case AV_CH_LAYOUT_7POINT1:
      conv_layout = ChannelLayout::SEVEN_LFE;
      break;
  }

  return conv_layout;
}

void FFMpegStream::extractFrameProperties()
{
  std::optional<media_handling::FieldOrder> order;

  if (auto tmp_frame = this->frame(0)) {
    tmp_frame->extractProperties();
    bool is_valid;
    auto val = tmp_frame->property<media_handling::FieldOrder>(MediaProperty::FIELD_ORDER, is_valid);
    if (is_valid) {
      MediaPropertyObject::setProperty(MediaProperty::FIELD_ORDER, val);
    }
    auto par = MediaPropertyObject::property<Rational>(MediaProperty::PIXEL_ASPECT_RATIO, is_valid);
    if (!is_valid || (par == Rational{0,1}) ) {
      // Couldn't find it or value doesn't make sense
      par = tmp_frame->property<Rational>(MediaProperty::PIXEL_ASPECT_RATIO, is_valid);
      if (is_valid) {
        MediaPropertyObject::setProperty(MediaProperty::PIXEL_ASPECT_RATIO, par);
      }
    }

  }

}



MediaFramePtr FFMpegStream::frame(AVFormatContext& format_ctx,
                                  AVCodecContext& codec_ctx,
                                  AVPacket& pkt,
                                  const int stream_idx) const
{
  int err_code = 0;

  AVFrameUPtr frame(av_frame_alloc());
  while (err_code >= 0)
  {
    av_packet_unref(&pkt);
    err_code = av_read_frame(&format_ctx, &pkt);
    if (err_code < 0) {
      av_strerror(err_code, err.data(), ERR_LEN);
      std::cerr << "Failed to read frame: " << err.data() << std::endl;
      break;
    }

    if (pkt.stream_index != stream_idx) {
      continue;
    }

    err_code = avcodec_send_packet(&codec_ctx, &pkt);
    if (err_code < 0) {
      av_strerror(err_code, err.data(), ERR_LEN);
      std::cerr << "Failed sending a packet for decoding: " << err.data() << std::endl;
      break;
    }

    int dec_err_code = 0;
    while (dec_err_code >= 0) {
      dec_err_code = avcodec_receive_frame(&codec_ctx, frame.get());
      if (dec_err_code == 0) {
        // successful read
        assert(type_ != media_handling::StreamType::UNKNOWN);
        return std::make_shared<media_handling::FFMpegMediaFrame>(std::move(frame), type_ == StreamType::VISUAL);
      }

      if ( (dec_err_code == AVERROR(EAGAIN)) || (dec_err_code == AVERROR_EOF) ) {
        break;
      } else {
        av_strerror(dec_err_code, err.data(), ERR_LEN);
        std::cerr << "Failed to decode: " << err.data() << std::endl;
        break;
      }
    }//while
  }//while
  return nullptr;
}
