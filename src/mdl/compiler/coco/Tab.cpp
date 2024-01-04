/*-------------------------------------------------------------------------
Tab -- Symbol Table Management
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

#include <string>
#include "Tab.h"
#include "Parser.h"
#include "BitArray.h"
#include "Scanner.h"

namespace Coco {

char const * const Tab::nTyp[] =
		{"    ", "t   ", "pr  ", "nt  ", "clas", "chr ", "wt  ", "any ", "eps ",
		 "sync", "sem ", "alt ", "iter", "opt ", "rslv"};

char const * const Tab::tKind[] = {"fixedToken", "classToken", "litToken", "classLitToken"};

Tab::Tab(
	Parser            &parser,
	std::string const &srcName,
	std::string const &srcDir,
	std::string const &nsName,
	std::string const &tokenPrefix,
	std::string const &frameDir,
	std::string const &outDir,
	bool              emitLines,
	bool              suppressRslvWarning)
: semDeclPos(NULL)
, ignored(NULL)
, gramSy(NULL)
, eofSy(NULL)
, noSym(NULL)
, allSyncSets(NULL)
, literals(new HashTable())
, srcName(srcName)
, srcDir(srcDir)
, nsName(nsName)
, tokenPrefix(tokenPrefix)
, frameDir(frameDir)
, outDir(outDir)
, checkEOF(true)
, emitLines(emitLines)
, suppressRslvWarning(suppressRslvWarning)
, curSy(NULL)
, parser(parser)
, trace(parser.trace)
, errors(parser.errors)
, terminals()
, pragmas()
, nonterminals()
, nodes()
, dummyNode(NewNode(Node::eps, (Symbol*)NULL, 0))
, classes()
, dummyName('A')
{
	for (int i = 0; i < 10; i++) {
		ddt[i] = false;
	}

	eofSy = NewSym(Node::t, "EOF", 0);
}

Symbol* Tab::NewSym(Node::Kind typ, char const *name, int line) {
	if (coco_string_length(name) == 2 && name[0] == '"') {
		parser.SemErr("empty token not allowed");
		name = coco_string_create("???");
	}
	Symbol *sym = new Symbol(typ, name, line);

	switch (typ) {
	case Node::t:
		sym->n = terminals.size(); terminals.push_back(sym);
		break;
	case Node::pr:
		pragmas.push_back(sym);
		break;
	case Node::nt:
		sym->n = nonterminals.size(); nonterminals.push_back(sym);
		break;
	default:
		break;
	}

	return sym;
}

Symbol* Tab::FindSym(char const *name) {
	for (Symbol *s : terminals) {
		if (strcmp(s->name, name) == 0)
			return s;
	}
	for (Symbol *s : nonterminals) {
		if (strcmp(s->name, name) == 0)
			return s;
	}
	return NULL;
}

int Tab::Num(Node *p) {
	if (p == NULL) return 0; else return p->n;
}

void Tab::PrintSym(Symbol *sym) {
	char *paddedName = Name(sym->name);
	fprintf(trace, "%3d %14s %s", sym->n, paddedName, nTyp[sym->typ]);
	coco_string_delete(paddedName);

	if (sym->attrPos==NULL) fprintf(trace, " false "); else fprintf(trace, " true  ");
	if (sym->typ == Node::nt) {
		fprintf(trace, "%5d", Num(sym->graph));
		if (sym->deletable) fprintf(trace, " true  "); else fprintf(trace, " false ");
	} else
		fprintf(trace, "            ");

	fprintf(trace, "%5d %s\n", sym->line, tKind[sym->tokenKind]);
}

void Tab::PrintSymbolTable() {
	fprintf(trace, "Symbol Table:\n");
	fprintf(trace, "------------\n\n");
	fprintf(trace, " nr name          typ  hasAt graph  del    line tokenKind\n");

	for (Symbol *sym : terminals) {
		PrintSym(sym);
	}
	for (Symbol *sym : pragmas) {
		PrintSym(sym);
	}
	for (Symbol *sym : nonterminals) {
		PrintSym(sym);
	}


	fprintf(trace, "\nLiteral Tokens:\n");
	fprintf(trace, "--------------\n");

	Iterator *iter = literals->GetIterator();
	while (iter->HasNext()) {
		DictionaryEntry *e = iter->Next();
		fprintf(trace, "%s%s =  %s.\n", tokenPrefix.c_str(), ((Symbol*) (e->val))->name, e->key);
	}
	fprintf(trace, "\n");
}

void Tab::PrintSet(BitArray *s, int indent) {
	int col, len;
	col = indent;
	for (Symbol *sym : terminals) {
		if ((*s)[sym->n]) {
			len = coco_string_length(sym->name);
			if (col + len >= 80) {
				fprintf(trace, "\n");
				for (col = 1; col < indent; col++) fprintf(trace, " ");
			}
			fprintf(trace, "%s ", sym->name);
			col += len + 1;
		}
	}
	if (col == indent) fprintf(trace, "-- empty set --");
	fprintf(trace, "\n");
}

//---------------------------------------------------------------------
//  Syntax graph management
//---------------------------------------------------------------------

Node* Tab::NewNode(Node::Kind typ, Symbol *sym, int line) {
	Node* node = new Node(typ, sym, line);
	node->n = nodes.size();
	nodes.push_back(node);
	return node;
}


Node* Tab::NewNode(Node::Kind typ, Node* sub) {
	Node* node = NewNode(typ, (Symbol*)NULL, 0);
	node->sub = sub;
	return node;
}

Node* Tab::NewNode(Node::Kind typ, int val, int line) {
	Node* node = NewNode(typ, (Symbol*)NULL, line);
	node->val = val;
	return node;
}


void Tab::MakeFirstAlt(Graph *g) {
	g->l = NewNode(Node::alt, g->l); g->l->line = g->l->sub->line;
	g->r->up = true;
	g->l->next = g->r;
	g->r = g->l;
}

// The result will be in g1
void Tab::MakeAlternative(Graph *g1, Graph *g2) {
	g2->l = NewNode(Node::alt, g2->l); g2->l->line = g2->l->sub->line;
	g2->l->up = true;
	g2->r->up = true;
	Node *p = g1->l; while (p->down != NULL) p = p->down;
	p->down = g2->l;
	p = g1->r; while (p->next != NULL) p = p->next;
	// append alternative to g1 end list
	p->next = g2->l;
	// append g2 end list to g1 end list
	g2->l->next = g2->r;
}

// The result will be in g1
void Tab::MakeSequence(Graph *g1, Graph *g2) {
	Node *p = g1->r->next; g1->r->next = g2->l; // link head node
	while (p != NULL) {  // link substructure
		Node *q = p->next; p->next = g2->l;
		p = q;
	}
	g1->r = g2->r;
}

void Tab::MakeIteration(Graph *g) {
	g->l = NewNode(Node::iter, g->l);
	g->r->up = true;
	Node *p = g->r;
	g->r = g->l;
	while (p != NULL) {
		Node *q = p->next; p->next = g->l;
		p = q;
	}
}

void Tab::MakeOption(Graph *g) {
	g->l = NewNode(Node::opt, g->l);
	g->r->up = true;
	g->l->next = g->r;
	g->r = g->l;
}

void Tab::Finish(Graph *g) {
	Node *p = g->r;
	while (p != NULL) {
		Node *q = p->next; p->next = NULL;
		p = q;
	}
}

void Tab::DeleteNodes() {
	nodes.clear();
	// FIXME: leak
	dummyNode = NewNode(Node::eps, (Symbol*)NULL, 0);
}

Graph* Tab::StrToGraph(char const *str) {
	char *subStr = coco_string_create(str, 1, coco_string_length(str)-2);
	char *s = Unescape(subStr);
	coco_string_delete(subStr);
	if (coco_string_length(s) == 0) parser.SemErr("empty token not allowed");
	Graph *g = new Graph();
	g->r = dummyNode;
	for (int i = 0; i < coco_string_length(s); i++) {
		Node *p = NewNode(Node::chr, (int)s[i], 0);
		g->r->next = p; g->r = p;
	}
	g->l = dummyNode->next; dummyNode->next = NULL;
	coco_string_delete(s);
	return g;
}


void Tab::SetContextTrans(Node *p) { // set transition code in the graph rooted at p
	while (p != NULL) {
		if (p->typ == Node::chr || p->typ == Node::clas) {
			p->code = Node::contextTrans;
		} else if (p->typ == Node::opt || p->typ == Node::iter) {
			SetContextTrans(p->sub);
		} else if (p->typ == Node::alt) {
			SetContextTrans(p->sub); SetContextTrans(p->down);
		}
		if (p->up) break;
		p = p->next;
	}
}

//------------ graph deletability check -----------------

bool Tab::DelGraph(Node const *p) {
	return p == NULL || (DelNode(p) && DelGraph(p->next));
}

bool Tab::DelSubGraph(Node const *p) {
	return p == NULL || (DelNode(p) && (p->up || DelSubGraph(p->next)));
}

bool Tab::DelNode(Node const *p) {
	if (p->typ == Node::nt) {
		return p->sym->deletable;
	}
	else if (p->typ == Node::alt) {
		return DelSubGraph(p->sub) || (p->down != NULL && DelSubGraph(p->down));
	}
	else {
		return p->typ == Node::iter || p->typ == Node::opt || p->typ == Node::sem
				|| p->typ == Node::eps || p->typ == Node::rslv || p->typ == Node::sync;
	}
}

//----------------- graph printing ----------------------

int Tab::Ptr(Node *p, bool up) {
	if (p == NULL) return 0;
	else if (up) return -(p->n);
	else return p->n;
}

char* Tab::Pos(Position *pos) {
	char *format = new char[10];
	if (pos == NULL) {
		coco_snprintf(format, 10, "     ");
	} else {
		coco_snprintf(format, 10, "%5d", pos->beg);
	}
	return format;
}

char* Tab::Name(const char *name) {
	char *name2 = coco_string_create_append(name, "           ");
	char *subName2 = coco_string_create(name2, 0, 12);
	coco_string_delete(name2);
	return subName2;
	// found no simpler way to get the first 12 characters of the name
	// padded with blanks on the right
}

void Tab::PrintNodes() {
	fprintf(trace, "Graph nodes:\n");
	fprintf(trace, "----------------------------------------------------\n");
	fprintf(trace, "   n type name          next  down   sub   pos  line\n");
	fprintf(trace, "                               val  code\n");
	fprintf(trace, "----------------------------------------------------\n");

	for (Node *p : nodes) {
		fprintf(trace, "%4d %s ", p->n, (nTyp[p->typ]));
		if (p->sym != NULL) {
			char *paddedName = Name(p->sym->name);
			fprintf(trace, "%12s ", paddedName);
			coco_string_delete(paddedName);
		} else if (p->typ == Node::clas) {
			CharClass *c = classes[p->val];
			char *paddedName = Name(c->name.c_str());
			fprintf(trace, "%12s ", paddedName);
			coco_string_delete(paddedName);
		} else {
			fprintf(trace, "             ");
		}
		fprintf(trace, "%5d ", Ptr(p->next, p->up));

		switch (p->typ) {
		case Node::t:
		case Node::nt:
		case Node::wt:
			fprintf(trace, "             %5s", Pos(p->pos));
			break;
		case Node::chr:
			fprintf(trace, "%5d %5d       ", p->val, p->code);
			break;
		case Node::clas:
			fprintf(trace, "      %5d       ", p->code);
			break;
		case Node::alt:
		case Node::iter:
		case Node::opt:
			fprintf(trace, "%5d %5d       ", Ptr(p->down, false), Ptr(p->sub, false));
			break;
		case Node::sem:
			fprintf(trace, "             %5s", Pos(p->pos));
			break;
		case Node::eps:
		case Node::any:
		case Node::sync:
			fprintf(trace, "                  ");
			break;
		default:
			break;
		}
		fprintf(trace, "%5d\n", p->line);
	}
	fprintf(trace, "\n");
}

//---------------------------------------------------------------------
//  Character class management
//---------------------------------------------------------------------


CharClass* Tab::NewCharClass(std::string const &name, CharSet *s) {
	CharClass *c;
	if (name =="#") {
		std::string temp = name + (char)dummyName++;
		c = new CharClass(temp, s);
	} else {
		c = new CharClass(name, s);
	}
	c->n = classes.size();
	classes.push_back(c);
	return c;
}

CharClass* Tab::FindCharClass(std::string const &name) {
	for (CharClass *c : classes) {
		if (c->name == name) {
			return c;
		}
	}
	return NULL;
}

CharClass* Tab::FindCharClass(CharSet *s) {
	for (CharClass *c : classes) {
		if (s->Equals(c->set)) {
			return c;
		}
	}
	return NULL;
}

CharSet* Tab::CharClassSet(int i) {
	return classes[i]->set;
}

//----------- character class printing

char* Tab::Ch(const char ch) {
	char* format = new char[10];
	if (ch < ' ' || ch >= 127 || ch == '\'' || ch == '\\') {
		coco_snprintf(format, 10, "%d", ch);
		return format;
	} else {
		coco_snprintf(format, 10, "'%c'", (char)ch);
		return format;
	}
}

void Tab::WriteCharSet(CharSet *s) {
	for (CharSet::Range *r = s->head; r != NULL; r = r->next) {
		if (r->from < r->to) {
			char *from = Ch(r->from);
			char *to = Ch(r->to);
			fprintf(trace, "%s .. %s ", from, to);
			delete [] from;
			delete [] to;
		}
		else {
			char *from = Ch(r->from);
			fprintf(trace, "%s ", from);
			delete [] from;
		}
	}
}

void Tab::WriteCharClasses () {
	for (CharClass *c : classes) {
		char* format2 = coco_string_create_append(c->name.c_str(), "            ");
		char* format  = coco_string_create(format2, 0, 10);
		coco_string_merge(format, ": ");
		fprintf(trace, "%s", format);

		WriteCharSet(c->set);
		fprintf(trace, "\n");
		coco_string_delete(format);
		coco_string_delete(format2);
	}
	fprintf(trace, "\n");
}

//---------------------------------------------------------------------
//  Symbol set computations
//---------------------------------------------------------------------

/* Computes the first set for the given Node. */
BitArray* Tab::First0(Node const *p, BitArray &mark) {
	BitArray *fs = new BitArray(terminals.size());
	while (p != NULL && !(mark[p->n])) {
		mark.Set(p->n, true);
		switch (p->typ) {
		case Node::nt:
			if (p->sym->firstReady) {
				fs->Or(*p->sym->first);
			} else {
				BitArray *fs0 = First0(p->sym->graph, mark);
				fs->Or(*fs0);
				delete fs0;
			}
			break;
		case Node::t:
		case Node::wt:
			fs->Set(p->sym->n, true);
			break;
		case Node::any:
			fs->Or(*p->set);
			break;
		case Node::alt:
			{
				BitArray *fs0 = First0(p->sub, mark);
				fs->Or(*fs0);
				delete fs0;
				fs0 = First0(p->down, mark);
				fs->Or(*fs0);
				delete fs0;
			}
			break;
		case Node::iter:
		case Node::opt:
			{
				BitArray *fs0 = First0(p->sub, mark);
				fs->Or(*fs0);
				delete fs0;
			}
			break;
		default:
			break;
		}

		if (!DelNode(p)) {
			break;
		}
		p = p->next;
	}
	return fs;
}

