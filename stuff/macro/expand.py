from macro import MacroExpander

class BoxerMacroExpand(MacroExpander):
  def macro_define_all(self, args):
    s = "p1 = Point[.x=0.5, .y=0.2]"
    return "(**expand:define-all*)\n%s\n(**end-expand.*)" % s


(STATE_NORMAL,
 STATE_EXPANDING) = range(2)


class BoxerMacroContract(MacroExpander):
  def __init__(self, *args):
    MacroExpander.__init__(self, *args)
    self.state = STATE_NORMAL

  def notify_error(self, msg):
    print msg

  def invoke_method(self, name, args):
    if self.state == STATE_EXPANDING:
      self.state = STATE_NORMAL
      if name != "end_expand":
        self.notify_error("Error: expecting end-expand macro.")
      return ""

    else:
      return MacroExpander.invoke_method(self, name, args)

  def subst_source(self, content):
    if self.state != STATE_EXPANDING:
      return content
    else:
      return ""

  def macro_expand(self, args):
    self.state = STATE_EXPANDING
    return "(**%s.*)" % args


if __name__ == "__main__":
  text = \
"""
(* Program written by Me Indeed *)

(**define-all.*)

(**define-all.*)

"""
  print "Original below:"
  print text

  mx1 = BoxerMacroExpand(text)
  print "Parsing..."
  mx1.parse()
  print "Output below:"
  expanded = mx1.get_output()
  print expanded

  mx2 = BoxerMacroContract(expanded)
  print "Parsing..."
  mx2.parse()
  print "Output below:"
  contracted = mx2.get_output()
  print contracted
