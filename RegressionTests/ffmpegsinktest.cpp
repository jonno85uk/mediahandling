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
  FFMpegSink thing("./test.mp4", {}, {Codec::WAV});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  ASSERT_FALSE(stream->setFrame(0, {}));
}

TEST (FFMpegSinkTest, SetupAudioEncoderFailNoBitrate)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::MP3});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'000);
  ASSERT_FALSE(stream->setFrame(0, {}));
}


TEST (FFMpegSinkTest, SetupAudioEncoderFailNoLayout)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::MP3});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'000);
  stream->setProperty(MediaProperty::BITRATE, 128'000);
  ASSERT_FALSE(stream->setFrame(0, {}));
}

TEST(FFMpegSinkTest, SetAudioEncoderInputFormat)
{

  FFMpegSink thing("./test.mp4", {}, {Codec::WAV});
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
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'000);
  ASSERT_FALSE(stream->setFrame(0, {}));
}

TEST (FFMpegSinkTest, SetupAudioEncoderSuccess)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::MP3});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);

  stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'000);
  stream->setProperty(MediaProperty::BITRATE, 128'000);
  ASSERT_TRUE(stream->setInputFormat(SampleFormat::SIGNED_16P));
  ASSERT_TRUE(stream->setFrame(0, {}));
}

TEST (FFMpegSinkTest, SetupAudioEncoderSuccessNoBitrateWAV)
{
  FFMpegSink thing("./test.mp4", {}, {Codec::WAV});
  thing.initialise();
  auto stream = thing.audioStream(0);
  ASSERT_TRUE(stream != nullptr);
  stream->setProperty(MediaProperty::AUDIO_LAYOUT, ChannelLayout::MONO);
  stream->setProperty(MediaProperty::AUDIO_SAMPLING_RATE, 22'000);
  stream->setInputFormat(SampleFormat::SIGNED_16P);
  ASSERT_TRUE(stream->setFrame(0, {}));
}

TEST (FFMpegSinkTest, SetupVideoEncoderFailNoProperties)
{
  FFMpegSink thing("./test.mp4", {Codec::H264}, {});
  ASSERT_TRUE(thing.initialise());
  auto stream = thing.visualStream(0);
  ASSERT_TRUE(stream != nullptr);
  ASSERT_FALSE(stream->setFrame(0, {}));
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
  ASSERT_TRUE(stream != nullptr);
  ASSERT_FALSE(stream->setFrame(0, {}));
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
  ASSERT_TRUE(stream != nullptr);
  ASSERT_FALSE(stream->setFrame(0, {}));
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
  ASSERT_TRUE(stream != nullptr);
  ASSERT_FALSE(stream->setFrame(0, {}));
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
  ASSERT_TRUE(stream != nullptr);
  ASSERT_FALSE(stream->setFrame(0, {}));
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
  ASSERT_TRUE(stream != nullptr);
  ASSERT_TRUE(stream->setFrame(0, {}));
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
  ASSERT_TRUE(stream != nullptr);
  ASSERT_TRUE(stream->setFrame(0, {}));
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
  ASSERT_TRUE(stream != nullptr);
  ASSERT_FALSE(stream->setFrame(0, {}));
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
  ASSERT_TRUE(stream != nullptr);
  ASSERT_TRUE(stream->setFrame(0, {}));
}