BitArray *Tab::First(Node const *p) {
	BitArray mark(nodes.size());
	BitArray *fs = First0(p, mark);
	if (ddt[DDT_TRACE_COMP_FIRST]) {
		fprintf(trace, "\n");
		if (p != NULL) {
			fprintf(trace, "First: node = %d\n", p->n);
		} else {
			fprintf(trace, "First: node = null\n");
		}
		PrintSet(fs, 0);
	}
	return fs;
}


void Tab::CompFirstSets() {
	for (Symbol *sym : nonterminals) {
		sym->first = new BitArray(terminals.size());
		sym->firstReady = false;
	}
	for (Symbol *sym : nonterminals) {
		sym->first = First(sym->graph);
		sym->firstReady = true;
	}
}

void Tab::CompFollow(Node const *p, BitArray &visited) {
	while (p != NULL && !(visited[p->n])) {
		visited.Set(p->n, true);
		if (p->typ == Node::nt) {
			BitArray *s = First(p->next);
			p->sym->follow->Or(*s);
			if (DelGraph(p->next)) {
				p->sym->nts->Set(curSy->n, true);
			}
		} else if (p->typ == Node::opt || p->typ == Node::iter) {
			CompFollow(p->sub, visited);
		} else if (p->typ == Node::alt) {
			CompFollow(p->sub, visited);
			CompFollow(p->down, visited);
		}
		p = p->next;
	}
}

