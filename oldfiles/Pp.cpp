/*	=============================================== */
/*	PP.C : pattern parser door J.Bakker 1-Mei-1992	*/
/* nieuwe versie 14-3-2001 voor bcb3/windows       */
/* unicode versie pp.cpp v.a. 10-6-2007 voor cbuilder 2007 */
/*	=============================================== */

/*	 INCLUDES :	*/

#include <tchar.h>
#include <wchar.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "Main.h"
#include "stack.h"
/*	 DEFINES :	*/
#include "definitions.h"

#define		BSIZE	16
#define		MAXLEN	65534  /* 65534 max. aantal bits (-1 error) */
#define        BIT     1
#define        STACKSIZE 4096

/*	 GLOBAL VARIABLES : */
//SETPTR letter::charsets[ 256 ]; // local to letter
FILE *PatFile; /* test input file */
//FILE *OutFile;
TStringList *OutFile;
unsigned NumOfCharsets = SETS;
unsigned CountNum = COUNTNUM;
int SpaceCounts = 0;
unsigned short NonBol;
NFASTATE **Nfastates;
	/* first allocated by 'init_nfa()'. index = state nr.
	   In NFASTATE[ 0 ], 'symbol' contains the initial state of the NFA.
	   'link1' contains the nr of NFASTATE *'s allocated,
	   'link2' the nr of NFASTATES so far allocated. See new_state()
	   'token' has the P_EOL of the new parstab.
	*/

extern ACHAR *SavePtr;
unsigned long *CharMap;
SETPTR All_Chars;
SETPTR *Charsets;		/* array of "Newsets", later entered in parstab */
SETPTR Newset;			/* holds charset under construction */
int Complement_set; 	/* if TRUE, reverse set (1's complement) */
int NewToken;			/* lex-token of parsed expr */
int Substring;			/* substring of search result ($1 etc ) */

COUNTFIELD **Counters;
int *Destin;

/*	 ALEX & PARSER :  */

ATYPE Lexval; /* global return parameter from alex() */

/* parser state variables: */
ATYPE Astack[ PGSTACKSIZE ]; /* contains synthesized attribute (user def) */
int Pstack[ PGSTACKSIZE ];	 /* contains parser STATE (index to Pactions[]) */
int Stackptr;				 /* same stackptr for both stacks */

int DummyRun;				 /* flag: exec P_ATTR() during parse */
int Trace=0;					 /* if TRUE, print parser info while parsing */
int IgnoreCase;             /* flag: change all pattern chars to lowercase */
				 /* MACROS for reg. expr.: common to all */
ACHAR **Macros;  /* macro body defs. */
ACHAR **M_names; /* macro names */
int Mac_found;	 /* search_macro() return parameter */

struct {					 /* Mstack: pulled by gettext(), */
	ACHAR		*text;		 /* pushed by ExpandMacro() */
	ACHAR		*buf;
	} Mstack[ MACDEPTH+1 ];

int Mcount = 0;              /* Mstack stackpointer */

NFA **result;
NFA *savenfa;
DFA **dfa;

/*	LEX/PGTAB : #include files from pm/pg	*/

#include "lextab1.h"
#include "pgtab1.c"


/*			PARSTAB : static REGEX decl.			*/

PARSTAB Regex = {	/* DATA IN PGTAB1.C EN LEXTAB1.H (from rxp.p) */
	NULL,			   /* P_TEXT */
	NULL,              /* P_BUF */
	0,                 /* P_LEN */
	DEFAULT_EOL,	   /* P_EOL */
	Pactions1,         /* P_ACTIONS */
	Pgoto1,            /* P_GOTO */
	Ptab1,             /* P_TAB */
	ERRTOKEN1,         /* P_ERRTOKEN */
	0,                 /* P_GOT */
	0,                 /* P_ERROR (not used anymore)*/
	Pattributes1,      /* P_ATTR */
	gettext,           /* P_GET */
	Alex1,             /* P_ALEX */
	Transitions1,      /* P_TRANSITIONS */
	NULL,              /* firsts */
	NULL,              /* lasts */
	Tokens1,           /* P_TOKENS */
	NULL,              /* P_PATNAMES */
	NULL,              /* P_NONBOLS */
	NULL,              /* P_OUTPUT */
	NULL,              /* P_PATTERNS */
	0,                 /* P_NUM */
	Setarray1,         /* P_CHARSETS */
	NULL               /* P_COUNTERS */
	};

	PARSTAB *Parstab[ MAXTABS ] = { &Regex, NULL, NULL };

int Parserror;

 /* BITARRAYS */
 /* defs in definitions.h */

void bcopy(
ACHAR *ptr,
ACHAR *txtptr,
int len )
{
	while( --len >= 0 )  *ptr++ = *txtptr++;
	*ptr = '\0';
}

SETPTR createset( unsigned short s )  /* allocate & clear 's' bits */
{   /* formaat: 1 word (bit aantal) + s/16+1 bytes */
	SETPTR m;

   m = (unsigned short *)calloc( 2, s / BSIZE + 2 );
	*(short *)m = s;
	return( m );
}


int b_enter( SETPTR b, unsigned short n )  /* zet bit nr n in array b */
{          /* return 1 if bit was set before, 0 if it wasn't */
	short int *ptr;
	short int mask, ret;

	ptr = (short *)b + 1 + n / BSIZE;
	mask = BIT << n % BSIZE;
	ret = (( *ptr & mask ) != FALSE );
	*ptr |= mask;
	return( ret );
}


int b_delete( SETPTR b, short unsigned n )  /* clear bit  n in array b */
{          /* return 1 if bit was set before, 0 if it wasn't */
	short int *ptr;
	short int mask, ret;

    ptr = (short *)b + 1 + n / BSIZE;
	mask = BIT << n % BSIZE;
    ret = (( *ptr & mask ) != FALSE );
    *ptr &= (~mask);
	return( ret );
}


unsigned b_in( SETPTR b, short unsigned n )  /* test bit n in array b */
{
	return(( *( b + 1 + n / BSIZE ) & (BIT << (n % BSIZE ))) != FALSE );
}


unsigned b_equal( SETPTR x, SETPTR y )  /* TRUE if array x == array y */
{
	short unsigned m, n;

	m = *x / BSIZE + 1;
	for ( n = 1; n <= m; n++ )
		if ( *( x + n ) != *( y + n )) return( FALSE );
	return( TRUE );
}

int b_assign( SETPTR x, SETPTR y )  /* array x = array y, return -1 if error */
{
    short unsigned n;

	if ( ( n = *x++ ) != *y++ ) return( TERROR );
	n = n / BSIZE + 1;
	while ( n-- != 0 ) *x++ = *y++;
	return ( FALSE );
}


int b_union( SETPTR x, SETPTR y )  /* array x = x OR y, return -1 if error */
{
	short unsigned n;

	if ( ( n = *x++ ) != *y++ ) return( TERROR );
	n = n / BSIZE + 1;
	while ( n-- != 0 ) *x++ |= *y++;
	return ( FALSE );
}
int b_and( SETPTR x, SETPTR y )  /* array x = x AND y, return -1 if error */
{
	short unsigned n;

	if ( ( n = *x++ ) != *y++ ) return( TERROR );
	n = n / BSIZE + 1;
	while ( n-- != 0 ) *x++ &= *y++;
	return ( FALSE );
}

int b_xor( SETPTR x, SETPTR y )  /* arr x = x XOR y, return -1 if error */
{
	short unsigned n;

	if ( ( n = *x++ ) != *y++ ) return( TERROR );
	n = n / BSIZE + 1;
	while ( n-- != 0 ) *x++ ^= *y++;
	return ( FALSE );
}


