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
     * @brief         Set the properties of this object. This will overwrite previous properties.
     * @param props   New map of properties
     */
    virtual void setProperties(const std::map<MediaProperty, std::any>& props);

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
