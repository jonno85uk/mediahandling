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

extern "C" {
//#include <libavformat/avformat.h>
//#include <libavfilter/avfilter.h>
//#include <libavfilter/buffersink.h>
//#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}


constexpr AVSampleFormat SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
constexpr auto ERR_LEN = 256;

using media_handling::ffmpeg::FFMpegStream;
using media_handling::MediaFramePtr;

namespace  {
  char err[ERR_LEN];
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
  avcodec_parameters_to_context(codec_ctx_, stream_->codecpar);
  extractProperties(*stream, *codec_ctx_);
  setupDecoder(stream_->codecpar->codec_id, opts_);
  // Open codec
  int err_code = avcodec_open2(codec_ctx_, codec_, &opts_);
  if (err_code < 0) {
    av_strerror(err_code, err, ERR_LEN);
    std::cerr << "Could not open codec: " << err << std::endl;
    throw std::exception();
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
  frame_ = av_frame_alloc();
  assert(frame_);
  av_frame_make_writable(frame_);
}

FFMpegStream::~FFMpegStream()
{
  stream_ = nullptr; //TODO: check this
  av_packet_free(&pkt_);
  av_frame_free(&frame_);
}

MediaFramePtr FFMpegStream::frame(const int64_t timestamp)
{
  assert(parent_);
  assert(codec_ctx_);
  assert(pkt_);

  if (timestamp < 0) {
    std::cerr << "Invalid timestamp: " << timestamp << std::endl;
    return nullptr;
  }
//  MediaFramePtr frame;
  if (last_timestamp_ != timestamp) {
    // TODO: more checks to prevent unneeded seek
    seek(timestamp);
  }
  if (!read(parent_, pkt_)) {
    std::cerr << "Failed to read frame, timestamp=" << timestamp << std::endl;
    return nullptr;
  }
  AVFrame* frame = av_frame_alloc();
  decode(codec_ctx_, frame, pkt_);
  return MediaFramePtr();
}


bool FFMpegStream::setFrame(const int64_t timestamp, MediaFramePtr sample)
{
  // TODO:
  return false;
}

std::string FFMpegStream::repr()
{
  std::stringstream ss;
  // TODO:
  return ss.str();
}

void FFMpegStream::setProperties(const std::map<MediaProperty, std::any>& props)
{
  properties_ = props;
}

void FFMpegStream::setProperty(const MediaProperty prop, std::any value)
{
  properties_[prop] = value;
}

std::any FFMpegStream::property(const MediaProperty prop, bool& is_valid) const
{
  if (properties_.count(prop) > 0) {
    is_valid = true;
    return properties_.at(prop);
  }
  is_valid = false;
  return {};
}

void FFMpegStream::extractProperties(const AVStream& stream, const AVCodecContext& context)
{
  assert(context.codec);
  this->setProperty(MediaProperty::CODEC, std::string(context.codec->name));
  // TODO: if we use this things could go wrong on container != essence
  auto base = stream.time_base;
  if (base.den > 0) {
    const double timescale = static_cast<double>(base.num) / base.den;
    this->setProperty(MediaProperty::TIMESCALE, timescale);
  }
  PixelFormat format = convert(context.pix_fmt);
  this->setProperty(MediaProperty::PIXEL_FORMAT, format);
  Dimensions dims {context.width, context.height};
  this->setProperty(MediaProperty::DIMENSIONS, dims);
  if (stream.sample_aspect_ratio.den > 0) {
    auto par = static_cast<double>(stream.sample_aspect_ratio.num) / stream.sample_aspect_ratio.den;
    this->setProperty(MediaProperty::PIXEL_ASPECT_RATIO, par);
  }

  this->setProperty(MediaProperty::AUDIO_CHANNELS, static_cast<int32_t>(context.channels));
  this->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, static_cast<int32_t>(context.sample_rate));
}

bool FFMpegStream::seek(const int64_t timestamp)
{
  assert(parent_);
  assert(stream_);
  assert(codec_ctx_);
  avcodec_flush_buffers(codec_ctx_);
  int ret = av_seek_frame(parent_, stream_->index, timestamp, AVSEEK_FLAG_BACKWARD);
  if (ret < 0) {
    av_strerror(ret, err, ERR_LEN);
    std::cerr << "Could not seek frame: " << err << std::endl;
    return false;
  }
  return true;
}


bool FFMpegStream::read(AVFormatContext* ctxt, AVPacket* pkt)
{
  int ret = av_read_frame(ctxt, pkt);
  if (ret < 0) {
    av_strerror(ret, err, ERR_LEN);
    std::cerr << "Failed to read frame: " << err << std::endl;
    return false;
  }
  return true;
}

void FFMpegStream::setupForVideo(const AVStream& strm, Buffers& bufs, AVFilterGraph& graph, int& pix_fmt) const
{
  return;
  //TODO: this should be done after decode
  char err[1024];
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
    av_strerror(ret, err, 1024);
    std::cerr << "Could not create filter(source): " << err << std::endl;
    return;
  }
  ret = avfilter_graph_create_filter(&bufs.sink_, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, &graph);
  if (ret < 0) {
    av_strerror(ret, err, 1024);
    std::cerr << "Could not create filter(sink): " << err << std::endl;
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
    char err[1024];
    av_strerror(err_code, err, 1024);
    std::cerr << "Could not set output sample format: " << err << std::endl;
  }

  int64_t channel_layouts[] = { AV_CH_LAYOUT_STEREO, static_cast<AVSampleFormat>(-1) };
  err_code = av_opt_set_int_list(bufs.sink_, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN);
  if (err_code < 0) {
    char err[1024];
    av_strerror(err_code, err, 1024);
    std::cerr << "Could not set output channel layout: " << err << std::endl;
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


bool FFMpegStream::decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt)
{
  int ret;

  do {
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
      av_strerror(ret, err, ERR_LEN);
      std::cerr << "Error sending a packet for decoding: " << err << std::endl;
      return false;
    }
    ret = avcodec_receive_frame(dec_ctx, frame);
  } while (ret == AVERROR(EAGAIN)); // not enough packets fed to decoder

  if (ret == AVERROR_EOF) {
    return false;
  }

  if (ret < 0) {
    av_strerror(ret, err, ERR_LEN);
    std::cerr << "Error during decoding " << err << std::endl;
    return false;
  }
  return true;
}


bool FFMpegStream::encode()
{
  // TODO:
  return false;
}


bool FFMpegStream::write()
{
  // TODO:
  return false;
}


media_handling::PixelFormat FFMpegStream::convert(const AVPixelFormat format) const
{
  PixelFormat converted;
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
    default:
      converted = PixelFormat::UNKNOWN;
  }
  return converted;
}