void b_complement( SETPTR x )  /* array x = NOT x, return -1 if error */
{
	short unsigned n;

	n = *x++ / BSIZE + 1;
	while ( n-- != 0 ) *x++ ^= 0xffff;
}

void b_clear( SETPTR x )  /* array x = EMPTY, return -1 if error */
{
     short unsigned n;

	n = *x++ / BSIZE + 1;
	while ( n-- != 0 ) *x++ = 0;
}

int b_next( SETPTR b, short unsigned i )  /* retn index van 1e bit vanaf i */
{   /* caller moet index verhogen om volgende te krijgen */
	for ( ;i <= *b; i++ )
		if ( b_in( b, i ))
			return( i );
	return( TERROR );
}

int b_empty( SETPTR x )  /* retn TRUE if empty, FALSE otherwise */
{              //  mistake! but it's not in use
    short unsigned n;

	n = *x++ / BSIZE + 1;
	while ( n-- != 0 ) if ( *x++ ) return( TRUE );
	return( FALSE );
}

void printarr( SETPTR a )
{    //doesn't print widechars
	int i=0;
	FILE *file;

	file = _wfopen( L"tabfile.txt", L"a" );
	while ( ( i = b_next( a, i )) != TERROR )
		fwprintf( (FILE *)file, L"%ld ", i++ );
	fwprintf( (FILE *)file, L"\n" );
    fclose( file );
}
								/* REGEX, SYNTAX, USERTAB1 */

/*unsigned long nextbit( deque<bool>& set, unsigned long index )
{   // deze ALLEEN VOOR nfastates
	unsigned long size;

	size = set.size();
	for( ; index < size; ++index )
	{ if( set[ index ]) return( index ); }
	return( -1L );
} */

void *myalloc(
unsigned long num,
unsigned long size )
{
	unsigned long * result;
	unsigned long m, n;

	m = num * size;
	n = m % 4;
	if( n != 0 ) m += (4 - n);  // align 32 bit
	result = (unsigned long *)malloc( m );
	m /= 4;
	for( n=0; n < m; ++n ) result[ n ] = 0L;
	return (void *)result;
}
/*	======== ALEX ================================= */

/*	 GETTEXT : read char-by-char from buffer		*/

ACHAR gettext(
PARSTAB *parstab )
{
	ACHAR c;
    unsigned long tmp;
	/* global Mstack[], Mcount */

	while( (c = (parstab->lex)[ (parstab->len) ]) == (ACHAR)(Mcount > 0 ? 0 : P_EOL ) )
	{
		if( Mcount > 0 )  // Mstack: stack of macro texts
		{
			--Mcount;
			(parstab->len) = 0;
			(parstab->lex) = Mstack[ Mcount ].text;
			(parstab->buf)  = Mstack[ Mcount ].buf;
		}
		else
		{
			return( EOL );
		}
	}
   if( IgnoreCase != 0 )            
   {
        tmp = CharMap[ c ]; // only 16-bit codepoints are loaded!
        if(tmp & 0xC0000000) c = (ACHAR)tmp;
   }
   return( c ); /* return char */
}

/*	 ALEX1 : get REGEX lexeme, #incl. lexact1.h	*/

int Alex1(   /* will be called by the regex parser */
PARSTAB *parstab )
{
	static unsigned token;

	if( P_GOT == TRUE )	/* return buffered token */
	{
		P_GOT = FALSE;
		return( token );
	}

	while ( 1 )
	{
		(parstab->lex) += (parstab->len);
		(parstab->len) = 0;
		switch( token = lexin( parstab, 1 ) )
		{
			default:
				break; /* do nothing */

			case EOL:
				return( EOL );
			case NOTHING:
				return( NOTHING );/* lexical error */

			#include "lexact1.h"  /* enter the userdef "lexical actions" */

		}
	}
}

/*	 ALEX2 : parse PG input, #include lexact2.h	. for future use*/

int Alex2(  /* will be called by the parser generator */
PARSTAB *parstab )
{
	static unsigned token;

	if( P_GOT == TRUE )	/* return buffered token */
	{
		P_GOT = FALSE;
		return( token );
	}

	while ( 1 )
	{
		(parstab->lex) += (parstab->len);
		(parstab->len) = 0;
		switch( token = lexin( parstab, 1 ) )
		{
			default:
				break; /* do nothing */

			case EOL:
				return( EOL );

			case NOTHING:
				return( NOTHING );/* lexical error */

			/*#include <lexact2.h>*/  /* enter the userdef "lexical actions" */

		}
	}
}

/*	 MATCH : match dfastate-gettext() chars		*/

int match(
PARSTAB *parstab,
unsigned long c )		/* char in transitions[ curr_state ] */
{
	unsigned long t;	/* text char returned by P_GET() */

	t = (*P_GET)( parstab );
	if( t == EOL ) return( EOL );
	else if( c & 256 )
	{
		if( t > 255 ) 
		{ 
			if( c != 267 /* anychar */ ) return FALSE;
			else { ++(parstab->len); return 1; }
		}
		if( !b_in( P_CHARSETS[ c & 0xff ], t )) return( FALSE );
	}
	else if( c != t )
	{
		return( 0 ); /* no match */
	}

	++(parstab->len);  /* increase p_len only if matched */

	return( 1 );
}  /* returns EOL if end-of-string, TRUE if matched,FALSE otherwise */

/*	 LEXIN : get a token							*/
/* get the longest possible lexeme, (may be empty) */
int lexin(
PARSTAB *parstab,
int curr_state ) /* curr_state: starting state of DFA (curr_state>0) */
{
	unsigned long *trans;			/* ptr to transitions of curr state */
	int matched;                    /* TRUE if a tr.char matched an input c. */
	int accept, accend;    /* last valid token, p_len of  a lexeme */

	/* Transitions[] : array of ptrs to transitions */
	/* *(Transitions[m]+n) n = even: transition char */
	/* *(Transitions[m]+n+1) next state ( 1..end )	if -1(NOTRANS):
					*(Transitions[m]+n)= token	*/
	/* No token: NOTHING */
	/* TOKENS[] array of tokens indexed by state */

	if( (*P_GET)( parstab ) == EOL ) return( EOL );
	trans = (parstab->tr_tab)[ curr_state ];

	accend = (parstab->len);					/* curr ptr to end of lexeme */
	accept = (parstab->tokens)[ curr_state ];	/* this state's token, if any */

	/* shift until no longer possible: */
   while( *( trans + 1 ) != (unsigned)NOTRANS )
   {
	   if(( matched = match( parstab, *trans )) == TRUE )
	   {	/* shift to nxt state */
		   curr_state = *( trans + 1 ); /* next state */
		   if( (parstab->tokens)[ curr_state ] != NOTHING )
		   {	/* accepted so far,if any */
			   accept = (parstab->tokens)[ curr_state ];
			   accend = (parstab->len);
		   }
		   trans = (parstab->tr_tab)[ curr_state ];	/* ptr to state */
		   continue;
	   }
	   else if( matched == FALSE ) trans += 2; /* next in same state */
	   else /* EOL: end of string */
	   {
		   (parstab->len) = accend;
		   //accept &= 0x80ff;
		   return( accept );
	   }
   }	/* END WHILE( 1 ) */

	if( *trans != (unsigned short)NOTHING )
	{
		accept = *trans; /* return token, keep 'p_len' */
	}
	else /* found NOTHING */
	{
		(parstab->len) = accend;	/* reset to last p_len found, keep 'accept' */
	}
		/* filter out flag if returning to Alex(): */
	//accept &= 0x81ff;
	return( accept );
	/* return previously accepted token ( possibly NOTHING ) */
}

