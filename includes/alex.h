#ifndef PARSER_DEFINED
#define PARSER_DEFINED

#include <wchar.h>
#include <iostream>
//#include <vector>

#include "bitsets.h"

using namespace std;

typedef wchar_t 		ACHAR;
typedef unsigned long	TOKEN;
typedef unsigned long	ulong;
//typedef vector<wstring> stringlist;

#define NFA_STATES_ALLOC 1024

UNIT calcInputBufferSize( wstring );
//stringlist *getLargestFilesize( wstring );


union SYMBOL {
	UNICODEPOINT cp;
	Uniset  *cset;
};

enum trans_type { tt_empty=0, tt_char, tt_charset, tt_byte, tt_word, tt_dword, tt_accept, tt_error }; /**< empty trans., ACHAR, set, binary vals, accept, error */
const int NOTRANS = 0;

struct NFA_Error {
	NFA_Error( const wchar_t *s ){ wcout << s << endl; };
};


struct Nfastate	{  /**< node of Nfa directed graph. At most one of the links, always link1, is non-empty (type != tt_empty) */
	unsigned	link1; /**< index of next nfastate or NOTRANS */
	unsigned	link2; /**< next state, is either NOTRANS (0) or an empty transition (not 0) */
	SYMBOL		symbol;
	trans_type	type;
	bool        accepting; /**< true if accepting state, false otherwise */
	TOKEN		token; /**< token val of nfa under construction */
};

struct SubNfa {  /**< subgraph, to be merged with Nfa */
	unsigned 	init_state;
	unsigned	final_state;
};

class Nfa {
private:
	Nfastate    *states;
	unsigned	initial_state; /**< initial state of this graph */
	unsigned    n_of_states;
	unsigned    states_allocated;
public:
	Nfa();
	~Nfa();
	Nfastate *getstate( const unsigned i ); /**< fetch a ref to an already created state. range check */
	unsigned newstate( const Nfastate& ); /**< create new state and copy the rhs into it, rtn index. May realloc the array */
	unsigned newstate(); /**< create new state (zero initialized), return its index. May realloc. */
	unsigned initial() const { return initial_state; }; /**< get the (index of) the initial state of the NFA. Will change as construction progresses */
	void initial( const unsigned i ){ initial_state = i; }; /**< set the initial state */
};




#endif // PARSER_DEFINED
