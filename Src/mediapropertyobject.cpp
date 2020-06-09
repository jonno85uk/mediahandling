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

#include "mediapropertyobject.h"

using media_handling::MediaPropertyObject;


std::string MediaPropertyObject::repr()
{
  return "TODO";
}


 bool MediaPropertyObject::hasProperty(const MediaProperty prop) const
 {
   return properties_.count(prop) > 0;
 }

void MediaPropertyObject::setProperties(std::map<media_handling::MediaProperty, std::any> props)
{
  properties_ = std::move(props);
}

void MediaPropertyObject::setProperty(const MediaProperty prop, const std::any& value)
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

std::map<media_handling::MediaProperty, std::any> MediaPropertyObject::properties() const
{
  return properties_;
}