/*	 UNGETTOKEN : push token back 				*/

void Ungettoken(
PARSTAB *parstab )
{
	P_GOT = TRUE;
}

/*	======= PARSER ================================ */

/*	 GET_ACTION : search parser state list		*/

int get_action(
PARSTAB *parstab,
int state,
int token )
{
	unsigned long *statelist, sym;

	statelist = P_ACTIONS[ state ];
	for( ; ( state = *(statelist+1) ) != TERROR; statelist += 2 )
	  if(( sym = *statelist ) == (unsigned)token || sym == ANY ) break;
	return( state );
}

/*	 NEXTTOKEN : parser calls Alex() for next tk	*/

int nexttoken(
PARSTAB *parstab )
{
	int token;

	token = (*P_ALEX)( parstab );
	if( token == EOL )
	{
	  token = 0;
	}
	else if( token == NOTHING )
	{
		Ungettoken( parstab );
		while(( token = (*P_ALEX)( parstab ) ) == NOTHING )
		{
			(parstab->len) = 0;
			++(parstab->lex);		/* lexical error( unknown char): skip */
		}
		Ungettoken( parstab );
		token = P_ERRTOKEN;
	}
	/*else if( (parstab->len) == 0 )       kan niet gebeuren!?
       fatal_err( "empty token!" ); */
	return( token );
}

/*	 REDUCE : Reduce & execute user actions		*/

int reduce(
PARSTAB *parstab,
int state )
{
	int *statelist, *ptr, newstate, nterm;
	/* global Pstack, Stackptr, DummyRun */

	ptr = P_TAB + 2 * state;
	if(( nterm = *ptr ) == TERROR ) return( TERROR ); /* can't be !? */
	Stackptr -= *( ptr + 1 ); /* production length */

	if( !DummyRun ) (*P_ATTR)( state ); /* attribute actions */

	/* search in Pgoto */
   state = Pstack[ Stackptr++ ];
	statelist = P_GOTO[ nterm ];
	for( ; ( newstate = *statelist ) != ANY; statelist += 2 )
		if( state == newstate ) break;
	Pstack[ Stackptr ] = *( statelist + 1 );
	return( 0 );
}

/*	 PARSE : parse an input string				*/

ATYPE *parse(
PARSTAB *parstab ) /* REGEX, SYNTAX, USERTAB1 */
{
	unsigned token; /* current lexical token */
    WideString s;
	int target; /* target = target state of shift action */
	int currstate; /* current state on top of stack */
	int errshift;  /* flag to make parser "skip until shift" after error */
	int stackbottom; /* remember stackbottom, in case of re-entrant parsing (future use)*/
	/* global Astack, Pstack, Stackptr, Trace,
	   globals used by P_ATTR() and P_ALEX() */

	Parserror = PARSE_OK;
	errshift = FALSE;
	stackbottom = Stackptr; /* for error checking */
	Pstack[ 0 ] = 1;
	while( 1 )
	{
	  currstate = Pstack[ Stackptr ];
	  token = nexttoken( parstab );
	  if( Parserror != PARSE_OK ) break;
	  if( token == P_ERRTOKEN )
	  {
		target = TERROR;
		Parserror = LEXERR;
	  }
	  else target = get_action( parstab, currstate, token );
	  /* search token in list for that state */

	  if( Trace )
	  {
		//fprintf( OutFile, "Stackptr %d, state %d, ", Stackptr, currstate );
        s = WideString( "Stackptr " ) + WideString( Stackptr ) +
              WideString( ", " ) + WideString( currstate );
    	OutFile->Add( s );
		if( target & REDUCT ) {
		  //fprintf( OutFile, "R %d, " , target&0x7fff );
            s = WideString( "R " ) + WideString( target&0x7fff ) +
                  WideString( ", " );
            OutFile->Add( s );
        }
		else {
		  //fprintf( OutFile, "S %d, " , target );
            s = WideString( "S " ) + WideString( target&0x7fff ) +
                  WideString( ", " );
            OutFile->Add( s );
        }

		//fprintf( OutFile, "token %d\n", token );
        s = WideString( "token " ) + WideString( token ) +
              WideString( "\n" );
        OutFile->Add( s );
	  }

	  if( target == ACCEPT )
	  {
		if( token == 0 ) break; /*** OK OUT ***/
		else
		{
		  target = TERROR;
		}
	  }
	  if( target == TERROR )
	  {
		if( !Parserror ) Parserror = SYNTERR;
		if( errshift ) continue; /* error found, but no shift yet */
		while(( target = get_action( parstab,currstate,P_ERRTOKEN )) & REDUCT )
		{ /* find a state with shift on Errtoken: (TERROR also & RED.) */
		  if( Stackptr == stackbottom ) break; /* give up */
		  currstate = Pstack[ --Stackptr ];
		}
		if( Stackptr == stackbottom ) break;
		currstate = target & ~REDUCT; /* target state is new current state */
		Pstack[ ++Stackptr ] = currstate; /* shift virtual ERRTOKEN */

		while(( target = get_action( parstab, currstate, token )) == TERROR )
		{
		  token = nexttoken( parstab );
		}
		if( target & REDUCT )
		  errshift = TRUE; /* skip input tokens until a shift is found */
		else
		  errshift = FALSE;
	  }
	  if( target & REDUCT ) /* reduce */
	  {
		Ungettoken( parstab );
		if( reduce( parstab, target & ~REDUCT ) == TERROR )
		{		/* target minus REDUCT = production number index */
			Parserror = TABERR; /* should never happen */
		}
	  }
	  else /* shift */
	  {
		errshift = FALSE;
		Pstack[ ++Stackptr ] = target;
		Astack[ Stackptr ].p = Lexval.p; /* default action */
	  }
	  if( Stackptr >= PGSTACKSIZE || Stackptr < stackbottom )
	  {
		Parserror = STACKERR;
		break;
	  }
	}
	Stackptr = stackbottom; /* ought to be redundant */
	return( &S_S ); /* return ptr to 'SS.p'. Valid if Parserror == PARSE_OK */
}

/*	======= REGEX PARSER ATTRIBUTES: ============== */

/*	 BACKSLASH :		get backslashed char		*/
unsigned backslash(
ACHAR *s )
{
	unsigned c;

	c = *s;

	switch ( c )
	{
		case 's': c = ' ';    break;	/* SPACE */
		case 't': c = '\t';   break;	/* TAB	 */
		case 'r': c = '\x0d'; break;	/* CR	 */
		case 'n': c = '\x0a'; break;	/* LF	 */
		case 'f': c = '\x0c'; break;	/* FF	 */
	}
	return( c );
}
/*	 GETHEX : 		hex --> unsigned			*/

unsigned long gethex(
ACHAR *s )
{
	unsigned long x, hex;

	hex = 0;
	while( 1 )
	{
		x = *s;

		if( x >= 'a' && x <= 'f' )
					 hex = x - 'a' + 10 + hex * 16;
		else if( x >= 'A' && x <= 'F' )
					 hex = x - 'A' + 10 + hex * 16;
		else if( x >= '0' && x <= '9' )
					 hex = x - '0' + hex * 16;
		else break;

		++s;
	}
	return( hex );
}

/*	 GETNUM : 		dec --> unsigned			*/

