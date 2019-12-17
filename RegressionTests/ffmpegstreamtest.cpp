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

#include <gtest/gtest.h>

#include "ffmpegstream.h"
#include "ffmpegsource.h"
#include "mediahandling.h"

using namespace media_handling;
using namespace media_handling::ffmpeg;

#ifdef PNG_OUT
#include <fstream>
#include <iostream>
#include <TinyPngOut/TinyPngOut.hpp>
#endif


TEST (FFMpegStreamTest, NullInst)
{
  AVFormatContext ctx;
  AVStream strm;
  EXPECT_THROW(FFMpegStream(nullptr, nullptr), std::exception);
  EXPECT_THROW(FFMpegStream(&ctx, nullptr), std::exception);
  EXPECT_THROW(FFMpegStream(nullptr, &strm), std::exception);
}

TEST (FFMpegStreamTest, Openh264FHDVisualStream)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  ASSERT_TRUE(src->visualStream(0));
  ASSERT_TRUE(src->visualStream(0)->type() == media_handling::StreamType::VIDEO);
  ASSERT_FALSE(src->visualStream(1));
}

class VisualStreamPixelFormatParameterTests : public testing::TestWithParam<std::tuple<std::string, media_handling::PixelFormat>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (VisualStreamPixelFormatParameterTests, CheckEqual)
{
  auto [path, format] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->visualStream(0);
  bool is_valid;
  auto prop_fmt = stream->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_fmt, format);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      VisualStreamPixelFormatParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", PixelFormat::YUV420),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", PixelFormat::YUV420),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", PixelFormat::YUV422)
));


class DimensionsParameterTests : public testing::TestWithParam<std::tuple<std::string, Dimensions>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};


TEST_P (DimensionsParameterTests, CheckEqual)
{
  auto [path, dims] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->visualStream(0);
  bool is_valid;
  auto prop_dims = stream->property<Dimensions>(MediaProperty::DIMENSIONS, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_dims.width, dims.width);
  ASSERT_EQ(prop_dims.height, dims.height);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      DimensionsParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", Dimensions{1920,1080}),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", Dimensions{1920,1080}),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", Dimensions{1920,1080})
));

class PARParameterTests : public testing::TestWithParam<std::tuple<std::string, Rational>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};


TEST_P (PARParameterTests, CheckEqual)
{
  auto [path, par] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->visualStream(0);
  bool is_valid;
  auto prop_par = stream->property<Rational>(MediaProperty::PIXEL_ASPECT_RATIO, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_par, par);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      PARParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", Rational{1,1}),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", Rational{1,1}),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", Rational{1,1})
));


TEST (FFMpegStreamTest, Openh264FHDVisualStreamDAR)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  bool is_valid;
  auto dar = stream->property<media_handling::Rational>(media_handling::MediaProperty::DISPLAY_ASPECT_RATIO, is_valid);
  ASSERT_FALSE(is_valid);
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamFrameCount)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  bool is_valid;
  auto frames = stream->property<int64_t>(media_handling::MediaProperty::FRAME_COUNT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_TRUE(frames == 748);
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamTimescale)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  bool is_valid;
  auto tscale = stream->property<Rational>(media_handling::MediaProperty::TIMESCALE, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_TRUE(tscale == Rational(1,12800));
}

class FieldOrderParameterTests : public testing::TestWithParam<std::tuple<std::string, FieldOrder>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (FieldOrderParameterTests, CheckIsEqual)
{
  auto [path, order] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->visualStream(0);
  bool is_valid;
  auto prop_order = stream->property<FieldOrder>(MediaProperty::FIELD_ORDER, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_order, order);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      FieldOrderParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", FieldOrder::PROGRESSIVE),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", FieldOrder::TOP_FIRST),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", FieldOrder::PROGRESSIVE)
));


class CodecParameterTests : public testing::TestWithParam<std::tuple<std::string, Codec>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (CodecParameterTests, CheckIsEqual)
{
  auto [path, codec] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->visualStream(0);
  bool is_valid;
  auto prop_codec = stream->property<Codec>(MediaProperty::CODEC, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_codec, codec);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      CodecParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", Codec::H264),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", Codec::H264),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", Codec::DNXHD)
));

class ColourInfoTests : public testing::TestWithParam<std::tuple<std::string, ColourPrimaries>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (ColourInfoTests, CheckIsEqual)
{
  auto [path, src_prim] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->visualStream(0);
  bool is_valid;
  auto prim = stream->property<ColourPrimaries>(MediaProperty::COLOUR_PRIMARIES, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prim, src_prim);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      ColourInfoTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", ColourPrimaries::UNKNOWN),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", ColourPrimaries::UNKNOWN),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", ColourPrimaries::UNKNOWN),
                      std::make_tuple("./ReferenceMedia/Video/h264/_20111231T141252_5.MP4", ColourPrimaries::BT_709)
));

