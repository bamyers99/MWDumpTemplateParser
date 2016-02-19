MWDumpTemplateParser
====================

MWDumpTemplateParser is a C++ wiki template parameter parser for a MediaWiki XML page dump.

It generates 3 files:

1. Template parameter value file
2. Template totals file
3. Template start offset in the Template parameter value file

Libraries
=========
* expat - SAX XML parser. Must be compiled for 8-bit mode.
* pcre - Perl compatible regular expression engine version 1. Must be compiled for 8-bit mode. Will use jit if available.

Directory structure
===================

	/src - Source code

Compiling
=========
-std=c++11

Testing
=======
To run all tests:

	cd /src
	MWDumpTemplateParser -t - - -

Sample usage
============
 * bunzip2 -c enwiki-pages-articles.xml.bz2 | ./MWDumpTemplateParser -v - enwikiTemplateParams enwikiTemplateTotals&
 * LC_ALL=C sort -n -k 1,1 -k 2,2 enwikiTemplateParams >enwikiTemplateParams.sorted
 * ./MWDumpTemplateParser -offsets enwikiTemplateParams.sorted enwikiTemplateOffsets