void Tab::Complete(Symbol *sym, BitArray &visited) {
	if (!visited[sym->n]) {
		visited.Set(sym->n, true);
		for (Symbol *s : nonterminals) {
			if ((*(sym->nts))[s->n]) {
				Complete(s, visited);
				sym->follow->Or(*s->follow);
				if (sym == curSy) {
					sym->nts->Set(s->n, false);
				}
			}
		}
	}
}

void Tab::CompFollowSets() {
	for (Symbol *sym : nonterminals) {
		sym->follow = new BitArray(terminals.size());
		sym->nts = new BitArray(nonterminals.size());
	}
	gramSy->follow->Set(eofSy->n, true);
	BitArray visited(nodes.size());
	for (Symbol *sym : nonterminals) {  // get direct successors of nonterminals
		curSy = sym;
		CompFollow(sym->graph, visited);
	}

	BitArray nt_visited(nonterminals.size());
	for (Symbol *sym : nonterminals) {  // add indirect successors to followers
		nt_visited.SetAll(false);
		curSy = sym;
		Complete(sym, nt_visited);
	}
}

Node* Tab::LeadingAny(Node *p) {
	if (p == NULL) {
		return NULL;
	}
	Node *a = NULL;
	if (p->typ == Node::any) {
		a = p;
	} else if (p->typ == Node::alt) {
		a = LeadingAny(p->sub);
		if (a == NULL) a = LeadingAny(p->down);
	} else if (p->typ == Node::opt || p->typ == Node::iter) {
		a = LeadingAny(p->sub);
	}
	if (a == NULL && DelNode(p) && !p->up) {
		a = LeadingAny(p->next);
	}
	return a;
}

