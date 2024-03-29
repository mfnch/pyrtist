#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(-1.0, 1.0); bbox2 = Point(1.0, -1.0); center1 = Point(0.0, 0.0)
p1 = Point(0.1381917585406386, 0.329695651405736)
#!PYRTIST:REFPOINTS:END
# This is a minimal example showing how pyrtist.opengl can be used to create
# a png from a GLSL (OpenGL) shader. It is typically possible (and pretty
# simple) to use pyrtist.opengl to reproduce a shader from ShaderToy and work
# on it from Pyrtist, although not all ShaderToy features are currently
# supported.
from pyrtist.opengl import *

w = FragmentWindow(bbox1, bbox2, position_name='frag_pos')

w << '''
in vec2 frag_pos;

out vec4 out_color;

vec2 tx(vec2 p) {
  return (p - bbox2) / (bbox1 - bbox2);
}

void main() {
  vec2 p = tx(frag_pos);

  vec4 col = vec4(p, 0.0, 1.0);

  if (length(p - tx(p1)) < 0.1)
    col += vec4(0.0, 0.0, 0.5, 0.0);

  if (min(p.x, p.y) < 0.0 || max(p.x, p.y) > 1.0)
    col = vec4(0.0, 0.0, 0.0, 1.0);

  out_color = col;
}
'''

w.set_vars(locals())
w.draw(gui)

#w.save('simplest.png')
