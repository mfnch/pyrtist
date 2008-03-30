# Copyright (c) 2005 - 2006 Hans Jeuken. All rights reserved.
# This program is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.

# This file was generated from the 'box.xml' file of the syntax highlight
# engine of the kate text editor (http://www.kate-editor.org

#kate xml version 1.25
#kate version 2.4
#generated: Sun Feb  3 22:02:04 2008, localtime

package Syntax::Highlight::Engine::Kate::Box;

use vars qw($VERSION);
$VERSION = '0.03';

use strict;
use warnings;
use base('Syntax::Highlight::Engine::Kate::Template');

sub new {
   my $proto = shift;
   my $class = ref($proto) || $proto;
   my $self = $class->SUPER::new(@_);
   $self->attributes({
      'Alert' => 'Alert',
      'Char' => 'Char',
      'Comment' => 'Comment',
      'Data Type' => 'DataType',
      'Member Type' => 'DataType',
      'Member' => 'Normal',
      'Decimal' => 'DecVal',
      'Float' => 'Float',
      'Hex' => 'BaseN',
      'Keyword' => 'Keyword',
      'Normal Text' => 'Normal',
      'Octal' => 'BaseN',
      'Prep. Lib' => 'Others',
      'Preprocessor' => 'Others',
      'Region Marker' => 'RegionMarker',
      'String' => 'String',
      'String Char' => 'Char',
      'Symbol' => 'Keyword',
   });
   $self->listAdd('keywords',
      'include',
   );

#         <RegExpr attribute="Data Type" context="#stay" String="\b[A-Z][A-Za-z0-9]*"/>
#         <RegExpr attribute="Member Type" context="#stay" String="[.][A-Z][A-Za-z0-9]*"/>
#         <RegExpr attribute="Member" context="#stay" String="[.][a-z][A-Za-z0-9]*"/>

#         <keyword attribute="Keyword" context="#stay" String="keywords"/>
#
#         <DetectChar attribute="Symbol" context="#stay" char="@"/>
#         <Detect2Chars attribute="Symbol" context="#stay" char="@" char1="@"/>
#         <Detect2Chars attribute="Symbol" context="#stay" char="-" char1=">"/>
#         <StringDetect context="#stay" attribute="Symbol" String="(])"/>
#         <StringDetect context="#stay" attribute="Symbol" String="([)"/>
#         <StringDetect context="#stay" attribute="Symbol" String="(;)"/>
#
#         <DetectChar attribute="Symbol" context="#stay" char="[" beginRegion="Brace1" />
#         <DetectChar attribute="Symbol" context="#stay" char="]" endRegion="Brace1" />
#
#         <Float attribute="Float" context="#stay">
#           <AnyChar String="fF" attribute="Float" context="#stay"/>
#         </Float>
#         <HlCOct attribute="Octal" context="#stay"/>
#         <HlCHex attribute="Hex" context="#stay"/>
#         <Int attribute="Decimal" context="#stay"/>
#         <HlCChar attribute="Char" context="#stay"/>
#
#         <DetectChar attribute="String" context="String" char="&quot;"/>
#
#         <Detect2Chars attribute="Comment" context="Commentar 1" char="/" char1="/"/>
#         <Detect2Chars attribute="Comment" context="Commentar 2" char="/" char1="*" beginRegion="Comment"/>
#         <AnyChar attribute="Normal Text" context="#stay" String=":!%&amp;()+,-/.*&lt;=&gt;?[]|~^&#59;"/>
#       </context>


   $self->contextdata({
      'Commentar 1' => {
         callback => \&parseCommentar1,
         attribute => 'Comment',
         lineending => '#pop',
      },
      'Commentar 2' => {
         callback => \&parseCommentar2,
         attribute => 'Comment',
      },
      'Commentar/Preprocessor' => {
         callback => \&parseCommentarPreprocessor,
         attribute => 'Comment',
      },
      'Define' => {
         callback => \&parseDefine,
         attribute => 'Preprocessor',
         lineending => '#pop',
      },
      'Normal' => {
         callback => \&parseNormal,
         attribute => 'Normal Text',
      },
      'Outscoped' => {
         callback => \&parseOutscoped,
         attribute => 'Comment',
      },
      'Outscoped intern' => {
         callback => \&parseOutscopedintern,
         attribute => 'Comment',
      },
      'Preprocessor' => {
         callback => \&parsePreprocessor,
         attribute => 'Preprocessor',
         lineending => '#pop',
      },
      'Region Marker' => {
         callback => \&parseRegionMarker,
         attribute => 'Region Marker',
         lineending => '#pop',
      },
      'String' => {
         callback => \&parseString,
         attribute => 'String',
         lineending => '#pop',
      },
   });
   $self->deliminators('\\s||\\.|\\(|\\)|:|\\!|\\+|,|-|<|=|>|\\%|\\&|\\*|\\/|;|\\?|\\[|\\]|\\^|\\{|\\||\\}|\\~|\\\\');
   $self->basecontext('Normal');
   $self->keywordscase(0);
   $self->initialize;
   bless ($self, $class);
   return $self;
}