unsigned getnum(  /* convert decimal -> int */
ACHAR *s )
{
	unsigned n;

	n = 0;
	for ( ; isdigit( *s ); ++s )
		n = 10 * n + *s - '0';
	return( n );
}

/*	 GETOCT : 		oct --> unsigned			*/

unsigned getoct(  /* convert octal -> int */
ACHAR *s )
{
	unsigned n;

	n = 0;
	for ( ; isdigit( *s ); ++s )
		n = 8 * n + *s - '0';
	return( n );
}

/*	 INIT_NFA :		initialize nfastates[]		*/

NFASTATE **Init_nfa(  /* init an array of ptrs to nfastates. */
unsigned init_size ) /*	The states themselves are not allocated except [ 0 ] */
{
	NFASTATE **nfastates;

	nfastates = (NFASTATE **)calloc( init_size + 1, sizeof( NFASTATE * ));
	nfastates[ 0 ] = (NFASTATE *)calloc( 1, sizeof( NFASTATE ));
	nfa_MAX = init_size;	/* max nr of subscr */
	nfa_USED = 0;			/* last one allocated (this one) */
	nfa_EOL = DEFAULT_EOL;
	return( nfastates );
}

/*	 NEW_STATE :		return nfastate index		*/

/*	returns the index of a Nfastate-struct. in array nfastates[],
	which must have been initialized: size in nfastates[ 0 ]->link1,
	index of last nfastate created in link2.
	First it tries to find a state that has been free()'d, marked by
	a NULL-pointer in nfastates[]. If not found, it creates a new state.
	If it grows larger than current maxsize, nfastates[] is reallocated.
*/
unsigned new_state( void )
{
	NFASTATE **nfastates;
	unsigned n, last;
	unsigned max;
	/* global Nfastates[] */

	nfastates = Nfastates;
	max = nfa_MAX;   /* current size of nfastates[] array */
	last = nfa_USED;  /* highest state index so far */
	for( n = 1; n < last; ++n )
	{
	  if( nfastates[ n ] == NULL ) break;
	}
	if( n >= last ) /* no null (free()'d nfastate) found, alloc new one */
	{
		if( ++last >= max )
		{
			max += GROWSIZE;			/* max reached: make array larger */
			Nfastates = nfastates =
				(sttype **)realloc( Nfastates,max * sizeof( NFASTATE * ));
			nfa_MAX = max;
		}
		nfastates[ 0 ]->link2 = last + 1;  // nfa_USED
	}
	else
	{
		last = n;
	}
	nfastates[ last ] = (sttype *)calloc( 1, sizeof( NFASTATE ));
	nfastates[ last ]->token = NOTOKEN;
	nfastates[ last ]->symbol = NOTHING; /* set defaults */
	nfastates[ last ]->link1 = nfastates[ last ]->link2 = NOLINK;
	return( last );
}

/*	 FREE_STATE : 	dealloc 1 or all nfastates	*/

void free_state( /* deallocate nfastates */
NFASTATE **nfastates,
int index )		/* if index == ALL, free all, else free Nfastate[index] */
{
	unsigned last, n;

	if( index == ALL )
	{
	  last = nfa_USED; 	/* last one allocated */
	  for( n=0; n <= last; ++n )
	  {
		if( nfastates[ n ] != NULL ) free( nfastates[ n ] );
	  }
	  free( nfastates );				/* ptr array */
	}
	else
	{
	  free( nfastates[ index ] );
	  nfastates[ index ] = NULL;		/* mark it as 'free' */
	}
}

/*	 ADDTOSET :		add a char(range) to set	*/

void AddToSet(
SETPTR set,
unsigned min,
unsigned max )
{
	for( ; min <= max; ++min )
	{
		b_enter( set, min );
	}
}

/*	 NEW_CHARSET :	add to Charsets[],rtn token */

unsigned long new_charset(  /* return charset sym */
SETPTR set )	/* set to be added */
{
	unsigned ix;
	SETPTR *charsets;
	/* global Charsets, Complement_set */

	if( Complement_set ) b_complement( set );
	Complement_set = FALSE;
	b_delete( set, '\0' );
	b_delete( set, '\r' );
	b_delete( set, '\n' );
//again:
	charsets = Charsets;
	for ( ix = 0; *charsets != NULL && ix < SETS; charsets++, ix++ )
	{
		if( b_equal( set, *charsets ))
		{
			free( set );
			break;
		}  /* SET exists already */
	}
//	if( ix >= NumOfCharsets )
//	{
//       NumOfCharsets *= 2;
//	   Charsets =
//		(unsigned short **)realloc( Charsets, NumOfCharsets * sizeof( SETPTR ));
//       goto again;
//	}
	if( *charsets == NULL ) *charsets = set;
	return( (ix | CHARSET) ); /* bit 31 wordt gezet tov single chars */
}

/*	 EXPAND MACRO :	redirect gettext()			*/

void ExpandMacro( 	/* push old Text pointers, set new ones */
PARSTAB *parstab )
{
	int i;
	/* global Mac_found, Mcount, Mstack[], Macros[] */

	if( Mcount < MACDEPTH )
	{
		i = search_macro( (parstab->lex)+1, (parstab->len)-1 ); /* pt. to 2nd char of '%ID' */
		if( Mac_found )
		{
			Mstack[ Mcount ].text =  (parstab->lex) + (parstab->len); /* begin of nxt lexeme */
			Mstack[ Mcount ].buf = (parstab->buf);	/* text buffer */
			++Mcount;						/* recursion counter */
			(parstab->len) = 0;						/* set to 0 by alex() */
			(parstab->lex) = (parstab->buf) = Macros[ i ];	/* set new Text buffer ptr */
		}
		else Parserror = MACERR1; /* NOT FOUND */
	}
	else
	{
		Parserror = MACERR2; /* NESTING TOO DEEP */
	}
}

/*	 SEARCH MACRO :	get macro body				*/

int search_macro(   /* in: ptr to lexeme, length */
ACHAR *line, 		  /* out: Mac_found==TRUE  , index of macro body */
int len )             /* or   Mac_found==FALSE , 1st free index */
{
	int n;
	ACHAR **names;
	/* global Mac_found, M_names[],  */

	names = M_names;
	Mac_found = FALSE;
	n = 0;
	while ( *names != NULL && n < MACSIZE )
	{
		if( !wcsncmp( *names, line, len ))
		{
			Mac_found = TRUE;
			break;
		}
		else
		{
			++n;
			++names;
		}
	}
	if( n >= MACSIZE )
       {Parserror = MACERR3; Mac_found = FALSE;}
	return( n );
}

/*	 DEFMACRO :		enter new macro def 		*/

void DefMacro(
ACHAR *text, /* ptr to "%identifier = macro-text\n" */
int len )             /* lexeme length */
{					 /* text pts to first letter of id. */
	int i;
	int mac;
	/* global Macros[], M_names[], Mac_found */

	i = 0;
	while ( isalnum( text[ i ] ) || text[ i ] == '_' ) ++i; /* measure length */
	mac = search_macro( text, len ); /* mac < MACSIZE */
    if( Parserror != PARSE_OK ) return;
	if( Mac_found )
	{
		free( Macros[ mac ] );
	}
	else
	{
		M_names[ mac ] = (ACHAR *)calloc( i + 1, sizeof( ACHAR ));
		bcopy( M_names[ mac ], text, i );
	}

	while( text[ i++ ] != '=' ); /* we know there is one */
	len -= i; /* subtr. length left of '=' */
	Macros[ mac ] = (ACHAR *)calloc( len + 1, sizeof( ACHAR ));
	bcopy( Macros[ mac ], text + i, len );
}

