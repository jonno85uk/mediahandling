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
#pragma once

#include <string>

namespace media_handling::logging
{
    enum class LogType
    {
        FATAL,
        CRITICAL,
        WARNING,
        INFO,
        DEBUG
    };


    typedef void (*LOGGINGFN)(const LogType, const std::string&);

    /**
     * @brief           Set the minimum log type shown in logging
     * @param   level   Logs including and greater in importance will be shown
     */
    void setLogLevel(const LogType level);
    /**
     * @brief       Assign a callback for library messages
     * @note        Library defaults to stderr
     * @param func  The callback function
     */
    void assignLoggerCallback(LOGGINGFN func);

    /**
     * @note intended for internal purposes only
     */
    void logMessage(const LogType log_type, const std::string& msg) noexcept;
}


template <typename T, size_t S>
inline constexpr size_t get_file_name_offset(const T (& str)[S], size_t i = S - 1)
{
    return (str[i] == '/' || str[i] == '\\') ? i + 1 : (i > 0 ? get_file_name_offset(str, i - 1) : 0);
}

template <typename T>
inline constexpr size_t get_file_name_offset(T (& str)[1])
{
    return 0;
}

#define LOG(_level, _msg, _file, _line) \
  media_handling::logging::logMessage(_level, fmt::format("{}:{}|{}", _file, _line, _msg))
#define LDEBUG(msg) LOG(media_handling::logging::LogType::DEBUG, msg, \
  &__FILE__[get_file_name_offset(__FILE__)], __LINE__)
#define LINFO(msg) LOG(media_handling::logging::LogType::INFO, msg, \
  &__FILE__[get_file_name_offset(__FILE__)], __LINE__)
#define LWARNING(msg) LOG(media_handling::logging::LogType::WARNING, msg, \
  &__FILE__[get_file_name_offset(__FILE__)], __LINE__)
#define LCRITICAL(msg) LOG(media_handling::logging::LogType::CRITICAL, msg, \
  &__FILE__[get_file_name_offset(__FILE__)], __LINE__)
