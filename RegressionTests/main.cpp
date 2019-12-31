#include <gtest/gtest.h>
#include "mediahandling.h"

void logFunc(const media_handling::LogType, const std::string&)
{
  // Do nothing
}

int main(int argc, char **argv) 
{
  ::testing::InitGoogleTest(&argc, argv);
#ifndef LOGGING  
  media_handling::assignLoggerCallback(&logFunc);
#endif
  return RUN_ALL_TESTS();
}
