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

#ifndef _MESH_H
#define _MESH_H

#include "geom.h"
#include "texture.h"

#include <memory>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>

class DepthBuffer;
class ARGBImageBuffer;

namespace deepsurface {

struct Face {
 public:
  using Index = int;
  static constexpr Index kInvalidIndex = -1;

  class Indices {
   public:
    Indices(Index v = kInvalidIndex,
            Index t = kInvalidIndex,
            Index n = kInvalidIndex)
        : vertex(v), tex(t), normal(n) { }

    Index vertex;
    Index tex;
    Index normal;
  };

  Face() : size(0) {}

  bool Append(Indices& item) {
    if (size >= indices.size())
      return false;
    indices[size++] = item;
    return true;
  }

  unsigned int size;
  std::array<Indices, 4> indices;
};


class Mesh {
 public:
  using Scalar = float;
  using Point2 = Point<Scalar, 2>;
  using Point3 = Point<Scalar, 3>;

  static Mesh* LoadObj(const char* file_name);
  static Mesh* LoadObj(std::istream& in);

  Mesh() {
    SetTexture(std::unique_ptr<Texture>(new UniformTexture(~UINT32_C(0))));
  }

  void AddVertex(const Point3& v) { vertices_.push_back(v); }

  void AddTexCoords(const Point2& tc) { tex_coords_.push_back(tc); }

  Face* CreateFace() {
    faces_.emplace_back();
    return &faces_.back();
  }

  void SetTexture(std::unique_ptr<Texture> texture) {
    if (tex_zones_.size() > 0 &&
        tex_zones_.back().starting_face == faces_.size()) {
      // No faces were added since the last texture definition, meaning that
      // the last material has no associated faces: replace it!
      tex_zones_.back().texture = std::move(texture);
    } else {
      tex_zones_.emplace_back();
      auto& new_zone = tex_zones_.back();
      new_zone.starting_face = faces_.size();
      new_zone.texture = std::move(texture);
    }
  }

  void Draw(DepthBuffer* db, ARGBImageBuffer* ib, const Affine3<Scalar>& mx);

 private:
  struct TextureZone {
    std::unique_ptr<Texture> texture;
    size_t starting_face;
  };

  bool ReadLine(std::istream& in);
  void DrawTriangle(DepthBuffer* db, ARGBImageBuffer* ib,
                    const Point3& p1, const Point3& p2, const Point3& p3,
                    uint32_t color);

  std::vector<TextureZone> tex_zones_;
  std::map<std::string, int> groups_;
  std::unordered_map<std::string, int> material_files_;
  std::vector<Point3> vertices_;
  std::vector<Point3> normals_;
  std::vector<Point2> tex_coords_;
  std::vector<Face> faces_;
};

}  // namespace deepsurface

#endif  // _MESH_H
