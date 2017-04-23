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

#ifndef _TEXTURE_H
#define _TEXTURE_H

#include <memory>

class ARGBImageBuffer;

namespace deepsurface {

class Texture {
 public:
  virtual ~Texture() { }
  virtual uint32_t ColorAt(float x, float y) = 0;
};

class UniformTexture : public Texture {
 public:
  UniformTexture(uint32_t color) : color_(color) { }

  uint32_t ColorAt(float x, float y) override {
    return color_;
  }

 private:
  uint32_t color_;
};

class ImageTexture : public Texture {
 public:
  ImageTexture(ARGBImageBuffer* image) : image_(image) { }

  uint32_t ColorAt(float x, float y) override;

 private:
  ARGBImageBuffer* image_;
};

}  // namespace deepsurface

#endif  // _TEXTURE_H