void Tab::FindAS(Node *p) { // find ANY sets
	Node *a;
	while (p != NULL) {
		if (p->typ == Node::opt || p->typ == Node::iter) {
			FindAS(p->sub);
			a = LeadingAny(p->sub);
			if (a != NULL) {
				BitArray *f = First(p->next);
				Sets::Subtract(*a->set, *f);
				delete f;
			}
		} else if (p->typ == Node::alt) {
			BitArray s1(terminals.size());
			Node *q = p;
			while (q != NULL) {
				FindAS(q->sub);
				a = LeadingAny(q->sub);
				if (a != NULL) {
					BitArray *f = First(q->down);
					f->Or(s1);
					Sets::Subtract(*a->set, *f);
					delete f;
				} else {
					BitArray *f = First(q->sub);
					s1.Or(*f);
					delete f;
				}
				q = q->down;
			}
		}

		// Remove alternative terminals before ANY, in the following
		// examples a and b must be removed from the ANY set:
		// [a] ANY, or {a|b} ANY, or [a][b] ANY, or (a|) ANY, or
		// A = [a]. A ANY
		if (DelNode(p)) {
			a = LeadingAny(p->next);
			if (a != NULL) {
				Node *q = (p->typ == Node::nt) ? p->sym->graph : p->sub;
				BitArray *f = First(q);
				Sets::Subtract(*a->set, *f);
				delete f;
			}
		}

		if (p->up) {
			break;
		}
		p = p->next;
	}
}

