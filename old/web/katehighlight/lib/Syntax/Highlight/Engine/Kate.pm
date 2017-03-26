
# Copyright (c) 2006 Hans Jeuken. All rights reserved.
# This program is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.

package Syntax::Highlight::Engine::Kate;

use vars qw($VERSION);
$VERSION = '0.04';
use strict;
use warnings;
use Carp;
use Data::Dumper;
use File::Basename;

use base('Syntax::Highlight::Engine::Kate::Template');

sub new {
	my $proto = shift;
	my $class = ref($proto) || $proto;
	my %args = (@_);

	my $add = delete $args{'plugins'};
	unless (defined($add)) { $add = [] };
	my $language = delete $args{'language'};
	unless (defined($language)) { $language = 'Off' };

	my $self = $class->SUPER::new(%args);

	$self->{'plugins'} = {};
	#begin autoinsert
	$self->{'extensions'} = {
		' *.cls' => ['LaTeX', ],
		' *.dtx' => ['LaTeX', ],
		' *.ltx' => ['LaTeX', ],
		' *.sty' => ['LaTeX', ],
		'*.4GL' => ['4GL', ],
		'*.4gl' => ['4GL', ],
		'*.ABC' => ['ABC', ],
		'*.ASM' => ['AVR Assembler', 'PicAsm', ],
		'*.BAS' => ['FreeBASIC', ],
		'*.BI' => ['FreeBASIC', ],
    '*.BOX' => ['Box', ],
    '*.BXH' => ['Box', ],
		'*.C' => ['C++', 'C', 'ANSI C89', ],
		'*.D' => ['D', ],
		'*.F' => ['Fortran', ],
		'*.F90' => ['Fortran', ],
		'*.F95' => ['Fortran', ],
		'*.FOR' => ['Fortran', ],
		'*.FPP' => ['Fortran', ],
		'*.GDL' => ['GDL', ],
		'*.H' => ['C++', ],
		'*.JSP' => ['JSP', ],
		'*.LOGO' => ['de_DE', 'en_US', 'nl', ],
		'*.LY' => ['LilyPond', ],
		'*.Logo' => ['de_DE', 'en_US', 'nl', ],
		'*.M' => ['Matlab', 'Octave', ],
		'*.MAB' => ['MAB-DB', ],
		'*.Mab' => ['MAB-DB', ],
		'*.PER' => ['4GL-PER', ],
		'*.PIC' => ['PicAsm', ],
		'*.PRG' => ['xHarbour', 'Clipper', ],
		'*.R' => ['R Script', ],
		'*.S' => ['GNU Assembler', ],
		'*.SQL' => ['SQL', 'SQL (MySQL)', 'SQL (PostgreSQL)', ],
		'*.SRC' => ['PicAsm', ],
		'*.V' => ['Verilog', ],
		'*.VCG' => ['GDL', ],
		'*.a' => ['Ada', ],
		'*.abc' => ['ABC', ],
		'*.ada' => ['Ada', ],
		'*.adb' => ['Ada', ],
		'*.ado' => ['Stata', ],
		'*.ads' => ['Ada', ],
		'*.ahdl' => ['AHDL', ],
		'*.ai' => ['PostScript', ],
		'*.ans' => ['Ansys', ],
		'*.asm' => ['AVR Assembler', 'Asm6502', 'Intel x86 (NASM)', 'PicAsm', ],
		'*.asm-avr' => ['AVR Assembler', ],
		'*.asp' => ['ASP', ],
		'*.awk' => ['AWK', ],
		'*.bas' => ['FreeBASIC', ],
		'*.basetest' => ['BaseTest', ],
		'*.bash' => ['Bash', ],
		'*.bi' => ['FreeBASIC', ],
		'*.bib' => ['BibTeX', ],
    '*.box' => ['Box', ],
    '*.bxh' => ['Box', ],
		'*.bro' => ['Component-Pascal', ],
		'*.c' => ['C', 'ANSI C89', 'LPC', ],
		'*.c++' => ['C++', ],
		'*.cc' => ['C++', ],
		'*.cfc' => ['ColdFusion', ],
		'*.cfg' => ['Quake Script', ],
		'*.cfm' => ['ColdFusion', ],
		'*.cfml' => ['ColdFusion', ],
		'*.cg' => ['Cg', ],
		'*.cgis' => ['CGiS', ],
		'*.ch' => ['xHarbour', 'Clipper', ],
		'*.cis' => ['Cisco', ],
		'*.cl' => ['Common Lisp', ],
		'*.cmake' => ['CMake', ],
		'*.config' => ['Logtalk', ],
		'*.cp' => ['Component-Pascal', ],
		'*.cpp' => ['C++', ],
		'*.cs' => ['C#', ],
		'*.css' => ['CSS', ],
		'*.cue' => ['CUE Sheet', ],
		'*.cxx' => ['C++', ],
		'*.d' => ['D', ],
		'*.daml' => ['XML', ],
		'*.dbm' => ['ColdFusion', ],
		'*.def' => ['Modula-2', ],
		'*.desktop' => ['.desktop', ],
		'*.diff' => ['Diff', ],
		'*.do' => ['Stata', ],
		'*.docbook' => ['XML', ],
		'*.dox' => ['Doxygen', ],
		'*.doxygen' => ['Doxygen', ],
		'*.e' => ['E Language', 'Eiffel', 'Euphoria', ],
		'*.ebuild' => ['Bash', ],
		'*.eclass' => ['Bash', ],
		'*.eml' => ['Email', ],
		'*.eps' => ['PostScript', ],
		'*.err' => ['4GL', ],
		'*.ex' => ['Euphoria', ],
		'*.exu' => ['Euphoria', ],
		'*.exw' => ['Euphoria', ],
		'*.f' => ['Fortran', ],
		'*.f90' => ['Fortran', ],
		'*.f95' => ['Fortran', ],
		'*.fe' => ['ferite', ],
		'*.feh' => ['ferite', ],
		'*.flex' => ['Lex/Flex', ],
		'*.for' => ['Fortran', ],
		'*.fpp' => ['Fortran', ],
		'*.frag' => ['GLSL', ],
		'*.gdl' => ['GDL', ],
		'*.glsl' => ['GLSL', ],
		'*.guile' => ['Scheme', ],
		'*.h' => ['C++', 'C', 'ANSI C89', 'Inform', 'LPC', 'Objective-C', ],
		'*.h++' => ['C++', ],
		'*.hcc' => ['C++', ],
		'*.hpp' => ['C++', ],
		'*.hs' => ['Haskell', ],
		'*.hsp' => ['Spice', ],
		'*.ht' => ['Apache Configuration', ],
		'*.htm' => ['HTML', ],
		'*.html' => ['HTML', 'Mason', ],
		'*.hxx' => ['C++', ],
		'*.i' => ['progress', ],
		'*.idl' => ['IDL', ],
		'*.inc' => ['POV-Ray', 'PHP (HTML)', 'LPC', ],
		'*.inf' => ['Inform', ],
		'*.ini' => ['INI Files', ],
		'*.java' => ['Java', ],
		'*.js' => ['JavaScript', ],
		'*.jsp' => ['JSP', ],
		'*.katetemplate' => ['Kate File Template', ],
		'*.kbasic' => ['KBasic', ],
		'*.kdelnk' => ['.desktop', ],
		'*.l' => ['Lex/Flex', ],
		'*.ldif' => ['LDIF', ],
		'*.lex' => ['Lex/Flex', ],
		'*.lgo' => ['de_DE', 'en_US', 'nl', ],
		'*.lgt' => ['Logtalk', ],
		'*.lhs' => ['Literate Haskell', ],
		'*.lisp' => ['Common Lisp', ],
		'*.logo' => ['de_DE', 'en_US', 'nl', ],
		'*.lsp' => ['Common Lisp', ],
		'*.lua' => ['Lua', ],
		'*.ly' => ['LilyPond', ],
		'*.m' => ['Matlab', 'Objective-C', 'Octave', ],
		'*.m3u' => ['M3U', ],
		'*.mab' => ['MAB-DB', ],
		'*.md' => ['Modula-2', ],
		'*.mi' => ['Modula-2', ],
		'*.ml' => ['Objective Caml', 'SML', ],
		'*.mli' => ['Objective Caml', ],
		'*.moc' => ['C++', ],
		'*.mod' => ['Modula-2', ],
		'*.mup' => ['Music Publisher', ],
		'*.not' => ['Music Publisher', ],
		'*.o' => ['LPC', ],
		'*.octave' => ['Octave', ],
		'*.p' => ['Pascal', 'progress', ],
		'*.pas' => ['Pascal', ],
		'*.pb' => ['PureBasic', ],
		'*.per' => ['4GL-PER', ],
		'*.per.err' => ['4GL-PER', ],
		'*.php' => ['PHP (HTML)', ],
		'*.php3' => ['PHP (HTML)', ],
		'*.phtm' => ['PHP (HTML)', ],
		'*.phtml' => ['PHP (HTML)', ],
		'*.pic' => ['PicAsm', ],
		'*.pike' => ['Pike', ],
		'*.pl' => ['Perl', ],
		'*.pls' => ['INI Files', ],
		'*.pm' => ['Perl', ],
		'*.po' => ['GNU Gettext', ],
		'*.pot' => ['GNU Gettext', ],
		'*.pov' => ['POV-Ray', ],
		'*.pp' => ['Pascal', ],
		'*.prg' => ['xHarbour', 'Clipper', ],
		'*.pro' => ['RSI IDL', ],
		'*.prolog' => ['Prolog', ],
		'*.ps' => ['PostScript', ],
		'*.py' => ['Python', ],
		'*.pyw' => ['Python', ],
		'*.rb' => ['Ruby', ],
		'*.rc' => ['XML', ],
		'*.rdf' => ['XML', ],
		'*.reg' => ['WINE Config', ],
		'*.rex' => ['REXX', ],
		'*.rib' => ['RenderMan RIB', ],
		'*.s' => ['GNU Assembler', 'MIPS Assembler', ],
		'*.sa' => ['Sather', ],
		'*.sce' => ['scilab', ],
		'*.scheme' => ['Scheme', ],
		'*.sci' => ['scilab', ],
		'*.scm' => ['Scheme', ],
		'*.sgml' => ['SGML', ],
		'*.sh' => ['Bash', ],
		'*.shtm' => ['HTML', ],
		'*.shtml' => ['HTML', ],
		'*.siv' => ['Sieve', ],
		'*.sml' => ['SML', ],
		'*.sp' => ['Spice', ],
		'*.spec' => ['RPM Spec', ],
		'*.sql' => ['SQL', 'SQL (MySQL)', 'SQL (PostgreSQL)', ],
		'*.src' => ['PicAsm', ],
		'*.ss' => ['Scheme', ],
		'*.t2t' => ['txt2tags', ],
		'*.tcl' => ['Tcl/Tk', ],
		'*.tdf' => ['AHDL', ],
		'*.tex' => ['LaTeX', ],
		'*.tji' => ['TaskJuggler', ],
		'*.tjp' => ['TaskJuggler', ],
		'*.tk' => ['Tcl/Tk', ],
		'*.tst' => ['BaseTestchild', ],
		'*.uc' => ['UnrealScript', ],
		'*.v' => ['Verilog', ],
		'*.vcg' => ['GDL', ],
		'*.vert' => ['GLSL', ],
		'*.vhd' => ['VHDL', ],
		'*.vhdl' => ['VHDL', ],
		'*.vl' => ['Verilog', ],
		'*.vm' => ['Velocity', ],
		'*.w' => ['progress', ],
		'*.wml' => ['PHP (HTML)', ],
		'*.wrl' => ['VRML', ],
		'*.xml' => ['XML', ],
		'*.xsl' => ['xslt', ],
		'*.xslt' => ['xslt', ],
		'*.y' => ['Yacc/Bison', ],
		'*.ys' => ['yacas', ],
		'*Makefile*' => ['Makefile', ],
		'*makefile*' => ['Makefile', ],
		'*patch' => ['Diff', ],
		'CMakeLists.txt' => ['CMake', ],
		'ChangeLog' => ['ChangeLog', ],
		'QRPGLESRC.*' => ['ILERPG', ],
		'apache.conf' => ['Apache Configuration', ],
		'apache2.conf' => ['Apache Configuration', ],
		'httpd.conf' => ['Apache Configuration', ],
		'httpd2.conf' => ['Apache Configuration', ],
		'xorg.conf' => ['x.org Configuration', ],
	};
	$self->{'sections'} = {
		'Assembler' => [
			'AVR Assembler',
			'Asm6502',
			'GNU Assembler',
			'Intel x86 (NASM)',
			'MIPS Assembler',
			'PicAsm',
		],
		'Configuration' => [
			'.desktop',
			'Apache Configuration',
			'Cisco',
			'INI Files',
			'WINE Config',
			'x.org Configuration',
		],
		'Database' => [
			'4GL',
			'4GL-PER',
			'LDIF',
			'SQL',
			'SQL (MySQL)',
			'SQL (PostgreSQL)',
			'progress',
		],
		'Hardware' => [
			'AHDL',
			'Spice',
			'VHDL',
			'Verilog',
		],
		'Logo' => [
			'de_DE',
			'en_US',
			'nl',
		],
		'Markup' => [
			'ASP',
			'BibTeX',
			'CSS',
			'ColdFusion',
			'Doxygen',
			'GNU Gettext',
			'HTML',
			'JSP',
			'Javadoc',
			'Kate File Template',
			'LaTeX',
			'MAB-DB',
			'PostScript',
			'SGML',
			'VRML',
			'Wikimedia',
			'XML',
			'txt2tags',
			'xslt',
		],
		'Other' => [
			'ABC',
			'Alerts',
			'CMake',
			'CSS/PHP',
			'CUE Sheet',
			'ChangeLog',
			'Debian Changelog',
			'Debian Control',
			'Diff',
			'Email',
			'JavaScript/PHP',
			'LilyPond',
			'M3U',
			'Makefile',
			'Music Publisher',
			'POV-Ray',
			'RPM Spec',
			'RenderMan RIB',
		],
		'Scientific' => [
			'GDL',
			'Matlab',
			'Octave',
			'TI Basic',
			'scilab',
		],
		'Script' => [
			'Ansys',
		],
		'Scripts' => [
			'AWK',
			'Bash',
			'Common Lisp',
			'Euphoria',
			'JavaScript',
			'Lua',
			'Mason',
			'PHP (HTML)',
			'PHP/PHP',
			'Perl',
			'Pike',
			'Python',
			'Quake Script',
			'R Script',
			'REXX',
			'Ruby',
			'Scheme',
			'Sieve',
			'TaskJuggler',
			'Tcl/Tk',
			'UnrealScript',
			'Velocity',
			'ferite',
		],
		'Sources' => [
			'ANSI C89',
			'Ada',
      'Box',
			'C',
			'C#',
			'C++',
			'CGiS',
			'Cg',
			'Clipper',
			'Component-Pascal',
			'D',
			'E Language',
			'Eiffel',
			'Fortran',
			'FreeBASIC',
			'GLSL',
			'Haskell',
			'IDL',
			'ILERPG',
			'Inform',
			'Java',
			'KBasic',
			'LPC',
			'Lex/Flex',
			'Literate Haskell',
			'Logtalk',
			'Modula-2',
			'Objective Caml',
			'Objective-C',
			'Pascal',
			'Prolog',
			'PureBasic',
			'RSI IDL',
			'SML',
			'Sather',
			'Stata',
			'Yacc/Bison',
			'xHarbour',
			'yacas',
		],
		'Test' => [
			'BaseTest',
			'BaseTestchild',
		],
	};
	$self->{'syntaxes'} = {
		'.desktop' => 'Desktop',
		'4GL' => 'FourGL',
		'4GL-PER' => 'FourGLminusPER',
		'ABC' => 'ABC',
		'AHDL' => 'AHDL',
		'ANSI C89' => 'ANSI_C89',
		'ASP' => 'ASP',
		'AVR Assembler' => 'AVR_Assembler',
		'AWK' => 'AWK',
		'Ada' => 'Ada',
		'Alerts' => 'Alerts',
		'Ansys' => 'Ansys',
		'Apache Configuration' => 'Apache_Configuration',
		'Asm6502' => 'Asm6502',
		'BaseTest' => 'BaseTest',
		'BaseTestchild' => 'BaseTestchild',
		'Bash' => 'Bash',
		'BibTeX' => 'BibTeX',
    'Box' => 'Box',
		'C' => 'C',
		'C#' => 'Cdash',
		'C++' => 'Cplusplus',
		'CGiS' => 'CGiS',
		'CMake' => 'CMake',
		'CSS' => 'CSS',
		'CSS/PHP' => 'CSS_PHP',
		'CUE Sheet' => 'CUE_Sheet',
		'Cg' => 'Cg',
		'ChangeLog' => 'ChangeLog',
		'Cisco' => 'Cisco',
		'Clipper' => 'Clipper',
		'ColdFusion' => 'ColdFusion',
		'Common Lisp' => 'Common_Lisp',
		'Component-Pascal' => 'ComponentminusPascal',
		'D' => 'D',
		'Debian Changelog' => 'Debian_Changelog',
		'Debian Control' => 'Debian_Control',
		'Diff' => 'Diff',
		'Doxygen' => 'Doxygen',
		'E Language' => 'E_Language',
		'Eiffel' => 'Eiffel',
		'Email' => 'Email',
		'Euphoria' => 'Euphoria',
		'Fortran' => 'Fortran',
		'FreeBASIC' => 'FreeBASIC',
		'GDL' => 'GDL',
		'GLSL' => 'GLSL',
		'GNU Assembler' => 'GNU_Assembler',
		'GNU Gettext' => 'GNU_Gettext',
		'HTML' => 'HTML',
		'Haskell' => 'Haskell',
		'IDL' => 'IDL',
		'ILERPG' => 'ILERPG',
		'INI Files' => 'INI_Files',
		'Inform' => 'Inform',
		'Intel x86 (NASM)' => 'Intel_x86_NASM',
		'JSP' => 'JSP',
		'Java' => 'Java',
		'JavaScript' => 'JavaScript',
		'JavaScript/PHP' => 'JavaScript_PHP',
		'Javadoc' => 'Javadoc',
		'KBasic' => 'KBasic',
		'Kate File Template' => 'Kate_File_Template',
		'LDIF' => 'LDIF',
		'LPC' => 'LPC',
		'LaTeX' => 'LaTeX',
		'Lex/Flex' => 'Lex_Flex',
		'LilyPond' => 'LilyPond',
		'Literate Haskell' => 'Literate_Haskell',
		'Logtalk' => 'Logtalk',
		'Lua' => 'Lua',
		'M3U' => 'M3U',
		'MAB-DB' => 'MABminusDB',
		'MIPS Assembler' => 'MIPS_Assembler',
		'Makefile' => 'Makefile',
		'Mason' => 'Mason',
		'Matlab' => 'Matlab',
		'Modula-2' => 'Modulaminus2',
		'Music Publisher' => 'Music_Publisher',
		'Objective Caml' => 'Objective_Caml',
		'Objective-C' => 'ObjectiveminusC',
		'Octave' => 'Octave',
		'PHP (HTML)' => 'PHP_HTML',
		'PHP/PHP' => 'PHP_PHP',
		'POV-Ray' => 'POVminusRay',
		'Pascal' => 'Pascal',
		'Perl' => 'Perl',
		'PicAsm' => 'PicAsm',
		'Pike' => 'Pike',
		'PostScript' => 'PostScript',
		'Prolog' => 'Prolog',
		'PureBasic' => 'PureBasic',
		'Python' => 'Python',
		'Quake Script' => 'Quake_Script',
		'R Script' => 'R_Script',
		'REXX' => 'REXX',
		'RPM Spec' => 'RPM_Spec',
		'RSI IDL' => 'RSI_IDL',
		'RenderMan RIB' => 'RenderMan_RIB',
		'Ruby' => 'Ruby',
		'SGML' => 'SGML',
		'SML' => 'SML',
		'SQL' => 'SQL',
		'SQL (MySQL)' => 'SQL_MySQL',
		'SQL (PostgreSQL)' => 'SQL_PostgreSQL',
		'Sather' => 'Sather',
		'Scheme' => 'Scheme',
		'Sieve' => 'Sieve',
		'Spice' => 'Spice',
		'Stata' => 'Stata',
		'TI Basic' => 'TI_Basic',
		'TaskJuggler' => 'TaskJuggler',
		'Tcl/Tk' => 'Tcl_Tk',
		'UnrealScript' => 'UnrealScript',
		'VHDL' => 'VHDL',
		'VRML' => 'VRML',
		'Velocity' => 'Velocity',
		'Verilog' => 'Verilog',
		'WINE Config' => 'WINE_Config',
		'Wikimedia' => 'Wikimedia',
		'XML' => 'XML',
		'Yacc/Bison' => 'Yacc_Bison',
		'de_DE' => 'De_DE',
		'en_US' => 'En_US',
		'ferite' => 'Ferite',
		'nl' => 'Nl',
		'progress' => 'Progress',
		'scilab' => 'Scilab',
		'txt2tags' => 'Txt2tags',
		'x.org Configuration' => 'Xorg_Configuration',
		'xHarbour' => 'XHarbour',
		'xslt' => 'Xslt',
		'yacas' => 'Yacas',
	};
	#end autoinsert
	$self->{'language '} = '';
	bless ($self, $class);
	if ($language ne '') {
		$self->language($language);
	}
	return $self;
}