sub language {
   return 'Box';
}

sub parseCommentar1 {
   my ($self, $text) = @_;
   # context => '##Alerts'
   # type => 'IncludeRules'
   if ($self->includePlugin('Alerts', $text)) {
      return 1
   }
   return 0;
};

sub parseCommentar2 {
   my ($self, $text) = @_;
   # attribute => 'Comment'
   # char => '*'
   # char1 => '/'
   # context => '#pop'
   # endRegion => 'Comment'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '*', '/', 0, 0, 0, undef, 0, '#pop', 'Comment')) {
      return 1
   }
   # context => '##Alerts'
   # type => 'IncludeRules'
   if ($self->includePlugin('Alerts', $text)) {
      return 1
   }
   return 0;
};

sub parseCommentarPreprocessor {
   my ($self, $text) = @_;
   # attribute => 'Comment'
   # char => '*'
   # char1 => '/'
   # context => '#pop'
   # endRegion => 'Comment2'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '*', '/', 0, 0, 0, undef, 0, '#pop', 'Comment')) {
      return 1
   }
   return 0;
};

sub parseDefine {
   my ($self, $text) = @_;
   # attribute => 'Preprocessor'
   # context => '#stay'
   # type => 'LineContinue'
   if ($self->testLineContinue($text, 0, undef, 0, '#stay', 'Preprocessor')) {
      return 1
   }
   return 0;
};

sub parseNormal {
   my ($self, $text) = @_;
   # String => '\b[A-Z][A-Za-z0-9]*'
   # attribute => 'Data Type'
   # context => '#stay'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, '\b[A-Z][A-Za-z0-9]*', 0, 0, 0, undef, 0, '#stay', 'Data Type')) {
      return 1
   }
   # String => '[.][A-Z][A-Za-z0-9]*'
   # attribute => 'Data Type'
   # context => '#stay'
   # type => 'Member Type'
   if ($self->testRegExpr($text, '[.][A-Z][A-Za-z0-9]*', 0, 0, 0, undef, 0, '#stay', 'Member Type')) {
      return 1
   }
   # String => '[.][a-z][A-Za-z0-9]*'
   # attribute => 'Data Type'
   # context => '#stay'
   # type => 'Member'
   if ($self->testRegExpr($text, '[.][a-z][A-Za-z0-9]*', 0, 0, 0, undef, 0, '#stay', 'Member')) {
      return 1
   }

#--------------------------------------------------------------------------------------------------------
# removed in box.xml
   # String => '#\s*if\s+0'
   # attribute => 'Preprocessor'
   # beginRegion => 'Outscoped'
   # context => 'Outscoped'
   # firstNonSpace => 'true'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, '#\\s*if\\s+0', 0, 0, 0, undef, 1, 'Outscoped', 'Preprocessor')) {
      return 1
   }
   # attribute => 'Preprocessor'
   # char => '#'
   # context => 'Preprocessor'
   # firstNonSpace => 'true'
   # type => 'DetectChar'
   if ($self->testDetectChar($text, '#', 0, 0, 0, undef, 1, 'Preprocessor', 'Preprocessor')) {
      return 1
   }
   # String => '//BEGIN'
   # attribute => 'Region Marker'
   # beginRegion => 'Region1'
   # context => 'Region Marker'
   # firstNonSpace => 'true'
   # type => 'StringDetect'
   if ($self->testStringDetect($text, '//BEGIN', 0, 0, 0, undef, 1, 'Region Marker', 'Region Marker')) {
      return 1
   }
   # String => '//END'
   # attribute => 'Region Marker'
   # context => 'Region Marker'
   # endRegion => 'Region1'
   # firstNonSpace => 'true'
   # type => 'StringDetect'
   if ($self->testStringDetect($text, '//END', 0, 0, 0, undef, 1, 'Region Marker', 'Region Marker')) {
      return 1
   }
