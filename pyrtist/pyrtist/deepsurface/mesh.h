#ifndef _MESH_H
#define _MESH_H

#include "geom.h"

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
    int max_nr_indices = sizeof(indices)/sizeof(indices[0]);
    if (size >= max_nr_indices)
      return false;
    indices[size++] = item;
    return true;
  }

  int size;
  std::array<Indices, 4> indices;
};


class Mesh {
 public:
  using Scalar = float;
  using Point2 = Point<Scalar, 2>;
  using Point3 = Point<Scalar, 3>;

  static Mesh* LoadObj(const char* file_name);
  static Mesh* LoadObj(std::istream& in);

  void Draw(DepthBuffer* db, ARGBImageBuffer* ib, const Affine3<Scalar>& mx);

 private:
  bool ReadLine(std::istream& in);
  void DrawTriangle(DepthBuffer* db, ARGBImageBuffer* ib,
                    Point3& p1, Point3& p2, Point3& p3);

  std::map<std::string, int> groups_;
  std::unordered_map<std::string, int> material_files_;
  std::vector<Point3> vertices_;
  std::vector<Point3> normals_;
  std::vector<Point2> tex_coords_;
  std::vector<Face> faces_;
};

}  // namespace deepsurface

#endif  // _MESH_H