sub extensions {
	my $self = shift;
	return $self->{'extensions'};
}

#overriding Template's initialize method. now it should not do anything.
sub initialize {
	my $cw = shift;
}

sub language {
	my $self = shift;
	if (@_) {
		$self->{'language'} = shift;
		$self->reset;
	}
	return $self->{'language'};
}

sub languageAutoSet {
	my ($self, $file) = @_;
	my $lang = $self->languagePropose($file);
	if (defined $lang) {
		$self->language($lang)
	} else {
		$self->language('Off')
	}
}

sub languageList {
	my $self = shift;
	my $l = $self->{'syntaxes'};
	return sort {uc($a) cmp uc($b)} keys %$l;
}

sub languagePropose {
	my ($self, $file) = @_;
	my $hsh = $self->extensions;
	foreach my $key (keys %$hsh) {
		my $reg = $key;
		$reg =~ s/\./\\./g;
		$reg =~ s/\+/\\+/g;
		$reg =~ s/\*/.*/g;
		$reg = "$reg\$";
		if ($file =~ /$reg/) {
			return $hsh->{$key}->[0]
		}
	}
	return undef;
}

sub languagePlug {
	my ($self, $req) = @_;
	unless (exists($self->{'syntaxes'}->{$req})) {
		warn "undefined language: $req";
		return undef;
	}
	return $self->{'syntaxes'}->{$req};
}

sub reset {
	my $self = shift;
	my $lang = $self->language;
	if ($lang eq 'Off') {
		$self->stack([]);
	} else {
		my $plug	= $self->pluginGet($lang);
		my $basecontext = $plug->basecontext;
		$self->stack([
			[$plug, $basecontext]
		]);
	}
	$self->out([]);
	$self->snippet('');
}

sub sections {
	my $self = shift;
	return $self->{'sections'};
}

sub syntaxes {
	my $self = shift;
	return $self->{'syntaxes'}
}


1;

__END__