#--------------------------------------------------------------------------------------------------------

   # attribute => 'Symbol'
   # char => '@'
   # char1 => '@'
   # context => '#stay'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '@', '@', 0, 0, 0, undef, 0, '#stay', 'Symbol')) {
      return 1
   }
   # attribute => 'Symbol'
   # char => '@'
   # context => '#stay'
   # type => 'DetectChar'
   if ($self->testDetectChar($text, '@', 0, 0, 0, undef, 0, '#stay', 'Symbol')) {
      return 1
   }
   # attribute => 'Symbol'
   # char => '-'
   # char1 => '>'
   # context => '#stay'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '-', '>', 0, 0, 0, undef, 0, '#stay', 'Symbol')) {
      return 1
   }
   # String => '(])'
   # attribute => 'Symbol'
   # context => '#stay'
   # type => 'StringDetect'
   if ($self->testStringDetect($text, '(])', 0, 0, 0, undef, 0, '#stay', 'Symbol')) {
       return 1
   }
   # String => '([)'
   # attribute => 'Symbol'
   # context => '#stay'
   # type => 'StringDetect'
   if ($self->testStringDetect($text, '([)', 0, 0, 0, undef, 0, '#stay', 'Symbol')) {
       return 1
   }
   # String => '(;)'
   # attribute => 'Symbol'
   # context => '#stay'
   # type => 'StringDetect'
   if ($self->testStringDetect($text, '(;)', 0, 0, 0, undef, 0, '#stay', 'Symbol')) {
       return 1
   }
   # String => 'keywords'
   # attribute => 'Keyword'
   # context => '#stay'
   # type => 'keyword'
   if ($self->testKeyword($text, 'keywords', 0, undef, 0, '#stay', 'Keyword')) {
      return 1
   }
   # type => 'DetectIdentifier'
   if ($self->testDetectIdentifier($text, 0, undef, 0, '#stay', undef)) {
      return 1
   }
   # attribute => 'Symbol'
   # beginRegion => 'Brace1'
   # char => '['
   # context => '#stay'
   # type => 'DetectChar'
   if ($self->testDetectChar($text, '[', 0, 0, 0, undef, 0, '#stay', 'Symbol')) {
      return 1
   }
   # attribute => 'Symbol'
   # char => ']'
   # context => '#stay'
   # endRegion => 'Brace1'
   # type => 'DetectChar'
   if ($self->testDetectChar($text, ']', 0, 0, 0, undef, 0, '#stay', 'Symbol')) {
      return 1
   }
   # String => '\b[0-9]+("."[0-9]+)?([eE][+-]?[0-9]+)?'
   # attribute => 'Float'
   # context => '#stay'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, '[0-9]+([.][0-9]+)?([eE][+-]?[0-9]+)?', 0, 0, 0, undef, 0, '#stay', 'Float')) {
      return 1
   }
   # attribute => 'Octal'
   # context => '#stay'
   # type => 'HlCOct'
   if ($self->testHlCOct($text, 0, undef, 0, '#stay', 'Octal')) {
      return 1
   }
   # attribute => 'Hex'
   # context => '#stay'
   # type => 'HlCHex'
   if ($self->testHlCHex($text, 0, undef, 0, '#stay', 'Hex')) {
      return 1
   }
   # String => '[0-9]+'
   # attribute => 'Int'
   # context => '#stay'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, '[0-9]+', 0, 0, 0, undef, 0, '#stay', 'Int')) {
      return 1
   }
   # attribute => 'Char'
   # context => '#stay'
   # type => 'HlCChar'
   if ($self->testHlCChar($text, 0, undef, 0, '#stay', 'Char')) {
      return 1
   }
   # attribute => 'String'
   # char => '"'
   # context => 'String'
   # type => 'DetectChar'
   if ($self->testDetectChar($text, '"', 0, 0, 0, undef, 0, 'String', 'String')) {
      return 1
   }
   # context => '##Doxygen'
   # type => 'IncludeRules'
   if ($self->includePlugin('Doxygen', $text)) {
      return 1
   }
   # attribute => 'Comment'
   # char => '/'
   # char1 => '/'
   # context => 'Commentar 1'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '/', '/', 0, 0, 0, undef, 0, 'Commentar 1', 'Comment')) {
      return 1
   }
   # attribute => 'Comment'
   # beginRegion => 'Comment'
   # char => '/'
   # char1 => '*'
   # context => 'Commentar 2'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '/', '*', 0, 0, 0, undef, 0, 'Commentar 2', 'Comment')) {
      return 1
   }
   # String => ':!%&()+,-/.*<=>?[]|~^;'
   # attribute => 'Normal Text'
   # context => '#stay'
   # type => 'AnyChar'
   if ($self->testAnyChar($text, ':!%&()+,-/.*<=>?[]|~^;', 0, 0, undef, 0, '#stay', 'Normal Text')) {
      return 1
   }
   return 0;
};

