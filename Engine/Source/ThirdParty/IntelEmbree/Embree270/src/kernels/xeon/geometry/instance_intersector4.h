// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "../../common/scene_instance.h"
#include "../../common/ray4.h"

namespace embree
{
  namespace isa
  {
    struct FastInstanceIntersector4
    {
      static void intersect(bool4* valid, const Instance* instance, Ray4& ray, size_t item);
      static void occluded (bool4* valid, const Instance* instance, Ray4& ray, size_t item);
    };
  }
}
