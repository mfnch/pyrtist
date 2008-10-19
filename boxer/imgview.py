"""
In this file we define the class ImgView, used to display the resulting image
produced by a Box program and to handle the reference points.
"""

class ImgView:
  """Class which allows to display the image produced by Box and the reference
  points.
  """

  def __init__(self, drawable):
    self.img = drawable
    self.bbox = None

  def set_from_file(self, filename, bbox):
    """Load the image from file with associated bounding box 'bbox'."""
    self.img.set_from_file(filename)
    self.bbox = bbox

  def get_size(self):
    """Return a couple of two reals: the width and height of the image
    view.
    """
    return (self.img.allocation.width, self.img.allocation.height)

  def map_coord_to_box(self, py_coord):
    """Map coordinates of the mouse on the GTK widget to Box coordinates."""

    if self.bbox == None:
      return None

    py_x, py_y = py_coord
    py_sx, py_sy = self.get_size()

    ((box_min_x, box_min_y), (box_max_x, box_max_y)) = self.bbox
    x = box_min_x + (box_max_x - box_min_x)*py_x/py_sx
    y = box_max_y + (box_min_y - box_max_y)*py_y/py_sy
    return (x, y)

  def map_coord_from_box(self, box_coord):
    """Map Box coordinates to on the GTK widget."""

    if self.bbox == None:
      return None

    ((box_min_x, box_min_y), (box_max_x, box_max_y)) = self.bbox
    box_x, box_y = box_coord
    py_sx, py_sy = self.get_size()
    x = py_sx*(box_x - box_min_x)/(box_max_x - box_min_x)
    y = py_sy*(box_y - box_min_y)/(box_max_y - box_min_y)
    return (x, y)

  def ref_point_draw(self, coords):
    """Draw the reference point markers over the image."""
    pixbuf = self.img.get_pixbuf()
    x, y = coords
    l = 3
    x0, y0 = (max(0, int(x) - l), max(0, int(y) - l))
    subbuf = pixbuf.subpixbuf(x0, y0, 2*l, 2*l)
    subbuf.fill(0x000000ff)

    l1 = l - 1
    x1, y1 = (max(0, int(x) - l1), max(0, int(y) - l1))
    subbuf = pixbuf.subpixbuf(x1, y1, 2*l1, 2*l1)
    subbuf.fill(0xffffffff)

    self.img.queue_draw_area(x0, y0, 2*l, 2*l)
    #debug()
