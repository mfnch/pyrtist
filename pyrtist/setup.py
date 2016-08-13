# Copyright (C) 2012-2016 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

import os
from distutils.core import setup

name = 'pyrtist'
version = '0.0.1'
description = 'A GUI which enables drawing using Python scripts'
author = 'Matteo Franchin'
author_email = 'fnch@users.sf.net'
url = 'http://boxc.sourceforge.net/',
download_url = ('http://sourceforge.net/project/'
                'platformdownload.php?group_id=218051')
license = 'GPL'
packages = ['pyrtist', 'pyrtist.gui', 'pyrtist.gui.dox',
            'pyrtist.gui.comparse', 'pyrtist.lib2d']
package_dir = {'pyrtist': 'src'}

examples = []

examples_fullpaths = list(os.path.join('examples', bn)
                          for bn in examples)

package_data = {'pyrtist': (examples_fullpaths +
                            ['icons/24x24/*.png',
                             'icons/32x32/*.png',
                             'icons/fonts/*.png'])}
script = 'scripts/pyrtist'
scripts = [script]

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
