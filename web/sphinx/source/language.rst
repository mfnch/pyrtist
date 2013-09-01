.. contents:: Table of Contents

Introduction to the Box Language
================================

Objects and types
-----------------

There are two main entities in the Box language: **types** and **objects**.
The types describe the structure of the data used in a program, while the
objects are instances of such data. In particular, objects always have an
associated type which describes their internal structure. A typical Box program
ends up defining some types and using these types to define some objects.

In the Box language, all type names start with an uppercase letter. For
example: ``MyNewFunkyType``. A number of pre-defined types exists,
like ``Int``, ``Real`` for integer and real numbers, respectively.
Pre-existing *types can be combined together to form new types.*
For example::

  NewType = Int            // NewType is just a new name for the type Int
  Tuple = (Int, Real)      // A tuple with anonymous members
  Point3D = (Real x, y, z) // A tuple formed by three members of type Real

The lines above show three ways of defining a new type in Box.
There are many more ways of combining already existing types to create new
ones. Note also the way **comments** are added to the sources.
The part of the line following ``//`` is a comment and is ignored.

There are a number of **intrinsic types** the user can start from.
``Int`` is the type for integer numbers, ``Real`` for real numbers,
``Str`` is the type for strings, ``Char`` is the type for characters.
Types can then be used to create new objects. For example, a new empty string
can be created as follows::

  s = Str[]

In general, a new instance for a given type ``MyType`` can be created using
the syntax ``MyType[]``. In the line above, we are also giving a name to
the new emtpy string created with ``Str[]``.
Names for instances, start with a lowercase letter. For example,
``myvariable``, ``my_variable`` or ``myVariable``.

The language also offers ad-hoc syntax to quickly create objects for most
of the intrinsic types. For example::

  s = "my string"  // String
  i = 123          // Integer number
  r = 1.23         // Real number
  c = 'a'          // Character

Similarly to types, also **objects can be combined together to create new
objects**. For example::

  Tuple = (Real, Real) // Creates a new structure type containing two Reals
  tuple = (1.2, 3.4)   // Creates a new instance of the type (Real, Real)
                       // containing the numbers 1.2 and 3.4

Creating types is useful, as it simplifies creating new objects. For example::

  Triple = (Real x, y, z)
  triple1 = Triple[]
  triple2 = Triple[.x = .y = .z = 1.0]

Commenting the sources
----------------------

In line comments can be added by using either ``#!`` or ``//``.
For example,

  #!/usr/bin/box
  // This is a Box program which prints "Hello world!".
  // This example does also show how to comment Box sources.
  a = "Hello world!"  // a is a string.
  Print[a;]           // here we print a.

