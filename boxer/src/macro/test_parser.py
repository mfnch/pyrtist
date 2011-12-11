from parser import Parser, SourceMapper, MacroExpander

def test_parser_uncomment():
  text_in = """
one line
(**)
a second line and "(* and \\"*)", a fake comment delimiter
(*comment*)
a third line
(*one(*comment*)and(*one*)more*)
a fourth line
"""

  text_expected = """
one line

a second line and "(* and \\"*)", a fake comment delimiter

a third line

a fourth line
"""

  def uncomment(text_in):
    out_text = []
    def fn(start, end, level=None, inline=None):
      out_text.append(text_in[start:end])
    p = Parser(text_in)
    p.notify_source = fn
    p.parse(out_text=fn)
    return "".join(out_text)

  text_out = uncomment(text_in)
  assert text_out == text_expected, \
    ("Uncommenter using Parser does not work. Input is {%s} output is "
     "{%s}, but the text above was expected: {%s}." 
     % (text_in, text_out, text_expected))

def test_source_mapper():
  text_in = \
"""one line
(**)
a second line and "(* and \\"*)", a fake comment delimiter
(*comment*)
bla //inline (*two*) comment
a third line
(*one(*comment*)and(*one*)more*)
a fourth line"""

  text_expected = text_in

  mp = SourceMapper(text_in)
  mp.parse()
  text_out = mp.get_output()
  assert text_out == text_expected, \
    ("Error in SourceMapper.")

def test_macro_expander():
  text_in = \
"""
(*Program written by (**author.*)*)
one line
(**)
a second line and "(* and \\"*)", a fake comment delimiter
(*comment*)
bla //inline (**author.*) comment
a third line written by (**author.*)
(*one(*comment*)and(*one*)more*)
a fourth line"""

  text_expected = """
(*Program written by Matteo Franchin*)
one line
(**)
a second line and "(* and \\"*)", a fake comment delimiter
(*comment*)
bla //inline (**author.*) comment
a third line written by Matteo Franchin
(*one(*comment*)and(*one*)more*)
a fourth line"""

  class MyExpander(MacroExpander):
    def macro_author(self, args):
      return "Matteo Franchin"
    
  mx = MyExpander(text_in)
  mx.parse()
  text_out = mx.get_output()
  assert text_out == text_expected, \
    ("Error in macro expander. Input {%s}, output {%s}, but the following "
     "was expected {%s}." % (text_in, text_out, text_expected ))

if __name__ == "__main__":
  test_parser_uncomment()
  test_source_mapper()
  test_macro_expander()
