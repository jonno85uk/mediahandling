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

class OpenFileParameterTests : public testing::TestWithParam<std::string>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (OpenFileParameterTests, CheckNoThrow)
{
  auto path = this->GetParam();
  EXPECT_NO_THROW(source_ = std::make_unique<FFMpegSource>(path));
}

TEST_P (OpenFileParameterTests, CheckFileName)
{
  auto path = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);

  bool is_valid;
  auto prop_fname = source_->property<std::string>(MediaProperty::FILENAME, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_fname, path);
}


INSTANTIATE_TEST_CASE_P(
      FFMpegSourceTest,
      OpenFileParameterTests,
      testing::Values("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4",
                      "./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS",
                      "./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov",
                      "./ReferenceMedia/Audio/ogg/5_1.ogg"
));


class DurationParameterTests : public testing::TestWithParam<std::tuple<std::string, int64_t>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (DurationParameterTests, CheckIsEqual)
{
  auto [path, duration] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  bool is_valid;
  auto result = source_->property<int64_t>(MediaProperty::DURATION, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(result, duration);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegSourceTest,
      DurationParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", 14976000LL),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", 5728000LL),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", 2000000LL),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", 3000000LL)
));

TEST (FFMpegSourceTest, h264FHDDimensions)
{
  std::string fname = "./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";
  FFMpegSource src(fname);

  bool is_valid;
  src.property<media_handling::Dimensions>(MediaProperty::DIMENSIONS, is_valid);
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

class StreamCountParameterTests : public testing::TestWithParam<std::tuple<std::string, int32_t>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P(StreamCountParameterTests, CheckTotal)
{
  auto [path, count] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);

  bool is_valid;
  auto streams = source_->property<int32_t>(MediaProperty::STREAMS, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(streams, count);
}


INSTANTIATE_TEST_CASE_P(
      FFMpegSourceTest,
      StreamCountParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", 2),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", 3),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", 3),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", 1)
));

class AudioStreamCountParameterTests : public testing::TestWithParam<std::tuple<std::string, int32_t>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P(AudioStreamCountParameterTests, CheckEqual)
{
  auto [path, count] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);

  bool is_valid;
  auto streams = source_->property<int32_t>(MediaProperty::AUDIO_STREAMS, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(streams, count);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegSourceTest,
      AudioStreamCountParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", 1),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", 1),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", 1),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", 1),
                      std::make_tuple("./ReferenceMedia/Audio/ac3/5_1.ac3", 1)
));

class VideoStreamCountParameterTests : public testing::TestWithParam<std::tuple<std::string, int32_t>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P(VideoStreamCountParameterTests, CheckEqual)
{
  auto [path, count] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);

  bool is_valid;
  auto streams = source_->property<int32_t>(MediaProperty::VIDEO_STREAMS, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(streams, count);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegSourceTest,
      VideoStreamCountParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", 1),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", 1),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", 1),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", 0),
                      std::make_tuple("./ReferenceMedia/Audio/ac3/5_1.ac3", 0)
));

class FileFormatParameterTests : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (FileFormatParameterTests, CheckEqual)
{
  auto [path, format] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  bool is_valid;
  auto prop_format = source_->property<std::string>(MediaProperty::FILE_FORMAT, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_format, format);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegSourceTest,
      FileFormatParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", "QuickTime / MOV"),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", "MPEG-TS (MPEG-2 Transport Stream)"),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", "QuickTime / MOV"),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", "Ogg"),
                      std::make_tuple("./ReferenceMedia/Audio/ac3/5_1.ac3", "raw AC-3")
));

class BitrateParameterTests : public testing::TestWithParam<std::tuple<std::string, int64_t>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (BitrateParameterTests, CheckEqual)
{
  auto [path, bitrate] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);
  bool is_valid;
  auto prop_rate = source_->property<int64_t>(MediaProperty::BITRATE, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_rate, bitrate);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegSourceTest,
      BitrateParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", 98630292LL),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", 22104670LL),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", 147038400LL),
                      std::make_tuple("./ReferenceMedia/Audio/ogg/5_1.ogg", 123194LL),
                      std::make_tuple("./ReferenceMedia/Audio/ac3/5_1.ac3", 191999LL)
));

class FrameRateParameterTests : public testing::TestWithParam<std::tuple<std::string, Rational>>
{
  public:
    std::unique_ptr<FFMpegSource> source_;
};

TEST_P (FrameRateParameterTests, CheckEqual)
{

  auto [path, framerate] = this->GetParam();
  source_ = std::make_unique<FFMpegSource>(path);

  bool is_valid;
  auto prop_frate = source_->property<Rational>(MediaProperty::FRAME_RATE, is_valid);
  ASSERT_TRUE(is_valid);
  ASSERT_EQ(prop_frate, framerate);
}

INSTANTIATE_TEST_CASE_P(
      FFMpegSourceTest,
      FrameRateParameterTests,
      testing::Values(std::make_tuple("./ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4", Rational(50,1)),
                      std::make_tuple("./ReferenceMedia/Video/mpeg2/interlaced_avc.MTS", Rational(25,1)),
                      std::make_tuple("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov", Rational(30,1))
));


