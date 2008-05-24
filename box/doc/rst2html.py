#!/usr/bin/python
"""
rst2html

A minimal front end to the Docutils Publisher, producing HTML,
with an extension for colouring code-blocks

Taken from http://lukeplant.me.uk/blog.php?id=1107301665
and modified to support Box syntax highlighting
(coming from modified package under ../../web/katehighlight)
Matteo Franchin, 24 May 2008
"""

katehighlight_bin = "../../web/katehighlight/bin/highlight"

try:
    import locale
    locale.setlocale(locale.LC_ALL, '')
except:
    pass


from docutils import nodes, parsers
from docutils.parsers.rst import states, directives
from docutils.core import publish_cmdline, default_description

import tempfile, os

def getCommandOutput2(command):
    child_stdin, child_stdout = os.popen2(command)
    child_stdin.close()
    data = child_stdout.read()
    err = child_stdout.close()
    if err:
        raise RuntimeError, '%s failed w/ exit code %d' % (command, err)
    return data

def highlight_haskell(text):
    fh, path = tempfile.mkstemp()
    os.write(fh, text)
    output = getCommandOutput2(["HsColour", "-css", "-partial", path])
    os.close(fh)
    return output

def get_katehighlight_highliter(language):
    def highlighter(text):
        src_fh, src = tempfile.mkstemp()
        dst = tempfile.mktemp()
        os.write(src_fh, text)
        cmnd_output = getCommandOutput2([katehighlight_bin, language, src, dst])
        os.close(src_fh)
        dst_fh = open(dst, "r")
        output = dst_fh.read()
        dst_fh.close()
        os.unlink(src)
        os.unlink(dst)
        return output
    return highlighter

def get_highlighter(language):
    if language == 'haskell':
        return highlight_haskell

    elif language == 'box':
        return get_katehighlight_highliter("Box")

    try:
        from pygments import lexers, util, highlight, formatters
        import StringIO

    except:
        return get_katehighlight_highliter(language)

    try:
        lexer = lexers.get_lexer_by_name(language)
    except util.ClassNotFound:
        return None

    formatter = formatters.get_formatter_by_name('html')
    def _highlighter(code):
        outfile = StringIO.StringIO()
        highlight(code, lexer, formatter, outfile)
        return outfile.getvalue()
    return _highlighter

# Docutils directives:
def code_block(name, arguments, options, content, lineno,
               content_offset, block_text, state, state_machine):
    """
    The code-block directive provides syntax highlighting for blocks
    of code.  It is used with the the following syntax::

    .. code-block:: python

       import sys
       def main():
           sys.stdout.write("Hello world")

    Currently support languages: python (requires pygments),
    haskell (requires HsColour), anything else supported by pygments
    """


    language = arguments[0]
    highlighter = get_highlighter(language)
    if highlighter is None:
        error = state_machine.reporter.error(
            'The "%s" directive does not support language "%s".' % (name, language),
            nodes.literal_block(block_text, block_text), line=lineno)

    if not content:
        error = state_machine.reporter.error(
            'The "%s" block is empty; content required.' % (name),
            nodes.literal_block(block_text, block_text), line=lineno)
        return [error]

    include_text = highlighter("\n".join(content))
    html = '<div class="codeblock %s">\n%s\n</div>\n' % (language, include_text)
    raw = nodes.raw('',html, format='html')
    return [raw]

code_block.arguments = (1,0,0)
code_block.options = {'language' : parsers.rst.directives.unchanged }
code_block.content = 1

# Register
directives.register_directive( 'code-block', code_block )


description = ('Generates (X)HTML documents from standalone reStructuredText '
               'sources.  ' + default_description)

# Command line
publish_cmdline(writer_name='html', description=description)