/*	 LEXOUTPUT :		enter replacement strings	*/

void Lexoutput(
PARSTAB *parstab,
int token )
{
	token &= 0xff; /* filter out flags */
	P_OUTPUT[ token ] = (ACHAR *)calloc( (parstab->len) - 1, sizeof( ACHAR ));
	bcopy( P_OUTPUT[ token ], (parstab->lex)+2, (parstab->len)-2 ); /* skip '%=' */
}

/*	 COUNTER :		create COUNTFIELD			*/

COUNTFIELD *Counter(
PARSTAB *parstab,
ACHAR *text ) /* ptr to "{ num }" or "{ num,num }" */
{
	unsigned lo, hi, i;
	COUNTFIELD *cfield;

	while( !isdigit( *text )) ++text; /* seek first number */
	lo = getnum( text );
	while( isdigit( *text )) ++text; /* skip first number */
	while( isspace( *text )) ++text; /* search ',number' or '}' */
	if( *text == ',' )
	{
		while( !isdigit( *text )) ++text; /* seek 2nd number */
		hi = getnum( text );
	}
	else hi = lo;
	if( lo > hi ) { i = lo; lo = hi; hi = i; }
	if( COUNTERS == NULL )
	{
		COUNTERS = (COUNTFIELD **)calloc( COUNTNUM+1, sizeof( COUNTFIELD * ));
	}
again:
	i = 0;
	while( COUNTERS[ i ] != NULL ) ++i;
	if( i >= CountNum )
	{
       CountNum *= 2;
	   COUNTERS =
		(COUNTFIELD **)realloc( COUNTERS, CountNum * sizeof( COUNTFIELD * ));
       goto again;
	}
	COUNTERS[ i ] = cfield = (COUNTFIELD *)calloc( 1, sizeof( COUNTFIELD ));
	cfield->low = lo;
	cfield->high = hi;
	return( cfield );
}

/*	 STATECOPY :		copy nfa & renumber links	*/

void statecopy(  /* copy states */
int first,	/* state to be copied if not already visited */
int final )	/* terminate recursion if first == final */
{
	NFASTATE *old_first, *new_first;
	int state, new1;

	if( Destin[ first ] == NOLINK ) /* if x already visited,.. */
	{								/* ..Destin[ x ] = index of copy of x */
		Destin[ first ] = new1 = new_state();
		old_first = Nfastates[ first ];
		new_first = Nfastates[ new1 ];
		if( first != final )
		{							/* in 'final' both links are 0 */
			if(( state = old_first->link1 ) > 0 )
			{
				statecopy( state, final );
				new_first->link1 = Destin[ state ];
			}
			if(( state = old_first->link2 ) > 0 )
			{
				statecopy( state, final );
				new_first->link2 = Destin[ state ];
			}
		}
		new_first->symbol = old_first->symbol;
		new_first->token = old_first->token;
	}
}

/************************************************************************/
/*			BUILD NFA													*/
/************************************************************************/
/*	build nfa in several modes:
   -MERGE: concats the previous nfa (nfa1) and a new one which repre-
	sents one input line, by creating a new init state which points at both.
	The new nfa does not have a single final state.
	(MERGE is called from outside the parser)
   -NEWLINE: marks final states (of nfa1) as Accepting states & writes Token.
   -NEWCHAR: creates a 2-state nfa repr. 1 char transition
   -CONCAT: create the concatenation of nfa1 and nfa2 by letting the last
	state of nfa1 point to the init state of nfa2.
   -ZERO_MORE, ONE_MORE, ZERO_ONE: * + ? closure of nfa in nfa1
   -UNIFY: 'inline' | - operator : new nfa = nfa1 OR nfa2. Creates a single
	final state.
   -STRING: any string of chars in the source text, surrounded by ' or ",
    are concatted into the nfa as they are.
   -SET_EOL: source {EOL} = c save runtime P_EOL char to nfastates[ 0 ]->token.
   -COUNTER: X{ min, max } = 0..min times X + min+1..max times (X or empty)
	Copies nfa1 max times to new nfastates, then sets links.

	'Build' works on a global array of Nfastates, each with two transition
	pointers and a transition symbol which may be empty (NOTHING).
	At most one of these ptrs, always link1, describes a non-empty transition.
	If a state is an accepting state, link1 has 'ACCEPTING' in it and link2 the
	token value ( Alex() return val ) of the regular expression.
*/

