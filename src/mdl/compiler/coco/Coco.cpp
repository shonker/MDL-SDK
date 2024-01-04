/*-------------------------------------------------------------------------
Compiler Generator Coco/R,
Copyright (c) 1990, 2004 Hanspeter Moessenboeck, University of Linz
extended by M. Loeberbauer & A. Woess, Univ. of Linz
ported to C++ by Csaba Balazs, University of Szeged
with improvements by Pat Terry, Rhodes University

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

As an exception, it is allowed to write an extension of Coco/R that is
used as a plugin in non-free software.

If not otherwise stated, any source code generated by Coco/R (other than
Coco/R itself) does not fall under the GNU General Public License.
-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------
  Trace output options
  0 | A: prints the states of the scanner automaton
  1 | F: prints the First and Follow sets of all nonterminals
  2 | G: prints the syntax graph of the productions
  3 | I: traces the computation of the First sets
  4 | J: prints the sets associated with ANYs and synchronisation sets
  6 | S: prints the symbol table (terminals, nonterminals, pragmas)
  7 | X: prints a cross reference list of all syntax symbols
  8 | P: prints statistics about the Coco run

  Trace output can be switched on by the pragma
    $ { digit | letter }
  in the attributed grammar or as a command-line option
  -------------------------------------------------------------------------*/


#include <cstdio>
#include <string>
#include "Scanner.h"
#include "Parser.h"
#include "Tab.h"

using namespace Coco;

int main(int argc, char *argv[]) {

	printf("Coco/R (May 02, 2023, NVidia modified)\n");

	std::string srcName;
	std::string nsName;
	std::string tokenPrefix = "_";
	std::string frameDir;
	std::string ddtString;
	std::string traceFileName;
	std::string outDir;
	bool emitLines = false;
	bool suppressRslvWarning = false;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-namespace") == 0 && i < argc - 1) nsName = argv[++i];
		else if (strcmp(argv[i], "-token_prefix") == 0 && i < argc - 1) tokenPrefix = argv[++i];
		else if (strcmp(argv[i], "-frames") == 0 && i < argc - 1) frameDir = argv[++i];
		else if (strcmp(argv[i], "-trace") == 0 && i < argc - 1) ddtString = argv[++i];
		else if (strcmp(argv[i], "-o") == 0 && i < argc - 1) outDir = std::string(argv[++i]) + "/";
		else if (strcmp(argv[i], "-lines") == 0) emitLines = true;
		else if (strcmp(argv[i], "-no_misplaced_resolver") == 0) suppressRslvWarning = true;
		else srcName = argv[i];
	}

	if (argc > 0 && !srcName.empty()) {
		size_t pos = srcName.rfind('/');
		if (pos == std::string::npos) {
			pos = srcName.rfind('\\');
		}
		std::string file = srcName;
		std::string srcDir = srcName.substr(0, pos != std::string::npos ? pos + 1 : pos);

		Coco::Scanner scanner(file.c_str());
		Coco::Parser  parser(&scanner);

		traceFileName = (!outDir.empty() ? outDir : srcDir) + "trace.txt";

		if ((parser.trace = fopen(traceFileName.c_str(), "w")) == NULL) {
			printf("-- could not open %s\n", traceFileName.c_str());
			return EXIT_FAILURE;
		}

		parser.tab  = new Coco::Tab(
			parser, srcName, srcDir, nsName, tokenPrefix, frameDir,
			!outDir.empty() ? outDir : srcDir, emitLines, suppressRslvWarning);
		parser.dfa  = new Coco::DFA(parser);
		parser.pgen = new Coco::ParserGen(parser);

		parser.errors->srcName = parser.tab->srcName.c_str();

		if (!ddtString.empty()) {
			parser.tab->SetDDT(ddtString.c_str());
		}

		parser.Parse();

		fclose(parser.trace);

		// obtain the FileSize
		parser.trace = fopen(traceFileName.c_str(), "r");
		fseek(parser.trace, 0, SEEK_END);
		long fileSize = ftell(parser.trace);
		fclose(parser.trace);
		if (fileSize == 0) {
			remove(traceFileName.c_str());
		} else {
			printf("trace output is in %s\n", traceFileName.c_str());
		}

		printf("%d errors detected\n", parser.errors->count);
		if (parser.errors->count != 0) {
			return EXIT_FAILURE;
		}

		delete parser.pgen;
		delete parser.dfa;
		delete parser.tab;
	} else {
		printf(
			"Usage: Coco Grammar.ATG {Option}\n"
			"Options:\n"
			"  -namespace <namespaceName>\n"
			"  -frames    <frameFilesDirectory>\n"
			"  -trace     <traceString>\n"
			"  -o         <outputDirectory>\n"
			"  -lines\n"
			"Valid characters in the trace string:\n"
			"  A  trace automaton\n"
			"  F  list first/follow sets\n"
			"  G  print syntax graph\n"
			"  I  trace computation of first sets\n"
			"  J  list ANY and SYNC sets\n"
			"  P  print statistics\n"
			"  S  list symbol table\n"
			"  X  list cross reference table\n"
			"Scanner.frame and Parser.frame files needed in ATG directory\n"
			"or in a directory specified in the -frames option.\n");
	}

	return EXIT_SUCCESS;
}
