#include <gtest/gtest.h>

#include "ffmpegstream.h"
#include "ffmpegsource.h"

using media_handling::ffmpeg::FFMpegStream;
using media_handling::ffmpeg::FFMpegSource;


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
  auto par = stream->property<double>(media_handling::MediaProperty::PIXEL_ASPECT_RATIO, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_DOUBLE_EQ(par, 1.0);
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


