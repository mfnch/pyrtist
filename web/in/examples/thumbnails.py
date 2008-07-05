#!/usr/bin/python

example_order = ["snake", "cycloid", "wheatstone", "multivibrator",
                 "yin-yang", "curved_arrow", "box-logo", "translucency",
                 "gradient", "fractree"]

import sys
import subst

f = open("thumbnails.dat")
ls = f.readlines()
f.close()

m = len(example_order)
items = {}
for l in ls:
  rs = l.split(",")
  html_file, thumb_file = rs[0:2]
  n = -1
  for i in range(len(example_order)):
    if example_order[i] in html_file:
      n = i
      break
  if n < 0:
    n = m
    m += 1
  items[n] = thumb_file.strip(), html_file.strip()

cols = 5

html = '<table border="3" cellspacing="1" cellpadding="1">\n'
row_started = False
for i in items:
  thumb_file, html_file = items[i]
  if i % cols == 0:
    if row_started: html += "</tr>\n"
    html += "<tr>\n"
    row_started = True
  img = '<a href="%s"><img src="%s" hspace="1"></a>' % (html_file, thumb_file)
  html += '<td align = "center">%s</td>\n' % img

if row_started: html += "</tr>\n"
html += "</table>"

html = '<div align="center">\n%s\n</div>' % html

variables = {'thumbnails_table':html,
             'example_order': ", ".join(example_order)}
subst.file_subst(variables, 'index.txt.in', 'index.txt')