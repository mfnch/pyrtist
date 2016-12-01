#ifndef _OBJ_PARSER_H
#define _OBJ_PARSER_H

#include "geom.h"

#include <array>
#include <vector>
#include <map>
#include <unordered_map>

class DepthBuffer;
class ARGBImageBuffer;


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


class ObjFile {
 public:
  using Scalar = float;
  using Point2 = deepsurface::Point<Scalar, 2>;
  using Point3 = deepsurface::Point<Scalar, 3>;

  static ObjFile* Load(const char* file_name);
  static ObjFile* Load(std::istream& in);

  void Draw(DepthBuffer* db, ARGBImageBuffer* ib,
            const deepsurface::Affine3<float>& mx);

 private:
  bool SkipLine(std::istream& in);
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

#endif  // _OBJ_PARSER_H