sub parseOutscoped {
   my ($self, $text) = @_;
   # type => 'DetectSpaces'
   if ($self->testDetectSpaces($text, 0, undef, 0, '#stay', undef)) {
      return 1
   }
   # context => '##Alerts'
   # type => 'IncludeRules'
   if ($self->includePlugin('Alerts', $text)) {
      return 1
   }
   # type => 'DetectIdentifier'
   if ($self->testDetectIdentifier($text, 0, undef, 0, '#stay', undef)) {
      return 1
   }
   # attribute => 'String'
   # char => '"'
   # context => 'String'
   # type => 'DetectChar'
   if ($self->testDetectChar($text, '"', 0, 0, 0, undef, 0, 'String', 'String')) {
      return 1
   }
   # context => '##Doxygen'
   # type => 'IncludeRules'
   if ($self->includePlugin('Doxygen', $text)) {
      return 1
   }
   # attribute => 'Comment'
   # char => '/'
   # char1 => '/'
   # context => 'Commentar 1'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '/', '/', 0, 0, 0, undef, 0, 'Commentar 1', 'Comment')) {
      return 1
   }
   # attribute => 'Comment'
   # beginRegion => 'Comment'
   # char => '/'
   # char1 => '*'
   # context => 'Commentar 2'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '/', '*', 0, 0, 0, undef, 0, 'Commentar 2', 'Comment')) {
      return 1
   }
   # String => '#\s*if'
   # attribute => 'Comment'
   # beginRegion => 'Outscoped'
   # context => 'Outscoped intern'
   # firstNonSpace => 'true'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, '#\\s*if', 0, 0, 0, undef, 1, 'Outscoped intern', 'Comment')) {
      return 1
   }
   # String => '#\s*(endif|else|elif)'
   # attribute => 'Preprocessor'
   # context => '#pop'
   # endRegion => 'Outscoped'
   # firstNonSpace => 'true'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, '#\\s*(endif|else|elif)', 0, 0, 0, undef, 1, '#pop', 'Preprocessor')) {
      return 1
   }
   return 0;
};

sub parseOutscopedintern {
   my ($self, $text) = @_;
   # type => 'DetectSpaces'
   if ($self->testDetectSpaces($text, 0, undef, 0, '#stay', undef)) {
      return 1
   }
   # context => '##Alerts'
   # type => 'IncludeRules'
   if ($self->includePlugin('Alerts', $text)) {
      return 1
   }
   # type => 'DetectIdentifier'
   if ($self->testDetectIdentifier($text, 0, undef, 0, '#stay', undef)) {
      return 1
   }
   # attribute => 'String'
   # char => '"'
   # context => 'String'
   # type => 'DetectChar'
   if ($self->testDetectChar($text, '"', 0, 0, 0, undef, 0, 'String', 'String')) {
      return 1
   }
   # context => '##Doxygen'
   # type => 'IncludeRules'
   if ($self->includePlugin('Doxygen', $text)) {
      return 1
   }
   # attribute => 'Comment'
   # char => '/'
   # char1 => '/'
   # context => 'Commentar 1'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '/', '/', 0, 0, 0, undef, 0, 'Commentar 1', 'Comment')) {
      return 1
   }
   # attribute => 'Comment'
   # beginRegion => 'Comment'
   # char => '/'
   # char1 => '*'
   # context => 'Commentar 2'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '/', '*', 0, 0, 0, undef, 0, 'Commentar 2', 'Comment')) {
      return 1
   }
   # String => '#\s*if'
   # attribute => 'Comment'
   # beginRegion => 'Outscoped'
   # context => 'Outscoped intern'
   # firstNonSpace => 'true'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, '#\\s*if', 0, 0, 0, undef, 1, 'Outscoped intern', 'Comment')) {
      return 1
   }
   # String => '#\s*endif'
   # attribute => 'Comment'
   # context => '#pop'
   # endRegion => 'Outscoped'
   # firstNonSpace => 'true'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, '#\\s*endif', 0, 0, 0, undef, 1, '#pop', 'Comment')) {
      return 1
   }
   return 0;
};

