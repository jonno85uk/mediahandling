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

using namespace media_handling;
using namespace media_handling::ffmpeg;


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
  ASSERT_TRUE(src->visualStream(0)->type() == media_handling::StreamType::VISUAL);
  ASSERT_FALSE(src->visualStream(1));
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamPixelFormat)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  bool is_valid;
  auto fmt = stream->property<media_handling::PixelFormat>(media_handling::MediaProperty::PIXEL_FORMAT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(fmt, media_handling::PixelFormat::YUV420);
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamDimensions)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  bool is_valid;
  auto dims = stream->property<media_handling::Dimensions>(media_handling::MediaProperty::DIMENSIONS, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(dims.width, 1920);
  ASSERT_EQ(dims.height, 1080);
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamPAR)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  bool is_valid;
  auto par = stream->property<media_handling::Rational>(media_handling::MediaProperty::PIXEL_ASPECT_RATIO, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_TRUE(par == 1);
}

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

TEST (FFMpegStreamTest, Openh264FHDVisualStreamFieldOrder)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  bool is_valid;
  auto order = stream->property<media_handling::FieldOrder>(media_handling::MediaProperty::FIELD_ORDER, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_TRUE(order == media_handling::FieldOrder::PROGRESSIVE);
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamReadFrame)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  auto frame = stream->frame(0);
  ASSERT_TRUE(frame != nullptr);
  ASSERT_TRUE(frame->size() > 0);
  ASSERT_EQ(frame->timestamp(), 0);
}

TEST (FFMpegStreamTest, Openh264FHDVisualStreamReadTenthFrame)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->visualStream(0);
  bool is_valid;
  auto t_base = stream->property<Rational>(MediaProperty::TIMESCALE, is_valid);
  auto pos = int64_t(std::round(10 / boost::rational_cast<double>(t_base)));
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


TEST (FFMpegStreamTest, Openh264FHDAudioStream)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  ASSERT_TRUE(src->audioStream(0));
  ASSERT_TRUE(src->audioStream(0)->type() == media_handling::StreamType::AUDIO);
  ASSERT_FALSE(src->audioStream(1));
}

TEST (FFMpegStreamTest, Openh264FHDAudioStreamChannels)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->audioStream(0);
  bool is_valid;
  auto channels = stream->property<int32_t>(media_handling::MediaProperty::AUDIO_CHANNELS, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(channels, 2);
}

TEST (FFMpegStreamTest, Openh264FHDAudioStreamSamplingRate)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->audioStream(0);
  bool is_valid;
  auto sample_rate = stream->property<int32_t>(media_handling::MediaProperty::AUDIO_SAMPLING_RATE, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(sample_rate, 48000);
}

TEST (FFMpegStreamTest, Openh264FHDAudioStreamSamplingFormat)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->audioStream(0);
  bool is_valid;
  auto sample_format = stream->property<media_handling::SampleFormat>(media_handling::MediaProperty::AUDIO_FORMAT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(sample_format, media_handling::SampleFormat::FLOAT_P);
}

TEST (FFMpegStreamTest, Openh264FHDAudioStreamReadFrame)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->audioStream(0);
  auto frame = stream->frame(0);
  ASSERT_TRUE(frame != nullptr);
  ASSERT_TRUE(frame->size() > 0);
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

TEST (FFMpegStreamTest, Openh264FHDAudioStreamChannelLayout)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->audioStream(0);
  bool is_valid;
  auto format = stream->property<media_handling::ChannelLayout>(media_handling::MediaProperty::AUDIO_LAYOUT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(format, ChannelLayout::STEREO);
}

TEST (FFMpegStreamTest, Openh264FHDAudioStreamReadFrameToEOS)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  media_handling::MediaSourcePtr src = std::make_shared<FFMpegSource>(fname);
  auto stream = src->audioStream(0);
  MediaFramePtr frame = stream->frame(0);
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

