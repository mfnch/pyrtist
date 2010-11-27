Installation instructions
=========================

There are different ways to get Box and Boxer running on your machine:

- `Using a package manager`_: use the package manager of your distribution;
- `Installation from source`_: can be done easily on Unix platforms
  (Linux, Mac OS) and somewhat less easily on Microsoft Windows platforms;
- `Installation on Windows`_: just a matter of downloading a zip
  archive. The executable can be used immediately after unzipping
  the archive.

Using a Package manager
-----------------------

Box and Boxer can be found in the package repositories of a number of
distributions:

- `Fink (Mac OS) <http://www.finkproject.org/>`__ (unstable);
- `Arch Linux <http://aur.archlinux.org/>`__
- FreeBSD: `Box <http://www.freshports.org/graphics/box/>`__,
  `Boxer <http://www.freshports.org/graphics/boxer/>`__

Other useful links:

- `Softpedia <http://www.softpedia.com/>`__:
  `Box <http://mac.softpedia.com/get/Developer-Tools/Box.shtml>`__,
  `100 % free award <http://mac.softpedia.com/progClean/Box-Clean-40234.html>`__

Installation from source
------------------------

Download the tarball from `<http://sourceforge.net/projects/boxc>`__.
This is usually a file with name such as ``box-X.Y.tar.gz``,
where X.Y are the major and minor version numbers.
Untar and configure the package with::

  tar xzvf box-0.2.tar.gz
  cd box-0.2
  ./configure --prefix=/usr

``configure`` outputs a summary at the end. Be sure it looks like that::

  Configuration summary:
  ----------------------
  Support for the Cairo 2D graphic library: yes

If you get a ``"no"`` then check that the Cairo graphics library
`<http://www.cairographics.org>`__ is installed on your system
together with the development files.
On Ubuntu and Debian derivatives, you can install it with::

  sudo aptitude install libcairo libcairo-dev

You should enter the root password when necessary
for the installation to proceed.
Reconfigure the package with ``./configure --prefix=/usr``
in case you had to install the Cairo development files.
You can then start the compilation process with::

  make

If the compilation is succesful::

  sudo make install

The Box executable, libraries and headers are installed
on your system. You can take a look at the man page with::

  man box

Further help and hints with the installation can be found
in the ``README`` and ``INSTALL`` files inside the package.

The manual is online at `<http://boxc.sourceforge.net>`__.
If you need further help, take a look at the examples.

Note for installation on Mac OS
-------------------------------

If you compiled Box from source on Mac OS and you experience problems when
loading libraries (``-l g`` seems not to work as it should), try to
reconfigure Box with the additional option ``--with-included-ltdl``,
such as::

  ./configure --prefix=/usr --with-included-ltdl

and then compile it again. This should force the use of the provided
ltdl (libtool) library and should fix the bug.

Installation on Windows
-----------------------

Download the zip file from `<http://sourceforge.net/projects/boxc>`__.
This is usually a file with name such as ``boxer-0.2.0.zip``.
Unzip the archive.
You'll find the Boxer executable in ``boxer-0.2.0\boxer.exe``.
Double click on the file and the Boxer windows should appear.

The manual is online at `<http://boxc.sourceforge.net>`__.
If you need further help, take a look at the provided examples.