sub parsePreprocessor {
   my ($self, $text) = @_;
   # attribute => 'Preprocessor'
   # context => '#stay'
   # type => 'LineContinue'
   if ($self->testLineContinue($text, 0, undef, 0, '#stay', 'Preprocessor')) {
      return 1
   }
   # String => 'define.*((?=\\))'
   # attribute => 'Preprocessor'
   # context => 'Define'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, 'define.*((?=\\\\))', 0, 0, 0, undef, 0, 'Define', 'Preprocessor')) {
      return 1
   }
   # String => 'define.*'
   # attribute => 'Preprocessor'
   # context => '#stay'
   # type => 'RegExpr'
   if ($self->testRegExpr($text, 'define.*', 0, 0, 0, undef, 0, '#stay', 'Preprocessor')) {
      return 1
   }
   # attribute => 'Prep. Lib'
   # char => '"'
   # char1 => '"'
   # context => '#stay'
   # type => 'RangeDetect'
   if ($self->testRangeDetect($text, '"', '"', 0, 0, undef, 0, '#stay', 'Prep. Lib')) {
      return 1
   }
   # attribute => 'Prep. Lib'
   # char => '<'
   # char1 => '>'
   # context => '#stay'
   # type => 'RangeDetect'
   if ($self->testRangeDetect($text, '<', '>', 0, 0, undef, 0, '#stay', 'Prep. Lib')) {
      return 1
   }
   # context => '##Doxygen'
   # type => 'IncludeRules'
   if ($self->includePlugin('Doxygen', $text)) {
      return 1
   }
   # attribute => 'Comment'
   # beginRegion => 'Comment2'
   # char => '/'
   # char1 => '*'
   # context => 'Commentar/Preprocessor'
   # type => 'Detect2Chars'
   if ($self->testDetect2Chars($text, '/', '*', 0, 0, 0, undef, 0, 'Commentar/Preprocessor', 'Comment')) {
      return 1
   }
   return 0;
};

sub parseRegionMarker {
   my ($self, $text) = @_;
   return 0;
};

sub parseString {
   my ($self, $text) = @_;
   # attribute => 'String'
   # context => '#stay'
   # type => 'LineContinue'
   if ($self->testLineContinue($text, 0, undef, 0, '#stay', 'String')) {
      return 1
   }
   # attribute => 'String Char'
   # context => '#stay'
   # type => 'HlCStringChar'
   if ($self->testHlCStringChar($text, 0, undef, 0, '#stay', 'String Char')) {
      return 1
   }
   # attribute => 'String'
   # char => '"'
   # context => '#pop'
   # type => 'DetectChar'
   if ($self->testDetectChar($text, '"', 0, 0, 0, undef, 0, '#pop', 'String')) {
      return 1
   }
   return 0;
};


1;

__END__

=head1 NAME

Syntax::Highlight::Engine::Kate::Box - a Plugin for Box syntax highlighting

=head1 SYNOPSIS

 require Syntax::Highlight::Engine::Kate::Box;
 my $sh = new Syntax::Highlight::Engine::Kate::Box([
 ]);

=head1 DESCRIPTION

Syntax::Highlight::Engine::Kate::Box is a  plugin module that provides syntax highlighting
for Box to the Syntax::Haghlight::Engine::Kate highlighting engine.

This code is generated from the syntax definition files used
by the Kate project.
It works quite fine, but can use refinement and optimization.

It inherits Syntax::Higlight::Engine::Kate::Template. See also there.

=cut

=head1 AUTHOR

Hans Jeuken (haje <at> toneel <dot> demon <dot> nl)

=cut

=head1 BUGS

Unknown. If you find any, please contact the author

=cut