TEST (FFMpegStreamTest, Openh264FHDVisualStreamReadFrame)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  auto frame = stream->frame(0);
  ASSERT_TRUE(frame != nullptr);
  ASSERT_EQ(frame->timestamp(), 0);
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamReadTenthFrame)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  bool is_valid;
  auto t_base = stream->property<Rational>(MediaProperty::TIMESCALE, is_valid);
  auto pos = int64_t(std::round(10 / t_base.toDouble()));
  auto frame = stream->frame(pos);
  ASSERT_TRUE(frame != nullptr);
  ASSERT_EQ(frame->timestamp(), pos);
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamReadNextFrame)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  MediaFramePtr frame = stream->frame(0);
  frame = stream->frame();
  ASSERT_EQ(frame->timestamp(), 256); // timebase/fps
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamReadToEOS)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  MediaFramePtr frame = stream->frame(0);

  auto sz = frame->lineSize(0);
  ASSERT_EQ(sz, 1920); // y
  sz = frame->lineSize(1);
  ASSERT_EQ(sz, 960); //  u
  sz = frame->lineSize(2);
  ASSERT_EQ(sz, 960); //  v

  int64_t timestamp = -1;
  int frames = 0;
  while (frame) {
    frame = stream->frame();
    if (frame) {
      frames++;
      timestamp = frame->timestamp();
    }
  }

  ASSERT_TRUE(frames > 0);
  ASSERT_TRUE(timestamp > 0);
}

TEST (FFMpegStreamTest, SetOutputFormatVideo)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);

  auto stream = src->visualStream(0);
  ASSERT_TRUE(stream->setOutputFormat(PixelFormat::YUV420));

  auto frame = stream->frame(100);
  auto data = frame->data();
  ASSERT_TRUE(data.data_ != nullptr);
  ASSERT_TRUE(data.data_size_ == 3110400);
  ASSERT_TRUE(data.line_size_ == 1920);
  ASSERT_TRUE(data.pix_fmt_ == PixelFormat::YUV420);
#ifdef PNG_OUT
  bool is_valid = false;
  auto dims = stream->property<Dimensions>(MediaProperty::DIMENSIONS, is_valid);
  if (is_valid) {
//    for (auto i = 0; i < dims.width * dims.height * 3; ++i)
//    {
//      uint8_t val = data[i];
//      auto thing = val * 2;
//    }
    std::ofstream out("test.png", std::ios::binary);
    TinyPngOut pngout(dims.width, dims.height, out);
    pngout.write(*data, dims.width * dims.height);
    out.close();

  }
#endif
}

TEST (FFMpegStreamTest, SetOutputFormatVideoScaled)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);

  auto stream = src->visualStream(0);
  ASSERT_TRUE(stream->setOutputFormat(PixelFormat::YUV420, {640, 480}));

  auto frame = stream->frame(0);
  auto data = frame->data();
  ASSERT_TRUE(data.data_ != nullptr);
  ASSERT_TRUE(data.data_size_ == 460800);
  ASSERT_TRUE(data.line_size_ == 640);
  ASSERT_TRUE(data.pix_fmt_ == PixelFormat::YUV420);
}


TEST (FFMpegStreamTest, Openh264FHDAudioStream)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  ASSERT_TRUE(src->audioStream(0));
  ASSERT_TRUE(src->audioStream(0)->type() == media_handling::StreamType::AUDIO);
  ASSERT_FALSE(src->audioStream(1));
}

class ChannelsParameterTests : public testing::TestWithParam<std::tuple<std::string, int32_t>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (ChannelsParameterTests, CheckEqual)
{
  auto [path, channels] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->audioStream(0);
  bool is_valid;
  auto prop_channels = stream->property<int32_t>(MediaProperty::AUDIO_CHANNELS, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_channels, channels);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      ChannelsParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", 2),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", 2),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", 2),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", 6),
                      std::make_tuple("./ReferenceMedia/Audio/ac3/5_1.ac3", 6)
));


class SampleFormatParameterTests : public testing::TestWithParam<std::tuple<std::string, SampleFormat>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (SampleFormatParameterTests, CheckEqual)
{
  auto [path, format] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->audioStream(0);
  bool is_valid;
  auto sample_format = stream->property<SampleFormat>(MediaProperty::AUDIO_FORMAT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(format, sample_format);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      SampleFormatParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", SampleFormat::FLOAT_P),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", SampleFormat::FLOAT_P),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", SampleFormat::SIGNED_16),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", SampleFormat::FLOAT_P),
                      std::make_tuple("./ReferenceMedia/Audio/ac3/5_1.ac3", SampleFormat::FLOAT_P)
));


