import re

from mapper import SourceMapper

macro_name_re = re.compile(r'([(][*][*]|///)[a-zA-Z_]+[:.]')

class MacroExpander(SourceMapper):
  '''A macro expander class. Parser a Box sources for multi-line comments
  having the form (**macroname:args*) and call the method function
  'macro_macroname' passing to it the string 'args'.
  If this method does not exist, then the method not_found is called and
  the string 'args' is given as an argument. By default this method displays
  a message (to stdout) warning that the macro was not found.
  This class forms the basis for the Boxer macro system.
  '''

  def not_found(self, name, args):
    print "Unknown macro '%s'" % name

  def subst_comment(self, content):
    '''Macro expander.'''
    if content.startswith("(**") or content.startswith("///"):
      match_obj = re.match(macro_name_re, content)
      if match_obj:
        i0 = match_obj.start() + 3
        i2 = match_obj.end()
        i1 = i2 - 1
        name = content[i0:i1].lower()
        method_name = "macro_" + name
        if hasattr(self, method_name):
          method = getattr(self, method_name)
          return method(content[i2:])

        else:
          self.not_found(name, args)

    return content


if __name__ == "__main__":
  text = \
"""(*Program written by (**author.*)*)
one line
(**)
a second line and "(* and \\"*)", a fake comment delimiter
(*comment*)
bla //inline (**author.*) comment
a third line
(*one(*comment*)and(*one*)more*)
a fourth line"""

  class MyExpander(MacroExpander):
    def macro_author(self, args):
      return "Matteo Franchin"
    
  mx = MyExpander(text)
  print "Parsing..."
  mx.parse()
  print "Output below:"
  print mx.get_output()