void Tab::CompAnySets() {
	for (Symbol *sym : nonterminals) {
		FindAS(sym->graph);
	}
}

BitArray* Tab::Expected(Node *p, Symbol *curSy) {
	BitArray *s = First(p);
	if (DelGraph(p)) {
		s->Or(*curSy->follow);
	}
	return s;
}

// does not look behind resolvers; only called during LL(1) test and in CheckRes
BitArray* Tab::Expected0(Node *p, Symbol *curSy) {
	if (p->typ == Node::rslv) {
		return new BitArray(terminals.size());
	} else {
		return Expected(p, curSy);
	}
}

void Tab::CompSync(Node *p, BitArray &visited) {
	while (p != NULL && !visited.Get(p->n)) {
		visited.Set(p->n, true);
		if (p->typ == Node::sync) {
			BitArray *s = Expected(p->next, curSy);
			s->Set(eofSy->n, true);
			allSyncSets->Or(*s);
			p->set = s;
		} else if (p->typ == Node::alt) {
			CompSync(p->sub, visited);
			CompSync(p->down, visited);
		} else if (p->typ == Node::opt || p->typ == Node::iter) {
			CompSync(p->sub, visited);
		}
		p = p->next;
	}
}

void Tab::CompSyncSets() {
	allSyncSets = new BitArray(terminals.size());
	allSyncSets->Set(eofSy->n, true);
	BitArray visited(nodes.size());

	for (Symbol *sym : nonterminals) {
		curSy = sym;
		CompSync(curSy->graph, visited);
	}
}

void Tab::SetupAnys() {
	for (Node *p : nodes) {
		if (p->typ == Node::any) {
			p->set = new BitArray(terminals.size(), true);
			p->set->Set(eofSy->n, false);
		}
	}
}

void Tab::CompDeletableSymbols() {
	bool changed;
	do {
		changed = false;
		for (Symbol *sym : nonterminals) {
			if (!sym->deletable && sym->graph != NULL && DelGraph(sym->graph)) {
				sym->deletable = true; changed = true;
			}
		}
	} while (changed);

	for (Symbol *sym : nonterminals) {
		if (sym->deletable) {
			printf("  %s is deletable\n", sym->name);
		}
	}
}

void Tab::RenumberPragmas() {
	int n = terminals.size();
	for (Symbol *sym : pragmas) {
		sym->n = n++;
	}
}

void Tab::CompSymbolSets() {
	CompDeletableSymbols();
	CompFirstSets();
	CompAnySets();
	CompFollowSets();
	CompSyncSets();
	if (ddt[DDT_LIST_SETS]) {
		fprintf(trace, "\n");
		fprintf(trace, "First & follow symbols:\n");
		fprintf(trace, "----------------------\n\n");

		for (Symbol *sym : nonterminals) {
			fprintf(trace, "%s\n", sym->name);
			fprintf(trace, "first:   "); PrintSet(sym->first, 10);
			fprintf(trace, "follow:  "); PrintSet(sym->follow, 10);
			fprintf(trace, "\n");
		}
	}
	if (ddt[DDT_PRINT_ANY_SYNC_SETS]) {
		fprintf(trace, "\n");
		fprintf(trace, "ANY and SYNC sets:\n");
		fprintf(trace, "-----------------\n");

		for (Node *p : nodes) {
			if (p->typ == Node::any || p->typ == Node::sync) {
				fprintf(trace, "%4d %4s ", p->n, nTyp[p->typ]);
				PrintSet(p->set, 11);
			}
		}
	}
}

//---------------------------------------------------------------------
//  String handling
//---------------------------------------------------------------------

char Tab::Hex2Char(const char* s) {
	int val = 0;
	int len = coco_string_length(s);
	for (int i = 0; i < len; i++) {
		char ch = s[i];
		if ('0' <= ch && ch <= '9') val = 16 * val + (ch - '0');
		else if ('a' <= ch && ch <= 'f') val = 16 * val + (10 + ch - 'a');
		else if ('A' <= ch && ch <= 'F') val = 16 * val + (10 + ch - 'A');
		else parser.SemErr("bad escape sequence in string or character");
	}
	if (val >= COCO_UNICODE_MAX) {/* pdt */
		parser.SemErr("bad escape sequence in string or character");
	}
	return (char) val;
}

std::string Tab::Char2Hex(const char ch) {
	char format[16];
	coco_snprintf(format, sizeof(format), "\\0x%04x", ch);
	return format;
}

