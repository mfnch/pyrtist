import sys, re, os.path

in_file_name = sys.argv[1]
subst_file_name = sys.argv[2]
out_file_name, _ = os.path.splitext(subst_file_name) 

f = open(in_file_name, "r")
code_in = f.read()
f.close()

code_blocks = {}

r = re.compile("//[{][^}]*[}][ \t]*[\n]")
def substitutor(var):
  code_block = var.group(0)
  try:
    code_lines = code_block.splitlines()
    name = (code_lines[0][3:]).strip()
    print "Found code-block '%s'" % name
    code_block = "".join(code_block.splitlines(True)[1:-1])
    code_blocks[name] = code_block

  except:
    print "Warning: cannot get the piece of code."
  return code_block

print "Finding code blocks..."
code_blocks["all"] = re.sub(r, substitutor, code_in)

print "Opening file '%s' for code-block substitution" % subst_file_name
f = open(subst_file_name, "r")
src = f.read()
f.close()

def indent(text, indentation="  "):
  ls = text.splitlines(True)
  return indentation + indentation.join(ls)

print "Writing output to '%s'" % out_file_name
def substitutor2(var):
  code_block = var.group(0)
  try:
    name = code_block
    lstripped_name = name.lstrip()
    indentation = name[0:len(name)-len(lstripped_name)]
    name = (code_block.strip()[3:-1]).strip()
    print "Substituting code-block '%s'" % name
    return indent(code_blocks[name], indentation)

  except:
    raise "Error: cannot get the code-block!"

r2 = re.compile("[ \t]*//[{][^}]*[}][ \t]*[\n]")
dest = re.sub(r2, substitutor2, src)
f = open(out_file_name, "w")
f.write(dest)
f.close()

