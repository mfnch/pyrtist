import re

r = re.compile("[$][^$]*[$]")

def subst(variables, data_in):
  def substitutor(var):
    try:
      var_name = var.group(0)[1:-1]
    except:
      raise "Error when substituting variable."
    if variables.has_key(var_name):
      return str(variables[var_name])
    print "WARNING: Variable '%s' not found!" % var_name
    return var.group(0)
  return re.sub(r, substitutor, data_in)

def file_subst(variables, file_in, file_out):
  f = open(file_in, "r")
  data_in = f.read()
  f.close()
  f = open(file_out, "w")
  f.write(subst(variables, data_in))
  f.close()
