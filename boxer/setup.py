from distutils.core import setup

# Try to use py2exe, if available (on Windows)
try:
  import py2exe
except:
  pass

setup(name='boxer',
      version='0.1',
      description=('A graphical user interface for '
                   'the Box vector graphics language'),
      author='Matteo Franchin',
      author_email='fnch@users.sourceforge.net',
      url='http://boxc.sourceforge.net/',
      download_url=('http://sourceforge.net/project/'
                    'platformdownload.php?group_id=218051'),
      license='GPL',
      packages=['boxer'],
      package_dir={'boxer': 'src'},
      package_data={'boxer': ['glade/*']},
      scripts=['scripts/boxer'],
      #data_files=[('share/boxer', ['src/boxer.glade'])]
      )
