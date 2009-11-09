
def strip_nonalpha(s, default=""):
  """Strip non alphabetic characters at the beginning of the string
  and return the result."""
  ss = s.strip()
  i = 0
  for c in ss:
    if c.isalpha():
      return ss[i:]
    i += 1
  return default

def strip_chars(s, keep_fn):
  """Strip characters in string for which keep_fn(c) == False."""
  out = ""
  for c in s:
    if keep_fn(c):
      out += c
  return out

def adjust_name(name, default="p"):
  """Adjust the given name so that it can really be a box variable name."""
  s = strip_nonalpha(name, default=default)
  if not s[0].islower():
    s = s[0].lower() + s[1:]
  s = strip_chars(s, lambda c: c.isalnum() or c == '_')
  if s == "":
    return default
  else:
    return s

def get_last_num(name):
  """Get the initial and final character index (as a tuple (initial, final))
  which correspond to the last integer number embedded in the string.
  Example: get_last_num("abc12_df34_123bbb") --> (11, 13)"""
  was_in_num = False
  i = 0
  start, end = (None, None)
  for c in name:
    is_in_num = c.isdigit()
    if is_in_num != was_in_num:
      if is_in_num:
        start = i
        end = len(name) + 1
      else:
        end = i 
    was_in_num = is_in_num
    i += 1
  return (start, end)
  
def generate_next_num(old_num, increment=1):
    l = len(old_num)
    nn = str(int(old_num) + increment)
    if len(nn) >= l:
        return nn
    else:
        return nn.rjust(l, "0")

def generate_next_name(old_name, increment=1):
  """Given the name of the previous point, generate a new name, by incrementing
  the last integer number embedded in the name.
  Examples: "p1" --> "p2" "abc12ef34gh" --> abc12ef35gh""."""
  n = adjust_name(old_name)
  start, end = get_last_num(n)
  if start == None:
    return n + "1"
  else:
    new_num = generate_next_num(n[start:end], increment=increment)
    return n[:start] + new_num + n[end:]

if __name__ == "__main__":
  while True:
    n = adjust_name(raw_input())
    print n, generate_next_name(n)
