#include "mediapropertyobject.h"

using media_handling::MediaPropertyObject;


std::string MediaPropertyObject::repr()
{
  return "TODO";
}

void MediaPropertyObject::setProperties(const std::map<MediaProperty, std::any>& props)
{
  properties_ = props;
}

void MediaPropertyObject::setProperty(const MediaProperty prop, std::any value)
{
  properties_[prop] = value;
}

std::any MediaPropertyObject::property(const MediaProperty prop, bool& is_valid) const
{
  if (properties_.count(prop) > 0) {
    is_valid = true;
    return properties_.at(prop);
  }
  is_valid = false;
  return {};
}
