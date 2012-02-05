'''This module defines some objects which allows to quickly define
and use a new menu. Example of usage::

  def menu_file_new(*args):
    print "Activated with", args

  mb = \
    MenuBar(MenuItem("_File",
                     MenuStock(gtk.STOCK_NEW).with_name("new"),
                     MenuStock(gtk.STOCK_OPEN).with_name("open")
                     ).with_name("file"),
            MenuItem("_Edit",
                     MenuStock(gtk.STOCK_UNDO).with_name("undo"),
                     MenuStock(gtk.STOCK_REDO).with_name("redo")
                     ).with_name("edit"))
  gtk_object = mb.instantiate()
  mb.file.new.instance.connect("activate", menu_file_new)

'''

import gtk

def _add_subobjs_as_attrs(obj):
  for item in obj.items:
    if item.name is not None:
      _add_subobjs_as_attrs(item)
      setattr(obj, item.name, item)


class MenuObject(object):
  num_args = 0

  def __init__(self, *items):
    n = self.num_args
    self.args = items[:n]
    self.items = map(self._get_default, items[n:])
    self.name = None
    self.instance = None
    _add_subobjs_as_attrs(self)

  def add(self, instance, items):
    for item in items:
      instance.add(item)

  def set_name(self, name):
    self.name = name

  def with_name(self, name):
    self.set_name(name)
    return self

  def _get_default(self, x):
    if type(x) == tuple:
      x_name, x_val = x
      x_val_final = self._get_default(x_val)
      x_val_final.set_name(x_name)
      return x_val_final

    else:
      return x

  def instantiate(self):
    if self.instance is not None:
      return self.instance

    else:
      instance = self.constructor(*self.args)
      items = []
      for item in self.items:
        items.append(item.instantiate())
      self.add(instance, items)
      self.instance = instance
      return instance

class MenuBar(MenuObject):
  constructor = gtk.MenuBar

class MenuSep(MenuObject):
  constructor = gtk.SeparatorMenuItem

class MenuItem(MenuObject):
  constructor = gtk.MenuItem
  num_args = 1

  def _get_default(self, x):
    if x is None:
      x = MenuSep()
    elif type(x) == str:
      x = MenuItem(x)
    return MenuObject._get_default(self, x)

  def add(self, instance, items):
    if len(items) > 0:
      m = gtk.Menu()
      for item in items:
        m.add(item)
      instance.set_submenu(m)

class MenuStock(MenuObject):
  constructor = gtk.ImageMenuItem
  num_args = 1
