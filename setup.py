# Copyright (C) 2012-2017, 2020 Matteo Franchin
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 2.1 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

'''Installation script for Pyrtist. Install as follows:

sudo python setup.py install
'''

import sys
import os
from setuptools import setup, Extension, Feature

# Get app information from info file.
info = {}
root_dir = os.path.dirname(os.path.realpath(__file__))
info_file = os.path.join(root_dir, 'pyrtist', 'gui', 'info.py')
if sys.version_info.major >= 3:
    exec(open(info_file).read(), info)
else:
    execfile(info_file, info)

# Take the long description from the README.rst file.
readme_path = os.path.join(root_dir, 'README.rst')
with open(readme_path, 'r') as f:
    long_description = f.read()

cfg = dict(name='pyrtist',
           version=info['version_string'],
           description='A GUI which enables drawing using Python scripts',
           author='Matteo Franchin',
           author_email='fnch@users.sf.net',
           url='https://github.com/mfnch/pyrtist',
           license='LGPL',
           packages=['pyrtist', 'pyrtist.gui', 'pyrtist.gui.dox',
                     'pyrtist.gui.comparse', 'pyrtist.lib2d',
                     'pyrtist.lib2d.prefabs',
                     'pyrtist.lib2deep', 'pyrtist.lib3d'],
           package_data={'pyrtist': ['examples/*.py',
                                     'examples/*.png',
                                      'icons/24x24/*.png',
                                      'icons/32x32/*.png',
                                      'icons/fonts/*.png']},
           classifiers=
             ['Programming Language :: Python :: 2',
              'Programming Language :: Python :: 2.7',
              'Development Status :: 3 - Alpha',
              'Topic :: Multimedia :: Graphics :: Editors :: Vector-Based',
              ('License :: OSI Approved :: GNU Lesser General Public '
               'License v2 or later (LGPLv2+)')],
           long_description=long_description)

# C++ extensions.
srcs = ['image_buffer.cc', 'depth_buffer.cc', 'deep_surface.cc',
        'py_image_buffer.cc', 'py_depth_buffer.cc', 'texture.cc',
        'mesh.cc', 'py_mesh.cc', 'obj_parser.cc', 'py_init.cc']
srcs_full_paths = [os.path.join('pyrtist', 'deepsurface', file_name)
                   for file_name in srcs]
deepsurface_module = [Extension('pyrtist.deepsurface',
                                extra_compile_args=['-std=c++11'],
                                sources=srcs_full_paths,
                                libraries=['cairo'])]

# Cython extensions.
try:
    from Cython.Build import cythonize
except:
    cython_modules = []
else:
    cython_modules = \
      cythonize(os.path.join('pyrtist', 'lib3d', 'perf_critical.pyx'))

srcs = ['image_buffer.cc', 'depth_buffer.cc', 'deep_surface.cc',
        'py_image_buffer.cc', 'py_depth_buffer.cc', 'texture.cc',
        'mesh.cc', 'py_mesh.cc', 'obj_parser.cc', 'py_init.cc']
srcs_full_paths = [os.path.join('pyrtist', 'deepsurface', file_name)
                   for file_name in srcs]
deepsurface_module = [Extension('pyrtist.deepsurface',
                                extra_compile_args=['-std=c++11'],
                                sources=srcs_full_paths,
                                libraries=['cairo'])]

features = \
  {'deepsurface': Feature('the deepsurface module.',
                          standard=False,
                          ext_modules=deepsurface_module),
   'cython-accel': Feature('critical computations speedups via Cython',
                           standard=False,
                           available=bool(cython_modules),
                           ext_modules=cython_modules)}
cfg.update(features=features,
           entry_points={'console_scripts': ['pyrtist=pyrtist.gui:main']})

setup(**cfg)
