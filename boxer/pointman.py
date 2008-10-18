import math

def round_metric(p1, p2):
  """Returns the common euclidean distance between two 2D points p1 and p2."""
  return math.sqrt((p2[0] - p1[0])**2 + (p2[1] - p1[1])**2)

def square_metric(p1, p2):
  """Square metric: the maximum of the absolute values of the coordinates
  of the point p2 - p1.
  When square_metric(p1, p2) <= x, the two 2D points p1, p2 lie inside
  boundaries of a square of side x."""
  return max(abs(p2[0] - p1[0]), abs(p2[1] - p1[1]))

class RefPoint:
  def __init__(self, point, name):
    self.point = point
    self.name = name

class RefPointManager:
  def __init__(self, radius=None, base_name="p"):
    self.radius = radius
    self.base_name = base_name

    self.points = []
    self.names = {}
    self.index = 0
    self.metric = round_metric
    self.sep_newline = "\n"
    self.sep_comma = ", "
    self.code_max_row = 79
    self.comment_line = lambda line: (("// %s" % line) + self.sep_newline)

  def distance(self, p1, p2):
    """Returns the distance between the object and the given point,
    using the currently selected metric.
    """
    return self.metric(p1, p2)

  def auto_name(self, name=None):
    """If name is None or is not given, create automatically a name starting
    from the 'base_name' giving during class initialisation.
    Example: if 'base_name="p"', then returns p1, or p2 (if p1 has been already
    used, etc.)"""
    if name != None: return name
    self.index += 1
    return self.base_name + str(self.index)

  def add(self, point, name=None):
    """Add a new reference point whose coordinates are 'point' (a couple
    of floats) and the name is 'name'. If 'name' is not given, then a name
    is generated automatically using the 'base_name' given during construction
    of the class.
    RETURN the name finally chosen for the point.
    """
    x, y = point
    p = (float(x), float(y))
    real_name = self.auto_name(name)
    if self.names.has_key(real_name):
      raise ("Trying to add a point with name '%s': "
             "this point already exists!" % real_name)
    self.names[real_name] = len(self.points)
    self.points.append(RefPoint(p, real_name))
    return real_name

  def nearest(self, point):
    """Find the ref. point which is nearest to the given point.
    Returns a couple containing made of the reference point and the distance.
    """
    current_d = None
    current = None
    for p in self.points:
      d = self.distance(point, p)
      if current_d == None or d <= current_d:
        current = (p, d)
    return current

  def set_radius(self, radius):
    """Set the radius used to decide whether the method 'pick' should
    pick a point or not."""
    self.radius = radius

  def pick(self, point):
    """Returns the ref. point closest to the given one (argument 'point').
    If the distance between the two is greater than the current 'radius'
    (see set_radius), return None."""
    current = self.nearest(point)
    #if radius == None: return current
    if current == None or current[0] > self.radius: return None
    return current

  def code_gen(self):
    """Generate Box code to set the ref. points."""
    line_and_whole = ["", ""]
    def add_to_line(piece):
      l = len(line_and_whole[0])
      if l == 0:
        line_and_whole[0] = piece
      elif l + len(piece) > self.code_max_row:
        line_and_whole[1] += line_and_whole[0] + self.sep_newline
        line_and_whole[0] = piece
      else:
        line_and_whole[0] += self.sep_comma + piece

    for p in self.points:
      piece = "%s = (%s, %s)" % (p.name, p.point[0], p.point[1])
      add_to_line(piece)

    line_and_whole[1] += line_and_whole[0] + self.sep_newline
    return line_and_whole[1]

  def __parse_piece(self, line, callback):
    left, right = line.split("=", 1)
    name = left.strip()
    left, remainder = right.split(")", 1)
    left = left.strip()
    if left[0] != "(":
      raise "Error parsing reference point assignment."
    left = left[1:]
    left, right = left.split(",")
    point = (float(left), float(right))
    callback(point, name)
    remainder = remainder.strip()
    if len(remainder) > 0 and remainder[0] == ",":
      remainder = remainder[1:]
    return remainder

  def code_parse(self, code, callback):
    """Parse the Box code containing the reference point assignment.
    For each point call callback with (point, name) as argument,
    where point is a tuple of two float and name is the name of the point.
    """
    for line in code.splitlines():
      remainder = line
      while len(remainder.strip()) > 0:
        remainder = self.__parse_piece(remainder, callback)

  def add_from_code(self, code):
    def callback(point, name):
      self.add(point, name)
    self.code_parse(code, callback)

if __name__ == "__main__":
  rp = RefPointManager()
  rp.add((3, 5))
  rp.add((4, 6))
  rp.add((5, 7))
  rp.add((6, 8))
  rp.add((3, 5))
  rp.add((4, 6))
  rp.add((5, 7))
  rp.add((6, 8))
  rp.add((3, 5))
  rp.add((4, 6))
  rp.add((5, 7))
  rp.add((6, 8))

  code = rp.code_gen()
  print "OUT:"
  print code

  print rp.add_from_code(code)

  print "OUT:"
  print rp.code_gen()
