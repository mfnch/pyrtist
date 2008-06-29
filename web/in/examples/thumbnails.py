#!/usr/bin/python

import sys
import subst

f = open("thumbnails.dat")
ls = f.readlines()
f.close()

num_items = 0
items = {}
for l in ls:
  rs = l.split(",")
  html_file, thumb_file = rs[0:2]
  items[num_items] = thumb_file.strip(), html_file.strip()
  num_items += 1

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

subst.file_subst({'thumbnails_table':html}, 'index.txt.in', 'index.txt')
