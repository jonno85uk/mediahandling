#include "ffmpegsourcetest.h"
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


FFMpegSourceTest::FFMpegSourceTest()
{

}

void FFMpegSourceTest::testCaseDefaultAllocate()
{
  FFMpegSourceTestable thing;
  bool is_valid;
  auto prop =  thing.property<std::string>(MediaProperty::FILENAME, is_valid);
  QVERIFY(is_valid == false);
}

void FFMpegSourceTest::testCaseSetFilePath()
{
  FFMpegSourceTestable thing;
  auto path = "test";
  thing.setFilePath(path);
  bool is_valid;
  auto prop = thing.property<std::string>(MediaProperty::FILENAME, is_valid);
  QVERIFY(is_valid == true);
  QVERIFY(prop == path);
}


void FFMpegSourceTest::testCaseStreamsInvalidFile()
{
  FFMpegSourceTestable thing;
  thing.setFilePath("test");
  Q_ASSERT(thing.audioStream(0) == nullptr);
  Q_ASSERT(thing.visualStream(0) == nullptr);
}