NFA *build(
NFA *nfa1,				/* nfa1, to be conc. or closed */
NFA *nfa2,				/* nfa2 to be merged with nfa1 */
int flag,			  	/* SWITCH case */
unsigned data )			/* contains character if NEWCHAR or LOOKAHEAD */
{
	NFASTATE *first_state, *final_state;	/* nieuwe nfastates */
	NFASTATE *old_final;
	int first, final; /* indexes of first,final state of new nfa */
	int temp;
	ACHAR lim, c, *cptr; /* string input help vars */
	NFA *newnfa; /* return ptr to NFA field = { int init; int final } */
	COUNTFIELD *cfield;
	int min, max, st_num, n, m;
	int veryfirst, verylast;
	/* uses global Nfastates[], NewToken, Destin[]
	   state->symbol defaults to NOTHING
	   state->token defaults to 0
	   state->link1&2 default to NOLINK (by newstate())
	   except in an accepting state, state->token is undef if symbol == NOTHING
	*/

	switch( flag )
	{
			case MERGE: /* nfa1=old nfa so far, 2=new nfa repr. 1 line */
			/* laat nwe init wijzen naar beide oude inits. Geen nwe final */

			if( nfa2 != NULL )
			{
				if( nfa1 == NULL )
				{
					nfa1 = nfa2;
					first = nfa1->init;
				}
				else
				{
				  first = new_state();
				  first_state = Nfastates[ first ];
				  first_state->link1 = nfa1->init;
				  first_state->link2 = nfa2->init;	/* ignore final states */
				  free( nfa2 );
				}
				NFA_INIT = nfa1->init = first;
			}
			newnfa = nfa1;
			break;
		case NEWLINE:	/* markeer final state van nfa1 met 'ACCEPTING' */

			if( nfa1 != NULL )
			{
				final_state = Nfastates[ nfa1->final ];
				final_state->link1 = ACCEPTING;/* mark final state accepting */
				final_state->link2 = NOLINK;
				final_state->symbol = NOTHING;
			   if( data == BOL ) final_state->token = NewToken | BOL;
				else final_state->token = NewToken;
			   /* token for lexical analysis */
			}
			newnfa = nfa1;
			break;
		case NEWCHAR:	/* create a state with a char transition */
			/* nwe init naar nwe final met transition symbol 'data' */

			first = new_state(); /* (index van) nwe init en .. */
			final = new_state(); /* ..nwe final state */

			first_state = Nfastates[ first ];
			first_state->symbol = data; /* transition sym */
			first_state->link1 = final;
			first_state->token = NewToken;

			final_state = Nfastates[ final ];
			final_state->link1 = LINKFLAG;/* flag to mark linkback */
			final_state->link2 = first; /* point back, save space in concat */

			if( data & CHARSET )
			{
				b_union( All_Chars, Charsets[ data & 0xFFFF ] );
			}
			else if( data < CHARSET )
			{
				b_enter( All_Chars, data );
			}

			newnfa = (NFA *)calloc( 1, sizeof( NFA ));
			newnfa->init = first;
			newnfa->final = final;
			break;
		case STRING:	/* enter string of new chars */
			/* while not end-of-string:
				 create new nfastate with next char in string
				 link to prev. state
			*/
			cptr = (ACHAR *)nfa1;
			lim = *cptr++;					/* delimiter " or ' */
			temp = NOLINK;
			first = final = new_state();
			while (( c = *cptr++ ) != lim )
			{
				b_enter( All_Chars, c );
				temp = final;
				final_state = Nfastates[ final ];
				final_state->symbol = (unsigned)c;
				final_state->token = NewToken;
				final = new_state();		/* next final state */
				final_state->link1 = final;
			} /* when done, 'first' pts to init, 'final' to final state,
				 and 'temp' points to the last but 1 */

			final_state = Nfastates[ final ];
			final_state->link1 = ( temp == NOLINK ? NOLINK : LINKFLAG );
			final_state->link2 = temp; /* temp == NOLINK if string empty */

			newnfa = (NFA *)calloc( 1, sizeof( NFA ));
			newnfa->init = first;
			newnfa->final = final;
			break;
		case CONCAT:	/* point end of first to beginning of next */
			/* geen nwe states. Point final1 naar init2. */
			/* verwijder final1 als deze door NEWCHAR of STRING is gemaakt */

			final = nfa1->final;
			first = nfa2->init;
			final_state = Nfastates[ final ];
			if( final_state->link1 == LINKFLAG )
			{			/* if there is an empty link state, shortcut it */
			  temp = final;
			  if(( final = final_state->link2 ) != NOLINK )
			  {
				final_state = Nfastates[ final ];
				free_state( Nfastates, temp );
			  }
			}
			final_state->link1 = first; /* set link */
			nfa1->final = nfa2->final;
			free( nfa2 );
			newnfa = nfa1;
			break;
		case ZERO_MORE: /* nul of meer maal current nfa */
			/* new-init naar old-init en naar new-final,
			   old-final naar new-final en naar old-init. */

			first = new_state(); /* index of new first... */
			final = new_state(); /* & final Nfastates */

			first_state = Nfastates[ first ]; /* point first to prev init */
			first_state->link1 = nfa1->init;  /* new-init to old-init */
			first_state->link2 = final;   /* 'empty' route to final state */

			final_state = Nfastates[ final ]; /* new final state */

			old_final = Nfastates[ nfa1->final ];
			old_final->link1 = final;		  /* to new final state */
			old_final->link2 = nfa1->init;	  /* back to old init state */
			old_final->symbol = NOTHING;

			newnfa = nfa1;
			newnfa->init = first;
			newnfa->final = final;
			break;
		case ONE_MORE:
			/* new-init naar old-init,
			   old-final naar new-final en naar old-init. */

			first = new_state();
			final = new_state();

			first_state = Nfastates[ first ];	/* point first to prev init */
			first_state->link1 = nfa1->init;

			final_state = Nfastates[ final ];

			old_final = Nfastates[ nfa1->final ];
			old_final->link1 = final; /* to new final state */
			old_final->link2 = nfa1->init; /* back to old init state */
			old_final->symbol = NOTHING; /* may contain link */

			newnfa = nfa1;
			newnfa->init = first;
			newnfa->final = final;
			break;
		case ZERO_ONE:
			/* new-init naar old-init en naar new-final,
			   old-final naar new-final. */

			first = new_state();
			final = new_state();

			first_state = Nfastates[ first ];	/* point first to prev init */
			first_state->link1 = nfa1->init; /* tenminste 1x */
			first_state->link2 = final; /* 'empty' route */

			final_state = Nfastates[ final ];

			old_final = Nfastates[ nfa1->final ];
			old_final->link1 = final; /* to new final state */
			old_final->link2 = NOLINK; /* no way back to old init state */
			old_final->symbol = NOTHING;

			newnfa = nfa1;
			newnfa->init = first;
			newnfa->final = final;
			break;
		case COUNTER:
		/* 1: create new init state (veryfirst)
		   2: if count is 0 shortcut nfa1.
		   3: copy nfa1 max times; if > min shortcut it, too
		*/

			veryfirst = new_state();
			first_state = Nfastates[ veryfirst ];
			first_state->link1 = first = nfa1->init;
			verylast = final = nfa1->final;
			final_state = Nfastates[ final ];
			if( final_state->link1 == LINKFLAG )
			{
				final_state->link1 = NOLINK;
				final_state->link2 = NOLINK;
			}

			cfield = (COUNTFIELD *)nfa2;
			min = cfield->low;
			max = cfield->high;
			st_num = NFA_USED;
			Destin = (int *)calloc( st_num+1,sizeof( int ));/* global for statecopy */
			if( max == 0 )
			{
				Nfastates[ veryfirst ]->link1 = final; /* cut it off */
			}
			else
			{
				if( min == 0 )
				{
					Nfastates[ veryfirst ]->link2 = final;
				}
				for( n = 1; n < max; ++n )
				{
					for( m = 0; m <= st_num; ++m ) Destin[ m ] = NOLINK;

					statecopy( first, final );/* copy nfastates recursively */

					Nfastates[ verylast ]->link1 = Destin[ first ];
					if( n >= min )
					{				/* empty route to final state */
						Nfastates[ verylast ]->link2 = Destin[ final ];
					}
					verylast = Destin[ final ];
				}
			}
			free( Destin );
			free( cfield );

			newnfa = nfa1;
			newnfa->init = veryfirst;
			newnfa->final = verylast;
			break;
		case UNIFY:
			/* new-init to old-init1 & old-init2 */
			/* old-final1 & old-final2 to new-final */

			first = new_state(); /* new first, final nfastates */
			final = new_state();

			first_state = Nfastates[ first ];  /* new first to 2 old init */
			first_state->link1 = nfa1->init;
			first_state->link2 = nfa2->init;

			//final_state = Nfastates[ final ];

			old_final = Nfastates[ nfa1->final ];
			old_final->link1 = final;
			old_final->link2 = NOLINK;
			old_final->symbol = NOTHING;

			old_final = Nfastates[ nfa2->final ];
			old_final->link1 = final;
			old_final->link2 = NOLINK;
			old_final->symbol = NOTHING;

			free( nfa2 );
			newnfa = nfa1;
			newnfa->init = first;
			newnfa->final = final;
			break;

	   case PERROR:
           Parserror = data;  /* global use! */
           newnfa = NULL;
           break;
	}
	return( newnfa );
}

/*	======= DFA =================================== */

/*	 E_CLOSURE :		close set, return token 	*/
/*	for each state in 'set' (s), add all target states (t) to set that can be
	reached by 'empty' transitions (state[s]->symbol==NOTHING), until no more
	states can be added. */

