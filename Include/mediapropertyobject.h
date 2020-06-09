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

#ifndef MEDIAPROPERTYOBJECT_H
#define MEDIAPROPERTYOBJECT_H

#include <map>
#include <any>

#include "types.h"

namespace media_handling
{

class MediaPropertyObject
{
  public:
    MediaPropertyObject() = default;

    virtual ~MediaPropertyObject() = default;

    /**
     * @brief Get a string representation of the object
     * @return
     */
    virtual std::string repr();
    /**
     * @brief         Identify if object has a single stored property
     * @param   prop  the property to check
     * @return  true==object has property
     */
    virtual bool hasProperty(const MediaProperty prop) const;
    /**
     * @brief         Set the properties of this object. This will overwrite previous properties.
     * @param props   New map of properties
     */
    virtual void setProperties(std::map<MediaProperty, std::any> props);

    /**
     * @brief         Set a single property of this object
     * @param prop    The property to set
     * @param value   The value of the property
     */
    virtual void setProperty(const MediaProperty prop, const std::any& value);

    /**
     * @brief           Retrieve a single property of this object
     * @param prop      The property to retrieve
     * @param is_valid  An 'out' parameter identifying if the returned value is valid
     * @return          The value of the property if valid
     */
    virtual std::any property(const MediaProperty prop, bool& is_valid) const;
    /**
     * @brief         Retrieve all stored properties
     * @return        property->value mapping
     */
    virtual std::map<MediaProperty, std::any> properties() const;

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
  private:
    std::map<MediaProperty, std::any> properties_;
};

}

#endif // MEDIAPROPERTYOBJECT_H
