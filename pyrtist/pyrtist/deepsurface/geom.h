#ifndef _OBJ_GEOM_H
#define _OBJ_GEOM_H

#include <array>
#include <ostream>
#include <typeinfo>

namespace deepsurface {

/// @brief N-dimensional point, consisting of N scalars of type T.
template <typename T, int N>
class Point {
 public:
  using Scalar = T;

  template <typename ...Args>
  Point(Args... args) : value_{args...} { }

  template <typename T1, int N1>
  friend std::ostream& operator<<(std::ostream& os, const Point<T1, N1>& p);

  Scalar& operator[](int idx) { return value_[idx]; }
  const Scalar& operator[](int idx) const { return value_[idx]; }

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

template <typename T, int M, int N>
class Matrix {
 public:
  using Scalar = T;
  static constexpr int kNumRows = M;
  static constexpr int kNumCols = N;
  using Row = std::array<Scalar, kNumCols>;

  Matrix() { Set(1.0); }
  Matrix(T factor) { Set(factor); }
  Matrix(const Point<T, M>& diagonal) { Set(diagonal); }

  void Set(T factor) {
    for (int r = 0; r < kNumRows; r++)
      for (int c = 0; c < kNumCols; c++)
        rows_[r][c] = (r == c) ? factor : 0.0;
  }

  void Set(const Point<T, M>& diagonal) {
    for (int r = 0; r < kNumRows; r++)
      for (int c = 0; c < kNumCols; c++)
        rows_[r][c] = (r == c) ? diagonal[r] : 0.0;
  }

  Point<T, M> Apply(const Point<T, N>& p) const {
    Point<T, M> out;
    for (int r = 0; r < kNumRows; r++) {
      T x = 0.0;
      for (int c = 0; c < kNumCols; c++)
        x += rows_[r][c]*p[c];
      out[r] = x;
    }
    return std::move(out);
  }

  template <typename T1, int M1, int N1>
  friend std::ostream& operator<<(std::ostream& os,
                                  const Matrix<T1, M1, N1>& m);

 protected:
  std::array<Row, kNumRows> rows_;
};

template <typename T, int M, int N>
std::ostream& operator<<(std::ostream& os, const Matrix<T, M, N>& m) {
  os << "Matrix<T, " << M << ", " << N << ">(" << std::endl;
  for (auto& row : m.rows_) {
    const char* sep = "\t";
    for (auto& entry : row) {
      os << sep << entry;
      sep = ",\t";
    }
    os << std::endl;
  }
  os << ")";
  return os;
}

template <typename T>
class Affine3 : public Matrix<T, 3, 4> {
 public:
  using Point3 = Point<T, 3>;

  // Inherit Matrix's constructors.
  using Matrix<T, 3, 4>::Matrix;

  void Translate(const Point3& t) {
    for (int r = 0; r < 3; r++)
      this->rows_[r][3] = t[r];
  }

  Point<T, 3> Apply(const Point3& p) const {
    Point3 out;
    for (int r = 0; r < 3; r++) {
      T x = 0.0;
      for (int c = 0; c < 3; c++)
        x += this->rows_[r][c]*p[c];
      out[r] = x + this->rows_[r][3];
    }
    return std::move(out);
  }
};

}  // deepsurface

#endif  // _OBJ_GEOM_H
