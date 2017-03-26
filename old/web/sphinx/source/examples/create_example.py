import sys, os, os.path, commands, re
from deps import find_deps

usage = "USAGE: python create_example.py box.example"

if len(sys.argv) != 2:
  raise ValueError("Expected one argument.\n" + usage)

example_file = sys.argv[1]
print "Working on '%s'..." % example_file

# Default values for variables which may be changed inside example_file
in_directory = ".."
box = "~/local/bin/box -l g"
convert = "convert"
convert_opts = ""
highlight = "%s/../../../katehighlight/bin/highlight" % in_directory
rst_skeleton = "skeleton"
rst_out = None
title = None
description = None
figure_caption = None
box_source = None
out_eps = None
out_png = None

_f = open(example_file)
exec(_f)
_f.close()

if title == None:
  title = "Box example: %s" % crumb

print "Removing old figure if present..."
if out_eps and os.access(out_eps, os.W_OK):
  try:
    os.remove(out_eps)
  except:
    print "Failed to remove the figure: continuing anyway..."

print "Executing the Box program..."
print commands.getoutput("%s %s" % (box, box_source))

have_figure = False
if out_eps and os.access(out_eps, os.R_OK):
  print "Adjusting eps figure..."
  out_png = os.path.splitext(out_eps)[0] + ".png"
  print commands.getoutput("%s %s %s %s" %
                           (convert, convert_opts, out_eps, out_png))

print out_png
have_figure = os.access(out_png, os.R_OK)

if not have_figure:
  raise ValueError("The figure '%s' has not been produced: stopping here!"
                   % out_png)

print "Highlighting the Box source..."
highlighted_source = "/tmp/h.html"
print commands.getoutput("%s Box %s %s" % (highlight, box_source, highlighted_source))
f = open(highlighted_source, "r")
htmlized_box_program = f.read()
f.close()

print "Opening the skeleton..."
f = open(rst_skeleton, "r")
data_skeleton = f.read()
f.close()

deps = find_deps([box_source])
deps_files = ", ".join(deps)
deps_links = ", ".join(["`%s <%s>`__" % (f, f) for f in deps])

vars_dict = {
  'title': title,
  'description': description,
  'crumb': crumb,
  'box_file':box_source,
  'box_source':box_source,
  'figure_caption':figure_caption,
  'image': out_png,
  'htmlized_box_program': htmlized_box_program,
  'deps_files': deps_files,
  'deps_links': deps_links
}

r = re.compile("[$][^$]*[$]")
def substitutor(var):
  try:
    var_name = var.group(0)[1:-1]
  except:
    raise ValueError("Error when substituting variable.")
  if var_name in vars_dict:
    return str(vars_dict[var_name])
  else:
    first_char = var_name[0]
    if first_char.isspace():
      var_name2 = var_name.lstrip()
      prespaces = var_name[:-len(var_name2)]
      if var_name2 in vars_dict:
        text = vars_dict[var_name2]
        return "".join([prespaces + l for l in text.splitlines(True)])
    elif not first_char.isalpha() and var_name[1:] in vars_dict:
      return first_char*len(str(vars_dict[var_name[1:]]))
  print "WARNING: Variable '%s' not found!" % var_name
  return var.group(0)

print "Filling the skeleton..."
out = re.sub(r, substitutor, data_skeleton)
f = open(rst_out, "w")
f.write(out)
f.close()

print "Output produced (%s)" % rst_out

print "Generating thumbnail..."
html_out = os.path.splitext(out_png)[0] + ".html"
out_thumb_png = "small_" + out_png
scale_opts = ("-scale 100x100^ "
              "-background white -flatten -alpha off "
              "-bordercolor black -border 1")
print commands.getoutput("%s %s %s %s"
                         % (convert, out_png, scale_opts, out_thumb_png))

f = open("thumbnails.dat", "a")
f.write("%s, %s\n" % (html_out, out_thumb_png))
f.close()
