import os

system_deps = ["g", "arrows", "electric"]

def _find_deps(filename, deps=None, exts=[".box", ".bxh"]):
  if deps == None:
    deps = []

  if not os.path.exists(filename):
    for ext in exts:
      new_filename = filename + ext
      if os.path.exists(new_filename):
        filename = new_filename
        break

  #print "Inspecting '%s'" % filename
  f = open(filename, "r")
  ls = f.read().splitlines()
  f.close()

  deps.append(filename)
  for l in ls:
    ll = l.lower().strip()
    if ll.startswith("include"):
      dep = ll[8:].strip()[1:-1]
      if not dep in system_deps and not dep in deps:
        _find_deps(dep, deps)
  return deps

def find_deps(filenames, exts=[".box", ".bxh"]):
  deps = []
  for filename in filenames:
    _find_deps(filename, deps, exts=exts)
  return deps

if __name__ == "__main__":
  import sys
  for dep in find_deps(sys.argv[1:]):
    print dep

