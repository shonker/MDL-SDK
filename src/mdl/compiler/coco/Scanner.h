/*----------------------------------------------------------------------
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
-----------------------------------------------------------------------*/


#if !defined(Coco_COCO_SCANNER_H__)
#define Coco_COCO_SCANNER_H__

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// io.h and fcntl are used to ensure binary read from streams on windows
#if _MSC_VER >= 1300
#include <io.h>
#include <fcntl.h>
#endif

#if _MSC_VER >= 1400
#define coco_snprintf sprintf_s
#elif _MSC_VER >= 1300
#define coco_snprintf _snprintf
#elif defined __MINGW32__
#define coco_snprintf _snprintf
#else
// assume every other compiler knows snprintf
#define coco_snprintf snprintf
#endif

#define COCO_UNICODE_MAX 0x10FFFF
#define COCO_MIN_BUFFER_LENGTH 1024
#define COCO_MAX_BUFFER_LENGTH (64*COCO_MIN_BUFFER_LENGTH)
#define COCO_HEAP_BLOCK_SIZE (64*1024)
#define COCO_CPP_NAMESPACE_SEPARATOR ':'

namespace Coco {


// string handling, utf-8 character
char* coco_string_create(const char *value);
char* coco_string_create(const char *value, int startIndex);
char* coco_string_create(const char *value, int startIndex, int length);
char* coco_string_create_upper(const char* data);
char* coco_string_create_lower(const char* data);
char* coco_string_create_lower(const char* data, int startIndex, int dataLen);
char* coco_string_create_append(const char* data1, const char* data2);
char* coco_string_create_append(const char* data, const char value);
void  coco_string_delete(char const *&data);
void  coco_string_delete(char *&data);
int   coco_string_length(const char* data);
bool  coco_string_endswith(const char* data, const char *value);
int   coco_string_indexof(const char* data, const char value);
int   coco_string_lastindexof(const char* data, const char value);
void  coco_string_merge(char* &data, const char* value);
unsigned coco_string_hash(char const *data);
unsigned coco_string_hash(char const *data, size_t len);

class Errors {
public:
	int count;			// number of errors detected
	char const *srcName;		// source file name if set

	Errors();
	void SynErr(int line, int col, int n);
	void Error(int line, int col, const char *s);
	void Warning(int line, int col, const char *s);
	void Warning(const char *s);
	void Exception(const char *s);

}; // Errors

class Token
{
public:
	int kind;     // token kind
	int pos;      // token position in bytes in the source text (starting at 0)
	int charPos;  // token position in characters in the source text (starting at 0)
	int col;      // token column (starting at 1)
	int line;     // token line (starting at 1)
	char *val;    // token value
	Token *next;  // ML 2005-03-11 Peek tokens are kept in linked list

	Token();
	~Token();
};

class Buffer {
// This Buffer supports the following cases:
// 1) seekable stream (file)
//    a) whole stream in buffer
//    b) part of stream in buffer
// 2) non seekable stream (network, console)
private:
	unsigned char *buf; // input buffer
	int bufCapacity;    // capacity of buf
	int bufStart;       // position of first byte in buffer relative to input stream
	int bufLen;         // length of buffer
	int fileLen;        // length of input stream (may change if the stream is no file)
	int bufPos;         // current position in buffer
	FILE* stream;       // input stream (seekable)
	bool isUserStream;  // was the stream opened by the user?
	
	int ReadNextStreamChunk();
	bool CanSeek();     // true if stream can be seeked otherwise false
	
public:
	static const int EoF = COCO_UNICODE_MAX + 1;

	Buffer(FILE* s, bool isUserStream);
	Buffer(const unsigned char* buf, int len);
	Buffer(Buffer *b);
	virtual ~Buffer();
	
