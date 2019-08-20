
#include <gtest/gtest.h>

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
