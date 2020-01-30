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
#include <cmath>

#include "ffmpegsink.h"
#include "ffmpegsource.h"
#include "rational.h"
#include "mediahandling.h"

using namespace media_handling;
using namespace media_handling::ffmpeg;

TEST (FFMpegSinkTest, AllocateNonExistent)
{
  EXPECT_ANY_THROW(FFMpegSink thing("null", {}, {}));
}

TEST (FFMpegSinkTest, AllocateOkPath)
{
  EXPECT_NO_THROW(FFMpegSink thing("./test.mp4", {}, {}));
}


TEST (FFMpegSinkTest, InitialiseFailNoCodec)
{
  FFMpegSink thing("./test.mp4", {}, {});
  ASSERT_FALSE(thing.initialise());
}

TEST (FFMpegSinkTest, InitialiseFailAudioAsVideoCodec)
{
  FFMpegSink thing("./test.mp4", {Codec::AAC}, {});
  ASSERT_FALSE(thing.initialise());
}

TEST (FFMpegSinkTest, InitialiseFailVideoAsAudioCodec)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::H264});
  ASSERT_FALSE(thing.initialise());
}

TEST (FFMpegSinkTest, InitialiseSuccessVideoCodec)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  ASSERT_TRUE(thing.visualStream(0) != nullptr);
  ASSERT_TRUE(!thing.visualStreams().empty());
  ASSERT_TRUE(thing.visualStreams().size() == 1);
}

TEST (FFMpegSinkTest, InitialiseSuccessAudioCodec)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::AAC});
  ASSERT_TRUE(thing.initialise());
  ASSERT_TRUE(thing.audioStream(0) != nullptr);
  ASSERT_TRUE(!thing.audioStreams().empty());
  ASSERT_TRUE(thing.audioStreams().size() == 1);
  ASSERT_TRUE(thing.audioStream(0) != nullptr);
  ASSERT_TRUE(!thing.audioStreams().empty());
  ASSERT_TRUE(thing.audioStreams().size() == 1);
}

TEST (FFMpegSinkTest, InitialiseSuccessCodecs)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {Codec::AAC});
  ASSERT_TRUE(thing.initialise());
  ASSERT_TRUE(thing.visualStream(0) != nullptr);
  ASSERT_TRUE(!thing.visualStreams().empty());
  ASSERT_TRUE(thing.visualStreams().size() == 1);
  ASSERT_TRUE(thing.audioStream(0) != nullptr);
  ASSERT_TRUE(!thing.audioStreams().empty());
  ASSERT_TRUE(thing.audioStreams().size() == 1);
}

TEST (FFMpegSinkTest, InitialiseSuccessMultiCodecs)
{
  FFMpegSink thing("./test.mp4", {Codec::H264, Codec::RAW}, {Codec::AAC, Codec::WAV, Codec::MP3});
  ASSERT_TRUE(thing.initialise());
  ASSERT_TRUE(thing.visualStream(0) != nullptr);
  ASSERT_TRUE(!thing.visualStreams().empty());
  ASSERT_TRUE(thing.visualStreams().size() == 2);
  ASSERT_TRUE(thing.audioStream(0) != nullptr);
  ASSERT_TRUE(!thing.audioStreams().empty());
  ASSERT_TRUE(thing.audioStreams().size() == 3);
}

TEST (FFMpegSinkTest, SetupAudioEncoderFailNoProperties)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::MP3});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupAudioEncoderFailNoBitrate)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::MP3});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'050);
  ASSERT_FALSE(stream->writeFrame( {}));
}


TEST (FFMpegSinkTest, SetupAudioEncoderFailNoLayout)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::MP3});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'050);
  stream->setProperty(MediaProperty::BITRATE, 128'000);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST(FFMpegSinkTest, SetAudioEncoderInputFormat)
{

  FFMpegSink thing("./test.mkv", {}, {Codec::WAV});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  ASSERT_TRUE(stream->setInputFormat(SampleFormat::SIGNED_16P));
  ASSERT_FALSE(stream->setInputFormat(SampleFormat::DOUBLE));
}

TEST (FFMpegSinkTest, SetupAudioEncoderNoInput)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::WAV});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);

  stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'050);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupAudioEncoderInvalidSamplerate)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::MP3});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 123456);
  stream->setProperty(MediaProperty::BITRATE, 128'000);
  ASSERT_TRUE(stream->setInputFormat(SampleFormat::SIGNED_16P));
  ASSERT_FALSE(stream->writeFrame( {}));
}


