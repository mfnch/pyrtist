# Copyright (C) 2012 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Boxer.
#
#   Boxer is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Boxer is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Boxer.  If not, see <http://www.gnu.org/licenses/>.

import os
from distutils.core import setup

name = 'boxer'
version = '0.3.7'
description = ('A graphical user interface for '
               'the Box vector graphics language')
author = 'Matteo Franchin'
author_email = 'fnch@users.sourceforge.net'
url = 'http://boxc.sourceforge.net/',
download_url = ('http://sourceforge.net/project/'
                'platformdownload.php?group_id=218051')
license = 'GPL'
packages = ['boxer', 'boxer.dox']
package_dir = {'boxer': 'src'}

examples = \
  ['fractal.box', 'fractree.box', 'wheatstone.box', 'planes.box',
   'tutorial*.box']

examples_fullpaths = list(os.path.join('examples', bn)
                          for bn in examples)

package_data = {'boxer': (examples_fullpaths + 
                          ['icons/24x24/*.png',
                           'icons/32x32/*.png',
                           'icons/fonts/*.png',
                           'hl/*.lang'])}
script = 'scripts/boxer'
scripts = [script]


# Try to use py2exe, if available (on Windows)
try:
  import py2exe
  setup(name=name,
        version=version,
        description=description,
        author=author,
        author_email=author_email,
        url=url,
        download_url=download_url,
        license=license,
        packages=packages,
        package_dir=package_dir,
        package_data=package_data,
        scripts=scripts,
        windows=[{'script': script}],
        options={'py2exe': {'packages': 'encodings',
                            'includes': ('cairo, pango, pangocairo, '
                                         'atk, gobject')}})

except:
  setup(name=name,
        version=version,
        description=description,
        author=author,
        author_email=author_email,
        url=url,
        download_url=download_url,
        license=license,
        packages=packages,
        package_dir=package_dir,
        package_data=package_data,
        scripts=scripts)

