#include "obj_parser.h"
#include "depth_buffer.h"

#include <memory>
#include <istream>
#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>

ObjFile* ObjFile::Load(const char* file_name) {
  ObjFile* ret = nullptr;
  std::ifstream ifs;
  ifs.open(file_name, std::ifstream::in);
  if (ifs.is_open()) {
    ret = Load(ifs);
    ifs.close();
  }
  return ret;
}

bool ObjFile::SkipLine(std::istream& in) {
  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  return in.good();
}

bool ObjFile::ReadLine(std::istream& in) {
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
        return SkipLine(in);
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

ObjFile* ObjFile::Load(std::istream& in) {
  ObjFile* obj_file = new ObjFile;
  while (obj_file->ReadLine(in));
  return obj_file;
}

#include <iostream>

void ObjFile::Draw(DepthBuffer* db, ARGBImageBuffer* ib) {
  for (auto&& face: faces_) {
    Point3 p[face.indices.size()];
    for (int idx = 0; idx < face.size; idx++) {
      uint32_t v_idx = static_cast<uint32_t>(face.indices[idx].vertex) - 1U;
      if (v_idx >= vertices_.size()) {
        std::cout << "ERROR IN ObjFile::Draw()" << std::endl;
        return;
      }
      p[idx] = vertices_[v_idx];
      p[idx][0] *= 30.0f;
      p[idx][1] *= -30.0f;
      p[idx][2] *= 10.0f;
      p[idx][0] += 300.0f;
      p[idx][1] += 300.0f;
    }
    DrawTriangle(db, ib, p[0], p[1], p[2]);
    if (face.indices.size() == 4)
      DrawTriangle(db, ib, p[2], p[3], p[0]);
  }
}

template <typename Scalar>
static bool InvertMatrix(Scalar* mx_out, Scalar* mx_in) {
  Scalar* m0 = &mx_in[0];
  Scalar* m1 = &mx_in[3];
  Scalar det = m0[0]*m1[1] - m0[1]*m1[0];
  if (det == 0.0)
    return false;

  Scalar* o0 = &mx_out[0];
  Scalar* o1 = &mx_out[3];
  o0[0] =  m1[1]/det; o0[1] = -m0[1]/det;
  o1[0] = -m1[0]/det; o1[1] =  m0[0]/det;
  o0[2] = -o0[0]*m0[2] - o0[1]*m1[2];
  o1[2] = -o1[0]*m0[2] - o1[1]*m1[2];
  return true;
}

void ObjFile::DrawTriangle(DepthBuffer* db, ARGBImageBuffer* ib,
                           Point3& p00, Point3& p10, Point3& p01) {
  float mx[6] = {p10[0] - p00[0], p01[0] - p00[0], p00[0],
                 p10[1] - p00[1], p01[1] - p00[1], p00[1]};
  float inv_mx[6];
  if (InvertMatrix(inv_mx, mx)) {
    float clip_start_x = std::min(std::min(p00[0], p10[0]), p01[0]);
    float clip_start_y = std::min(std::min(p00[1], p10[1]), p01[1]);
    float clip_end_x   = std::max(std::max(p00[0], p10[0]), p01[0]);
    float clip_end_y   = std::max(std::max(p00[1], p10[1]), p01[1]);
    db->DrawTriangle(clip_start_x, clip_start_y,
                     clip_end_x, clip_end_y,
                     inv_mx, p00[2], p10[2], p01[2]);
  }
}
