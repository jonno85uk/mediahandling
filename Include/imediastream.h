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

#ifndef IMEDIASTREAM_H
#define IMEDIASTREAM_H

#include <memory>
#include <any>
#include <map>
#include <iostream>

#include "types.h"
namespace media_handling
{
  class IMediaPropertyObject
  {
    public:
      virtual ~IMediaPropertyObject() {}

      /**
       * @brief Get a string representation of the object
       * @return
       */
      virtual std::string repr() = 0;
      /**
       * @brief         Set the properties of this object. This will overwrite previous properties.
       * @param props   New map of properties
       */
      virtual void setProperties(const std::map<MediaProperty, std::any>& props) = 0;

      /**
       * @brief         Set a single property of this object
       * @param prop    The property to set
       * @param value   The value of the property
       */
      virtual void setProperty(const MediaProperty prop, std::any value) = 0;

      /**
       * @brief           Retrieve a single property of this object
       * @param prop      The property to retrieve
       * @param is_valid  An 'out' parameter identifying if the returned value is valid
       * @return          The value of the property if valid
       */
      virtual std::any property(const MediaProperty prop, bool& is_valid) const = 0;

      /**
       * @brief   Templated function to cast the property value from a std::any type
       * @note    See the MediaProperty enum in types.h for valid type for a property
       */
      template <typename T>
      T property(const MediaProperty prop, bool& is_valid) const
      {
        std::any val = this->property(prop, is_valid);
        if (val.has_value()) {
          try {
            T cast_val = std::any_cast<T>(val);
            return cast_val;
          } catch (const std::bad_any_cast& ex) {
#ifdef DEBUG
            std::cerr << ex.what() << std::endl;
#endif
          }
        }
        is_valid = false;
        return {};
      }
  };

  /**
   * @brief The "Essence"
   */
  class IMediaStream : public IMediaPropertyObject
  {
    public:
      virtual ~IMediaStream() override {}

      /**
       * @brief frame       Retrieve a frame-sample from the stream
       * @param timestamp   The position in the stream for retrieval
       * @return            Frame sample on success or null
       */
      virtual MediaFramePtr frame(const int64_t timestamp) = 0;

      /**
       * @brief setFrame    Set the frame-sample for the stream
       * @param timestamp   Position in the stream
       * @param sample      Frame sample
       * @return            true==success
       */
      virtual bool setFrame(const int64_t timestamp, MediaFramePtr sample) = 0;

      /* IMediaPropertyObject */
      virtual std::string repr() override = 0;
      virtual void setProperties(const std::map<MediaProperty, std::any>& props) override = 0;
      virtual void setProperty(const MediaProperty prop, std::any value) override = 0;
      virtual std::any property(const MediaProperty prop, bool& is_valid) const override = 0;
      template<typename T>
      T property(const MediaProperty prop, bool& is_valid) const
      {
        return IMediaPropertyObject::property<T>(prop, is_valid);
      }

  };

  using IMediaStreamPtr = std::shared_ptr<IMediaStream>;
}

#endif // IMEDIASTREAM_H
