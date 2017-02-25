// Copyright (C) 2017 by Matteo Franchin (fnch@users.sf.net)
//
// This file is part of Pyrtist.
//   Pyrtist is free software: you can redistribute it and/or modify it
//   under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Pyrtist is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

#include <assert.h>

#include "deep_surface.h"

// Utility functions.
static uint32_t GetA(uint32_t argb) { return (argb >> 24) & 0xff; }
static uint32_t GetR(uint32_t argb) { return (argb >> 16) & 0xff; }
static uint32_t GetG(uint32_t argb) { return (argb >> 8) & 0xff; }
static uint32_t GetB(uint32_t argb) { return argb & 0xff; }
static uint32_t GetARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
  return ((static_cast<uint32_t>(a) << 24) |
          (static_cast<uint32_t>(r) << 16) |
          (static_cast<uint32_t>(g) << 8) |
          (static_cast<uint32_t>(b)));
}

// Blending between premultiplied ARGB values.
// Premultiplied means that R, G, B are intended as already multiplied by the
// corresponding A value, which is the format we use here.
static uint32_t BlendSrcOverDst(uint32_t src_argb, uint32_t dst_argb) {
  uint32_t src_a = GetA(src_argb);
  if (src_a == 0xff)
    return src_argb;
  uint32_t src_am1 = UINT32_C(0xff) - src_a;
  uint32_t out_a = src_a          + (GetA(dst_argb)*src_am1)/UINT32_C(0xff);
  uint32_t out_r = GetR(src_argb) + (GetR(dst_argb)*src_am1)/UINT32_C(0xff);
  uint32_t out_g = GetG(src_argb) + (GetG(dst_argb)*src_am1)/UINT32_C(0xff);
  uint32_t out_b = GetB(src_argb) + (GetB(dst_argb)*src_am1)/UINT32_C(0xff);
  return GetARGB(out_a, out_r, out_g, out_b);
}

template <typename MergeFn>
bool MergeBuffers(DepthBuffer* src_depth, ARGBImageBuffer* src_image,
                  DepthBuffer* dst_depth, ARGBImageBuffer* dst_image,
                  MergeFn merge_fn) {
  if (!(src_depth->IsValid() && src_image->IsValid() &&
        dst_depth->IsValid() && dst_image->IsValid()))
    return false;

  int nx = src_depth->GetWidth();
  int stride = src_depth->GetStride();
  int new_line = stride - nx;
  int ny = src_depth->GetHeight();

  auto isrc = src_image->GetPtr();
  auto dsrc = src_depth->GetPtr();
  auto idst = dst_image->GetPtr();
  auto ddst = dst_depth->GetPtr();

  size_t offset = 0U;
  for (int iy = 0; iy < ny; iy++, offset += new_line) {
    size_t final_line_offset = offset + nx;
    for (; offset < final_line_offset; offset++)
      merge_fn(isrc[offset], dsrc[offset], &idst[offset], &ddst[offset]);
  }

  return true;
}

bool DeepSurface::Transfer(DepthBuffer* src_depth, ARGBImageBuffer* src_image,
                           DepthBuffer* dst_depth, ARGBImageBuffer* dst_image) {
  auto merge_fn = [](uint32_t src_argb, float src_depth,
                     uint32_t* dst_argb, float* dst_depth) -> void
  {
    if (GetA(src_argb) == 0 || DepthBuffer::IsInfiniteDepth(src_depth))
      // Source is transparent or has infinite depth. Nothing to do.
      return;

    if (DepthBuffer::IsInfiniteDepth(*dst_depth) || src_depth >= *dst_depth) {
      *dst_depth = src_depth;
      *dst_argb = BlendSrcOverDst(src_argb, *dst_argb);
    } else {
      assert(src_depth < *dst_depth);
      *dst_argb = BlendSrcOverDst(*dst_argb, src_argb);
    }
  };

  return MergeBuffers(src_depth, src_image, dst_depth, dst_image, merge_fn);
}

bool DeepSurface::Sculpt(DepthBuffer* src_depth, ARGBImageBuffer* src_image,
                         DepthBuffer* dst_depth, ARGBImageBuffer* dst_image) {
  auto merge_fn = [](uint32_t src_argb, float src_depth,
                     uint32_t* dst_argb, float* dst_depth) -> void
  {
    if (GetA(src_argb) == 0 || DepthBuffer::IsInfiniteDepth(src_depth) ||
        GetA(*dst_argb) == 0 || DepthBuffer::IsInfiniteDepth(*dst_depth))
      // Source/dest are transparent or have infinite depth. Nothing to do.
      return;
    *dst_depth += src_depth;
    *dst_argb = BlendSrcOverDst(src_argb, *dst_argb);
  };

  return MergeBuffers(src_depth, src_image, dst_depth, dst_image, merge_fn);
}

#ifdef TEST_DEPTH_SURFACE

// Build as a standalone C++ program for testing.
int main() {
  DeepSurface ds_src{320, 320, 200};
  DeepSurface ds_dst{320, 320, 200};

  auto ib = ds_src.GetImageBuffer();
  auto db = ds_src.GetDepthBuffer();

  ib->DrawCircle(107, 70, 80, 0xffff0000);
  db->DrawSphere(107, 70, 0, 80, 1.0);
  DeepSurface::Transfer(&ds_src, &ds_dst);

  ib->DrawCircle(214, 130, 80, 0xffffff00);
  db->DrawSphere(214, 130, 10, 80, 1.0);
  DeepSurface::Transfer(&ds_src, &ds_dst);

  ds_dst.SaveToFiles("spheres-image.png", "spheres-normals.png");
}
#endif
