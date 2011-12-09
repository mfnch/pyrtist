import re

from mapper import SourceMapper

macro_name_re = re.compile(r'([(][*][*]|///)[a-zA-Z_-]+[:.]')

class MacroExpander(SourceMapper):
  '''A macro expander class. Macros are special multi-line comments which are
  delimited by the three-character delimiter (** rather than the normal
  two-character comment delimiter (*. The macros have the form
  (**macroname:arguments*) or simply (**macroname*), with no spaces in it. If
  there are spaces, or if the comment does not follow the form as above, then
  it is treated as a regular comment and is not processed by the MacroExpander
  class. This means that the comment goes as it is to the output.
  The final output can be retrieved using the method get_output. For example::

    me = MacroExpander(source)
    me.parse()
    output = me.get_output()

  A macro (**macroname:arguments*) is processed by calling the method
  ``macro_macroname`` of the MacroExpander class, if it exists.  The string
  "arguments" is passed to the method and the return value, which must be a
  string, is used to expand the macro. 

  If ``macro_macroname`` does not exist, then the method ``not_found`` is
  called, instead. Two arguments are passed to it. The first is the macro name,
  the second is the macro argument string.

  Macros returning None are not expanded.

  Expansions are done recursively: the deepest macros are expanded first.
  '''

  def not_found(self, name, args):
    '''Called when a macro is not found (may be overridden).'''
    print "Unknown macro '%s'" % name
    return None

  def invoke_method(self, name, args):
    '''Method called to execute the macros. As a default this method, tries to
    call a method with name "macro_" + name, passing the given arguments, or
    calls the method not_found, if such a macro does not exist.
    '''
    method_name = "macro_" + name
    method = getattr(self, method_name, None)

    if method != None:
      return method(args)

    else:
      return self.not_found(name, args)

  def subst_comment(self, content):
    '''Macro expander.'''
    if content.startswith("(**") or content.startswith("///"):
      match_obj = re.match(macro_name_re, content)
      if match_obj:
        i0 = match_obj.start() + 3
        i2 = match_obj.end()
        i1 = i2 - 1
        name = content[i0:i1].lower().replace("-", "_")
        args = content[i2:-2]
        repl = self.invoke_method(name, args)
        return repl if repl != None else content

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
