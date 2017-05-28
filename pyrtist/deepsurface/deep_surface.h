// Copyright (C) 2017 Matteo Franchin
//
// This file is part of Pyrtist.
//   Pyrtist is free software: you can redistribute it and/or modify it
//   under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation, either version 2.1 of the License, or
//   (at your option) any later version.
//
//   Pyrtist is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _DEEP_SURFACE_H
#define _DEEP_SURFACE_H

#include "image_buffer.h"
#include "depth_buffer.h"

class DeepSurface {
 public:
  /// Transfer the surface to the given destination surface.
  static bool Transfer(DepthBuffer* src_depth, ARGBImageBuffer* src_image,
                       DepthBuffer* dst_depth, ARGBImageBuffer* dst_image);

  /// Sculpt the source surface on top of the destination surface.
  static bool Sculpt(DepthBuffer* src_depth, ARGBImageBuffer* src_image,
                     DepthBuffer* dst_depth, ARGBImageBuffer* dst_image);
};

#endif /* _DEEP_SURFACE_H */