unsigned short E_closure(
SETPTR set,	/* ptr to set (of nfastates) to be closed */
NFASTATE **nfastates )	/* array of states */
{			/* if the set contains accepting states,
					   return their token (lowest has priority) */
	//unsigned token = NOTOKEN;	/* NOTOKEN: no token found */
   unsigned short token;
	int s, t; /* state nrs. */
	NFASTATE *currstate; /* running ptr to current nfastate */
	stack<int> fstack;

   token = NOTOKEN;
   NonBol = 256;
	for ( s = 0; ( s = b_next( set, s + 1 )) != TERROR; )
		fstack.push( s ); /* push all states in set. state 0 not used */

	while ( !fstack.empty() ) /* start pulling */
	{	/* s=source nfastate, t=target of link */
		s = fstack.top();
		fstack.pop();
		currstate = nfastates[ s ];
		if(( t = currstate->link1 ) == ACCEPTING )
		{	/* tokens found in reverse of order of creation */
			if( token == NOTOKEN || (currstate->token&0xff) < (token&0xff) )
		   {
				token = currstate->token; /* return lowest */
		   }
		   if( currstate->token != NOTOKEN && currstate->token < NonBol )
			   NonBol = currstate->token;
		}
		else if( t != NOLINK )
		{	/* not a final state: push states with empty symbol field */
			 /* symbol is NOTHING if only E-trans. */
			if( b_in(set, t) ) /* push only if not already in set */
			{
				if( currstate->symbol == NOTHING )    //!
				{
					fstack.push( t );
					b_enter(set, t); /* push & enter in set */
				}
			}	/* link2 only if link1 != NOLINK */
			t = currstate->link2; /* check 2nd transition (always E) */
			if( t != NOLINK && set[ t ] == false )
			{
				fstack.push( t );
				b_enter(set, t); /* push & enter in set */
			}
		} /* end else */
	} /* endwhile: stack empty */

	s = 0;	/* delete states with only E-transitions: */
	while (( s = nextbit( set, s + 1 )) != TERROR )
		if( nfastates[ s ]->symbol == NOTHING ) set[ s ] = false;
   if( NonBol == 256 ) NonBol = 0;
   fstack.~stack<int>();
   return( token );
	/* SET bevat nu alle states die vanuit de oorspronkelijke set
	   te bereiken zijn door alleen E-transities, en zelf wel
	   char-transities hebben.	*/
}

/*	 INIT_DFA :		init. dfa[] array			*/

DFA **Init_dfa(  /* initialize dfa[] array */
int initsize )		/* states not allocated except [ 0 ] */
{			/* dfa[ 0 ]->mark is set to arraysize */
	DFA **dfa;

	dfa = (DFA **)calloc( initsize, sizeof( DFA * ));
	dfa[ 0 ] = (DFA *)calloc( 1, sizeof( DFA ));
	dfa[ 0 ]->mark = initsize;
	return( dfa );
}

/*	 NEWDFASTATE :	alloc new dfastate			*/

DFA **NewDfaState(
DFA **dfa,
unsigned n )
{
	unsigned max, m;

	max = dfa[ 0 ]->mark; /* contains arraysize, set by init_dfa() */
	if( n >= max )
	{
		max += GROWSIZE;
		dfa = (DFA **)realloc( dfa, max * sizeof( DFA * ));
		dfa[ 0 ]->mark = max;
	}
	dfa[ n ] = (DFA *)calloc( 1, sizeof( DFA ));
	dfa[ 0 ]->token = n; /* 'laststate' in Nfa_to_Dfa() */
	return( dfa );
}

/*	 FREEDFA :		dealloc dfa[]				*/

void free_dfa( /* free dfa[] structs. */
DFA **dfa )
{
	int n, m;

   if( dfa == NULL ) return;
	n = dfa[ 0 ]->token;	/* number of states */
	for( m = 1; m <= n; ++m )
	{
		free(dfa[ m ]->states);
		if( dfa[ m ]->trans != NULL )
				free( dfa[ m ]->trans );
		free( dfa[ m ] );
	}
	free(dfa[ 0 ]->states);
	free( dfa[ 0 ] );
	free( dfa );
}

/*	 NFA_TO_DFA :		construct transition table	*/

/*
WHILE there is an unmarked state in Dfa[ 1..curr ]
	mark current state
	FOR each symbol 0..MAXNUMCHAR  in All_Chars  ( 0..127 or 0..255 )
		collect the transitions on that symbol in united_states
		Token = E_closure( united_states )   ( close the set, retn token )
		FOR all dfastates S so far created
			IF united_states[ S ] is equal to united_states
				nextstate = S       ( state exists )
			ELSE
				nextstate = ++laststate  ( create new unmarked state )

		* NB at some stage, all nextstate's will be found to exist already,
		* so laststate is not incremented and the loop terminates.

		TRANS[ currstate ][ symbol ] = nextstate;
*/

DFA **Nfa_to_Dfa( PARSTAB *parstab, NFASTATE **nfastates )
//PARSTAB *parstab;		/* struct filled in by transition_table() */
//NFASTATE **nfastates;	/* curr Nfastate[] array */
{
	unsigned currstate;	/* current dfa state in main loop */
	unsigned laststate; /* last state allocated */
//	SETPTR united_states;	/* the set of nfastates that nextstate represents */
//	SETPTR currnstates;	/* the set of nfastates that currstate represents */
	unsigned currsym;  	/* current transition char from currstate */
	unsigned nextstate; /* target state of currsym transition */
	DFA **dfa;			/* array of dfa states */
	unsigned short token;		/* lex-token of nextstate */
	int add;			/* TRUE if new state created */
	int found;			/* TRUE if currstate has transition(s) on currsym */
	unsigned nstate;
	unsigned nsym;
							/* symbol, index of nfastates in currnstates */
	long sum=0;			/* total nr of transitions needed for NEXT and CHECK */
	unsigned short first, last;	/* lowest and highest symbols in a trans-list */
	unsigned short temp[ MAXNUMCHAR ];	/* temporary array for dfa[ x ]->trans */
	int /*prevstate,*/ n, m;
	unsigned lastchar, firstchar, nof_chars;
	SETPTR *charsets;
	NFASTATE *nfacurr;
	DFA *dfacurr;

	charsets = P_CHARSETS;
	P_EOL = nfa_EOL;				/* set EOL-flag for P_GET() */
	dfa = Init_dfa( INIT_DFA_SIZE );
	//deque<bool> united_states( nfa_USED );	/* number of nfa states */
	//deque<bool> currnstates( nfa_USED );
	united_states = createset( nfa_USED );	/* number of nfa states */
	currnstates  = createset( nfa_USED );
	b_enter(united_states, nfa_INIT);		/* enter init state */
	dfa = NewDfaState( dfa, 1 );    /* alloc state 1 (dfa may be realloc.) */
		/* closure of initial set: ( E_closure retns token )  */
	dfa[ 1 ]->token = E_closure( united_states, nfastates );
	dfa[ 1 ]->nonbol = NonBol;  /* global output param. E_closure() */

	dfa[ 1 ]->states = united_states;
	currstate = laststate = 1;
	for( ; currstate <= laststate && !dfa[ currstate ]->mark; ++currstate )
	{
		dfacurr = dfa[ currstate ];
		dfacurr->mark = TRUE; /* mark current state */
		//currnstates.assign( dfacurr->states.begin(), dfacurr->states.end() );
		currnstates = dfacurr->states;
		add = TRUE;
		first = last = -1;
		for( currsym = 0; currsym < MAXNUMCHAR; ++currsym ) temp[ currsym ] = 0;
		currsym = 0;
		//prevstate = 0;
		while(( currsym = b_next( All_Chars, currsym )) != (unsigned)TERROR )
		{
			if( add == TRUE ) united_states = createset( nfa_USED );
			else b_clear( united_states );
			/* collect the transitions on that char in united_states: */
			nstate = 0;
			found = FALSE;
			while(( nstate =
				b_next( currnstates, nstate + 1 )) != (unsigned)TERROR )
			{
				nfacurr = nfastates[ nstate ];
				nsym = nfacurr->symbol;
				if( nsym & CHARSET )
				{
					nsym &= 0xFFFF;
					if( b_in( charsets[ nsym ], currsym ))
					{
						b_enter(united_states, nfacurr->link1);
						found = TRUE;
					}
				}
				else if( nsym == currsym )
				{
					b_enter(united_states, nfacurr->link1);
					b_delete(currnstates, nstate);
					found = TRUE;
				}
			} /* endwhile next nstate (transitions collected) */

			add = FALSE;
			if( found )
			{
				token = E_closure( united_states, nfastates );
				add = TRUE;
				for( nextstate = 1; nextstate <= laststate; ++nextstate )
				{ /* search existing list for equal set */
				  if( b_equal( united_states, dfa[ nextstate ]->states )  &&
					   dfa[ nextstate ]->token == token )
				  {
					add = FALSE; /* no new state: recycle united_states */
					break;
				  }
				} /* if add, nextstate == laststate + 1 */
				if( add == TRUE )
				{
					laststate = nextstate;
					dfa = NewDfaState( dfa, nextstate );
					dfa[ nextstate ]->states = united_states;
					dfa[ nextstate ]->token = token;
					dfa[ nextstate ]->nonbol = NonBol;
				}
				/* now enter { currsym, nextstate } in transition-arrays: */
				temp[ currsym ] = nextstate; /* nextstate != 0 */

				if( first == (unsigned short)-1 ) first = currsym;
				last = currsym;
				/*if( nextstate != prevstate )
				{
					dfacurr->diff += 1;
				}
				prevstate = nextstate;  */
			}
			++currsym;
		} /* endfor each currsym 0..MAXNUMCHAR in All_Chars */
		/*if( last >= 0 ) */
		{
			dfacurr->trans =
				(unsigned short *)myalloc( last-first+1, sizeof( short ));
			dfacurr->first = first;
			dfacurr->last = last;
			for( n = 0; n <= last-first; ++n )
			{
				(dfacurr->trans)[ n ] = temp[ n + first ];
			}
			sum += last - first + 1;
		}

	} /* endwhile there are unmarked states */

   parstab = (Parstab[ 2 ]);  // usertab
   (parstab->tr_tab) = (unsigned long **)calloc( laststate+2, sizeof( short * ));
   (parstab->tokens) = (unsigned short *)calloc( laststate+2, sizeof(unsigned short));
   P_NONBOLS = (unsigned short *)calloc( laststate+2, sizeof(unsigned short));
   parstab->lasts = (unsigned short *)calloc( laststate+2, sizeof(unsigned short));
   parstab->firsts = (unsigned short *)calloc( laststate+2, sizeof(unsigned short));
   sum += ((laststate+2)*5);
   for( n=1; n <= laststate; ++n )
   {
	   (parstab->tokens)[ n ] = dfa[ n ]->token;
	   if(dfa[ n ]->token & BOL) P_NONBOLS[ n ] = dfa[ n ]->nonbol;
	   else P_NONBOLS[ n ] = NOTOKEN;
       lastchar = dfa[ n ]->last;
       firstchar = dfa[ n ]->first;
       nof_chars = (lastchar - firstchar) + 1;
	   (parstab->tr_tab)[ n ] = (unsigned long *)myalloc( nof_chars, sizeof( long ));
	   sum += ( nof_chars * 2 );
	   for( m=0; m < nof_chars; ++m )
	   {
		   (parstab->tr_tab)[ n ][ m ] = (dfa[ n ]->trans)[ m ];
	   }
       (parstab->lasts)[ n ] = lastchar;
       (parstab->firsts)[ n ] = firstchar;
   }
//   if( laststate == 1 ) dfa = NULL;
	//united_states.~deque<bool>();
	//currnstates.~deque<bool>();
	//ShowMessage( AnsiString( sum ));
	return( dfa );
}
//-----------------------------------------------------------------------------
/*	 CHROUT : 		output nonprint chars		*/

