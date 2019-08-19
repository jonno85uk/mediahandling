#ifndef FFMPEGSOURCETEST_H
#define FFMPEGSOURCETEST_H

#include <QtTest>
#include <QObject>

class FFMpegSourceTest : public QObject
{
  Q_OBJECT
public:
  FFMpegSourceTest();
private slots:
  void testCaseDefaultAllocate();

  void testCaseSetFilePath();

  void testCaseStreamsInvalidFile();
};

#endif // FFMPEGSOURCETEST_H
