#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(-1.0, 1.0); bbox2 = Point(1.0, -1.0)
p1 = Point(0.005915725440309938, 0.4292796620151452)
p2 = Point(-0.6037344681589227, 0.3224207560221355)
p3 = Point(-0.058935600414610656, 0.7036301462273848)
p4 = Point(0.3811384502210118, 0.39619572957356775)
p5 = Point(-0.07356610214501103, 0.0004188136050577995)
p6 = Point(-0.12794320625171318, 0.37342386078416256)
#!PYRTIST:REFPOINTS:END
# Simple example to show how to use textures with pyrtist.opengl.
# The example generates an image using Pyrtist's 2D vector graphics library
# (pyrtist.lib2d), it then loads the texture in OpenGL and uses it in a shader.

from pyrtist.lib2d import *

w = Window()
w << BBox(bbox1, bbox2)

s = Border(0.02, Color.black, Dash(0.05))
w << Line(s, p2, p3, p4, p5, Close.yes, Cap.round)
w << Text(p6, "Hello\nworld!", Font(0.1), Offset(0.5, 0.5),
          Color(1, 0.5, 0))
w.save('texture.png', resolution=1000)


from pyrtist.opengl import *

w = FragmentWindow(bbox1, bbox2,
                   position_name='frag_pos',
                   allow_unset_uniforms=False)

noise = Texture()
noise.set_content_from_file('texture.png')
noise.set(wrap_x=Texture.Wrap.REPEAT,
          wrap_y=Texture.Wrap.REPEAT,
          mag_filter=Texture.Filter.LINEAR,
          min_filter=Texture.Filter.LINEAR)
w.set_uniforms(noise=noise)

w << '''
in vec2 frag_pos;

out vec4 out_color;

uniform sampler2D noise;

vec2 tx(vec2 p) {
  return (p - bbox2) / (bbox1 - bbox2);
}

void main() {
  vec2 p = tx(frag_pos);
  vec4 col = vec4(p, 0.0, 1.0);

  if (min(p.x, p.y) < 0.0 || max(p.x, p.y) > 1.0)
    col = vec4(0.0, 0.0, 0.0, 1.0);

  vec4 tex_col = texture(noise, 0.5 * frag_pos + 0.5);
  float d = smoothstep(0.1, 0.3, length(p - tx(p1)));

  out_color = mix(tex_col, col, d);
}
'''

w.set_vars(locals())
w.draw(gui)

#w.save('simplest.png')
