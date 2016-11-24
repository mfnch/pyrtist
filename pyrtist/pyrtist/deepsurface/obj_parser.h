#ifndef _OBJ_PARSER_H
#define _OBJ_PARSER_H

#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <ostream>
#include <typeinfo>


class DepthBuffer;
class ARGBImageBuffer;


template <typename T, int N>
class Point {
 public:
  using Scalar = T;

  template <typename ...Args>
  Point(Args... args) : value_{args...} { }

  template <typename T1, int N1>
  friend std::ostream& operator<<(std::ostream& os, const Point<T1, N1>& p);

  Scalar& operator[](int idx) { return value_[idx]; }

 protected:
  std::array<Scalar, N> value_;
};

template <typename T, int N>
std::ostream& operator<<(std::ostream& os, const Point<T, N>& p) {
  os << "Point<" << typeid(T).name() << "," << N << ">(";
  if (N > 0) {
    os << p.value_[0];
    for (int i = 1; i < N; i++)
      os << ", " << p.value_[i];
  }
  os << ")";
  return os;
}


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
  using Point2 = Point<Scalar, 2>;
  using Point3 = Point<Scalar, 3>;

  static ObjFile* Load(const char* file_name);
  static ObjFile* Load(std::istream& in);

  void Draw(DepthBuffer* db, ARGBImageBuffer* ib);

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