void chrout(  /* output nonprint chars as escape seq. */
wchar_t c )
{
	WideString s;
    wchar_t cstr[ 2 ];

	if( c == ' ' ) s = "\\s"; //fprintf( OutFile, "\\s" );
	else if( c == '\t' ) s = "\\t"; //fprintf( OutFile, "\\t" );
	else if( c == '\f' ) s = "\\f"; //fprintf( OutFile, "\\f" );
	else if( c == '\x0d' ) s = "\\r"; //fprintf( OutFile, "\\r" );
	else if( c == '\x0a' ) s = "\\n"; //fprintf( OutFile, "\\n" );
	else if( c >= '\x7f' || c < ' ' )
			s = WideString("\\") + WideString( IntToHex(c & 0xFFFF, 4 ));
            //fprintf( OutFile, "\\%hu", c & 0xFFFF );
	else {
    	cstr[ 0 ] = c; cstr[ 1 ] = 0;
    	s = WideString( cstr ); //fprintf( OutFile, "%c", c );
    }
    OutFile->Add( s );
}


/*	======= SEARCH ================================ */

/*	 SEARCH : 		search a file using t.t 	*/

/*	 PRINTNFA :		print Nfastates[] to OutFile */

/*	 MAIN :										*/

PARSTAB *alexinit( void )
{
	PARSTAB *parstab; //the lowercase name is the local pointer to Parstab[ 2 ]. Used in #defines
						// also #defined USERTAB1
    if(Parstab[ 2 ] != NULL) return Parstab[ 2 ];
    Stackptr = 0;
	savenfa = NULL;
	parstab = (PARSTAB *)calloc( 1, sizeof( PARSTAB ));  /* parstab */
	Parstab[ 2 ] = parstab;
	//(parstab->buf) = (parstab->lex) = (ACHAR *)calloc( BUFSIZE, sizeof( ACHAR ) );
	SavePtr = parstab->buf;
	P_EOL = L'\0';
	Charsets = (SETPTR *)calloc( SETS, sizeof( SETPTR ));
	All_Chars = createset( CSETSIZE );
	P_GET = gettext;
	Macros = (ACHAR **)calloc( MACSIZE, sizeof( ACHAR * ));
	M_names = (ACHAR **)calloc( MACSIZE, sizeof( ACHAR * ));
	(Parstab[ 2 ])->output = (ACHAR **)calloc( TOKNUM, sizeof( ACHAR * ));
	(Parstab[ 2 ])->pats = (ACHAR **)calloc( TOKNUM, sizeof( ACHAR * ));
	(Parstab[ 2 ])->patnames = (ACHAR **)calloc( TOKNUM, sizeof( ACHAR * ));
	(Parstab[ 2 ])->charsets = NULL;
	NewToken = 0;
	Nfastates = Init_nfa( INIT_NFA_SIZE );
		/* Nfastates is global because of possible realloc. by new_state() */
	(Parstab[ 0 ])->buf = (ACHAR *)calloc( BUFSIZE, sizeof( ACHAR ));
   return( parstab );
}


int parse1line( ACHAR *line )
{
	Parstab[ 0 ] = &Regex;
	//P_GET = gettext;
	//P_ALEX = Alex1;
	wcscpy( (Parstab[ 0 ])->buf, line );
	(Parstab[ 0 ])->lex = (Parstab[ 0 ])->buf;
	(Parstab[ 0 ])->len = 0;
	(Parstab[ 0 ])->got = 0;
   ++NewToken;
	/* parse 1 line: */
	result = (NFA **)parse( (Parstab[ 0 ]) ); /* result in Nfastates[], REGEX-parstab */
	if( Parserror == PARSE_OK )
	{	/* if ok, merge prev. NFA (savenfa) and new result */
		savenfa = build( savenfa, *result, MERGE, 0 );
	}
	else
	{
		NewToken = 0;
		/*fprintf( OutFile, "error nr. %d\n", Parserror );*/
	}
   return NewToken; /* token val, without the BOL flag */
}

