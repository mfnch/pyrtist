=======
Pyrtist
=======

Pyrtist is a library and IDE which enables **drawing using Python scripts**.
Multiple Python APIs can be used for drawing, such as
`PyCairo <https://pycairo.readthedocs.io/>`_,
`PIL <https://pillow.readthedocs.io/>`_, OpenGL
(using fragment shaders like in `Shadertoy <https://www.shadertoy.com/>`_) and
Pyrtist's own drawing APIs.

.. figure:: doc/seascape.png
   :align: center

   **Figure 1**: Fragment shader from
   `Shadertoy <https://www.shadertoy.com/view/Ms2SD1>`_ running in the Pyrtist GUI.
   See `here <pyrtist/examples/draw_with_glsl.py>`_ for a minimal GLSL (OpenGL)
   example.

The Pyrtist window is subdivided in two halves:

* The **script view** allows to write and adjust the Python script which draws
  the graphics,

* The **picture view** shows the graphical output produced by the script and
  allows to interact with the picture and adjust it, by moving some reference
  points (with the mouse for example), which the Python script sees as regular
  variables.

.. figure:: doc/pyrtist_house.png
   :align: center

   **Figure 2**: The cartoon house example opened in the Pyrtist GUI.

The two views allow you to create pictures that can exploit the scripting
capabilities of Python while still allowing you to naturally interact with the
graphics. Your drawings can be saved to disk as ordinary Python scripts.
You can execute them on their own, outside the Pyrtist user interface,
or you can re-open them with Pyrtist if you need to change them.

Your scripts can exploit the full power of Python and can be as smart as you
want them to be. If you are a programmer, you can then use the tools you are
familiar with. You can track different versions of your drawings using
`git <https://git-scm.com/>`_. You can use Makefiles to automatically rebuild a
large collection of Pyrtist drawings, which all import the same module defining
color and style. Need a different color scheme or line width scheme for all
your pictures? Just change one file and type ``make``. They will all be
generated automatically for you. Importantly, you can easily reuse old
drawings. They are text and as such you can easily copy and paste them. You can
create modules containing them and you can couple them with the thousands
Python libraries out there.

.. figure:: doc/draw_with_cairo.png
   :align: center

   **Figure 3**: Example showing how to use the PyCairo API from Pyrtist.
   `See source <pyrtist/examples/draw_with_cairo.py>`_.

======
Status
======

This software is under development. Pyrtist's own drawing API is not fully
stable and not well documented. The `examples <pyrtist/examples>`_
may give some guidance. Alternatively, other APIs can be used for drawing
such as PyCairo, PIL and GLSL.

Any feedback and contributions are welcome.
