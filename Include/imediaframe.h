#ifndef IMEDIAFRAME_H
#define IMEDIAFRAME_H

#include <optional>
#include <memory>

#include "mediapropertyobject.h"

namespace media_handling
{

  class IMediaFrame : public MediaPropertyObject
  {
    public:
      IMediaFrame() = default;

      ~IMediaFrame() override = default;

      virtual std::optional<bool> isAudio() const = 0;

      virtual std::optional<bool> isVisual() const = 0;

      /**
       * @brief Size of the data held
       * @return size in bytes
       */
      virtual int64_t size() const = 0;

      /**
       * @brief Obtain the sample data of this frame, either read from a stream (decode) or written to (encode)
       * @return  data or null
       * @see setData
       */
      virtual std::shared_ptr<uint8_t**> data() const = 0;

      /**
       * @brief             Set the data of this frame for encode
       * @param frame_data
       * @param size        Size in bytes
       * @see data
       */
      virtual void setData(std::shared_ptr<uint8_t**> frame_data, const int64_t size) = 0;

      /**
       * @brief It is not always resource-wise to extract all properties for every frame when decoding
       *        so call this at least once before reading any properties
       */
      virtual void extractProperties() = 0;

      virtual int64_t timestamp() const = 0;
  };

  using MediaFramePtr = std::shared_ptr<IMediaFrame>;
}

#endif // IMEDIAFRAME_H
