
# Copyright (c) 2006 Hans Jeuken. All rights reserved.
# This module is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.

package Syntax::Highlight::Engine::Kate::All;

use vars qw($VERSION);
$VERSION = '0.02';

use Syntax::Highlight::Engine::Kate::AHDL;
use Syntax::Highlight::Engine::Kate::ANSI_C89;
use Syntax::Highlight::Engine::Kate::ASP;
use Syntax::Highlight::Engine::Kate::AVR_Assembler;
use Syntax::Highlight::Engine::Kate::AWK;
use Syntax::Highlight::Engine::Kate::Ada;
use Syntax::Highlight::Engine::Kate::Alerts;
use Syntax::Highlight::Engine::Kate::Asm6502;
use Syntax::Highlight::Engine::Kate::BaseTest;
use Syntax::Highlight::Engine::Kate::BaseTestchild;
use Syntax::Highlight::Engine::Kate::Bash;
use Syntax::Highlight::Engine::Kate::BibTeX;
use Syntax::Highlight::Engine::Kate::C;
use Syntax::Highlight::Engine::Kate::CGiS;
use Syntax::Highlight::Engine::Kate::CMake;
use Syntax::Highlight::Engine::Kate::CSS;
use Syntax::Highlight::Engine::Kate::CUE_Sheet;
use Syntax::Highlight::Engine::Kate::Cdash;
use Syntax::Highlight::Engine::Kate::Cg;
use Syntax::Highlight::Engine::Kate::ChangeLog;
use Syntax::Highlight::Engine::Kate::Cisco;
use Syntax::Highlight::Engine::Kate::Clipper;
use Syntax::Highlight::Engine::Kate::ColdFusion;
use Syntax::Highlight::Engine::Kate::Common_Lisp;
use Syntax::Highlight::Engine::Kate::ComponentminusPascal;
use Syntax::Highlight::Engine::Kate::Cplusplus;
use Syntax::Highlight::Engine::Kate::D;
use Syntax::Highlight::Engine::Kate::Debian_Changelog;
use Syntax::Highlight::Engine::Kate::Debian_Control;
use Syntax::Highlight::Engine::Kate::Desktop;
use Syntax::Highlight::Engine::Kate::Diff;
use Syntax::Highlight::Engine::Kate::Doxygen;
use Syntax::Highlight::Engine::Kate::E_Language;
use Syntax::Highlight::Engine::Kate::Eiffel;
use Syntax::Highlight::Engine::Kate::Euphoria;
use Syntax::Highlight::Engine::Kate::Ferite;
use Syntax::Highlight::Engine::Kate::Fortran;
use Syntax::Highlight::Engine::Kate::FourGL;
use Syntax::Highlight::Engine::Kate::FourGLminusPER;
use Syntax::Highlight::Engine::Kate::GDL;
use Syntax::Highlight::Engine::Kate::GLSL;
use Syntax::Highlight::Engine::Kate::GNU_Assembler;
use Syntax::Highlight::Engine::Kate::GNU_Gettext;
use Syntax::Highlight::Engine::Kate::HTML;
use Syntax::Highlight::Engine::Kate::Haskell;
use Syntax::Highlight::Engine::Kate::IDL;
use Syntax::Highlight::Engine::Kate::ILERPG;
use Syntax::Highlight::Engine::Kate::INI_Files;
use Syntax::Highlight::Engine::Kate::Inform;
use Syntax::Highlight::Engine::Kate::Intel_x86_NASM;
use Syntax::Highlight::Engine::Kate::JSP;
use Syntax::Highlight::Engine::Kate::Java;
use Syntax::Highlight::Engine::Kate::JavaScript;
use Syntax::Highlight::Engine::Kate::Javadoc;
use Syntax::Highlight::Engine::Kate::KBasic;
use Syntax::Highlight::Engine::Kate::LDIF;
use Syntax::Highlight::Engine::Kate::LPC;
use Syntax::Highlight::Engine::Kate::LaTeX;
use Syntax::Highlight::Engine::Kate::Lex_Flex;
use Syntax::Highlight::Engine::Kate::LilyPond;
use Syntax::Highlight::Engine::Kate::Literate_Haskell;
use Syntax::Highlight::Engine::Kate::Lua;
use Syntax::Highlight::Engine::Kate::MABminusDB;
use Syntax::Highlight::Engine::Kate::MIPS_Assembler;
use Syntax::Highlight::Engine::Kate::Makefile;
use Syntax::Highlight::Engine::Kate::Mason;
use Syntax::Highlight::Engine::Kate::Matlab;
use Syntax::Highlight::Engine::Kate::Modulaminus2;
use Syntax::Highlight::Engine::Kate::Music_Publisher;
use Syntax::Highlight::Engine::Kate::Octave;
use Syntax::Highlight::Engine::Kate::PHP_HTML;
use Syntax::Highlight::Engine::Kate::PHP_PHP;
use Syntax::Highlight::Engine::Kate::POVminusRay;
use Syntax::Highlight::Engine::Kate::Pascal;
use Syntax::Highlight::Engine::Kate::Perl;
use Syntax::Highlight::Engine::Kate::PicAsm;
use Syntax::Highlight::Engine::Kate::Pike;
use Syntax::Highlight::Engine::Kate::PostScript;
use Syntax::Highlight::Engine::Kate::Progress;
use Syntax::Highlight::Engine::Kate::Prolog;
use Syntax::Highlight::Engine::Kate::PureBasic;
use Syntax::Highlight::Engine::Kate::Python;
use Syntax::Highlight::Engine::Kate::Quake_Script;
use Syntax::Highlight::Engine::Kate::REXX;
use Syntax::Highlight::Engine::Kate::RPM_Spec;
use Syntax::Highlight::Engine::Kate::RSI_IDL;
use Syntax::Highlight::Engine::Kate::R_Script;
use Syntax::Highlight::Engine::Kate::RenderMan_RIB;
use Syntax::Highlight::Engine::Kate::Ruby;
use Syntax::Highlight::Engine::Kate::SGML;
use Syntax::Highlight::Engine::Kate::SML;
use Syntax::Highlight::Engine::Kate::SQL;
use Syntax::Highlight::Engine::Kate::SQL_MySQL;
use Syntax::Highlight::Engine::Kate::SQL_PostgreSQL;
use Syntax::Highlight::Engine::Kate::Sather;
use Syntax::Highlight::Engine::Kate::Scheme;
use Syntax::Highlight::Engine::Kate::Scilab;
use Syntax::Highlight::Engine::Kate::Sieve;
use Syntax::Highlight::Engine::Kate::Spice;
use Syntax::Highlight::Engine::Kate::Stata;
use Syntax::Highlight::Engine::Kate::TI_Basic;
use Syntax::Highlight::Engine::Kate::Tcl_Tk;
use Syntax::Highlight::Engine::Kate::Txt2tags;
use Syntax::Highlight::Engine::Kate::UnrealScript;
use Syntax::Highlight::Engine::Kate::VHDL;
use Syntax::Highlight::Engine::Kate::VRML;
use Syntax::Highlight::Engine::Kate::Velocity;
use Syntax::Highlight::Engine::Kate::Verilog;
use Syntax::Highlight::Engine::Kate::WINE_Config;
use Syntax::Highlight::Engine::Kate::XHarbour;
use Syntax::Highlight::Engine::Kate::XML;
use Syntax::Highlight::Engine::Kate::XML_Debug;
use Syntax::Highlight::Engine::Kate::Xslt;
use Syntax::Highlight::Engine::Kate::Yacas;
use Syntax::Highlight::Engine::Kate::Yacc_Bison;

1;