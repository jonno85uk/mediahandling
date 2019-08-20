#include <gtest/gtest.h>

#include "ffmpegstream.h"

using media_handling::ffmpeg::FFMpegStream;


TEST (FFMpegStreamTest, NullInst)
{
  AVFormatContext ctx;
  AVStream strm;
  EXPECT_THROW(FFMpegStream(nullptr, nullptr), std::exception);
  EXPECT_THROW(FFMpegStream(&ctx, nullptr), std::exception);
  EXPECT_THROW(FFMpegStream(nullptr, &strm), std::exception);
}


