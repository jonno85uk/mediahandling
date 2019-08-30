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

#include "ffmpegsource.h"

using namespace media_handling;
using namespace media_handling::ffmpeg;

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

TEST (FFMpegSourceTest, h264FHDFrameRate)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  auto frate = src.property<Rational>(MediaProperty::FRAME_RATE, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(frate, Rational(50,1));
}
