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

#include "texture.h"
#include "image_buffer.h"

namespace deepsurface {

uint32_t ImageTexture::ColorAt(float x, float y) {
  if (x < 0.0 || x > 1.0 || y < 0.0 || y > 1.0) {
    return 0;
  }

  size_t ix = static_cast<size_t>(x * image_->GetWidth());
  size_t iy = static_cast<size_t>((1.0f - y) * image_->GetHeight());
  size_t idx = iy * image_->GetStride() + ix;

  return image_->GetPtr()[idx];
}

}  // namespace deepsurface
