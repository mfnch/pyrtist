# Copyright (C) 2012-2017 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
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

# In systems where the setuptools module cannot be used, the variable below
# can be used to select whether to build the deepsurface module.
BUILD_DEEPSURFACE = False

import os

try:
    from setuptools import setup, Extension, Feature
except ImportError:
    from distutils.core import setup, Extension
    Feature = None
    print('*** USING distutils')
    print('*** Edit the setup.py script to enable building deepsurface')
    print('***')

name = 'pyrtist'
version = '0.0.1'
description = 'A GUI which enables drawing using Python scripts'
author = 'Matteo Franchin'
author_email = 'fnch@users.sf.net'
url = 'http://boxc.sourceforge.net/',
download_url = ('http://sourceforge.net/project/'
                'platformdownload.php?group_id=218051')
license_name = 'LGPL'
packages = ['pyrtist', 'pyrtist.gui', 'pyrtist.gui.dox',
            'pyrtist.gui.comparse', 'pyrtist.lib2d', 'pyrtist.lib2deep',
            'pyrtist.lib3d']

package_data = {'pyrtist': ['examples/*.py',
                            'icons/24x24/*.png',
                            'icons/32x32/*.png',
                            'icons/fonts/*.png']}
classifiers = \
  ['Programming Language :: Python :: 2',
   'Programming Language :: Python :: 2.7',
   'Development Status :: 3 - Alpha',
   'Topic :: Multimedia :: Graphics :: Editors :: Vector-Based',
   'License :: OSI Approved :: GNU Lesser General Public License v3 or later (LGPLv3+)']

# C++ extensions.
ext_modules = []
srcs = ['image_buffer.cc', 'depth_buffer.cc', 'deep_surface.cc',
        'py_image_buffer.cc', 'py_depth_buffer.cc', 'mesh.cc', 'py_mesh.cc',
        'obj_parser.cc', 'py_init.cc']
srcs_full_paths = [os.path.join('pyrtist', 'deepsurface', file_name)
                   for file_name in srcs]
ext_modules.append(Extension('pyrtist.deepsurface',
                             extra_compile_args=['-std=c++11'],
                             sources=srcs_full_paths,
                             libraries=['cairo']))

# Add the extensions differently depending on whether we are using
# setuptools or distutils.
kwargs = {}
if Feature is not None:
    # setuptools way.
    kwargs['features'] = \
      {'deepsurface': Feature('the deepsurface module.',
                              standard=False, ext_modules=ext_modules)}
    kwargs['entry_points'] = \
      {'console_scripts': ['pyrtist=pyrtist:main']}
elif BUILD_DEEPSURFACE:
    # distutils way.
    kwargs['ext_modules'] = ext_modules
    kwargs['scripts'] = ['scripts/pyrtist']

setup(name=name,
      version=version,
      description=description,
      author=author,
      author_email=author_email,
      url=url,
      download_url=download_url,
      license=license_name,
      packages=packages,
      package_data=package_data,
      classifiers=classifiers,
      **kwargs)
