# Copyright (C) 2010-2011 Matteo Franchin (fnch@users.sourceforge.net)
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

import sys
import os
import time

import config
from config import Configurable
from exec_command import run_script
from refpoints import RefPoint, RefPoints

# This is the version of the document (which may be different from the version
# of Boxer which was used to write it. We should save to file all this info,
# but we currently do not (have to improve this!)
version = (0, 2, 0)

max_chars_per_line = 79
marker_cursor_here = "#!PYRTIST:CURSOR:HERE"

#from os import linesep as endline
endline = "\n"

def cmp_version(v1, v2):
  """Compare two version tuples."""
  return cmp(v1[0], v2[0]) or cmp(v1[1], v2[1]) or cmp(v1[2], v2[2])

def text_writer(pieces, sep=", ", line_sep=None, max_line_width=None):
  """Similarly to str.join, this function concatenates the strings contained
  in the list ``pieces`` separating them with the separator ``sep``.
  However, the width of the line is always kept below ``max_line_width``,
  if possible. This means that when a line would exceed this limit, a new line
  is started.
  NOTE: each element of pieces is filtered through the function str() before
    being used.
  """

  mlw = max_line_width
  if mlw == None:
    mlw = max_chars_per_line
  lsep = line_sep
  if lsep == None:
    lsep = endline
  text = ""

  this_line = ""
  cur_sep = ""
  for piece in pieces:
    p = str(cur_sep) + str(piece)
    lp = len(p)
    lts = len(this_line)
    if lts == 0 or lts + lp <= mlw:
      this_line += p
      cur_sep = sep

    else:
      text += this_line + lsep
      this_line = str(piece)
      cur_sep = sep

  if len(this_line) > 0:
    text += this_line + lsep
  return text

# Enumerate the different possible ways of building a representation
# for a Document:
# - MODE_ORIG: original, as given by the user;
# - MODE_STORE: as it should be when saving it to disk;
# - MODE_EXEC: as it should be when executed within the GUI.
(MODE_ORIG,
 MODE_STORE,
 MODE_EXEC) = range(3)


class DocumentBase(Configurable):
  """Class to load, save, display and edit Boxer documents.
  A Boxer document is basically a Box program plus some metainfo contained
  in comments."""

  accepted_versions = []         # Versions accepted by the Document class

  def __init__(self, filename=None, callbacks=None,
               config=None, from_args=None):
    """filename=None :the filename associated to the document."""
    super(DocumentBase, self).__init__(config=config, from_args=from_args)

    self.filename = None         # File name corresponding to this document
    self.version = None          # Version of the file format
    self.preamble = None         # Preamble of the document
    self.usercode = None         # Body of the document

    if callbacks == None:
      callbacks = {}
    callbacks.setdefault("box_document_execute", None)
    callbacks.setdefault("box_document_executed", None)
    callbacks.setdefault("box_exec_output", None)

    self._fns = callbacks
    self.refpoints = RefPoints(callbacks=self._fns)

  def notify(self, t, msg):
    '''Used to notify problems during load/save.'''
    sys.stdout.write("%s: %s\n" % (t, msg))

  def new(self, preamble, refpoints=[], code='\n'):
    self.refpoints.load(refpoints)
    self.preamble = preamble
    self.usercode = code
    self.version = version

  def get_refpoints(self):
    '''Get a list of the reference points in the document (list of GUIPoint)'''
    return self.refpoints

  def get_boot_code(self, preamble=None):
    return preamble if preamble is not None else self.preamble

  def set_boot_code(self, boot_code):
    """Set the content of the additional Box code used by Boxer."""
    self.preamble = boot_code

  def set_user_code(self, code):
    """Set the content of the user Box code."""
    self.usercode = code

  def get_part_version(self):
    """Produce the version line."""
    raise NotImplementedError("get_part_version")

  def get_part_boot_code(self, boot_code):
    """Produce the boot code (auxiliary code used to communicate the output
    back to the GUI).
    """
    return (boot_code or '')

  def get_part_def_refpoints(self):
    """Produce the part of the file which stores information about the
    reference points.
    """
    raise NotImplementedError()

  def get_part_preamble(self, mode=None, boot_code=None):
    """Return the first part of the Box file, where meta-information such as
    the version of the file format and the reference points are defined.
    """
    raise NotImplementedError()

  def get_part_user_code(self, mode=None):
    """Get the user part of the Box source."""
    return self.usercode

  def new(self):
    '''Start working on a new document (un-named).'''
    self.filename = None

  def load_from_str(self, boxer_src):
    raise NotImplementedError("DocumentBase.load_from_str")

  def load_from_file(self, filename, remember_filename=True):
    f = open(filename, "r")
    self.load_from_str(f.read())
    f.close()

    if remember_filename:
      self.filename = filename

  def save_to_file(self, filename, version=version, remember_filename=True):
    with open(filename, "w") as f:
      f.write(self.save_to_str(version=version))

    if remember_filename:
      self.filename = filename

  def box_query(self, variable):
    raise NotImplementedError("This function is obsolete")

  def execute(self, preamble=None, callback=None):
    fn = self._fns["box_document_execute"]
    if fn is not None:
      fn(self)

    presrc_content = self.get_part_preamble(mode=MODE_EXEC,
                                            boot_code=preamble)
    original_userspace = self.get_part_user_code(mode=MODE_EXEC)
    src = presrc_content + '\n' + original_userspace

    # If the Box source is saved (rather than being a temporary unsaved
    # script) then execute it from its parent directory. Also, make sure to
    # add the same directory among the include directories. This allows
    # including scripts in the same directory.
    cwd = None
    src_name = '<New file>'
    if self.filename is not None:
      cwd = os.path.split(self.filename)[0]
      # Put the file name between angle brackets to prevent the backtrace
      # formatter from trying to load the file when showing where the error
      # is. This would not work as the script we are executing is not exactly
      # the same as the one which sits on the disk.
      src_name = '<{}>'.format(self.filename)

    fn = self._fns['box_document_executed']
    def my_callback(name, *args):
      if name == 'exit':
        if fn is not None:
          fn(self)
        if callback:
          callback(name, *args)
      else:
        cb = self._fns.get(name)
        if cb is not None:
          cb(*args)

    return run_script(src_name, src, callback=my_callback, cwd=cwd)


if __name__ == "__main__":
  import sys
  d = Document()
  d.load_from_file(sys.argv[1])
  sys.stdout.write("Executing...\n")
  raw_input()

  def out_fn(s):
    sys.stdout.write(s)

  d.execute(out_fn=out_fn)
  sys.stdout.write("Done\n")
  import time
  time.sleep(0.1)