class SampleRateParameterTests : public testing::TestWithParam<std::tuple<std::string, int32_t>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (SampleRateParameterTests, CheckEqual)
{
  auto [path, rate] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->audioStream(0);
  bool is_valid;
  auto sample_rate = stream->property<int32_t>(MediaProperty::AUDIO_SAMPLING_RATE, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(rate, sample_rate);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      SampleRateParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", 48000L),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", 48000L),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", 48000L),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", 44100LL),
                      std::make_tuple("./ReferenceMedia/Audio/ac3/5_1.ac3", 44100LL)
));


TEST (FFMpegStreamTest, Openh264FHDAudioStreamReadFrame)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->audioStream(0);
  auto frame = stream->frame(0);
  ASSERT_TRUE(frame != nullptr);
}

TEST (FFMpegStreamTest, Openh264FHDAudioStreamReadFrameProperties)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->audioStream(0);
  auto frame = stream->frame(0);
  frame->extractProperties();
  bool is_valid;
  auto format = frame->property<media_handling::SampleFormat>(media_handling::MediaProperty::AUDIO_FORMAT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(format, media_handling::SampleFormat::FLOAT_P);
}

class AudioLayoutParameterTests : public testing::TestWithParam<std::tuple<std::string, ChannelLayout>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (AudioLayoutParameterTests, CheckEqual)
{
  auto [path, layout] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  auto stream = source_->audioStream(0);
  bool is_valid;
  auto prop_layout = stream->property<ChannelLayout>(MediaProperty::AUDIO_LAYOUT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_layout, layout);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      AudioLayoutParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", ChannelLayout::STEREO),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", ChannelLayout::STEREO),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", ChannelLayout::STEREO),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", ChannelLayout::FIVE_STEREO_LFE),
                      std::make_tuple("./ReferenceMedia/Audio/ac3/5_1.ac3", ChannelLayout::FIVE_LFE)
));

TEST (FFMpegStreamTest, Openh264FHDAudioStreamReadFrameToEOS)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->audioStream(0);
  MediaFramePtr frame = stream->frame(0);

  auto sz = frame->lineSize(0);
  ASSERT_EQ(sz, 8192);
  sz = frame->lineSize(1);
  ASSERT_EQ(sz, 0);

  int64_t timestamp = -1;
  int frames = 0;
  while (frame) {
    frame = stream->frame();
    if (frame) {
      frames++;
      timestamp = frame->timestamp();
    }
  }

  ASSERT_TRUE(frames > 0);
  ASSERT_TRUE(timestamp > 0);
}

TEST (FFMpegStreamTest, SetOutputFormatAudio)
{
  auto fname = "./ReferenceMedia/Audio/ac3/5_1.ac3";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);

  auto stream = src->audioStream(0);
  ASSERT_TRUE(stream->setOutputFormat(SampleFormat::FLOAT));
}


class ImageReadingTests : public testing::TestWithParam<std::tuple<std::string, int, int, PixelFormat, Codec, int, bool>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (ImageReadingTests, PropertiesCorrect)
{
  auto [path, width, height, fmt, cdc, v_streams, sequence] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  if (sequence) {
    source_->setProperty(MediaProperty::SEQUENCE_PATTERN, std::string(""));
    ASSERT_TRUE(source_->initialise());
  }
  ASSERT_EQ(source_->visualStreams().size(), v_streams);
  ASSERT_EQ(source_->audioStreams().size(), 0);
  auto stream = source_->visualStream(0);
  ASSERT_TRUE(stream->type() == StreamType::IMAGE);
  bool okay = false;
  auto dims = stream->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(dims.width, width);
  ASSERT_EQ(dims.height, height);
  auto strm_fmt = stream->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(strm_fmt, fmt);
  auto strm_cdc = stream->property<Codec>(MediaProperty::CODEC, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(strm_cdc, cdc);
  
}

INSTANTIATE_TEST_CASE_P(
      FFMpegStreamTest,
      ImageReadingTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Image/test-001.png", 474, 889, PixelFormat::RGBA, Codec::PNG, 1, false),
                      std::make_tuple("./ReferenceMedia/Image/test-002.jpg", 800, 530, PixelFormat::YUVJ420, Codec::JPEG, 1, false),
                      std::make_tuple("./ReferenceMedia/Image/test-003.tiff", 640, 360, PixelFormat::RGB24, Codec::TIFF, 1, false),
                      std::make_tuple("./ReferenceMedia/Image/test-004.jp2", 640, 531, PixelFormat::RGBA, Codec::JPEG2000, 1, false),
                      std::make_tuple("./ReferenceMedia/Image/sequence/im_sequence-0001.dpx", 256, 144, PixelFormat::RGB24, Codec::DPX, 1, true)
));



