#include <gtest/gtest.h>
#include "mediahandling.h"

void logFunc(const media_handling::logging::LogType, const std::string&)
{
  // Do nothing
}

int main(int argc, char **argv) 
{
  ::testing::InitGoogleTest(&argc, argv);
#ifndef LOGGING  
  // media_handling::assignLoggerCallback(&logFunc);
  media_handling::logging::setLogLevel(media_handling::logging::LogType::INFO);
  media_handling::enableBackendLogs(false);
#endif
  return RUN_ALL_TESTS();
}