char *Tab::Unescape(const char* s) {
	/* replaces escape sequences in s by their Unicode values. */
	std::string buf;
	int i = 0;
	int len = coco_string_length(s);
	while (i < len) {
		if (s[i] == '\\') {
			switch (s[i+1]) {
				case '\\': buf += '\\'; i += 2; break;
				case '\'': buf += '\''; i += 2; break;
				case '\"': buf += '\"'; i += 2; break;
				case 'r': buf += '\r'; i += 2; break;
				case 'n': buf += '\n'; i += 2; break;
				case 't': buf += '\t'; i += 2; break;
				case '0': buf += '\0'; i += 2; break;
				case 'a': buf += '\a'; i += 2; break;
				case 'b': buf += '\b'; i += 2; break;
				case 'f': buf += '\f'; i += 2; break;
				case 'v': buf += '\v'; i += 2; break;
				case 'u': case L'x':
					if (i + 6 <= coco_string_length(s)) {
						char *subS = coco_string_create(s, i+2, 4);
						buf += Hex2Char(subS); i += 6;
						coco_string_delete(subS);
					} else {
						parser.SemErr("bad escape sequence in string or character");
						i = coco_string_length(s);
					}
					break;
				default:
						parser.SemErr("bad escape sequence in string or character");
					i += 2; break;
			}
		} else {
			buf += s[i];
			++i;
		}
	}

	return coco_string_create(buf.c_str());
}


std::string Tab::Escape(const char* s) {
	std::string buf;
	char ch;
	int len = coco_string_length(s);
	for (int i = 0; i < len; ++i) {
		ch = s[i];
		switch (ch) {
		case '\\': buf += "\\\\"; break;
		case '\'': buf += "\\'"; break;
		case '\"': buf += "\\\""; break;
		case '\t': buf += "\\t"; break;
		case '\r': buf += "\\r"; break;
		case '\n': buf += "\\n"; break;
		default:
			if ((ch < ' ') || (ch > 0x7f)) {
				buf += Char2Hex(ch);
			} else {
				buf += ch;
			}
			break;
		}
	}
	return buf;
}


//---------------------------------------------------------------------
//  Grammar checks
//---------------------------------------------------------------------

bool Tab::GrammarOk() {
	bool ok = NtsComplete()
		&& AllNtReached()
		&& NoCircularProductions()
		&& AllNtToTerm();
	if (ok) { CheckResolvers(); CheckLL1(); }
	return ok;
}


//--------------- check for circular productions ----------------------

void Tab::GetSingles(Node *p, std::vector<Symbol *> &singles) {
	if (p == NULL) return;  // end of graph
	if (p->typ == Node::nt) {
		if (p->up || DelGraph(p->next)) {
			singles.push_back(p->sym);
		}
	} else if (p->typ == Node::alt || p->typ == Node::iter || p->typ == Node::opt) {
		if (p->up || DelGraph(p->next)) {
			GetSingles(p->sub, singles);
			if (p->typ == Node::alt) GetSingles(p->down, singles);
		}
	}
	if (!p->up && DelNode(p)) GetSingles(p->next, singles);
}

bool Tab::NoCircularProductions() {
	bool ok, changed, onLeftSide, onRightSide;
	std::vector<CNode *> list;

	for (Symbol *sym : nonterminals) {
		std::vector<Symbol *> singles;
		GetSingles(sym->graph, singles); // get nonterminals s such that sym-->s
		for (Symbol *s : singles) {
			list.push_back(new CNode(sym, s));
		}
	}

	do {
		changed = false;
		for (size_t i = 0, e = list.size(); i < e;) {
			CNode *n = list[i];
			onLeftSide = false; onRightSide = false;
			for (CNode *m : list) {
				if (n->left == m->right) onRightSide = true;
				if (n->right == m->left) onLeftSide = true;
			}
			if (!onLeftSide || !onRightSide) {
				list.erase(list.begin() + i);
				--e;
				changed = true;
			} else {
				++i;
			}
		}
	} while(changed);

	ok = true;
	for (CNode *n : list) {
		ok = false;
		++errors->count;
		printf("  %s --> %s", n->left->name, n->right->name);
	}
	return ok;
}


//--------------- check for LL(1) errors ----------------------

void Tab::LL1Error(Tab::LL1_error err, Symbol *sym) {
	printf("  LL1 warning in %s: ", curSy->name);
	if (sym != NULL) {
		printf("%s is ", sym->name);
	}
	switch (err) {
		case START_OF_SEVERL_ALTERNATIVES:
			printf("start of several alternatives\n"); break;
		case START_AND_SUCCESSOR_OF_DELETABLE:
			printf("start & successor of deletable structure\n"); break;
		case ANY_NODE_MATCHES_NO_SYMBOL:
			printf("an ANY node that matches no symbol\n"); break;
		case OPTIONAL_CONTENT_MUST_NOT_BE_DELETABLE:
			printf("contents of [...] or {...} must not be deletable\n"); break;
	}
}


