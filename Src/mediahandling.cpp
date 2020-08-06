/*
  Copyright (c) 2019, Jonathan Noble
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

#include "mediahandling.h"
#include <iostream>
#include <filesystem>
#include <regex>
#include <sstream>


#include "ffmpegsource.h"
#include "ffmpegsink.h"
#include "ffmpegmediaframe.h"
#include "ffmpegtypes.h"

constexpr auto DEFAULT_BACKEND_LOGS = true;
static std::atomic<media_handling::BackendType> media_backend = media_handling::BackendType::FFMPEG;
std::atomic<bool> media_handling::global::auto_detect_img_sequence = true;

namespace mhl = media_handling::logging;


bool media_handling::utils::pathIsInSequence(const std::string& path)
{
  const std::filesystem::path file_path(path);
  std::regex pattern(SEQUENCE_MATCHING_PATTERN, std::regex_constants::icase);
  std::smatch match;
  // strip path
  const auto fname(file_path.filename().string());
  if (!std::regex_search(fname, match, pattern)) {
    logMessage(mhl::LogType::WARNING, std::string(SEQUENCE_MATCHING_PATTERN) + " doesn't match filename " + path);
    return false;
  }

  // Ensure to match using the first bit of the filename
  pattern = std::regex(std::string("^") + match.str(1) + SPECIFIC_MATCHING_PATTERN, std::regex_constants::icase);
  auto match_count = 0;

  // Iterate through directory looking for matching files
  for (const auto& entry : std::filesystem::directory_iterator(file_path.parent_path())) {
    if (std::regex_match(entry.path().filename().string(), pattern)) {
      match_count++;
      if (match_count > 1) {
        // Thats enough
        logMessage(mhl::LogType::INFO, path + " is a sequence");
        break;
      }
    }
  }

  return (match_count > 1);
}

std::optional<std::string> media_handling::utils::generateSequencePattern(const std::string& path)
{
  const std::filesystem::path file_path(path);
  std::regex pattern(SEQUENCE_MATCHING_PATTERN, std::regex_constants::icase);
  std::smatch match;
  // strip path
  const auto fname(file_path.filename().string());
  if (!std::regex_search(fname, match, pattern)) {
    logMessage(mhl::LogType::DEBUG, std::string(SEQUENCE_MATCHING_PATTERN) + " doesn't match filename " + path);
    return {};
  }

  switch (media_backend) {
    case BackendType::FFMPEG:
    {
      std::stringstream ss;
      ss << match.str(1) << "%0" << match.str(2).length() << "d." <<  match.str(3);
      auto par_path = file_path.parent_path();
      par_path /= ss.str();
      return par_path.string();
    }
    case BackendType::GSTREAMER:
    [[fallthrough]];
    case BackendType::INTEL:
    [[fallthrough]];
    default:
      break;
  }
  return {};
}


int media_handling::utils::getSequenceStartNumber(const std::string& path)
{
  const std::filesystem::path file_path(path);
  std::regex pattern(SEQUENCE_MATCHING_PATTERN, std::regex_constants::icase);
  std::smatch match;
  // strip path
  const auto fname(file_path.filename().string());
  if (!std::regex_search(fname, match, pattern)) {
    logMessage(mhl::LogType::WARNING, std::string(SEQUENCE_MATCHING_PATTERN) + " doesn't match filename " + path);
    return -1;
  }
  return stoi(match.str(2));
}


bool media_handling::initialise(const BackendType backend)
{
  enableBackendLogs(DEFAULT_BACKEND_LOGS);
  media_backend = backend;
  if (backend == BackendType::FFMPEG) {
    return true;
  }
  logMessage(mhl::LogType::WARNING, "Chosen backend type is not available");
  return false;
}


void media_handling::enableBackendLogs(const bool enabled)
{
  switch (media_backend) {
    case BackendType::FFMPEG:
    {
      const int level = enabled ? AV_LOG_VERBOSE : AV_LOG_PANIC;
      av_log_set_level(level);
      av_log_set_callback(media_handling::ffmpeg::types::logCallback);
      break;
    }
    case BackendType::GSTREAMER:
    [[fallthrough]];
    case BackendType::INTEL:
    [[fallthrough]];
    default:
      break;
  }
}




media_handling::MediaSourcePtr media_handling::createSource(std::string file_path)
{
  switch (media_backend) {
    case BackendType::FFMPEG:
      return std::make_shared<ffmpeg::FFMpegSource>(file_path);
    case BackendType::GSTREAMER:
    [[fallthrough]];
    case BackendType::INTEL:
    [[fallthrough]];
    default:
      return {};
  }
}


media_handling::MediaSinkPtr media_handling::createSink(std::string file_path,
                                                        std::vector<Codec> video_codecs,
                                                        std::vector<Codec> audio_codecs)
{
  switch (media_backend) {
    case BackendType::FFMPEG:
      return std::make_shared<ffmpeg::FFMpegSink>(std::move(file_path), std::move(video_codecs), std::move(audio_codecs));
    case BackendType::GSTREAMER:
    [[fallthrough]];
    case BackendType::INTEL:
    [[fallthrough]];
    default:
      return {};
  }
}


media_handling::MediaFramePtr media_handling::createFrame()
{
  switch (media_backend) {
    case BackendType::FFMPEG:
      return std::make_shared<ffmpeg::FFMpegMediaFrame>();
    case BackendType::GSTREAMER:
    [[fallthrough]];
    case BackendType::INTEL:
    [[fallthrough]];
    default:
      return {};
  }
}

void media_handling::autoDetectImageSequences(const bool value) noexcept
{
  media_handling::global::auto_detect_img_sequence = value;
}

bool media_handling::autoDetectImageSequences() noexcept
{
  return media_handling::global::auto_detect_img_sequence;
}
