from parser import Parser


class Subst(Parser):
  '''This class is built on top of the Parser class and provides a simplified
  way to filter comments and source in a Box file. In particular, this class
  takes care of recursive substitution/expansion of comments.
  The user should inherit from this class and redefine the methods subst_source
  and subst_comment. These functions receive the source or comments and are
  expected to return a string to use for replacing the original content.
  '''

  def __init__(self, source):
    Parser.__init__(self, source)
    self.output = ""

  def get_output(self):
      return self.output

  def subst_source(self, content):
    '''Method used to substitute uncommented Box source.'''
    return content

  def subst_comment(self, content):
    '''Method used to substitute Box comments.'''
    return content

  def notify_comment_begin(self, text_slice):
    text_slice.children = []

  def notify_comment_end(self, text_slice, begin="(*", end="*)"):
    # First, take care of substituting the children
    output = begin
    text_pos = text_slice.begin
    for child in text_slice.children:
      output += self.text[text_pos:child.begin] + child.output
      text_pos = child.end
    output += self.text[text_pos:text_slice.end] + end

    # Now we can substitute this comment
    text_slice.output = self.subst_comment(output)

    # We now detach the children (helps with circular refs)
    text_slice.children = []

    # We also add it to the list of the children of its parent...
    if text_slice.parent != None:
      text_slice.parent.children.append(text_slice)

    else:
      # ... or, if this is a level-1 comment, we just append its output
      # to the source.
      self.output += text_slice.output

  def notify_inline_comment(self, text_slice):
    self.notify_comment_begin(text_slice)
    self.notify_comment_end(text_slice, begin="", end="")

  def notify_source(self, start, end):
    self.output += self.text[start:end]


if __name__ == "__main__":
  text = \
"""one line
(**)
a second line and "(* and \\"*)", a fake comment delimiter
(*comment*)
bla //inline (*two*) comment
a third line
(*one(*comment*)and(*one*)more*)
a fourth line"""

  mp = Subst(text)
  mp.parse()
  print mp.get_output()
