import re

token_re = re.compile(r'([(][*]|[*][)]|\'|"|[\\]|//|\r\n|\n\r|\n)')
endline_re = re.compile(r'(\r\n|\n\r|\n)')

(NO_MORE_DELIMS,
 OPEN_DELIM,
 CLOSE_DELIM,
 LINECOMMENT,
 STRING_DELIM) = range(5)

(STATE_COMMENT,
 STATE_SOURCE,
 STATE_STRING) = range(3)


class ParseError(Exception): pass


class Tokenizer(object):
  def __init__(self, text, start=0, line=1):
    self.text = text
    self.start = start
    self.token = None
    self.line = line

  def next(self):
    if self.start < len(self.text):
      text = self.text
      while True:
        match_obj = token_re.search(text, self.start)
        if match_obj:
          self.start = end = match_obj.end()
          n = start = match_obj.start()
          self.token = (start, end)

          if text[n] in "\n\r":
            self.line += 1

          elif text[n] == "(":
            return OPEN_DELIM

          elif text[n] == "*":
            return CLOSE_DELIM

          elif text[n] == "\\":
            # Skip the next character
            self.start += 1

          elif text[n] == "/":
            return LINECOMMENT

          else:
            return STRING_DELIM

        else:
          break

    self.token = (len(text), len(text))
    return NO_MORE_DELIMS 

  def skip_nl(self):
    match_obj = endline_re.search(self.text, self.start)
    if match_obj:
      self.start = match_obj.end()

    else:
      self.start = len(self.text)
    return self.start


class TextSlice(object):
  def __init__(self, begin, lineno, end=None, parent=None):
    self.begin = begin
    self.end = end
    self.lineno = lineno
    self.parent = parent


class Parser(object):
  '''Base parser for Box sources. This parser can go over the sources and
  distinguish comments from source code. This is the basic parser over which
  the Boxer macro system is built. The parser calls several method to notify
  the occurrence of several events. The user is expected to inherit from this
  class and redefine the methods: notify_comment_begin, notify_comment_end,
  notify_inline_comment and notify_source.
  '''

  def __init__(self, text):
    self.text = text

  def notify_comment_begin(self, text_slice):
    pass

  def notify_comment_end(self, text_slice):
    pass

  def notify_inline_comment(self, text_slice):
    pass

  def notify_source(self, start, end):
    pass

  def parse(self, out_text=None, out_comment=None):
    text = self.text
    tok = Tokenizer(text)

    state = STATE_SOURCE
    source_pos = 0
    comments = []

    while True:
      token = tok.next()
      token_start, token_end = tok.token

      if state == STATE_SOURCE:
        if token == NO_MORE_DELIMS:
          self.notify_source(source_pos, token_start)
          return

        elif token == OPEN_DELIM:
          self.notify_source(source_pos, token_start)
          text_slice = TextSlice(token_start, tok.line)
          comments.append(text_slice)
          self.notify_comment_begin(text_slice)
          state = STATE_COMMENT

        elif token == LINECOMMENT:
          self.notify_source(source_pos, token_start)
          text_slice = TextSlice(token_start, tok.line)
          text_slice.end = source_pos = tok.skip_nl()
          self.notify_inline_comment(text_slice)

        elif token == STRING_DELIM:
          state = STATE_STRING

        else:
          assert token == CLOSE_DELIM
          raise ParseError("Line %d: Closing comment outside comment"
                           % tok.line)

      elif state == STATE_COMMENT:
        if token == NO_MORE_DELIMS:
          lineno = comments[-1].lineno
          raise ParseError("Reached end of file inside a comment "
                           "(comment opened at line %d)" % lineno)

        elif token == OPEN_DELIM:
          text_slice = TextSlice(token_start, tok.line,
                                 parent=comments[-1])
          self.notify_comment_begin(text_slice)
          comments.append(text_slice)

        elif token == CLOSE_DELIM:
          comment = comments.pop()
          comment.end = token_end
          self.notify_comment_end(comment)
          if len(comments) == 0:
            state = STATE_SOURCE
            source_pos = token_end

      elif state == STATE_STRING:
        if token == STRING_DELIM:
          state = STATE_SOURCE


if __name__ == "__main__":
  text = """
one line
(**)
a second line and "(* and \\"*)", a fake comment delimiter
(*comment*)
a third line
(*one(*comment*)and(*one*)more*)
a fourth line
"""

  def uncomment(in_text):
    out_text = []
    def fn(start, end, level=None, inline=None):
      out_text.append(text[start:end])
    p = Parser(in_text)
    p.notify_source = fn
    p.parse(out_text=fn)
    return "".join(out_text)

  print uncomment(text)

