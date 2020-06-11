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

#ifndef MEDIAHANDLING_H
#define MEDIAHANDLING_H

#include <atomic>

#include "imediasource.h"
#include "imediasink.h"
#include "rational.h"
#include "timecode.h"
#include "logging.h"


namespace media_handling
{

  constexpr auto SEQUENCE_MATCHING_PATTERN = "^(.+?)([0-9]+)\\.(bmp|dpx|exr|jpeg|jpg|png|tiff|jp2|tga)$";
  constexpr auto SPECIFIC_MATCHING_PATTERN = "([0-9]+)\\.(bmp|dpx|jpeg|jpg|exr|png|tiff|jp2|tga)$";

  namespace global
  {
    extern std::atomic<bool> auto_detect_img_sequence;
  }

  namespace utils
  {
    /**
     * @brief       Identify if path is part of a contiguous image sequence
     * @param path  File path to existing file
     * @return      true==path is in sequence
     */
    bool pathIsInSequence(const std::string& path);
    /**
     * @brief       Generate a sequence pattern ffmpeg understands
     * @param path  File path to generate agains
     * @return      Valid sequence pattern if value exists
     */
    std::optional<std::string> generateSequencePattern(const std::string& path);
    /**
     * @brief       Extract the start number of an image sequence file path
     * @param path  File path to existing file
     * @return      >= 0 indicates a valid value
     */
    int getSequenceStartNumber(const std::string& path);
  }

  enum class BackendType
  {
    FFMPEG,
    GSTREAMER,
    INTEL
  };

  /**
   * @brief Initialise the library with a selected backend
   * @note  This does nothing as only FFMpeg is available
   * @return true==initialised ok
   */
  bool initialise(const BackendType backend);

  /**
   * @brief           Enable/disable printing of the backend library messages to console
   * @param enabled
   */
  void enableBackendLogs(const bool enabled);

  /**
   * @brief             Create a new media source from a file using pre-selected backend
   * @param file_path   Path (absolute or relative) to the file 
   * @return  valid MediaSourcePtr or null
   */
  MediaSourcePtr createSource(std::string file_path);

  /**
   * @brief               Create a new media sink with the selected filepath and codecs for writing
   * @param file_path     Path to a new/existing file. The parent directory must exist.
   * @param video_codecs  List of video codecs to use per stream.
   *                      The amount of codecs indicate the amount of video streams in this file
   * @param audio_codecs  List of audio codecs to use per stream.
   *                      The amount of codecs indicate the amount of video streams in this file
   * @return              valid MediaSinkPtr or null
   */
  MediaSinkPtr createSink(std::string file_path, std::vector<Codec> video_codecs, std::vector<Codec> audio_codecs);

  /**
   * @brief     Create a mew media frame for populating data to be encoded
   * @return    valid MediaFramePtr or null
   */
  MediaFramePtr createFrame();

  /**
   * @brief Globally set the ability to auto-detect image sequences
   * @param value true==auto-detecting
   */
  void autoDetectImageSequences(const bool value) noexcept;

  /**
   * @brief Obtain the global setting for auto-detect of image sequences
   * @return true==auto-detecting
   */
  bool autoDetectImageSequences() noexcept;
}

#endif // MEDIAHANDLING_H
