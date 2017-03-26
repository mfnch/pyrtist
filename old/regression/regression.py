import os
from commands import getstatusoutput
import glob

(MSG_INFO,
 MSG_WARNING,
 MSG_ERROR,
 MSG_SUCCESS) = range(4)

COLOR_RED = '\033[00;31m'
COLOR_GREEN = '\033[00;32m'
COLOR_YELLOW = '\033[00;33m'
COLOR_NORM = '\033[0m'

colors = \
  {MSG_INFO: COLOR_NORM,
   MSG_WARNING: COLOR_YELLOW,
   MSG_ERROR: COLOR_RED,
   MSG_SUCCESS: COLOR_GREEN}

def log_msg(level, msg):
  print(colors[level] + str(msg) + COLOR_NORM)

def mkdir_p(path):
  '''Recursively create directories.'''
  if not os.path.exists(path):
    parentdir, childdir = os.path.split(path)
    if 0 < len(parentdir) < len(path):
      mkdir_p(parentdir)
    os.mkdir(path)

def find_files(path, ffilter=None, relative=False):
  files = []
  def callback(_, dirname, fnames):
    full_fnames = \
      (fnames if relative
       else map(lambda fname: os.path.join(dirname, fname), fnames))
    ffiles = (filter(ffilter, full_fnames) if full_fnames != None
              else full_fnames)
    files.extend(ffiles)
  os.path.walk(path, callback, None)
  return files


class RegressionTestRunner(object):

  def __init__(self,
               test_root_path="..",
               regression_path="RegressionData",
               output_path="OutputData",
               source_pattern="test_*.box",
               excluded_fnames=[],
               box_exec="box"):

    self.test_root_path = test_root_path
    self.regression_path = regression_path
    self.output_path = output_path
    self.source_pattern = source_pattern
    self.box_exec = box_exec
    self.excluded_fnames = excluded_fnames
    self.output_file = "__stdout_stderr.txt"

  def _run_test(self, dirname, fname):
    fpath = os.path.realpath(os.path.join(self.test_root_path, dirname, fname))
    out_dir = os.path.join(self.output_path, dirname, fname)
    reg_dir = os.path.join(self.regression_path, dirname, fname)
    assert not os.path.exists(out_dir), \
      ("Output directory '%s' exists. You should make sure that '%s' is "
       "empty before launching the regression test."
       % (out_dir, self.output_path))

    if not os.path.exists(reg_dir):
      log_msg(MSG_WARNING, "Directory '%s' does not exist: creating "
              "reference data..." % reg_dir)
      mkdir_p(reg_dir)

      exit_status = self._run_box_source(fpath, reg_dir)
      if exit_status == 0:
        self._check_is_not_empty(reg_dir)

      else:
        log_msg(MSG_ERROR, "Failure when executing '%s'. Exit status %s."
                % (fpath, exit_status))

    else:
      mkdir_p(out_dir)
      exit_status = self._run_box_source(fpath, out_dir)
      if exit_status == 0:
        self._check_files_do_match(reg_dir, out_dir)

      else:
        log_msg(MSG_ERROR, "Failure when executing '%s'. Exit status %s."
                % (fpath, exit_status))

  def _check_is_not_empty(self, path):
    '''Check that the provided directory is not empty.'''
    if len(find_files(path)) == 1:
      log_msg(MSG_WARNING, "Directory '%s' is empty after executing the test"
              % path)

  def _check_file_pair(self, new_file, reference_file):
    '''Check that two files are ``similar enough''.'''
    ext = os.path.splitext(reference_file)[1]
    if ext in  ('.png', '.jpg'):
      exit_status, output = \
        getstatusoutput('compare -fuzz 1\% -metric AE '
                        + '"%s" "%s" /dev/null' % (new_file, reference_file))
      return (exit_status == 0 and int(output) == 0)
    else:
      exit_status = os.system('diff "%s" "%s" >/dev/null 2>&1'
                              % (new_file, reference_file))
      return (exit_status == 0)

  def _check_files_do_match(self, reg_dir, out_dir):
    '''Check that files in the output directory 'out_dir' match with those
    stored in 'reg_dir'.'''
    success = True
    reg_files = find_files(reg_dir, relative=True)
    out_files = find_files(out_dir, relative=True)
    for reg_file in reg_files:
      rf = os.path.join(reg_dir, reg_file)
      of = os.path.join(out_dir, reg_file)

      if reg_file in out_files:
        out_files.remove(reg_file)
        if not self._check_file_pair(of, rf) != 0:
          log_msg(MSG_ERROR, "File '%s' not matching regression '%s'"
                  % (of, rf))
          success = False

      else:
        log_msg(MSG_ERROR, "Missing file '%s'" % of)
        success = False

    if success:
      log_msg(MSG_SUCCESS, "Successful regression for '%s'" % reg_dir)

  def _run_box_source(self, box_src, out_dir):
    '''Run the box sources in the given directory.'''
    cwd = os.getcwd()
    os.chdir(out_dir)
    src_path = os.path.split(box_src)[0]
    cmd = ('%s -l g -I "%s" %s > "%s" 2>&1'
           % (self.box_exec, src_path, box_src, self.output_file))
    exit_status = os.system(cmd)
    os.chdir(cwd)
    return exit_status

  def _run_tests_in_subdir(self, paths, dirname, fnames):
    '''Run all the tests inside a given subdirectory. Meant to be called
    internally (by self.run).'''
    assert dirname.startswith(self.test_root_path)
    relative_dirname = dirname[len(self.test_root_path):].lstrip(os.path.sep)
    for fname in fnames:
      if (glob.fnmatch.fnmatch(fname, self.source_pattern)
          and os.path.isfile(os.path.join(dirname, fname))
          and fname not in self.excluded_fnames):
        self._run_test(relative_dirname, fname)
    
  def run(self):
    '''Run the regression tests.'''
    os.path.walk(self.test_root_path, self._run_tests_in_subdir, [])


if __name__ == '__main__':
  import shutil
  import sys

  if len(sys.argv) != 2:
    print "Usage: python regression.py path/to/box/executable"
    sys.exit(1)

  # Path to the Box executable
  box_exec = sys.argv[1]

  # Repository collecting regression data
  regression_repos = "https://bitbucket.org/franchin/boxer-regression"

  # Directory where the regression data is located
  rddir = "RegressionData"
  if not os.path.isdir(rddir):
    print ("Cannot find RegressionData directory. Please, check it out with:\n"
           "\n  hg clone %s %s\n\n"
           "(this command should be given from the directory containing this"
           "script)." % (regression_repos, rddir))
    sys.exit(1)

  # The directory which will contain the OutputData
  oddir = "OutputData"
  if os.path.isdir(oddir):
    shutil.rmtree(oddir)

  # Web page tests
  rtr = RegressionTestRunner(test_root_path='../web/sphinx/source',
                             source_pattern="*.box",
                             regression_path="RegressionData/web",
                             output_path=oddir,
                             box_exec=box_exec)
  rtr.run()

  # Box compiler tests
  rtr = RegressionTestRunner(test_root_path='../box',
                             source_pattern="test_*.box",
                             excluded_fnames=["test_mylib.box"],
                             regression_path="RegressionData/box",
                             output_path=oddir,
                             box_exec=box_exec)
  rtr.run()

  # Boxer compiler tests
  rtr = RegressionTestRunner(test_root_path='../boxer/src/icons',
                             source_pattern="*.box",
                             excluded_fnames=["fontall.box"],
                             regression_path="RegressionData/boxer",
                             output_path=oddir,
                             box_exec=box_exec)
  rtr.run()