In-line comments can be added using the two characters ``//``.  The part of the
line following the first occurrence of``//`` or ``#!`` is ignored. Multi-line
comments can be added using ``(*`` and `*)`` like this:

  (* This is a multi-line comment.
     This kind of comments can be nested, (* like this *)
     The comment ends here. *)

Boxes and type combinations
---------------------------

Creating objects is not the only thing one may want to do in a program. It is
also important to **combine objects together in order to do something useful**
with them. In this section, we explain how this can be done in the Box
language.

Let's start from the line::

  instance = MyType[]

We have seen that the line above can be used to create a new object of type
``MyType`` and assign it to the variable ``instance``.  The text fragment
``MyType[]`` is what we call a *box*.  Boxes can be empty, as in the previous
example, or can contain a group of statements. For example::

  instance = MyType[statement1, statement2, statement3]

``statement1``, ``statement2``, and ``statement3`` can be used to perform three
"actions" on the object which is being created. For example, let's imagine we
are creating our own graphic library and we are trying to implement a way to
define a circle shape. Then we may do it as follows::

  circle = Circle[(0.0, 0.0), 10.0]

The box ``Circle[(0.0, 0.0), 10.0]`` has type ``Circle`` and contains two
statements. The first statement, ``(0.0, 0.0)``, provides the center of the
circle. The second statement, ``10.0``, provides the radius of the circle.

The examples we have seen so far tell us three things:

1. numbers, strings, etc. are themselves statements,
2. statements are grouped together inside boxes and serve to construct or
   modify an object (whose type is specified just before the opening square
   bracket),
3. a statement can have a type and a value. This value can be used by the box
   the statement belongs to (for example, a number ``10.0`` can be used by a
   ``Circle`` box to set the radius of a newly created circle).

Note that statements always have type and value. Indeed, the type is what
determines its role in the parent box. For example, in the line::

  s = Str[3.14]

a new string object is created. The floating point number ``3.14`` is passed to
``Str``, is converted to a string and used to create a new string in ``s``.
The result is that ``s`` is given the value ``"3.14"``.
Another example is given below::

  s = Str["My favourite number is ", 3.14]

Here ``Str`` receives two statements: one of type ``Str`` and one of type
``Real``. The effect is to create in ``s`` the string ``"My favourite number is
3.14"``. This shows that different objects can have different effects inside
the same box.  For example, a ``Real`` given to a ``Circle`` may set its
radius, while a ``Point`` may set its center.  The particular action invoked
when a statement of type ``Child`` is given inside a box of type ``Parent`` is
called "combination" and is denoted with ``Child@Parent``.
Combinations can be defined by the programmer using the following syntax::

  Child@Parent[...]

Inside the square brackets, the symbol ``$`` can be used to refer to the
particular child instance, while the symbol ``$$`` can be used to refer to the
parent.

Returning to the example of the ``Circle`` object, we may define::

  Circle = (Real radius, Point center)

and define a combination as follows::

  Real@Circle[$$.radius = $]

The line above defines a combination which is executed whenever a ``Real`` is
given to a ``Circle``. The line can be ideally split in two parts, one left
part ``Real@Circle`` and one right part ``[$$.radius = $]``. The left part
indicates when the code should be executed. The right part provides the actual
code to execute (the implementation of the combination). The combination takes
a Real (which is referred to by using the ``$`` character) and assigns its
value to the ``radius`` member of the ``Circle`` object (represented by
``$$``).  Similarly, we may define::

  Point@Circle[$$.center = $]

These lines specifies what should happen when ``Real`` and ``Point`` objects
are given to ``Circle`` box. In particular, the line below::

  circle = Circle[(0, 0), 10.0]

has the same effect of the lines::

  circle = Circle[]
  circle.center = (0, 0)
  circle.radius = 10.0

In our new graphic library, we may then specify how to draw a ``Circle`` inside
a sheet of paper ``SheetOfPaper`` by defining another combination::

  Circle@SheetOfPaper[
    // Implementation of circle drawing...
    // Here we draw $ (the Circle instance) inside $$ (the particular
    // SheetOfPaper instance)
  ]

Type combination is one of the fundamental ideas behind the Box language.  As a
Box programmer you are invited to identify which types of objects you need in
order to implement your algorithm effectively and to identify how these types
can be combined together to make something useful.

Why Boxes rather than functions?
--------------------------------

Most programming languages put an extraordinary and often unjustified attention
to the order of things. For example, if you want to draw a circle on the screen
in C, you may have to use a function like::

  void draw_circle(screen *scr, double x, double y, double radius);

This order of arguments in this function prototype is largely arbitrary, but
the programmer is forced to remember it anyway. Indeed, ``draw_circle`` may be
alternatively defined as::

  void draw_circle(screen *scr, double radius, double x, double y);

Whenever the programmer will have to use the function ``draw_circle`` he will
also have to think to the arguments this function takes and their precise
order. Some programming languages provide optional arguments, which may be seen
as a possible way to address the flaw. In Python, for example, you may use the
following code to circumvent the order constraint::

  def draw_circle(screen, point=None, radius=None):
    # check that point and radius are not None
    ...

You can then draw the circle in one of the following ways::

  draw_circle(screen, point=point, radius=radius)
  draw_circle(screen, radius=radius, point=point)

For some aspects, however, this is worse than the former version: while we now
do not need to remember the order of arguments, we have to remember their
name. The resulting function call is also unnecessarily verbose.

So far we have identified two problems that often occur while programming::

- the order of things is often arbitrary, and the programmer should not be
  forced to remember it: a circle is defined just by its center and its radius;
  there is no reason why one should specify center and radius in this precise
  order and not the opposite!

- the programmer should not be forced to remember names when unnecessary.
  More importantly, it is important to reduce the number of times t

: type
  names, function
  names, optional argument names.

Box addresses these flaws by inviting the user to think about which object
types are necessary and how these objects should be combined together.  The
concept of function or class method is then replaced with the concept of type
combination.  This way the number of names the user needs to remember is
drastically reduced.

For example, returning to our simple graphic library, we may define some shapes
to draw ``Circle``, ``Rectangle``, ``Polygon``.  These entities have to be
drawn somewhere, right? Then we need another type, ``SheetOfPaper``.  We may
also want to provide a fifth object, ``Area``, to compute the area of the
shapes. For example, if we have a ``Circle`` then the area is simply
``pi*radius*radius``, where ``pi`` is the constant 3.141592...
For a ``Rectangle`` it will be ``side_a*side_b``.

What Box requires, is just to define these 5 types. It is then clear how to use
them. If we give a shape to a ``SheetOfPaper`` then the shape is drawn on the
paper. If we give it to ``Area`` then the area is computed. We do not need
introduce new class methods with arbitrary names that a user would need to
remember. A fragment of Box source code using our library may then read::

  sheet = SheetOfPaper[Circle[(0, 0), 10]
                       Rectangle[(10, 10), (20, 20)]]
  area = Area[Circle[(0, 0), 15]]

In the code above, the user is free to forget the order of the arguments given
to ``Circle`` or ``Rectangle``.  He is not forced to learn names of methods
to use in order to draw a shape or to calculate its area.
Also, all this is achieved within a statically typed language, which, in
principle, can be converted to efficient machine code.

Hello world in Box
------------------

We should now be ready to face the hello-world example: a program which just
prints "Hello world!" out on the screen::

  Print["Hello world!";]

Here we use the ``Print`` box to show a string.
``Print`` is a type which descend from the type ``Void``.
``Void`` and its derivatives are very special types: they are empty types which
are always ignored.

This example also shows how the semicolon, the third statement separator
(together with commas and newlines) may be used in Box programs.  While commas
and newlines have a pure syntactical role (just as it happens for semicolons in
C) and are ignored, semicolons have both a syntactical role as separators and
an effect on execution. The Box programmer can decide which effect a semicolon
should have inside a box of a given type. For Print, the semicolon means "go to
the next line".

The reader may have understood, at this point, that ``boxes`` are fundamentally
different from ``functions``. A box is really a compound statement: it collects
the statements required to create a certain object.  You can put loops,
conditionals and any other statement inside the square brackets. For example,
the code::

  Print["You can print from here!";]
  int = Int[1, Print["but also from here!";]]

is perfectly legitimate.

Statements inside a box are rarely ignored.
One exception is made for the assignment operator.
The code below::

  Print[a = "Hello world!"; a;]

prints the string ``"Hello world!"`` only once, even if the statement
``a = "Hello world!"`` has itself a defined type (it is a ``Str`` object)
and value. The reason for this behaviour is simple: one usually does not want
to pass the value of the assigned quantity to the underlying box.
To really do that, one should put round brackets
around the assignment expression::

  Print[(a = "Hello world!"); a;]

This line will print twice the string ``"Hello world!"``.

There may be cases when one wants to explicitly ignore a value.
The line above shows the syntax to use in such cases::

  Print[\ 1, "nothing appears before the first occurrence 'nothing'";]

The ``\`` character discards the valued expression on its right:
the expression is not passed to the underlying box.

At this point it may be clear why boxes are so central:
they do not simply group statements, but they provide
the functionality of function calls.

Conditional execution
---------------------


more to come soon...
--------------------
