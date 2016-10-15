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
