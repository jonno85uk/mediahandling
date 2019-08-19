#ifndef FFMPEGSTREAMTEST_H
#define FFMPEGSTREAMTEST_H

#include <QObject>

class FFMpegStreamTest : public QObject
{
    Q_OBJECT
  public:
    explicit FFMpegStreamTest(QObject *parent = nullptr);

  signals:

  private slots:
    void testCaseNullInst();
};

#endif // FFMPEGSTREAMTEST_H