void Tab::CheckOverlap(BitArray const &s1, BitArray const &s2, LL1_error err) {
	for (Symbol *sym : terminals) {
		if (s1[sym->n] && s2[sym->n]) {
			LL1Error(err, sym);
		}
	}
}

void Tab::CheckAlts(Node *p) {
	while (p != NULL) {
		if (p->typ == Node::alt) {
			Node *q = p;
			BitArray s1(terminals.size());
			while (q != NULL) { // for all alternatives
				BitArray *s2 = Expected0(q->sub, curSy);
				CheckOverlap(s1, *s2, START_OF_SEVERL_ALTERNATIVES);
				s1.Or(*s2);
				CheckAlts(q->sub);
				q = q->down;
				delete s2;
			}
		} else if (p->typ == Node::opt || p->typ == Node::iter) {
			if (DelSubGraph(p->sub)) {
				LL1Error(OPTIONAL_CONTENT_MUST_NOT_BE_DELETABLE, NULL); // e.g. [[...]]
			} else {
				BitArray *s1 = Expected0(p->sub, curSy);
				BitArray *s2 = Expected(p->next, curSy);
				CheckOverlap(*s1, *s2, START_AND_SUCCESSOR_OF_DELETABLE);
				delete s2;
				delete s1;
			}
			CheckAlts(p->sub);
		} else if (p->typ == Node::any) {
			if (Sets::Elements(*p->set) == 0) {
				// e.g. {ANY} ANY or [ANY] ANY or ( ANY | ANY )
				LL1Error(ANY_NODE_MATCHES_NO_SYMBOL, NULL);
			}
		}
		if (p->up) {
			break;
		}
		p = p->next;
	}
}

void Tab::CheckLL1() {
	for (Symbol *sym : nonterminals) {
		curSy = sym;
		CheckAlts(curSy->graph);
	}
}

//------------- check if resolvers are legal  --------------------

void Tab::ResErr(Node const *p, char const *msg) {
	errors->Warning(p->line, p->pos->col, msg);
}

void Tab::CheckRes(Node const *p, bool rslvAllowed) {
	while (p != NULL) {
		Node const *q;
		if (p->typ == Node::alt) {
			BitArray *expected = new BitArray(terminals.size());
			for (q = p; q != NULL; q = q->down) {
				BitArray *e = Expected0(q->sub, curSy);
				expected->Or(*e);
				delete e;
			}
			BitArray *soFar = new BitArray(terminals.size());
			for (q = p; q != NULL; q = q->down) {
				if (q->sub->typ == Node::rslv) {
					BitArray *fs = Expected(q->sub->next, curSy);
					if (Sets::Intersect(*fs, *soFar))
						ResErr(q->sub, "Warning: Resolver will never be evaluated. Place it at previous conflicting alternative.");
					if (!suppressRslvWarning && !Sets::Intersect(*fs, *expected)) {
						ResErr(q->sub, "Warning: Misplaced resolver: no LL(1) conflict.");
					}
				} else {
					BitArray *e = Expected(q->sub, curSy);
					soFar->Or(*e);
					delete e;
				}
				CheckRes(q->sub, true);
			}
		} else if (p->typ == Node::iter || p->typ == Node::opt) {
			if (p->sub->typ == Node::rslv) {
				BitArray *fs = First(p->sub->next);
				BitArray *fsNext = Expected(p->next, curSy);
				if (!suppressRslvWarning && !Sets::Intersect(*fs, *fsNext)) {
					ResErr(p->sub, "Warning: Misplaced resolver: no LL(1) conflict.");
				}
			}
			CheckRes(p->sub, true);
		} else if (p->typ == Node::rslv) {
			if (!rslvAllowed) {
				ResErr(p, "Warning: Misplaced resolver: no alternative.");
			}
		}

		if (p->up) break;
		p = p->next;
		rslvAllowed = false;
	}
}

void Tab::CheckResolvers() {
	for (Symbol *sym : nonterminals) {
		curSy = sym;
		CheckRes(curSy->graph, false);
	}
}


//------------- check if every nts has a production --------------------

bool Tab::NtsComplete() {
	bool complete = true;
	for (Symbol *sym : nonterminals) {
		if (sym->graph == NULL) {
			complete = false;
			++errors->count;
			printf("  No production for %s\n", sym->name);
		}
	}
	return complete;
}

//-------------- check if every nts can be reached  -----------------

void Tab::MarkReachedNts(Node *p, BitArray &visited) {
	while (p != NULL) {
		if (p->typ == Node::nt && !visited[p->sym->n]) { // new nt reached
			visited.Set(p->sym->n, true);
			MarkReachedNts(p->sym->graph, visited);
		} else if (p->typ == Node::alt || p->typ == Node::iter || p->typ == Node::opt) {
			MarkReachedNts(p->sub, visited);
			if (p->typ == Node::alt) {
				MarkReachedNts(p->down, visited);
			}
		}
		if (p->up) {
			break;
		}
		p = p->next;
	}
}

