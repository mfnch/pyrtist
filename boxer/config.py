import sys

box_syntax_highlighting = sys.path[0]

# By default this is the text put inside a new program
# created with File->New
box_source_of_new = \
"""w = Window[][
  .Show[(0, 0), (100, 50)]
]

GUI[w]
"""

class Config:
  """Class to store global configuration settings."""

  def __init__(self):
    self.default = {"font": "Monospace 10"}

  def get_default(self, name, default=None):
    """Get a default configuration setting."""
    if self.default.has_key(name):
      return self.default[name]
    else:
      return default