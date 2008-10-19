"""
In this file we define the class ImgView, used to display the resulting image
produced by a Box program and to handle the reference points.
"""

def debug():
  import sys
  from IPython.Shell import IPShellEmbed
  calling_frame = sys._getframe(1)
  IPShellEmbed([])(local_ns  = calling_frame.f_locals,
                   global_ns = calling_frame.f_globals)

class ImgViewRefpoint:
  def __init__(self):
    self.coords = None
    self.box_coords = None # not supposed to be changed manually...
    self.background = None # ... use the method ImgView.ref_point_coords_set

class ImgView:
  """Class which allows to display the image produced by Box and the reference
  points.
  """

  def __init__(self, drawable, ref_point_size=3):
    self.img = drawable
    self.bbox = None
    self.ref_point = {}
    self.ref_point_size = ref_point_size

  def set_from_file(self, filename, bbox):
    """Load the image from file with associated bounding box 'bbox'."""
    self.ref_point_hide_all()
    self.img.set_from_file(filename)
    self.bbox = bbox
    # The py_coords changed when loading the new image file,
    # therefore here we need to convert back from box_coords
    for _, ref_point in self.ref_point.items():
      ref_point.coords = self.map_coords_from_box(ref_point.box_coords)
    self.ref_point_show_all()

  def get_size(self):
    """Return a couple of two reals: the width and height of the image
    view.
    """
    return (self.img.allocation.width, self.img.allocation.height)

  def map_coords_to_box(self, py_coords):
    """Map coordinates of the mouse on the GTK widget to Box coordinates."""

    if self.bbox == None:
      return None

    py_x, py_y = py_coords
    py_sx, py_sy = self.get_size()

    ((box_min_x, box_min_y), (box_max_x, box_max_y)) = self.bbox
    x = box_min_x + (box_max_x - box_min_x)*py_x/py_sx
    y = box_max_y + (box_min_y - box_max_y)*py_y/py_sy
    return (x, y)

  def map_coords_from_box(self, box_coords):
    """Map Box coordinates to on the GTK widget."""

    if self.bbox == None:
      return None

    ((box_min_x, box_min_y), (box_max_x, box_max_y)) = self.bbox
    box_x, box_y = box_coords
    py_sx, py_sy = self.get_size()
    x = py_sx*(box_x - box_min_x)/(box_max_x - box_min_x)
    y = py_sy*(box_max_y - box_y)/(box_max_y - box_min_y)
    return (x, y)

  def ref_point_coords_set(self, ref_point, coords):
    """Set the coordinates for the ImgViewRefpoint object 'p'."""
    ref_point.coords = coords
    ref_point.box_coords = self.map_coords_to_box(coords)

  def __cut_point(self, x, y):
    mx, my = self.get_size()
    return (max(0, min(mx-1, x)), max(0, min(my-1, y)))

  def __cut_square(self, x, y, dx, dy):
    x, y = (int(x), int(y))
    x0, y0 = self.__cut_point(x, y)
    x1, y1 = self.__cut_point(x+dx, y+dy)
    sx, sy = (x1 - x0, y1 - y0)
    if sx < 1 or sy < 1: return (x0, y0, -1, -1)
    return (x0, y0, sx+1, sy+1)

  def ref_point_draw(self, coords, size, return_background=True):
    x, y = coords
    l0 = size
    dl0 = l0*2
    x0, y0, dx0, dy0 = self.__cut_square(x-l0, y-l0, dl0, dl0)
    if dx0 < 1 or dy0 < 1: return None

    pixbuf = self.img.get_pixbuf()
    subbuf = pixbuf.subpixbuf(x0, y0, dx0, dy0)

    # Before filling, save the background, if necessary
    background = None
    if return_background:
      background = (dx0, dy0, x0, y0, subbuf.copy())

    subbuf.fill(0x000000ff) # Fill it!

    l1 = l0 - 1
    dl1 = 2*l1
    x1, y1, dx1, dy1 = self.__cut_square(x-l1, y-l1, dl1, dl1)
    if dx1 > 0 and dy1 > 0:
      subbuf = pixbuf.subpixbuf(x1, y1, dx1, dy1)
      subbuf.fill(0xffffffff)

    self.img.queue_draw_area(x0, y0, dx0, dy0)
    return background

  def restore_background(self, background):
    if background == None: return
    sx, sy, x, y, pixbuf = background
    dest_pixbuf = self.img.get_pixbuf()
    pixbuf.copy_area(0, 0, sx, sy, dest_pixbuf, x, y)
    self.img.queue_draw_area(x, y, sx, sy)

  def ref_point_new(self, name, coords):
    """Creates a new reference point."""
    if self.ref_point.has_key(name):
      raise "A reference point with the same name exists (%s)" % name

    background = self.ref_point_draw(coords, self.ref_point_size)
    box_coords = self.map_coords_to_box(coords)
    box_coords = self.map_coords_to_box(coords)
    ref_point = ImgViewRefpoint()
    ref_point.background = background
    self.ref_point_coords_set(ref_point, coords)
    self.ref_point[name] = ref_point

  def ref_point_move(self, name, coords):
    ref_point = self.ref_point[name]
    self.restore_background(ref_point.background)
    self.ref_point_coords_set(ref_point, coords)
    ref_point.background = self.ref_point_draw(coords, self.ref_point_size)

  def ref_point_del(self, name):
    ref_point = self.ref_point[name]
    self.restore_background(ref_point.background)
    self.ref_point.pop(name)

  def ref_point_del_all(self):
    for name in self.ref_point:
      self.ref_point_del(name)

  def ref_point_hide_all(self):
    for _, ref_point in self.ref_point.items():
      self.restore_background(ref_point.background)
      ref_point.background = None

  def ref_point_show_all(self):
    for _, ref_point in self.ref_point.items():
      self.restore_background(ref_point.background)
      ref_point.background = self.ref_point_draw(ref_point.coords,
                                                 self.ref_point_size)
