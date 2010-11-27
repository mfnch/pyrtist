Introduction to the Box Language
================================

Compound statements
-------------------
One concept Box put emphasis in is the one of compound statement.
For the C programming language a compound statement is just a list
of statements separated by ';' and enclosed in curly braces::

  {statement1; statement2; ...; statementN}

This C construct may be regarded as the starting point for the Box
language. Compound statements in Box are called just boxes
and follow a different syntax with respect to C::

  [statement1 SEP statement2 SEP ... SEP statementN]

or::

  Type[statement1 SEP statement2 SEP ... SEP statementN]

where SEP is a separator. Three different separators can be used:

- a comma ``,``
- a newline
- a semicolon ``;``

Commas and newlines have a pure syntactical role, just as it happens for
semicolons in C. But, in Box, semicolons, beside their syntactical
role as separators, have also a semantical value: they signal that something
should be done and have effect on the execution. We'll return on this later.
The main difference between a box and a C compound statment,
is that the former can have a type::

  int = Int[1.0]

In the line above a box of type Int is used to create an integer,
which is then assigned to the variable ``int``.
The reader may think, at this point, that ``box`` is a funny way
to say ``function``. This is not true. A box is really a compound statement:
it collects the statements required to create a certain object.
You can put loops, conditionals and any other statement inside
the square brackets. For example, the code::

  Print["You can print from here!";]
  int = Int[1, Print["but also from here!";]]

is perfectly legitimate.
And indeed, the number ``1.0`` in ``Int[1.0]`` is itself a statement
with type Real and value 1.0.
This example shows that - Similarly to C - statements often have
type and value.
However - unlike C - **values of statements are rarely ignored!**
The value ``1.0`` is used to create the integer
which is then assigned to the variable ``int``.

We also notice that **type names always begin with an uppercase letter**,
such as ``Int``, ``Real``, ``MyType``, while variable names
always begin with a lowercase letter: ``int`` is a variable
in the line above, but also ``iNT`` may be used as a variable name.
Box intrinsic types are: ``Char``, ``Int``, ``Real``, ``Point`` and ``Void``.

We should now be ready to face the hello-world example: a program
which just prints "Hello world!" out on the screen::

  Print["Hello world!";]

Here we use the ``Print`` box to show a string.
``Print`` is a type which descend from the type ``Void``.
``Void`` and its derivatives are very special types:
``Void`` boxes do not produce any object.

This example also shows how the semicolon may be used in Box programs.
For the Print statement the semicolon means "go to the next line".
In general the meaning of the semicolon can change from type to type,
but is meant to signal a kind-of pause in the Box construction.

As said before statements inside a box are rarely ignored.
One exception concerns the assignment operator::

  Print[a = "Hello world!"; a;]

prints the string "Hello world!" only once, even if the statement
``a = "Hello world!"`` has itself a defined type (it is an array of ``Char``)
and value. The reason for this behaviour is simple: one usually does not want
to pass the value of the assigned quantity to the underlying box.
To really do that, one should put round brackets
around the assignment expression::

  Print[(a = "Hello world!"); a;]

This line will print twice the string "Hello world!".

There may be cases when one wants to explicitly ignore a value.
This is the syntax to use in such cases::

  Print[\ 1, "nothing appears before the first occurrence 'nothing'";]

The ``\`` character avoids the valued expression to be passed
to the underlying box.

At this point it may be clear why boxes are so central:
they do not simply group statements, but they provide
the functionality of function calls.
While in C language, to draw a circle on the screen,
one may write something like::

  double center[2] = {1.0, 2.0}, radius = 1.0;
  Circle *my_circle = draw_circle(center, radius);

in Box the same functionality is achieved by opening a box with type Circle::

  my_circle = Circle[(1.0, 2.0), 1.0]

or::

  my_circle = Circle[1.0, (1.0, 2.0)]

One important difference between the two approaches concerns the order
of the arguments. While for C the order matters, for Box the order
is not crucial. this is a design goal of Box.
One should not be forced to always remember the order of arguments,
since it is often meaningless: a circle is defined just by its center
and its radius; there is no reason why one should specify center and radius
in this precise order and not the opposite!

At this point we can reveal what is really happening in the line
we have just seen, which is substantially different
from a simple function call. One may translate it into the following C-code::

  Circle *my_circle = circle_new();
  circle_begin(my_circle);
  circle_real(my_circle, radius);
  circle_point(my_circle, center);
  circle_end(my_circle);

Every value the ``Circle`` box gets, is used to call a method
of the object ``Circle``. In fact the Box declaration of the object ``Circle``
may look like this::

  Circle = (Real radius, Point center)
  Real@Circle[$$.radius = $]
  Point@Circle[$$.center = $]
  ([)@Circle[$$.radius = 0.0, $$.center = (0.0, 0.0)]
  (])@Circle[ ...finalisation code... ]

We will explain in details the syntax in the next sections.

Types and values
----------------

more to come soon...