bool Tab::AllNtReached() {
	bool ok = true;
	BitArray visited(nonterminals.size());
	visited.Set(gramSy->n, true);
	MarkReachedNts(gramSy->graph, visited);
	for (Symbol *sym : nonterminals) {
		if (!visited[sym->n]) {
			ok = false;
			++errors->count;
			printf("  %s cannot be reached\n", sym->name);
		}
	}
	return ok;
}

//--------- check if every nts can be derived to terminals  ------------

bool Tab::IsTerm(Node *p, BitArray &mark) { // true if graph can be derived to terminals
	while (p != NULL) {
		if (p->typ == Node::nt && !mark[p->sym->n]) {
			return false;
		}
		if (p->typ == Node::alt && !IsTerm(p->sub, mark) &&
			(p->down == NULL || !IsTerm(p->down, mark))) {
			return false;
		}
		if (p->up) {
			break;
		}
		p = p->next;
	}
	return true;
}


bool Tab::AllNtToTerm() {
	bool changed, ok = true;
	BitArray mark(nonterminals.size());
	// a nonterminal is marked if it can be derived to terminal symbols
	do {
		changed = false;

		for (Symbol *sym : nonterminals) {
			if (!mark[sym->n] && IsTerm(sym->graph, mark)) {
				mark.Set(sym->n, true);
				changed = true;
			}
		}
	} while (changed);
	for (Symbol *sym : nonterminals) {
		if (!mark[sym->n]) {
			ok = false;
			++errors->count;
			printf("  %s cannot be derived to terminals\n", sym->name);
		}
	}
	return ok;
}

//---------------------------------------------------------------------
//  Cross reference list
//---------------------------------------------------------------------

void Tab::XRef() {
	SortedList *xref = new SortedList();
	// collect lines where symbols have been defined
	for (Symbol *sym : nonterminals) {
		std::vector<int> *list = (std::vector<int> *)(xref->Get(sym));
		if (list == NULL) {list = new std::vector<int>(); xref->Set(sym, list);}
		list->push_back(-sym->line);
	}
	// collect lines where symbols have been referenced
	for (Node *n : nodes) {
		if (n->typ == Node::t || n->typ == Node::wt || n->typ == Node::nt) {
			std::vector<int> *list = (std::vector<int> *)(xref->Get(n->sym));
			if (list == NULL) {list = new std::vector<int>(); xref->Set(n->sym, list);}
			list->push_back(n->line);
		}
	}
	// print cross reference list
	fprintf(trace, "\n");
	fprintf(trace, "Cross reference list:\n");
	fprintf(trace, "--------------------\n\n");

	for (int i = 0; i < xref->Count; ++i) {
		Symbol *sym = (Symbol*)(xref->GetKey(i));
		char *paddedName = Name(sym->name);
		fprintf(trace, "  %12s", paddedName);
		coco_string_delete(paddedName);
		std::vector<int> *list = (std::vector<int> *)(xref->Get(sym));
		int col = 14;
		for (int line: *list) {
			if (col + 5 > 80) {
				fprintf(trace, "\n");
				for (col = 1; col <= 14; col++) {
					fprintf(trace, " ");
				}
			}
			fprintf(trace, "%5d", line); col += 5;
		}
		fprintf(trace, "\n");
	}
	fprintf(trace, "\n\n");
}

void Tab::SetDDT(char const *s) {
	char* st = coco_string_create_upper(s);
	char ch;
	int len = coco_string_length(st);
	for (int i = 0; i < len; i++) {
		ch = st[i];
		if ('0' <= ch && ch <= '9') ddt[ch - L'0'] = true;
		else switch (ch) {
			case 'A' : ddt[DDT_TRACE_AUTOMATON]     = true; break;
			case 'F' : ddt[DDT_LIST_SETS]           = true; break;
			case 'G' : ddt[DDT_PRINT_SYNTAX_GRAPH]  = true; break;
			case 'I' : ddt[DDT_TRACE_COMP_FIRST]    = true; break;
			case 'J' : ddt[DDT_PRINT_ANY_SYNC_SETS] = true; break;
			case 'P' : ddt[DDP_PRINT_STATS]         = true; break;
			case 'S' : ddt[DDT_LIST_SYMBOL_TABLE]   = true; break;
			case 'X' : ddt[DDT_LIST_XREF_TABLE]     = true; break;
			default : break;
		}
	}
	coco_string_delete(st);
}


void Tab::SetOption(char const *s) {
	// example: $namespace=xxx
	//   index of '=' is 10 => nameLenght = 10
	//   start index of xxx = 11

	int nameLenght = coco_string_indexof(s, '=');
	int valueIndex = nameLenght + 1;

	std::string name(s, 0, nameLenght);
	std::string value(s + valueIndex);

	if ("$namespace" == name) {
		if (nsName.empty()) {
			nsName = value;
		}
	} else if ("$tokenPrefix" == name) {
		tokenPrefix = value;
	} else if ("$checkEOF" == name) {
		checkEOF = "true" == value;
	}
}


}; // namespace
