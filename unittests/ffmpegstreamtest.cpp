#include "ffmpegstreamtest.h"
#include "ffmpegstream.h"
#include <QTest>

using media_handling::ffmpeg::FFMpegStream;

FFMpegStreamTest::FFMpegStreamTest(QObject *parent) : QObject(parent)
{

}

void FFMpegStreamTest::testCaseNullInst()
{
  AVFormatContext ctx;
  AVStream strm;
  QVERIFY_EXCEPTION_THROWN(FFMpegStream(nullptr, nullptr), std::exception);
  QVERIFY_EXCEPTION_THROWN(FFMpegStream(&ctx, nullptr), std::exception);
  QVERIFY_EXCEPTION_THROWN(FFMpegStream(nullptr, &strm), std::exception);
}


