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

#include "ffmpegsink.h"
#include "ffmpegsource.h"
#include "rational.h"

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

TEST (FFMpegSinkTest, SetupVideoDNXHDInvalidInputFormat)
{
  FFMpegSink thing("./test.mp4", {Codec::DNXHD}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
  stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({1920, 1080}));
  stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
  stream->setProperty(MediaProperty::BITRATE, 45'000'000);
  stream->setProperty(MediaProperty::PROFILE, Profile::DNXHD);
  ASSERT_FALSE(stream->setInputFormat(PixelFormat::YUV444));
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
  ASSERT_FALSE(stream->setInputFormat(PixelFormat::YUV420));
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
  ASSERT_EQ(bitrate, 1225655);
}

TEST(FFMpegSinkTest, WriteMP3)
{

  FFMpegSource source("./ReferenceMedia/Audio/ogg/monotone.ogg");
  auto source_stream = source.audioStream(0);
  source_stream->setOutputFormat(SampleFormat::FLOAT_P);
  bool okay;
  const auto sample_rate = source_stream->property<int32_t>(MediaProperty::AUDIO_SAMPLING_RATE, okay);

  FFMpegSink sink("/tmp/vorbis.mka", {}, {Codec::VORBIS});
  ASSERT_TRUE(sink.initialise());
  sink.supportedAudioCodecs();
  auto sink_stream = sink.audioStream(0);
  sink_stream->setProperty(MediaProperty::BITRATE, 64'000);
  sink_stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, sample_rate);
  sink_stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  sink_stream->setInputFormat(SampleFormat::FLOAT_P);
  while (auto frame = source_stream->frame()) {
    ASSERT_TRUE(sink_stream->writeFrame(frame));
  }
  ASSERT_TRUE(sink_stream->writeFrame(nullptr));

  sink.finish();
}


