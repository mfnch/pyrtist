# Copyright (C) 2011 Matteo Franchin (fnch@users.sourceforge.net)
#
# This file is part of Boxer.
#
#   Boxer is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Boxer is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Boxer. If not, see <http://www.gnu.org/licenses/>.

import re

token_re = re.compile(r'([(][*]|[*][)]|\'|"|[\\]|//|\r\n|\n\r|\n)')
endline_re = re.compile(r'(\r\n|\n\r|\n)')

macro_name_re = re.compile(r'([(][*][*]|///)[a-zA-Z_-]+[:.]')

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


class SourceMapper(Parser):
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

  def notify_comment_end(self, text_slice):
    # First, take care of substituting the children
    output = ""
    text_pos = text_slice.begin
    for child in text_slice.children:
      output += self.text[text_pos:child.begin] + child.output
      text_pos = child.end
    output += self.text[text_pos:text_slice.end]

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
    self.notify_comment_end(text_slice)

  def notify_source(self, start, end):
    self.output += self.subst_source(self.text[start:end])


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
