import os
import sys
import shutil

def file_is_older(fa, fb):
  return os.path.getmtime(fa) < os.path.getmtime(fb)

for f in sys.argv[1:]:
  execfile(f)
  copied_box_source = os.path.basename(box_source)
  if (not os.path.exists(copied_box_source)
      or file_is_older(copied_box_source, box_source)):
    print "Copying %s --> %s" % (box_source, copied_box_source)
    shutil.copyfile(box_source, copied_box_source)

