
// scalar -> (> char, int, real >)
// point -> (> point2d, point3d >)
//
// generic -> specie( scalar, point3d )
//
// NO: a = 5#int // (array of 5 integers)
// NO: a#1 = 16384
// OK: a = (10)Intg
// OK: a(1) = 16384
//
// NO: a = 5#int, repeat[5, .i = 0][a#.i = .i]
// C:  int a[5]; for(i = 0; i < 5; i++) a[i] = i;
// OK: a = (5)Intg, Repeat[5][a(.i) = .i]
//
// NO: a = 5#(int, (/real, intg, char/))
// OK: A = (5)(Intg, (Real > Intg > Char))
// C:  struct {int, real} a[5];
// OK: a = (5)(Intg, Real)
//
// Char = char
// Intg = (/intg, char/)
// Real = (/real, Intg/)
//
// String = #char
//
// String = ()Char
//
// a = b = c = String
// a = "Ciao"
//
// a = 5#int
// a#1 = 1
//
//
