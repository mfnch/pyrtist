#!/usr/bin/python

import sys
import subst

suffix = sys.argv[1]
example_order = sys.argv[2:]


def create_substs(example_order=None, suffix="", substs=None):
  if substs == None:
    substs = {}
  if example_order == None:
    example_order = []

  thumbnails_dat = "%s/thumbnails.dat" % suffix
  var_defs = "defs" + suffix
  var_images = "images" + suffix

  f = open(thumbnails_dat)
  ls = f.readlines()
  f.close()

  m = len(example_order)
  items = {}
  for l in ls:
    rs = l.split(",")
    html_file, thumb_file = rs[0:2]
    n = -1
    for i in range(len(example_order)):
      if example_order[i]+".html" == html_file:
        print example_order[i], html_file
        n = i
        break
    if n < 0:
      n = m
      m += 1
    items[n] = thumb_file.strip(), html_file.strip()

  cols = 5

  table = []
  defs = []
  row_started = False

  for i in range(len(items)):
    img_file, img_link = items[i]

    img_name = "img%s%d" % (suffix, i)
    table.append("|%s|" % img_name)
    defs.append(".. |%s| image:: %s/%s\n" % (img_name, suffix, img_file) +
                "   :target: %s/%s\n" % (suffix, img_link))

  images = " ".join(table)
  defs = "".join(defs)

  substs[var_images] = images
  substs[var_defs] = defs
  return substs

def main(args, infile, outfile):
  args = list(args)
  variables = {}
  while len(args) > 0:
    suffix = args.pop(0)
    example_order = args.pop(0).split(",")
    variables = create_substs(example_order, suffix, variables)

  subst.file_subst(variables, infile, outfile)

if __name__ == "__main__":
  import sys
  main(sys.argv[1:], infile='gallery.rst.in', outfile='gallery.rst')
