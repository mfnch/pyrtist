import re

match_code = "``([^`]|`[^`])*``"
match_italic = "\*[^\*]+\*"
match_bold = "\*\*([^\*]|\*[^\*])*\*"

_rst_code_re = re.compile(match_code)
#_rst__re = re.compile("(%s|%s)" % (match_code, match_italic))

def parse_code(s, out=None):
  code_fragments = []
  for match in _rst_code_re.finditer(s):
    start = match.start()
    end = match.end()
    code_fragments.append((start, end, s[start+2:end-2]))

  content = (out if out != None else [])
  i = 0
  for code_fragment in code_fragments:
    start, end, code = code_fragment
    content.append(s[i:start])
    content.append((code, "code"))
    i = end
  content.append(s[i:])
  return content

def parse(content):
  out_content = []
  for piece in content:
    if type(piece) == str:
      parse_code(piece, out_content)
    else:
      out_content.append(piece)
  return out_content

if __name__ == "__main__":
  s = "here is some code ``code`` and here is **some bold text** and *some italic*. That's it!"
  print(parse_code(s))
