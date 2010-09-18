from distutils.core import setup

name = 'boxer'
version = '0.2.0'
description = ('A graphical user interface for '
               'the Box vector graphics language')
author = 'Matteo Franchin'
author_email = 'fnch@users.sourceforge.net'
url = 'http://boxc.sourceforge.net/',
download_url = ('http://sourceforge.net/project/'
                'platformdownload.php?group_id=218051')
license = 'GPL'
packages = ['boxer']
package_dir = {'boxer': 'src'}
package_data = {'boxer': ['glade/*', 'examples/*']}
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
        #data_files=[('share/boxer', ['src/boxer.glade'])]