TEST (FFMpegSinkTest, SetupAudioEncoderSuccess)
{
  FFMpegSink thing("/tmp/mp3.mp4", {}, {Codec::MP3});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'050);
  stream->setProperty(MediaProperty::BITRATE, 128'000);
  ASSERT_TRUE(stream->setInputFormat(SampleFormat::SIGNED_16P));
  ASSERT_TRUE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupAudioEncoderSuccessNoBitrateWAV)
{
  FFMpegSink thing("/tmp/wav.mkv", {}, {Codec::WAV});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'050);
  stream->setInputFormat(SampleFormat::SIGNED_16P);
  ASSERT_TRUE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoEncoderFailNoProperties)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  ASSERT_TRUE(stream != nullptr);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoEncoderFailNoBitrate)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setInputFormat(PixelFormat::YUV420);
  ASSERT_FALSE(stream->writeFrame( {}));
}


TEST (FFMpegSinkTest, SetupVideoEncoderFailNoInputFormat)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoEncoderFailNoDimensions)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  stream->setInputFormat(PixelFormat::YUV420);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoEncoderFailNoFrameRate)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  stream->setInputFormat(PixelFormat::YUV420);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoEncoderSuccess)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  stream->setInputFormat(PixelFormat::YUV420);
  ASSERT_TRUE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoH264EncoderOptionsWrongProfile)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 10'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::MPEG2_422);
  stream->setProperty(MediaProperty::PRESET, Preset::X264_FAST);
  stream->setInputFormat(PixelFormat::YUV420);
  ASSERT_TRUE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoH264EncoderOptionsWrongPixelFormat)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::H264_MAIN);
  stream->setProperty(MediaProperty::PRESET, Preset::X264_FAST);
  stream->setInputFormat(PixelFormat::YUV422); // 422 isn't possible with main
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoH264EncoderOptions)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::H264_HIGH422);
  stream->setProperty(MediaProperty::PRESET, Preset::X264_FAST);
  stream->setInputFormat(PixelFormat::YUV422);
  ASSERT_TRUE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoMPEG2EncoderOptionsWrongProfile)
{
  FFMpegSink thing("./test.mp4", {Codec::MPEG2_VIDEO}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::H264_HIGH422);
  stream->setInputFormat(PixelFormat::YUV422);
  ASSERT_TRUE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoMPEG2EncoderOptionsWrongPixelFormat)
{
  FFMpegSink thing("./test.mp4", {Codec::MPEG2_VIDEO}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::MPEG2_MAIN);
  stream->setInputFormat(PixelFormat::YUV422);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoMPEG2EncoderOptions)
{
  FFMpegSink thing("./test.mp4", {Codec::MPEG2_VIDEO}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::MPEG2_HIGH);
  stream->setInputFormat(PixelFormat::YUV422);
  ASSERT_TRUE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoDNXHDInvalidDims)
{
  FFMpegSink thing("./test.mp4", {Codec::DNXHD}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320, 240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 45'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::DNXHD);
  stream->setInputFormat(PixelFormat::YUV422);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoDNXHDInvalidBitrate)
{
  FFMpegSink thing("./test.mp4", {Codec::DNXHD}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({1920, 1080}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 4'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::DNXHD);
  stream->setInputFormat(PixelFormat::YUV422);
  ASSERT_FALSE(stream->writeFrame( {}));
}

TEST (FFMpegSinkTest, SetupVideoDNXHD)
{
  FFMpegSink thing("./test.mov", {Codec::DNXHD}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({1920, 1080}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 45'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::DNXHD);
  stream->setInputFormat(PixelFormat::YUV422);
  ASSERT_TRUE(stream->writeFrame( {}));
}

TEST(FFMpegSinkTest, WriteH264)
{
  FFMpegSource source("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov");
  auto source_v_stream = source.visualStream(0);
  ASSERT_TRUE(source_v_stream != nullptr);
  source_v_stream->setOutputFormat(PixelFormat::YUV422, {1280, 720});
  FFMpegSink sink("/tmp/h264.mp4", {Codec::H264}, {});
  ASSERT_TRUE(sink.initialise());
  auto stream = sink.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({1280,720}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 10'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::H264_HIGH422);
  stream->setProperty(MediaProperty::PRESET, Preset::X264_FAST);
  stream->setInputFormat(PixelFormat::YUV422);

  while (auto frame = source_v_stream->frame()) {
    ASSERT_TRUE(stream->writeFrame(frame));
  }
  ASSERT_TRUE(stream->writeFrame(nullptr));
  sink.finish();

  FFMpegSource written_file("/tmp/h264.mp4");
  ASSERT_TRUE(written_file.visualStreams().size() == 1);
  ASSERT_TRUE(written_file.audioStreams().empty());
  auto v_s = written_file.visualStream(0);
  bool okay;
  auto dims = v_s->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(dims.width, 1280);
  ASSERT_EQ(dims.height, 720);
  auto profile = v_s->property<Profile>(MediaProperty::PROFILE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(profile, Profile::H264_HIGH422);
  auto pix_fmt = v_s->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(pix_fmt, PixelFormat::YUV422);
  auto bitrate = v_s->property<int32_t>(MediaProperty::BITRATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(bitrate/1'000'000, 10);
}

TEST(FFMpegSinkTest, WriteMPEG2)
{
  FFMpegSource source("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov");
  auto source_v_stream = source.visualStream(0);
  ASSERT_TRUE(source_v_stream != nullptr);
  source_v_stream->setOutputFormat(PixelFormat::YUV420, {426, 240});
  FFMpegSink sink("/tmp/mpeg2.mp4", {Codec::MPEG2_VIDEO}, {});
  ASSERT_TRUE(sink.initialise());
  auto stream = sink.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({426, 240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::MPEG2_SIMPLE);
  stream->setProperty(MediaProperty::LEVEL, Level::MPEG2_LOW);
  stream->setInputFormat(PixelFormat::YUV420);

  while (auto frame = source_v_stream->frame()) {
    ASSERT_TRUE(stream->writeFrame(frame));
  }
  ASSERT_TRUE(stream->writeFrame(nullptr));

  sink.finish();

  FFMpegSource written_file("/tmp/mpeg2.mp4");
  ASSERT_TRUE(written_file.visualStreams().size() == 1);
  ASSERT_TRUE(written_file.audioStreams().empty());
  auto v_s = written_file.visualStream(0);
  bool okay;
  auto dims = v_s->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(dims.width, 426);
  ASSERT_EQ(dims.height, 240);
  auto profile = v_s->property<Profile>(MediaProperty::PROFILE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(profile, Profile::MPEG2_SIMPLE);
  auto pix_fmt = v_s->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(pix_fmt, PixelFormat::YUV420);
  auto bitrate = v_s->property<int32_t>(MediaProperty::BITRATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(bitrate/1'000'000, 1);
}

TEST(FFMpegSinkTest, WriteMJPEG)
{
  FFMpegSource source("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov");
  auto source_v_stream = source.visualStream(0);
  ASSERT_TRUE(source_v_stream != nullptr);
  source_v_stream->setOutputFormat(PixelFormat::YUVJ420, {426, 240});
  FFMpegSink sink("/tmp/mjpeg.mp4", {Codec::JPEG}, {});
  ASSERT_TRUE(sink.initialise());
  auto stream = sink.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({426, 240}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 1'000'000);
  ASSERT_TRUE(stream->setInputFormat(PixelFormat::YUVJ420));

  while (auto frame = source_v_stream->frame()) {
    ASSERT_TRUE(stream->writeFrame(frame));
  }
  ASSERT_TRUE(stream->writeFrame(nullptr));

  sink.finish();

  FFMpegSource written_file("/tmp/mjpeg.mp4");
  ASSERT_TRUE(written_file.visualStreams().size() == 1);
  ASSERT_TRUE(written_file.audioStreams().empty());
  auto v_s = written_file.visualStream(0);
  bool okay;
  auto dims = v_s->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(dims.width, 426);
  ASSERT_EQ(dims.height, 240);
  auto pix_fmt = v_s->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(pix_fmt, PixelFormat::YUVJ420);
  auto bitrate = v_s->property<int32_t>(MediaProperty::BITRATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(bitrate/1'000'000, 1);
}

TEST(FFMpegSinkTest, WriteMOV)
{
  FFMpegSource source("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov");
  auto source_v_stream = source.visualStream(0);
  ASSERT_TRUE(source_v_stream != nullptr);
  source_v_stream->setOutputFormat(PixelFormat::YUV422, {1920, 1080});
  FFMpegSink sink("/tmp/h264.mov", {Codec::H264}, {});
  ASSERT_TRUE(sink.initialise());
  auto stream = sink.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({1920, 1080}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 2'000'000);
  ASSERT_TRUE(stream->setInputFormat(PixelFormat::YUV422));

  while (auto frame = source_v_stream->frame()) {
    ASSERT_TRUE(stream->writeFrame(frame));
  }
  ASSERT_TRUE(stream->writeFrame(nullptr));

  sink.finish();

  FFMpegSource written_file("/tmp/h264.mov");
  ASSERT_TRUE(written_file.visualStreams().size() == 1);
  ASSERT_TRUE(written_file.audioStreams().empty());
  auto v_s = written_file.visualStream(0);
  bool okay;
  auto dims = v_s->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(dims.width, 1920);
  ASSERT_EQ(dims.height, 1080);
  auto pix_fmt = v_s->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(pix_fmt, PixelFormat::YUV422);
  auto bitrate = v_s->property<int32_t>(MediaProperty::BITRATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(bitrate/1'000'000, 1);
}

TEST(FFMpegSinkTest, WriteAVI)
{
  FFMpegSource source("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov");
  auto source_v_stream = source.visualStream(0);
  ASSERT_TRUE(source_v_stream != nullptr);
  source_v_stream->setOutputFormat(PixelFormat::YUV422, {1920, 1080});
  FFMpegSink sink("/tmp/h264.avi", {Codec::H264}, {});
  ASSERT_TRUE(sink.initialise());
  auto stream = sink.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({1920, 1080}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 2'000'000);
  ASSERT_TRUE(stream->setInputFormat(PixelFormat::YUV422));

  while (auto frame = source_v_stream->frame()) {
    ASSERT_TRUE(stream->writeFrame(frame));
  }
  ASSERT_TRUE(stream->writeFrame(nullptr));

  sink.finish();

  FFMpegSource written_file("/tmp/h264.avi");
  ASSERT_TRUE(written_file.visualStreams().size() == 1);
  ASSERT_TRUE(written_file.audioStreams().empty());
  auto v_s = written_file.visualStream(0);
  bool okay;
  auto dims = v_s->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(dims.width, 1920);
  ASSERT_EQ(dims.height, 1080);
  auto pix_fmt = v_s->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(pix_fmt, PixelFormat::YUV422);
  auto bitrate = v_s->property<int32_t>(MediaProperty::BITRATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(bitrate/1'000'000, 1);
}

TEST(FFMpegSinkTest, WriteMXF)
{
  FFMpegSource source("./ReferenceMedia/Video/dnxhd/fhd_dnxhd.mov");
  auto source_v_stream = source.visualStream(0);
  ASSERT_TRUE(source_v_stream != nullptr);
  source_v_stream->setOutputFormat(PixelFormat::YUV422, {1920, 1080});
  FFMpegSink sink("/tmp/mpeg2.mxf", {Codec::MPEG2_VIDEO}, {});
  ASSERT_TRUE(sink.initialise());
  auto stream = sink.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({1920, 1080}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::CBR);
  stream->setProperty(MediaProperty::BITRATE, 10'000'000);
  ASSERT_TRUE(stream->setInputFormat(PixelFormat::YUV422));

  while (auto frame = source_v_stream->frame()) {
    ASSERT_TRUE(stream->writeFrame(frame));
  }
  ASSERT_TRUE(stream->writeFrame(nullptr));

  sink.finish();

  FFMpegSource written_file("/tmp/mpeg2.mxf");
  ASSERT_TRUE(written_file.visualStreams().size() == 1);
  ASSERT_TRUE(written_file.audioStreams().empty());
  auto v_s = written_file.visualStream(0);
  v_s->index();
  bool okay;
  auto dims = v_s->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(dims.width, 1920);
  ASSERT_EQ(dims.height, 1080);
  auto pix_fmt = v_s->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(pix_fmt, PixelFormat::YUV422);
  auto bitrate = v_s->property<int32_t>(MediaProperty::BITRATE, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(bitrate/100000, 12);
}

TEST(FFMpegSinkTest, AutoInputFormatConv)
{

  auto fmt = SampleFormat::FLOAT; // Not possible with PCM
  FFMpegSink sink("/tmp/autoformat.wav", {}, {Codec::PCM_S16_LE});
  ASSERT_TRUE(sink.initialise());
  auto sink_stream = sink.audioStream(0);
  ASSERT_FALSE(sink_stream->setInputFormat(fmt));
  sink_stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22050);
  ASSERT_FALSE(sink_stream->setInputFormat(fmt));
  sink_stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  ASSERT_TRUE(sink_stream->setInputFormat(fmt));
}

TEST(FFMpegSinkTest, AutoInputFormatConvTransCode)
{
  FFMpegSource source("./ReferenceMedia/Audio/ogg/monotone.ogg");
  auto source_stream = source.audioStream(0);
  bool okay;
  auto fmt = source_stream->property<SampleFormat>(MediaProperty::AUDIO_FORMAT, okay);

  FFMpegSink sink("/tmp/autoformat.m4a", {}, {Codec::AAC});
  ASSERT_TRUE(sink.initialise());
  auto sink_stream = sink.audioStream(0);
  sink_stream->setProperty(MediaProperty::BITRATE, 64000);
  sink_stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22050);
  sink_stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  ASSERT_TRUE(sink_stream->setInputFormat(fmt));

  while (auto frame = source_stream->frame()) {
    ASSERT_TRUE(sink_stream->writeFrame(frame));
  }

}
//#define FFMPEG_COMPRESS_MUX

TEST(FFMpegSinkTest, WriteMP3)
{
  constexpr auto destination = "/tmp/vorbis.ogg";
  FFMpegSource source("./ReferenceMedia/Audio/ogg/monotone.ogg");
//  FFMpegSource source("./ReferenceMedia/Audio/wav/monotone.wav");
  auto source_stream = source.audioStream(0);
  bool okay;
  auto format = source_stream->property<SampleFormat>(MediaProperty::AUDIO_FORMAT, okay);
  ASSERT_TRUE(okay);
  auto layout = source_stream->property<ChannelLayout>(MediaProperty::AUDIO_LAYOUT, okay);
  ASSERT_TRUE(okay);
  source_stream->setOutputFormat(SampleFormat::FLOAT_P);
  const auto sample_rate = source_stream->property<int32_t>(MediaProperty::AUDIO_SAMPLING_RATE, okay);

#ifdef FFMPEG_COMPRESS_MUX
  auto fformat = types::convertSampleFormat(format);
  AVFormatContext *oc;
  /* allocate the output media context */
  int ret = avformat_alloc_output_context2(&oc, nullptr, nullptr, destination);
  ASSERT_TRUE(oc != nullptr);
  ASSERT_TRUE(ret >= 0);

  AVOutputFormat *fmt = oc->oformat;
  if (fmt->audio_codec != AV_CODEC_ID_NONE) {

    AVCodec* codec = avcodec_find_encoder(fmt->audio_codec);
    ASSERT_TRUE(codec != nullptr);
    AVStream* st = avformat_new_stream(oc, nullptr);
    st->id = oc->nb_streams - 1;
    ASSERT_TRUE(st != nullptr);
    AVCodecContext* c = avcodec_alloc_context3(codec);
    ASSERT_TRUE(c != nullptr);
    c->sample_fmt = fformat;
    c->bit_rate = 64000;
    c->sample_rate = sample_rate;
    c->channels = 1;
    c->channel_layout = AV_CH_LAYOUT_MONO;
    st->time_base = {1, c->sample_rate};
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    ret = avcodec_open2(c, codec, nullptr);
    ASSERT_TRUE(ret >= 0);
    ret = avcodec_parameters_from_context(st->codecpar, c);
    ASSERT_TRUE(ret >= 0);

    av_dump_format(oc, 0, destination, 1);
    ret = avio_open(&oc->pb, destination, AVIO_FLAG_WRITE);
    ASSERT_TRUE(ret >= 0);
    ret = avformat_write_header(oc, nullptr);
    ASSERT_TRUE(ret >= 0);

    int64_t sample_count = 0;
    AVFrame* fframe = av_frame_alloc();
    fframe->format = fformat;
    fframe->channel_layout = AV_CH_LAYOUT_MONO;
    fframe->sample_rate = sample_rate;
    fframe->nb_samples = c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE ? 10000 : c->frame_size;
    ret = av_frame_get_buffer(fframe, 0);
    ASSERT_TRUE(ret >= 0);

    while (auto frame = source_stream->frame())
    {
      auto data = frame->data();
      AVPacket pkt = { nullptr, 0, 0 }; // data and size must be 0;
      av_init_packet(&pkt);


      ret = av_frame_make_writable(fframe);
      ASSERT_TRUE(ret >= 0);
      fframe->pts = av_rescale_q(sample_count, {1, sample_rate}, c->time_base);
      ASSERT_GE(fframe->pts, -1);
      fframe->data[0] = data.data_[0];


      int got_packet;
      ret = avcodec_encode_audio2(c, &pkt, fframe, &got_packet);
      ASSERT_TRUE(ret >= 0);
      if (got_packet == 1) {
        av_packet_rescale_ts(&pkt, c->time_base, st->time_base);
        pkt.stream_index = st->index;
        ret = av_interleaved_write_frame(oc, &pkt);
        ASSERT_TRUE(ret >= 0);
      }

      sample_count += data.data_size_;
    }

    av_frame_free(&fframe);
    ret = av_write_trailer(oc);
    ASSERT_TRUE(ret >= 0);
  }
#else
  FFMpegSink sink(destination, {}, {Codec::VORBIS});
  ASSERT_TRUE(sink.initialise());
  auto sink_stream = sink.audioStream(0);
  sink_stream->setProperty(MediaProperty::BITRATE, 192'000);
  sink_stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, sample_rate);
  sink_stream->setProperty(MediaProperty::AUDIO_LAYOUT, layout);
  sink_stream->setInputFormat(format);
  while (auto frame = source_stream->frame()) {
    ASSERT_TRUE(sink_stream->writeFrame(frame));
  }
  ASSERT_TRUE(sink_stream->writeFrame(nullptr));

  sink.finish();
#endif
}

TEST (FFMpegSinkTest, WriteSilence)
{
  FFMpegSink sink("/tmp/silence.wav", {}, {Codec::PCM_S16_LE});
  ASSERT_TRUE(sink.initialise());
  auto sink_stream = sink.audioStream(0);
  sink_stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22050);
  sink_stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  sink_stream->setInputFormat(SampleFormat::SIGNED_16);

  IMediaFrame::FrameData data;
  data.samp_fmt_ = SampleFormat::SIGNED_16;
  data.sample_count_ = 20;
  data.data_ = new uint8_t*[8];
  std::vector<uint8_t> samples(20, 0);
  data.data_[0] = samples.data();

  MediaFramePtr frame = media_handling::createFrame();
  frame->setData(data);
  ASSERT_TRUE(sink_stream->writeFrame(frame));
  ASSERT_TRUE(sink_stream->writeFrame(frame));
  ASSERT_TRUE(sink_stream->writeFrame(frame));
  ASSERT_TRUE(sink_stream->writeFrame(frame));
  ASSERT_TRUE(sink_stream->writeFrame(frame));
  sink.finish();

}

TEST(FFMpegSinkTest, WriteSine)
{
  FFMpegSink sink("/tmp/sine.wav", {}, {Codec::PCM_S16_LE});
  ASSERT_TRUE(sink.initialise());
  auto sink_stream = sink.audioStream(0);
  sink_stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22050);
  sink_stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  sink_stream->setInputFormat(SampleFormat::SIGNED_16);

  IMediaFrame::FrameData data;
  data.samp_fmt_ = SampleFormat::SIGNED_16;
  data.sample_count_ = 360;
  data.data_ = new uint8_t*[8];
  std::vector<uint8_t> samples;
  for (auto x = 0; x < 100; x++){
    for (auto i = 0; i < 360; i+=2) {
      int16_t val = 0x7FFF * sin(i*3.14/180);
      samples.push_back(val & 0xFF);
      samples.push_back(val >> 8);
    }
  }


  data.data_[0] = samples.data();

  MediaFramePtr frame = media_handling::createFrame();
  frame->setData(data);
  ASSERT_TRUE(sink_stream->writeFrame(frame));
  sink.finish();
}

TEST(FFMpegSinkTest, StreamPropertiesLocked)
{
  FFMpegSink sink("/tmp/properties.wav", {}, {Codec::PCM_S16_LE});
  sink.initialise();
  auto sink_stream = sink.audioStream(0);
  sink_stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22050);
  sink_stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  sink_stream->setInputFormat(SampleFormat::SIGNED_16);
  sink_stream->writeFrame(nullptr);
  sink_stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::STEREO);
  bool okay;
  const auto layout = sink_stream->property<ChannelLayout>(MediaProperty::AUDIO_LAYOUT, okay);
  ASSERT_TRUE(okay);
  ASSERT_EQ(layout, ChannelLayout::MONO);

}