TEST (FFMpegStreamTest, ImageSequenceAuto)
{
  const auto f_path = "./ReferenceMedia/Image/sequence/im_sequence-0001.dpx";
  auto source = std::make_unique<FFMpegSource>(f_path);
  ASSERT_EQ(source->visualStreams().size(), 1);
  ASSERT_EQ(source->audioStreams().size(), 0);
  auto stream = source->visualStream(0);
  ASSERT_TRUE(stream->type() == StreamType::VIDEO);
  bool okay = false;
  auto duration = source->property<int64_t>(MediaProperty::DURATION, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(duration, 4040000);
  auto rate = source->property<Rational>(MediaProperty::FRAME_RATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(rate, Rational(25,1));
}


TEST (FFMpegStreamTest, ImageSequenceAutoCAPEXT)
{
  const auto f_path = "./ReferenceMedia/Image/sequence/jp2/im_sequence0001.JP2";
  auto source = std::make_unique<FFMpegSource>(f_path);
  ASSERT_EQ(source->visualStreams().size(), 1);
  ASSERT_EQ(source->audioStreams().size(), 0);
  auto stream = source->visualStream(0);
  ASSERT_TRUE(stream->type() == StreamType::VIDEO);
  bool okay = false;
  auto duration = source->property<int64_t>(MediaProperty::DURATION, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(duration, 200000);
  auto rate = source->property<Rational>(MediaProperty::FRAME_RATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(rate, Rational(25,1));
}


TEST (FFMpegStreamTest, ImageSequenceManualFailure)
{
  const auto f_path = "./ReferenceMedia/Image/sequence/im_sequence-0001.dpx";
  auto source = std::make_unique<FFMpegSource>(f_path);
  source->setProperty(MediaProperty::SEQUENCE_PATTERN, std::string("im_sequence-%05d.dpx"));
  ASSERT_FALSE(source->initialise());
  source->setProperty(MediaProperty::SEQUENCE_PATTERN, std::string("im_sequence.%04d.dpx"));
  ASSERT_FALSE(source->initialise());
  source->setProperty(MediaProperty::SEQUENCE_PATTERN, std::string("im_sequence--%04d.dpx"));
  ASSERT_FALSE(source->initialise());
  source->setProperty(MediaProperty::SEQUENCE_PATTERN, std::string("im_sequence-%04d.exr"));
  ASSERT_FALSE(source->initialise());
  source->setProperty(MediaProperty::SEQUENCE_PATTERN, std::string("im_sequence-%04d.DPX"));
  ASSERT_FALSE(source->initialise());
  source->setProperty(MediaProperty::SEQUENCE_PATTERN, std::string("IM_SEQUENCE-%04d.DPX"));
  ASSERT_FALSE(source->initialise());
  source->setProperty(MediaProperty::SEQUENCE_PATTERN, std::string("im_sequence-%04d.dpx"));
  ASSERT_TRUE(source->initialise());
}

TEST (FFMpegStreamTest, ImageSequenceManualSuccess)
{
  const auto f_path = "./ReferenceMedia/Image/sequence/im_sequence-0001.dpx";
  auto source = std::make_unique<FFMpegSource>(f_path);
  source->setProperty(MediaProperty::SEQUENCE_PATTERN, std::string("im_sequence-%04d.dpx"));
  ASSERT_TRUE(source->initialise());
  ASSERT_EQ(source->visualStreams().size(), 1);
  ASSERT_EQ(source->audioStreams().size(), 0);
  auto stream = source->visualStream(0);
  ASSERT_TRUE(stream->type() == StreamType::VIDEO);
  bool okay = false;
  auto duration = source->property<int64_t>(MediaProperty::DURATION, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(duration, 4040000);
  auto rate = source->property<Rational>(MediaProperty::FRAME_RATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(rate, Rational(25,1));
}

TEST (FFMpegStreamTest, ImageSequenceAutoDisabled)
{
  media_handling::autoDetectImageSequences(false);
  const auto f_path = "./ReferenceMedia/Image/sequence/im_sequence-0001.dpx";
  auto source = std::make_unique<FFMpegSource>(f_path);
  ASSERT_EQ(source->visualStreams().size(), 1);
  ASSERT_EQ(source->audioStreams().size(), 0);
  auto stream = source->visualStream(0);
  ASSERT_TRUE(stream->type() == StreamType::IMAGE);
  bool okay = false;
  auto rate = source->property<Rational>(MediaProperty::FRAME_RATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(rate, Rational(25,1));
}

