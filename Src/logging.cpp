/*
  Copyright (c) 2020, Jonathan Noble
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

#include "logging.h"

#include <iostream>
#include <mutex>
#include <fmt/format.h>
#include <chrono>

#include "date.h"

namespace mhl = media_handling::logging;

namespace
{
    std::mutex log_mtx;
    mhl::LogType log_level = mhl::LogType::WARNING;
}



void defaultLog(const mhl::LogType log_type, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(log_mtx);
    const auto prefix = [log_type]() -> std::string_view {
      switch (log_type) {
          case mhl::LogType::FATAL:     return "   FATAL";
          case mhl::LogType::CRITICAL:  return "CRITICAL";
          case mhl::LogType::WARNING:   return " WARNING";
          case mhl::LogType::INFO:      return "    INFO";
          case mhl::LogType::DEBUG:     return "   DEBUG";
      }
      return "--------";
    };
    const auto now(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()));
    const auto now_str(date::format("%F %T", now));
    std::cout << fmt::format("{}|{}|{}", 
                              prefix(), 
                              now_str, 
                              msg) << std::endl;
}

static mhl::LOGGINGFN logging_func = defaultLog;
void mhl::setLogLevel(const mhl::LogType level)
{
  log_level = level;
}

void mhl::assignLoggerCallback(mhl::LOGGINGFN func)
{
  logging_func = func;
}


void mhl::logMessage(const mhl::LogType log_type, const std::string& msg) noexcept
{
  if (logging_func == nullptr) {
    // Logging has been explicitly disabled.
    return;
  }
  if (static_cast<int>(log_type) > static_cast<int>(log_level)) {
    // Ignore. Filtered out.
    return;
  }
  try {
    logging_func(log_type, msg);
  }  catch (...) {
    // TODO: stderr
  }
}