	virtual void Close();
	virtual int Read();
	virtual int Peek();
	virtual int GetPos();
	virtual void SetPos(int value);
};

class UTF8Buffer : public Buffer {
public:
	UTF8Buffer(Buffer *b) : Buffer(b) {};
	virtual int Read();
};

//-----------------------------------------------------------------------------------
// StartStates  -- maps characters to start states of tokens
//-----------------------------------------------------------------------------------
class StartStates {
private:
	class Elem {
	public:
		int key, val;
		Elem *next;

		Elem(int key, int val) : key(key), val(val), next(NULL) {}
	};

	Elem **tab;

public:
	StartStates() : tab(new Elem*[128]) { memset(tab, 0, 128 * sizeof(Elem*)); }
	~StartStates() {
		for (int i = 0; i < 128; ++i) {
			Elem *e = tab[i];
			while (e != NULL) {
				Elem *next = e->next;
				delete e;
				e = next;
			}
		}
		delete [] tab;
	}

	void set(int key, int val) {
		Elem *e = new Elem(key, val);
		int k = ((unsigned int) key) % 128;
		e->next = tab[k]; tab[k] = e;
	}

	int state(int key) {
		Elem *e = tab[((unsigned int) key) % 128];
		while (e != NULL && e->key != key) e = e->next;
		return e == NULL ? 0 : e->val;
	}
};

//-------------------------------------------------------------------------------------------
// KeywordMap  -- maps strings to integers (identifiers to keyword kinds)
//-------------------------------------------------------------------------------------------
class KeywordMap {
private:
	class Elem {
	public:
		char const *key;
		int len;
		int val;
		Elem *next;

		Elem(char const *key, int val)
			: key(coco_string_create(key))
			, len(coco_string_length(key))
			, val(val)
			, next(NULL)
		{}
		~Elem() { coco_string_delete(key); }
	};

	Elem **tab;

public:
	KeywordMap() : tab(new Elem*[128]) { memset(tab, 0, 128 * sizeof(Elem*)); }
	~KeywordMap() {
		for (int i = 0; i < 128; ++i) {
			Elem *e = tab[i];
			while (e != NULL) {
				Elem *next = e->next;
				delete e;
				e = next;
			}
		}
		delete [] tab;
	}

	void set(char const *key, int val) {
		Elem *e = new Elem(key, val);
		int k = coco_string_hash(key) % 128;
		e->next = tab[k]; tab[k] = e;
	}

	int get(int len, char const *key, int defaultVal) {
		Elem *e = tab[coco_string_hash(key, len) % 128];
		while (e != NULL && (len != e->len || strncmp(e->key, key, len) != 0)) {
			e = e->next;
		}
		return e == NULL ? defaultVal : e->val;
	}
};

class Scanner {
private:
	void *firstHeap;
	void *heap;
	void *heapTop;
	void **heapEnd;

	unsigned char EOL;
	int eofSym;
	int noSym;
	int maxT;
	StartStates start;
	KeywordMap keywords;

	Token *t;         // current token
	char *tval;       // text of current token
	int tvalLength;   // length of text of current token
	int tlen;         // length of current token

	Token *tokens;    // list of tokens already peeked (first token is a dummy)
	Token *pt;        // current peek token

	int ch;           // current input character

	int pos;          // byte position of current character
	int charPos;      // position by unicode characters starting with 0
	int line;         // line number of current character
	int col;          // column number of current character
	int oldEols;      // EOLs that appeared in a comment;

	void CreateHeapBlock();
	Token* CreateToken();
	void AppendVal(Token *t);
	void SetScannerBehindT();
	void UndeterminedComment(int line, int col);

	void Init();
	void NextCh();
	void AddCh();
	bool Comment0();
	bool Comment1();

	Token* NextToken();

public:
	Buffer *buffer;   // scanner buffer
	Errors *errors;
	
	Scanner(const unsigned char* buf, int len);
	Scanner(const char* fileName);
	Scanner(FILE* s);
	~Scanner();
	Token* Scan();
	Token* Peek();
	void ResetPeek();

}; // end Scanner

} // namespace


#endif

