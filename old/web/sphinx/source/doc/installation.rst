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

Box
^^^

Download the tarball from `<http://sourceforge.net/projects/boxc>`__.
This is usually a file with name such as ``box-X.Y.Z.tar.gz``,
where X.Y.Z are the major, minor and patch version numbers.
Untar and configure the package with::

  tar xzvf box-0.3.1.tar.gz
  cd box-0.3.1
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
If you need further help, take a look at the 

Boxer
^^^^^

Download the tarball from `<http://sourceforge.net/projects/boxc>`__.
This is usually a file with name such as ``boxer-X.Y.Z.tar.gz``, where X.Y.Z
are the major, minor and patch version numbers.
Make sure that ``PyGTK`` and ``pygtksourceview`` are installed on your system.
On Ubuntu 11.10 you can install them using::

  apt-get install python-gtk2 python-gtksourceview2

Untar and install the package with::

  tar xzvf boxer-0.3.5.tar.gz
  cd boxer-0.3.5
  sudo python setup.py install

Boxer needs Box to run properly. If you installed it into a system path, then
it should be able to find it automatically, otherwise you may have to specify
where it should be found. For example::

  boxer --box-exec=/home/myname/local/bin/box

Next time you start Boxer, you can do it simply by typing ``boxer`` as Boxer
will remember the path to the Box executable, if it could run it.

Note for installation in /usr/local
-----------------------------------

If you install to ``/usr/local`` you must make sure that libraries in
``/usr/local/lib`` are found by the linker. On Ubuntu 11.10, installation
on ``/usr/local`` requires this::

  bash
  export LD_LIBRARY_PATH=/usr/local/lib
  boxer --box-exec=/usr/local/bin/box

You can alternatively modify your ``/etc/ld.so.conf`` file, to add the
directory ``/usr/local/lib`` to the list used for resolving libraries.

Note that the problem does not arise when you install to non-standard
locations. For example, if you install with ``--prefix=/home/myname/local``
then Box will work out of the box. This is because, ``/home/myname/local``
is recognized as a non-standard location and flags are added during the
compilation to cope with this (``-rpath``). In contrast, ``/usr/local/lib``
is a standard location and system is expected to look inside it (this is how
``autoconf`` behaves by default).


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
This is usually a file with name such as ``boxer-0.3.2.zip``.
Unzip the archive.
You'll find the Boxer executable in ``boxer-0.3.2\boxer.exe``.
Double click on the file and the Boxer windows should appear.

The manual is online at `<http://boxc.sourceforge.net>`__.
If you need further help, take a look at the provided examples.

