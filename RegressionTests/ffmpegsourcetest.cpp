
#include <gtest/gtest.h>

#include "ffmpegsource.h"

using media_handling::ffmpeg::FFMpegSource;
using media_handling::MediaProperty;

class FFMpegSourceTestable : public FFMpegSource
{
  public:
    virtual ~FFMpegSourceTestable() {}
    bool initialise() final
    {
      return true;
    }
};

TEST (FFMpegSourceTest, DefaultAllocate)
{
  FFMpegSourceTestable thing;
  bool is_valid;
  auto prop =  thing.property<std::string>(MediaProperty::FILENAME, is_valid);
  ASSERT_FALSE(is_valid);
}

TEST (FFMpegSourceTest, SetFilePath)
{
  FFMpegSourceTestable thing;
  auto path = "test";
  thing.setFilePath(path);
  bool is_valid;
  auto prop = thing.property<std::string>(MediaProperty::FILENAME, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop, path);
}


TEST (FFMpegSourceTest, StreamsInvalidFile)
{
  FFMpegSourceTestable thing;
  thing.setFilePath("test");
  ASSERT_TRUE(thing.audioStream(0) == nullptr);
  ASSERT_TRUE(thing.visualStream(0) == nullptr);
}


TEST (FFMpegSourceTest, Openh264FHDFileNoThrow)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  EXPECT_NO_THROW(FFMpegSource src(fname));
}

TEST (FFMpegSourceTest, h264FHDDuration)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto dur = src.property<int64_t>(MediaProperty::DURATION, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_TRUE(dur == 14976000);
}

TEST (FFMpegSourceTest, h264FHDDimensions)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto dims = src.property<media_handling::Dimensions>(MediaProperty::DIMENSIONS, is_valid);
  ASSERT_FALSE(is_valid); // Property is of the stream
}

TEST (FFMpegSourceTest, h264FHDCodec)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto codec = src.property<std::string>(MediaProperty::CODEC, is_valid);
  ASSERT_FALSE(is_valid); // Property is of the stream
}

TEST (FFMpegSourceTest, h264FHDStreamCount)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto streams = src.property<int32_t>(MediaProperty::STREAMS, is_valid);
  ASSERT_TRUE(is_valid); 
  ASSERT_TRUE(streams == 2);
}

TEST (FFMpegSourceTest, h264FHDAudioStreamCount)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto streams = src.property<int32_t>(MediaProperty::AUDIO_STREAMS, is_valid);
  ASSERT_TRUE(is_valid); 
  ASSERT_TRUE(streams == 1);
}

TEST (FFMpegSourceTest, h264FHDVideoStreamCount)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto streams = src.property<int32_t>(MediaProperty::VIDEO_STREAMS, is_valid);
  ASSERT_TRUE(is_valid); 
  ASSERT_TRUE(streams == 1);
}

TEST (FFMpegSourceTest, h264FHDFilename)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto prop_fname = src.property<std::string>(MediaProperty::FILENAME, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_TRUE(prop_fname == fname);
}

TEST (FFMpegSourceTest, h264FHDFileformat)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto format = src.property<std::string>(MediaProperty::FILE_FORMAT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_TRUE(format == "QuickTime / MOV");
}

TEST (FFMpegSourceTest, h264FHDBitrate)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto bitrate = src.property<int64_t>(MediaProperty::BITRATE, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_TRUE(bitrate == 98630292);
}
