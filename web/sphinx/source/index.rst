.. Box documentation master file, created by
   sphinx-quickstart on Wed Oct 20 11:13:27 2010.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to the Box project!
===========================

Introduction
------------

Box(er) is made by two equally important components:

- **a programming language, Box**, designed with vector graphics in mind,
- **a graphical user interface, Boxer**, which allows you to edit the Box
  program and - at the same time - see the figure, zoom in and out,
  interact with it, browse the documentation interactively.

The aim of the project is hence to maintain the ease-of-use offered
by mouse-driven graphical editors, adding - on top of it - the customizability
which only a powerful dedicated programming language can provide.
Below you find a screenshot of latest version of Boxer.

.. image:: boxer0.3.2_screenshot.png
   :align: center

The small squares are "reference points" for the figure.
They can be moved using the mouse and can be used in the Box source
as pre-set variables. This way, it is possible to use the mouse
to interact directly with the Box source.

Features
--------

Box provides most of the features needed to create quality pictures and figures.
Box has the following advantages over traditional GUI graphics editors:

- **Smart figures:** while you can still see what you draw and interact
  with it as in traditional vector graphics editors, you can also add custom
  code to make your figures "smart",
- **Reusability:** you can reuse your past figures just via cut & paste from
  other Box programs,
- **Flexibility:** you always draw things in Window objects.
  You can then translate, rotate, scale and put such Window objects
  inside other Window objects,
- **Powerful tools:** Transformation matrices can be calculated automatically
  from constraints given by the user,
- **Flexible syntax:** don't need to declare variables when redundant,
  don't need to put semicolons everywhere, don't need to give function
  parameters in a precise order just for the sake of it, use default values
  as needed (you can specify many parameters, but you don't have to)...
- **Customizability:** you can base your figure on a set of variables which
  you define at the beginning. Changing colors, line sizes, shapes and styles
  is then a matter of seconds...
- Being a programming language, Box allows you to draw fractal pictures
  and Boxer allows you to interact with them!

Box has also the following features:

- you can save your figures to EPS, PDF, PNG and SVG.
  You can hence load your SVG figure with `Inkscape <http://inkscape.org/>`_
  and continue to work with the latter, if you prefer.
- translucency, radial and linear color gradients, lines with variable width,
- Box compiles under Linux, Mac and Windows.
  Source code is provided for UNIX like OSes, while a zip archive
  (containing the executable) is available for Windows.

Table of contents
-----------------

.. toctree::
   :maxdepth: 1

   examples/gallery
   doc/manual
   The language <language>
   doc/installation
   Download <http://sourceforge.net/projects/boxc/>
