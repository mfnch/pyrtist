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

#include "mesh.h"
#include "depth_buffer.h"

#include <memory>
#include <istream>
#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>

namespace deepsurface {

Mesh* Mesh::LoadObj(const char* file_name) {
  Mesh* ret = nullptr;
  std::ifstream ifs;
  ifs.open(file_name, std::ifstream::in);
  if (ifs.is_open()) {
    ret = LoadObj(ifs);
    ifs.close();
  }
  return ret;
}

Mesh* Mesh::LoadObj(std::istream& in) {
  Mesh* obj_file = new Mesh;
  while (obj_file->ReadLine(in));
  return obj_file;
}

static bool SkipLine(std::istream& in) {
  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  return in.good();
}

bool Mesh::ReadLine(std::istream& in) {
  in >> std::ws;
  std::string line_head;
  in >> line_head;
  auto line_head_size = line_head.size();
  if (line_head_size < 1) {
    return SkipLine(in);
  }

  // Comments.
  if (line_head[0] == '#')
    return SkipLine(in);

  if (line_head_size == 1) {
    switch (line_head[0]) {
      case 'v':
        // Vertices.
        float x, y, z;
        in >> x >> y >> z;
        vertices_.emplace_back(x, y, z);
        return SkipLine(in);
      case 'f':
        {
          faces_.emplace_back();
          Face& face = faces_.back();

          std::string line;
          if (!std::getline(in, line, '\n'))
            return false;

          std::stringstream ls(line);
          std::string piece;
          while (std::getline(ls, piece, ' ')) {
            if (piece.size() > 0) {
              std::stringstream ss(piece);
              int nr_indices = 0;
              Face::Index indices[3];

              do {
                int index;
                ss >> index;
                indices[nr_indices++] = index;
              } while (nr_indices < 3 && ss.peek() == '/' && ss.get());

              while (nr_indices < 3)
                indices[nr_indices++] = Face::kInvalidIndex;

              Face::Indices item{indices[0], indices[1], indices[2]};
              face.Append(item);
            }
          }
        }
        return true;
      case 'g':
        {
          std::string group_name;
          in >> group_name;
          groups_[group_name] = 0;
        }
        return SkipLine(in);
      case 's':
        std::cout << "s" << std::endl;
        return SkipLine(in);
    }
  } else if (line_head_size == 2) {
    if (line_head[0] == 'v') {
      switch (line_head[1]) {
        case 'n':
          // Vertex normal.
          float x, y, z;
          in >> x >> y >> z;
          normals_.emplace_back(x, y, z);
          return SkipLine(in);
        case 't':
          // Texture coordinates.
          float u, v;
          in >> u >> v;
          tex_coords_.emplace_back(u, v);
          return SkipLine(in);
      }
    }
  } else if (line_head == "mtllib") {
    std::string material;
    in >> material;
    material_files_[material] = material_files_.size();
    return SkipLine(in);
  } else if (line_head == "usemtl") {
    std::cout << "usemtl" << std::endl;
    return SkipLine(in);
  }

  std::cout << "UNKNOWN LINE HEAD: " << line_head << std::endl;
  return false;
}

}  // namespace deepsurface
