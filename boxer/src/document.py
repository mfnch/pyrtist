
endline = "\n"

def get_marker(line):
  """Extract the arguments of a marker line or return None if the given
  string is not a marker line."""
  if not line.lstrip().startswith("//!BOXER"):
    return None
  else:
    return line.split(":")[1:]

def parse_given_eq_smthg(s, fixed_part):
  """Parse the given string 's' assuming it has the form
  "fixed_part = something" where fixed_part must match the second argument
  received by the function. Return 'something' if this is the case, otherwise
  return None."""
  left, right = s.split("=", 1)
  if left.strip() == fixed_part:
    return right.strip()
  else:
    return None

def parse_guipoint_new(s):
  """Parse a string representation of a GUIPoint and return the corresponding
  GUIPoint object."""
  try:
    lhs, rhs = s.split("=", 1)
    rhs, s_next = rhs.split("]", 1)
    point_str, rem_str = rhs.split("[", 1)
    if point_str.strip() != "Point":
      return None
    str_x, str_y = rem_str.split(",", 1)
    x = float(parse_given_eq_smthg(str_x, ".x"))
    y = float(parse_given_eq_smthg(str_y, ".y"))
    return (lhs.strip(), [x, y])

  except:
    return None

def parse_guipoint_v0_1_0(s):
  """Similar to parse_guipoint_new, but for Boxer 0.1."""
  try:
    lhs, rhs = s.split("=", 1)
    rem_str, _ = rhs.split(")", 1)
    rem_str, x_y_str = rem_str.split("(", 1)
    if rem_str.strip() != "":
      return None
    str_x, str_y = x_y_str.split(",", 1)
    return (lhs.strip(), [float(str_x), float(str_y)])
  except:
    return None

def parse_guipoint(s):
  """Try to parse a representation of a GUIPoint first with parse_guipoint_new
  and then with parse_guipoint_v0_1_0 if the former failed."""
  guipoint = parse_guipoint_new(s)
  if guipoint != None:
    return guipoint
  else:
    return parse_guipoint_v0_1_0(s)

class GUIPoint:
  def __init__(self, id, value=None):
    if value != None:
      self.id = id
      self.value = value
    else:
      self.id, self.value = parse_guipoint(id)

  def __repr__(self):
    return "%s = Point[.x = %f, .y = %f]" \
           % (self.id, self.value[0], self.value[1])
 
def default_notifier(level, msg):
  print "%s: %s" % (level, msg)

def search_first(s, things, start=0):
  found = -1
  for thing in things:
    i = s.find(thing, start)
    if found == -1:
      found = i
    elif i != -1 and i < found:
      found = i
  return found

def match_close(src, start):
  open_bracket = src[start]
  close_bracket = {"(":")", "[":"]"}[open_bracket]
  while True:
    next = search_first(src, [open_bracket, close_bracket], start + 1)
    if next == -1:
      raise "Missing closing parethesis!"
    if src[next] == open_bracket:
      start = match_close(src, next) + 1
    else:
      return next + 1

def get_next_guipoint_string(src, start=0):
  pos = start
  while True:
    next = search_first(src, [",", endline, "(", "["], pos)
    if next < 0:
      # no comma nor parenthesis: we are parsing the last bit
      return (src[start:], len(src))

    c = src[next]
    if c == "," or c == endline :
      # comma comes before next open parenthesis
      return (src[start:next], next + 1)
    elif c == "(":
      pos = match_close(src, next)

def parse_guipoint_part(src, start=0):
  while start < len(src):
    line, start = get_next_guipoint_string(src, start)
    print GUIPoint(line)

class Document:
  def __init__(self, filename=None):
    self.attributes = {}
    self.notifier = default_notifier
    
    if filename != None:
      self.load_from_file(filename)

  def _get_arg_num(self, arg, possible_args):
    i = 0
    for possible_arg in possible_args:
      if possible_arg == arg:
        return i
      i += 1
    self.notify("WARNING", "unrecognized marker argument '%s'" % arg)
    return None
    
  def load_from_src(self, boxer_src):
    parts = {}           # different text parts of the source
    context = "preamble" # initial context/part

    # default attributes
    self.attributes["version"] = (0, 1, 0)

    # specify how many arguments each marker wants
    marker_wants = {"REFPOINTS": 1,
                    "VERSION": 3}

    # process the file and interpret the markers
    lines = boxer_src.splitlines()
    for line in lines:
      marker = get_marker(line)
      if marker == None:
        if parts.has_key(context):
          parts[context] += "\n" + line
        else:
          parts[context] = line

      else:
        if len(marker) < 1:
          raise "Internal error in Document.load_from_src"

        else:
          marker_name = marker[0]
          if not marker_wants.has_key(marker_name):
            self.notify("WARNING", "Unknown marker '%s'" % marker_name)

          else:
            if len(marker) < marker_wants[marker_name] + 1:
              self.notify("WARNING", 
                          "Marker has less arguments than expected")

            elif marker_name == "REFPOINTS":
              arg_num = self._get_arg_num(marker[1], ["BEGIN", "END"])
              if arg_num == None:
                return False
              else:
                context = ["refpoints", "userspace"][arg_num]

            elif marker_name == "VERSION":
              try:
                self.attributes["version"] = \
                  (int(marker[1]), int(marker[2]), int(marker[3]))
              except:
                self.notify("WARNING", "Cannot determine Boxer version which "
                            "generated the file")

    print parts
    parse_guipoint_part(parts["refpoints"])
    return True
      

  def load_from_file(self, filename):
    f = open(filename, "r")
    self.load_from_src(f.read())
    f.close()

d = Document("experiment.box")



