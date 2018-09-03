
/*****************/
/*  DATA DEFS    */
/*****************/

/* global vars are capitalized, #defines are all uppercase */

#include <ctype.h>
#include <string.h>
#include <alloc.h>
#include <stdio.h>

#define ATTR_DEF
#define PARSE_DEF
#include "definitions.h"

#define fatal fataal
#define balloc b_alloc

#define HASHLEN        101
#define BUFSIZE         256
#define LEFT_ASS        1
#define RIGHT_ASS       2
#define NON_ASS         3
#define TERMINAL        -1
#define LEFT            1
#define RIGHT           2
#define N_OF_SYMBOLS    (Symnum+Start)
//#define ACCEPT          0
#define REDUCT          0x8000
#define ANY             0x7fff
#define PRINT           1 /* switch Options */
#define LEX             2
#define NO_OPTIM	4
#define EOL             -1
#define ENDset          Start  /* 'END' appears in sets with index 'Start'.. */
#define END             0      /* ..and in Actions[] as '0' */
#define ERROR           -1
#define b_clear( set )	b_difference( set, set ) /* clear set */

//----------------------------------------------------------------------------------

typedef short SYMBOL;
typedef short STATE;

char Suffix[ 8 ];
char Namebuf[ 32 ] = "pgtab.c";
FILE *Infile, *Tabfile, *Optfile, *Errfile;
/* Infile = input file( grammar )
Tabfile = output file containing parsing tables ( name.l, pgtab.c )
Optfile = output file readable tables info ( name.i ) dft. stdout
Errfile = error output: if -i option to name.i else to stdout */
short Insize = 4 * BUFSIZE; /* dynamic size of buffer */
short Begin; /* index van 1e non-space in Text */
char *Text;  /* Text[ Insize ] for input, dynamic size */
short Option, Linenum, RRconfl, SRconfl, Errors;
/* Option:cmdline options,Linenum:cr input count,error counts */
SYMBOL Errtoken, *Errstate;
/* Errtoken:dummy error return by parser.
Errstate[]: array,index by state,indicating parsing error conditions */
unsigned long Bytes; /* size of output tables in bytes */

typedef struct stacktype
{
	struct stacktype * st;
	SETPTR             val;
} STACK;

typedef struct symtab /********** SYMBOL TABLE ***********/
{
	struct symtab  *next;   /* linked list ptr to next entry */
	char 	       *name;   /* ptr to name-string, /0 termin. */
	SETPTR		closure;/* set of syms X : {(this) A ==> Xy } */
	SETPTR		follow; /* set of syms y : { X ==> aAyb } */
	SETPTR		first;  /* set of syms FIRST( A ) */
	SYMBOL		symbol; /* terminal token value or Symnum+Start */
	short		assoc;  /* associativity L,R,N */
	short		prior;  /* value: priority */
	short		zero;   /* index of E-prod, > 0 if any */
	short		index;  /* Startindex in 'productions' array, */
} TABENTRY, *TABPTR;        /* if index=-1, entry is a terminal */
/* ..if 0, Start or undefined */

TABPTR Hashtab[ HASHLEN ]; /* array of ptrs to 'buckets' of TABENTRY's */
TABPTR *Entry;   /* link array of TABPTRs, indexed by SYMBOL */
short Tabsize = 256, Prod_max = 256; /* default table sizes */

short Pr_index = 1, Last; /* index to Productions[] , index to States[] */
SYMBOL Symnum; /* first symbol: Start+1 */
SYMBOL Firstprod[] = { 0 , 0 }; /* space for Startproduction s' ==> s */
SYMBOL Start; /* Start production S' ==> S =  {START} ==> {START+1} */
SETPTR Uniset;

/* list of productions by NT. 'TABPTR->index' contains each NT's
lowest index in Productions[]:                                */

struct prodtype  {  /********** PRODUCTION ***********/
	SYMBOL		*p_rule;      /* production right side. */
	char 		*p_action;    /* string repr. compiler action */
	SYMBOL		 p_left;      /* prod. left side (entry-number). */
	short		 p_prior;     /* priority & assoc., its own or */
	short		 p_assoc;     /*  from its rightmost terminal */
	short		 p_length;    /* number of symbols in rule */
} *Productions;               /* index 'Pr_index' max 'Prod_max' */

/* format:
short SYMBOL := ( 0 ): Empty ( end of rule )
		(1 - ERROR (Start-1)): terminal
		(Start): Start production ( Productions[ 0 ].p_rule )
		(> Start): nonterminal nr: Symnum + Start
p_rule := array of SYMBOLs, length in p_length.
p_left := prod. left side symbol.
p_action := { c-format } compiler actions for each alternative

TABPTR Entry[ 'symbol' ] points to symbol-table entry for 'symbol'
*/

typedef struct itype {  /******** ITEM **********/
	struct itype	*i_next;  /* link ptr */
	STACKPTR	 i_stack; /* set of parent lookahead sets */
	SETPTR		 i_ahead; /* set of lookahead symbols */
	struct ttype	*i_goto;  /* 'item to item' transition-list */
	short		 i_prod;  /* Production[] index */
	short		 i_dot;   /* index in Prod's rule */
} ITEM;

/************* State[ x ] : ptr to linked list of ITEMs **************/
ITEM **State;
short States_max = 256; /* dynamic size */

typedef struct ttype { /****** temporary GOTO LIST ********/
	struct ttype	*t_link;   /* linked list ptr */
	ITEM		*t_target; /* ptr to target item */
	short		 t_state;  /* target state */
	SYMBOL		 t_symbol; /* transition symbol */
} TRANS; /* each item's transitions and lookaheads kept separate until
		'actions()' unifies them into their target State */
ITEM *Found_item; /* output par from 'find_trans' */

/****************************************************************************/
/* Action[ state ][ offset ] = { transition SYMBOL, next state } for shift */
/* or { transition SYMBOL, Production-index | REDUCT }  for reduce */
/* transition SYMBOL may be 'ANY' for default reduction */
/* or { SYMBOL, 0 | REDUCT } for end of parsing ( REDUCT = bit 15 ) */
/* each row terminated by { -1, -1 } */
/****************************************************************************/

short **Action;
short *Action_index; /* current index into Action[] list */
short *Action_size;  /* size of current row in Action[] list */

/****************************************************************************/
/* Go_to[ symbol ][ offset ]:{ this state, next state } until { ANY, n.s. } */
/****************************************************************************/

short **Go_to;
short *Go_index; /* current index into Go_to[] list */
short *Go_size;  /* size of current (row of) Go_to[] list */

/*
SETPTR createset();
short b_union(), b_delete(), b_enter(), b_in(), b_assign(), b_difference();
*/
//void fatal( /* char *msg; */ );
void *balloc( unsigned short,unsigned short );
void push( STACK **,SETPTR  );
SETPTR pull( STACK ** );
short backslash( UCHAR * );
short strip ( char *s );
short get_num( char *s );
unsigned short hashval( char *, short );
short scan( char * );
short equal( char *, char *, short );
TABPTR lookup( char *, short );
void putstring( char *, char *, short );
TABPTR install( void );
SYMBOL nt_install( short );
void print_symbol( SYMBOL );
void print_set( SETPTR );
void dump( void );
void print_item( short, short );
void dump_pr( void );
void printstates( void );
void read_terms( void );
unsigned gettext( void );
void read_nonterms( void );
void check_syms( void );
short unite( SETPTR, SETPTR );
short unify( SETPTR, SYMBOL );
short unify_string( SETPTR, SYMBOL * );
void first_syms( void );
void add_trans( ITEM *, STATE, SYMBOL, ITEM * );
STATE find_trans( ITEM *, SYMBOL );
STATE search_item( short, short );
ITEM *create_state( ITEM *, STATE, short, short, SYMBOL );
void items( void );
void insert_Goto( short, SYMBOL, short );
void solve_rr( short, short *, short );
void solve_sr( short, short *, short );
void insert_action( short, SYMBOL, short );
void actions( void );
long optimize( short **, short *oldindex );
void optim_goto( short **, short * );
void output_table( short **, short *, long );
void print_tab( void );
//short main( /* short argc; char *argv[]; */ );

//****************************************************************************
//              UTILITIES
//****************************************************************************

void fatal( char *msg ) /* print linenr, message & abort */
{
	if( Linenum != ERROR ) printf( "LINE %d:", Linenum );
	printf( "%s\n", msg );
	exit( ERROR );
}

void *balloc( unsigned short n, unsigned short s ) /* calloc with error abort */
{
	char *r;

	if (( r = calloc( n, s )) == NULL ) fatal( "out of memory" );
	return( r );
}

void push( STACK **adres, SETPTR value ) /* push a SETPTR on stack */
{
	STACK *n;

	n = balloc( 1, sizeof( STACK ));
	n->st = *adres;
	n->val = value;
	*adres = n;
}

SETPTR pull( STACK **adres )
{
	SETPTR s;

	if( *adres == NULL ) return( NULL );
	s = (*adres)->val;
	*adres = (*adres)->st; /* memory is NOT deallocated! */
	return( s );
}


short backslash( UCHAR *line )  /* check if last char is a backslash */
{
	short i;

	i = strip( line ) -  1;

	if ( line[ i ] == '\\' ) return( i );
	else return( 0 );
}

short strip ( char *s )   /* strip trailing newline from patterns input line */
{
	char *p;

	p = s + strlen( s ) - 1;
	while (( *p == '\n' || *p == '\r' ) && p >= s ) *p-- = '\0';
	return( strlen( s ) );  /* return new strlen */
}

short get_num( char *s ) /* convert string -> integer */
{
	short n;

	for ( n=0; isdigit( *s ); ++s )
	{ n = 10 * n + *s - '0'; ++Begin; }
	return( n );
}

//****************************************************************************
//              SYMBOL TABLE
//****************************************************************************

unsigned short hashval( char *txtptr, short len )  /* calculate hashing value */
{
	unsigned long t=0L;

	/* hashval = XOR & shift left all bytes */
	while( --len >= 0 )
	{
		t ^= (unsigned long)*txtptr;
		t <<= 1;
		++txtptr;
	}
	return( (unsigned short)( t % HASHLEN ));
}

short scan( char *txtptr )        /* skip spaces and count following non-spaces */
/* also set index(Begin) to first non-space char */
{
	short len;

	while( *txtptr == ' ' || *txtptr == '\t' )
	{
		Begin++;
		txtptr++;
	} /* skip spaces */
	len = 0;
	while( *txtptr != ' ' && *txtptr != '\t' && *txtptr ) { ++txtptr; ++len; }
	return( len );
}

short equal( char *txtptr, char *table, short len ) /* check equality of asci strings */
{ /* id in *TXTPTR delimited by len (scan). id in *TABLE 0 termin.*/
	while( --len >= 0 )
	{
		if ( *table == '\0' || *txtptr != *table ) return( FALSE );
		++txtptr;
		++table;
	}
	return( *table == '\0' );
}

TABPTR lookup( char *txtptr, short len ) /* lookup an id in hash table */
/* txtptr = ptr to begin text */
{
	TABPTR ptr;

	for( ptr=Hashtab[ hashval( txtptr, len )]; ptr!=NULL; ptr=ptr->next )
	if ( equal( txtptr, ptr->name, len )) return( ptr );

	return( NULL );
}

void putstring( char *ptr, char *txtptr, short len )
{
	while( --len >= 0 )  *ptr++ = *txtptr++;
	*ptr = '\0';
}

TABPTR install( void ) /* install an identifier in the hash table */
{
	TABPTR ptr;
	unsigned short hval,len;
	char *t;

	len = scan( Text+Begin ); /* get length */
	if( len == 0 ) return( NULL );
	if (( ptr=lookup( Text+Begin, len )) == NULL )
	{
		hval = hashval( Text+Begin, len );
		ptr = (TABPTR)balloc( 1, sizeof( TABENTRY ));
		ptr->next = Hashtab[ hval ]; /* enter into 'bucket' */
		Hashtab[ hval ] = ptr;
		t = (char *)malloc( len + 1 );
		putstring( t, Text+Begin, len );
		ptr->name = t;
	}
	Begin += len;
	return( ptr );
}

SYMBOL nt_install( short side ) /* install a nonterminal in symbol table */
/* side = flag: if installing left side, enter Pr_index */
{
	TABPTR ptr;

	ptr = install(); /* install its name */

	if( side == LEFT )
	{
		if( ptr->index > 0 )
		{
			++Errors;
			printf( "LINE %d:symbol '%s' def. twice\n", Linenum, ptr->name );
		}
		if( ptr->index < 0 )
		{
			++Errors;
			printf( "LINE %d:illegal terminal '%s'\n", Linenum, ptr->name );
		}
		ptr->index = Pr_index;
	}
	if( ptr->symbol > 0 || ptr->index == TERMINAL )
	return( ptr->symbol ); /* not a new symbol */

	ptr->symbol = ++Symnum + Start; /* global counter for new symbols */
	Entry[ ptr->symbol ] = ptr; /* link */
	if( Symnum+Start >= Tabsize-2 )
	Entry = realloc( Entry, Tabsize += 32 );
	if( Entry == NULL ) fatal( "out of memory" );
	return( ptr->symbol );
}

void print_symbol( SYMBOL sym ) /* print syntactic-variable's name */
{
	char *s;

	if( sym == Start ) fprintf( Optfile, "{START} " );
	else if( sym > 0 )
	{
		s = Entry[ sym ]->name;
		fprintf( Optfile, "%s ", s );
	}
	else fprintf( Optfile, "{EMPTY} " );
}

void print_set( SETPTR set ) /* print names of SYMBOLS in a set */
{
	short i = -1; /* set starts at 0 */

	while(( i = b_next( set, i+1 )) != ERROR )
	{
		if( i == ENDset ) fprintf( Optfile, "{END}" );
		else print_symbol( i );
	}
	fprintf( Optfile, "\n" );
}

void dump( void )        /* dump symbol table */
{
	TABPTR ptr;
	short i;
	char *asso;

	fprintf( Optfile, "SYMBOL:NAME,ASSOC,PRIOR\n" );
	for ( i = 0; i <= N_OF_SYMBOLS; i++ )
	{
		if (( ptr = Entry[ i ] ) != NULL )
		{
			if( ptr->assoc == LEFT_ASS ) asso = "L";
			else if( ptr->assoc == RIGHT_ASS ) asso = "R";
			else if( ptr->assoc == NON_ASS ) asso = "N";
			else asso = "undef";
			if( ptr->name == NULL ) fprintf( Optfile, "undef name" );
			else fprintf( Optfile, "%d:'%s',%d,%s\n", ptr->symbol, ptr->name,
			ptr->prior, asso );

			if( ptr->first != NULL )
			{ fprintf( Optfile, "  First:" ); print_set( ptr->first ); }
			if( ptr->follow != NULL )
			{ fprintf( Optfile, "  Follow:" ); print_set( ptr->follow ); }
			if( ptr->closure != NULL )
			{ fprintf( Optfile, "  Closure:" ); print_set( ptr->closure ); }

		}
	}
	fprintf( Optfile, "\n\n" );
}

void print_item( short index, short dot ) /* print a gramm. rule with its dot */
/* index = index to Prod.  if dot < 0 or > length no dot is printed */
{
	short i, l; // l = length

	print_symbol( Productions[ index ].p_left );
	fprintf( Optfile, " ==> " );
	l = Productions[ index ].p_length;
	for( i=0; i == 0 || i < l; ++i )
	{
		if( i == dot ) fprintf( Optfile, ". " );
		print_symbol( *( Productions[ index ].p_rule + i ));
		fprintf( Optfile, " " );
	}
	if( i == dot ) fprintf( Optfile, ".\n" );
	else fprintf( Optfile, "\n" );
}

void dump_pr( void )
{
	short i;
	char *s;
	struct prodtype *prod;

	for( i=0; i<Pr_index; i++ )
	{
		prod = Productions + i;
		if( prod->p_assoc == LEFT_ASS ) s = "L";
		else if( prod->p_assoc == RIGHT_ASS ) s = "R";
		else if( prod->p_assoc == NON_ASS ) s = "N";
		else s = "undef";
		fprintf( Optfile, "Prod %d:prio %d,ass %s\n", i, prod->p_prior, s );

		if( prod->p_action != NULL )
		fprintf( Optfile, "  %s\n", prod->p_action );
		else fprintf( Optfile, " (no code)\n" );

		print_item( i, -1 );
		fprintf( Optfile, "\n" );
	}
	fprintf( Optfile, "\n" );
}

void printstates() /* print items of all states */
{
	short i,n,a, *action;
	ITEM *item;
	TRANS *ptr;
	char *str;

	fprintf( Optfile, "\n\nITEM-LIST:\n" );
	for( n = 1; n < Last; ++n )
	{
		fprintf( Optfile, "\nSTATE %d:\n", n );
		item = State[ n ];
		while ( item != NULL ) /* process list of items of each state */
		{
			fprintf( Optfile, "Item:", n );
			print_item( item->i_prod, item->i_dot );

			if( item->i_ahead != NULL )
			{
				fprintf( Optfile, "Ahead: " );
				print_set( item->i_ahead );
			}

			if(( ptr = item->i_goto ) != NULL )
			{
				do
				{
					if( ptr->t_symbol > Start )
					{
						str = Entry[ ptr->t_symbol ]->name;
						fprintf( Optfile, "after: %-16s, goto %d\n", str, ptr->t_state );
					}
				} while(( ptr = ptr->t_link ) != NULL );
			}
			item = item->i_next;
		}
		action = Action[ n ];
		if( action == NULL ) { fprintf( Optfile, "NO ACTIONS!\n" ); continue; }

		for( i = 0; action[ i ] != EOL; i += 2 )
		{
			if( action[ i ] == ANY ) str = "ANY";
			else if( action[ i ] == END ) str = "END";
			else  str = Entry[ action[ i ] ]->name;
			a = action[ i + 1 ];
			if( a & REDUCT )
			{
				a &= ~REDUCT;
				if( a == 0 ) fprintf( Optfile, "on: %-20s, ACCEPT.\n", str );
				else
				{
					fprintf( Optfile, "on: %-20s, reduce(%d): ", str, a );
					print_item( a, -1 );
				}
			}
			else fprintf( Optfile, "on: %-20s, shift %d\n", str, a );
		}
		fprintf( Optfile, "\n" );
	}
}
//****************************************************************************
//              READ LINES & FILL DATA STRUCTURE
//****************************************************************************

void read_terms( void ) /* input terminals list */
{
	SYMBOL token=0;
	short ass=0, prio=0;
	TABPTR ptr;

	while ( 1 )
	{
		Begin = 0;
		if( gettext() == ERROR ) break; /* EOF */
		if ( Text[ 0 ] == '\0' ) break;
		if ( Text[ 0 ] == ';' ) continue;
		if ( Text[ 0 ] == '%' && Text[ 1 ] == '%' ) break;
		/* enter terminals into symbol table */
		if( isdigit( Text[ Begin ] )) token = get_num( Text+Begin );
		else ++token;
		scan( Text + Begin );
		if( Text[ Begin ] != '%' )
		{
			if (( ptr = install()) == NULL ) continue;
			ass = 0;
			prio = 0;
			scan( Text + Begin );
			if( isdigit( Text[ Begin ] ))
			{
				prio = get_num( Text + Begin );
				scan( Text + Begin );
				if( Text[ Begin ] == 'L' ) { ass = LEFT_ASS; ++Begin; }
				else if( Text[ Begin ] == 'R' ) { ass = RIGHT_ASS; ++Begin; }
				else if( Text[ Begin ] == 'N' ) { ass = NON_ASS; ++Begin; }
				scan( Text + Begin );
			}

			ptr->symbol = token;
			if( token >= Start ) Start = token + 1;
			ptr->prior = prio;
			ptr->assoc = ass;
			ptr->index = TERMINAL;
			Entry[ token ] = ptr;
		}
		if( Option & LEX )
		if( Text[ Begin ] && Text[ Begin++ ] == '%' )
		{
			fprintf( Tabfile, "%s\n", Text+Begin );
		}
	}
}

unsigned gettext( void ) /* get input file */
{
	short len=0;

	do {
		if ( fgets( Text+len, BUFSIZE, Infile ) == NULL ) return( ERROR );
		if ( len >= Insize - BUFSIZE )
		{
			Insize += BUFSIZE;
			Text = realloc( Text, Insize );
			if ( Text == NULL ) fatal( "out of memory" );
		}
	} while ( (len = backslash( Text )) != 0 );
	++Linenum;
	return( 0 );
}

void read_nonterms( void ) /* input nonterminals list */
{
	SYMBOL curr_sym=0;
	SYMBOL *p;
	short i;
	struct prodtype *prod;
	short prio=0, assoc, prio_def, len;
	short temp[ 256 ];
	short newleftside = TRUE;

	while ( Pr_index < Prod_max )
	{
		Begin = 0;

		if( gettext() == ERROR ) return; /* EOF */
		if ( Text[ 0 ] == '%' && Text[ 1 ] == '%' ) break; /* output rest */
		if ( Text[ 0 ] == ';' ) continue;

		/* enter a prod. left side into symbol table: */
		scan( Text + Begin );
		prio = 0;
		assoc = 0;
		prio_def = FALSE;
		if ( Text[ Begin ] == ';' ) { newleftside = TRUE; continue; }
		if( isdigit( Text[ Begin ] ))
		{
			prio = get_num( Text + Begin );
			scan( Text + Begin );
			prio_def = TRUE;
			if( Text[ Begin ] == 'L' ) { assoc = LEFT_ASS; ++Begin; }
			else if( Text[ Begin ] == 'R' ) { assoc = RIGHT_ASS; ++Begin; }
			else if( Text[ Begin ] == 'N' ) { assoc = NON_ASS; ++Begin; }
			scan( Text + Begin );
		}
		if( isalpha( Text[ Begin ] ))
		{
			if( newleftside == FALSE )
			printf( "LINE %d: warning: missing ';'\n", Linenum );
			newleftside = TRUE;
			curr_sym = nt_install( LEFT );
			scan( Text+Begin );
		}
		else if( newleftside == TRUE ) fatal( "missing leftside" );

		if(( newleftside == TRUE && Text[ Begin ] == '=' )
				|| ( newleftside == FALSE && Text[ Begin ] == '|' )) ++Begin;
		else fatal( "syntax error" );
		newleftside = FALSE;
		/* process right side: */
		i = 0;
		while( scan( Text+Begin ) && Text[ Begin ] != '{' )
		{
			temp[ i ] = nt_install( RIGHT );
			if( temp[ i ] < Start  && prio_def == FALSE )
			{
				prio = Entry[ temp[ i ] ]->prior; /* leaves rightmost */
				assoc = Entry[ temp[ i ] ]->assoc;
			}
			i++;
		}

		temp[ i++ ] = 0; /* end of right side */
		prod = Productions + Pr_index;
		p = prod->p_rule = balloc( i+1, sizeof( SYMBOL ));
		prod->p_length = i-1;
		if( i == 1 )
		{
			if( Entry[ curr_sym ]->zero != 0 )
			{
				printf( "LINE %d:more than 1 empty derivations of '%s'\n",
				Linenum, Entry[ curr_sym ]->name );
				++Errors;
			}
			Entry[ curr_sym ]->zero = Pr_index; /* index of E-prod. */
		}
		while ( --i >= 0 ) p[ i ] = temp[ i ];
		prod->p_prior = prio;
		prod->p_assoc = assoc;
		prod->p_left = curr_sym;
		if ( Text[ Begin ] == '{' )
		{
			len = strlen( Text+Begin );
			(char *)p = prod->p_action = malloc( len+1 );
			putstring( p, Text+Begin, len );
		}
		Pr_index++;
	}
	if( Pr_index >= Prod_max ) fatal( "too many productions" );
	while( gettext() != ERROR )
	{
		fprintf( Tabfile, "%s\n", Text ); /* send rest of file to 'pgtab.c' */
	}
}

void check_syms( void ) /* all symbols in symtab must be either terminal or leftside */
{
	short i, num;
	TABPTR ptr;

	num = N_OF_SYMBOLS;
	for( i = Start; i <= num; ++i )
	{
		ptr = Entry[ i ];
		ptr->closure = createset( num ); /* also allocate sets */
		ptr->follow = createset( num );
		ptr->first = createset( num );

		if( i > Start && ptr->index == 0 ) /* symbol does not appear at left */
		{
			++Errors;
			printf( "variable \x22%s\x22 is undefined\n", ptr->name );
		}
	}
}
//****************************************************************************
//              CREATE TABLES
//****************************************************************************

short unite( SETPTR set1, SETPTR set2 ) /* union which excludes 0 and returns 'changed'-flag */
{
	short emptysym, change = FALSE;

	/* unify and record if change occurs */
	b_assign( Uniset, set1 ); /* temp = set1 */
	emptysym = b_delete( set2, 0 );
	b_union( set1, set2 ); /* set1 = set1 OR set2 */
	if( emptysym ) b_enter( set2, 0 );
	if( !b_equal( Uniset, set1 )) change = TRUE; /* if temp != set1 */

	return( change );
}

short unify( SETPTR set, SYMBOL sym ) /* unify a set and a set of symbols that a sym derives */
{                                    /* and return TRUE if any changes made to the set */
	short change=FALSE;
	TABPTR ptr;

	/* b_enter(s,i) returns TRUE iff i was already in s */
	if ( b_enter( set, sym ) == FALSE ) change = TRUE;
	if ( sym < Start ) return( change ); /* sym is a terminal or 0 */
	ptr = Entry[ sym ];
	if( unite( set, ptr->closure ) == TRUE ) change = TRUE;
	return( change );/* EMPTY ( 0 ) not in this union unless sym ==> EMPTY */
}

short unify_string( SETPTR set, SYMBOL *string )  //   unify a set and a string of symbols
{        /* string = 0 termin. string of grammar symbols */
	short change = FALSE;

	while ( 1 )
	{
		if( b_enter( set, *string ) == FALSE ) change = TRUE;
		if( *string < Start ) break;
		if( unite( set, Entry[ *string ]->first ) == TRUE ) change = TRUE;
		if( b_in( Entry[ *string ]->closure, 0 )) ++string;
		else break;
	}
	return( change ); /* set has 0 in it if end of string is reached */
}

void first_syms( void )
{
	/* 1:for all nonterminals in Productions[], calculate the set of leftmost
symbols that may start a derivation of that nonterminal ( 'closure'
without the root nonterm ).
2:first: the set of terminals that may start strings derived from
a nonterminal ( as 1, but includes the next symbol in string, if the
current one can be empty. Contains 0 if symbol 'sees' end of string. */

	TABPTR lptr;
	short ix, change;
	SYMBOL *rule;
	SYMBOL cursym;

	change = TRUE;
	while( change == TRUE )
	{
		change = FALSE;
		for( ix = 0; ix < Pr_index; ix++ )
		{
			cursym = Productions[ ix ].p_left; /* current left side */
			lptr = Entry[ cursym ]; /* its symtab entry */

			rule = Productions[ ix ].p_rule;
			/* b_in( closure ,0 ) IFF there is a production { cursym => EMPTY } */
			if( unify( lptr->closure, *rule ) == TRUE ) change = TRUE;
			/* b_in( first,0 ) IFF all syms in rule can be empty: */
			unite( lptr->first, lptr->closure ); /* quicker that way */
			if( unify_string( lptr->first, rule ) == TRUE ) change = TRUE;
		}
	}
}

void add_trans( ITEM *item, STATE to, SYMBOL symbol, ITEM *target ) /* add a transition to list */
{               /* item =  source item, target = target item */
	TRANS *golist; /* golist: state,symbol,target item, link. */

	if( (golist = item->i_goto) == NULL )
	{        /* create new list */
		item->i_goto = golist = balloc( 1, sizeof( TRANS ));
	}
	else
	{
		while( golist->t_link != NULL ) golist = golist->t_link;
		/* append to end of list */
		golist->t_link = balloc( 1, sizeof( TRANS ));
		golist = golist->t_link;
	}
	golist->t_state = to;
	golist->t_symbol = symbol;
	golist->t_target = target;
}

STATE find_trans( ITEM *item, SYMBOL symbol ) /* find target of a transition */
{              /* item = from-item, symbol = transition term or nonterm  */
	TRANS *golist;

	if( (golist = item->i_goto) != NULL )
	{ /* returns ptr to target item in 'Found_item' */
		do {
			if( golist->t_symbol == symbol )
			{
				Found_item = golist->t_target;
				return ( golist->t_state );
			}
		} while( (golist = golist->t_link) != NULL );
	}

	Found_item = (ITEM *)NULL;
	return( ERROR ); /* no transition yet */
}

STATE search_item( short rule, short dot ) /* check if item already exists */
{   /* Initial items of all States will be different. */
	STATE i;
	ITEM *temp;

	for( i=1; i < Last; ++i ) /* State 0 not used */
	{
		temp = State[ i ];
		if( (temp->i_prod == rule) && (temp->i_dot == dot) ) return( i );
	}
	return( 0 );
}

/*******************************************************/
/*          create State and return its address        */
/*******************************************************/
ITEM *create_state( ITEM *source, STATE curr, short rule, short dot, SYMBOL sym )
/* source = source item adr */
/* curr = source state index 'curr' under construction in items() */
/* rule, dot = index of rule , dot position */
/* sym = transition symbol */
{
	ITEM *item;
	STATE target = ERROR;
	short found, new_transition;

	for( item = State[ curr ]; item != NULL; item = item->i_next )
	{
		target = find_trans( item, sym ); /* target state index */
		if( target != ERROR ) break;
	}

	if( item != source || target == ERROR ) new_transition = TRUE;
	else new_transition = FALSE;
	/* State may already have generated other transitions on sym ( the
	parent item transition is created first ). They must go to same
	State, but maybe different target item. */
	if( target != ERROR ) /* we have a transition: */
	{
		found = FALSE;
		item = Found_item; /* from Find_trans */
		while( item != NULL )
		{
			if( item->i_prod == rule && item->i_dot == dot ) /* existing item */
			{ found = TRUE; break; }
			if( item->i_next != NULL ) item = item->i_next;
			else break;
		} /* exit with 'item' pointing to found, resp. last item */
		if ( found == FALSE )
		{ /* append item as described by input pars */
			item = item->i_next = balloc( 1, sizeof( ITEM ));
			item->i_prod = rule;
			item->i_dot = dot;
			item->i_ahead = createset( N_OF_SYMBOLS );
		}
	}
	else  /* search for States with first item as descr. */
	{
		target = search_item( rule, dot );
		if( target == 0 ) /* not found */
		{        /* create new State */
			target = Last++;
			item = State[ target ] = balloc( 1, sizeof( ITEM ));
			item->i_prod = rule;
			item->i_dot = dot;
			item->i_ahead = createset( N_OF_SYMBOLS );
			if( Last >= States_max - 2 )
			{
				States_max += 64;
				State = realloc( State, States_max * sizeof( ITEM * ));
				Errstate = realloc( Errstate, States_max * sizeof( short ));
			}
		}
		else item = State[ target ];
	} /* add to transition-list and,if not an existing target,add target ptr */
	if( new_transition )
	add_trans( source, target, sym, item );
	if( sym == Errtoken ) Errstate[ curr ] = TRUE;
	return( item );
}

/******************************************/
/*           create ITEMS                 */
/******************************************/
void items( void )
{
	STATE curr, trstate, substate;
	short i, empty_parent, change;
	ITEM *item;
	SETPTR lookahead;
	SETPTR closure;
	ITEM *newitem;
	ITEM *subtarget;
	TABPTR symptr;
	SYMBOL symbol, trsym, left, leftmost, match, *rule;
	STACKPTR top;
	/*
REPEAT
FOR all new states S
	FOR all items( X ==> a.bc ) in a state
	    IF NOT dot-at-end
	        S = create_state( item( X ==> ab.c ))
	        IF b is a nonterm
	            FOR all nonterms Y in closure( b )
		            FOR all productions( Y ==> .de )
		                S = create_state( item( Y ==> d.e ))
UNTIL no new states are created
propagate-lookaheads
*/

	Errstate = balloc( States_max+1, sizeof( short ));
	State = balloc( States_max+1, sizeof( ITEM * ));
	State[ 1 ] = balloc( 1, sizeof( ITEM )); /* State 0 not used */
	State[ 1 ]->i_ahead = createset( N_OF_SYMBOLS );
	b_enter( State[ 1 ]->i_ahead, ENDset ); /* Start doubles as 'ENDset'.. */
	b_enter( Entry[ Start ]->follow, ENDset ); /* ..in lookahead sets */
	closure = createset( N_OF_SYMBOLS );
	lookahead = createset( N_OF_SYMBOLS );
	curr = 1;
	Last = 2;
	for( ; curr < Last; ++curr ) /* Last: global, set by create_state() */
	{ /* loop until no more new states are created */
		item = State[ curr ];
		for( ; item != NULL; item = item->i_next )
		{         /* process list of items of each state */
			/* closure of current item: */
			b_clear( closure );
			rule = Productions[ item->i_prod ].p_rule;
			symbol = *( rule + item->i_dot );
			if( symbol > Start ) /* sym is a nonterm. Start is not in any rule */
			{
				symptr = Entry[ symbol ]; /* tabptr to symbol at dot */
				b_assign( closure, symptr->closure );
			} /* closure: */
			/* parent item + symbol itself + all possible 'first_syms' at dot */
			b_enter( closure, symbol );

			if( item->i_dot < Productions[ item->i_prod ].p_length )
			{ /* shift parent item: */
				newitem = create_state( item,curr,item->i_prod,item->i_dot+1,symbol );
				/* ..create target State   {B => a.Cb} ==> {B => aC.b} */
				b_clear( lookahead );
				/* follow(C) = first(b) */
				unify_string( lookahead, rule + item->i_dot + 1 );
				b_delete( lookahead, Errtoken ); /* errors not in lookahead */
				/* E in i->ahead */
				if( b_in( lookahead, 0 )) b_enter( item->i_ahead, 0 );
				if( symbol > Start )
				{
					b_union( symptr->follow, lookahead ); /* follow(C) */
					/* from {B -> a.Cb}) via closure C *==> Ay to all {A ==> X.z} */
					match = Start; /* closure of C: */
					while (( match = b_next( closure, match+1 )) != ERROR )
					{
						i = Entry[ match ]->index;
						for ( ; Productions[ i ].p_left == match; ++i )
						{
							b_clear( lookahead );
							leftmost = *(Productions[ i ].p_rule); /* X in {A ==> .Xz} */
							if( leftmost > 0 )
							{
								newitem = create_state( item, curr, i, 1, leftmost );
								/* trans to {A ==> X.z} (SHIFT/GOTO( curr,leftmost ))*/
								unify_string( lookahead, Productions[ i ].p_rule + 1 );
								b_delete( lookahead, Errtoken ); /* first(z) ^ */
								if( b_in( lookahead, 0 ))
								b_enter( newitem->i_ahead, 0 ); /* E in i->ahead */
							}
							else b_enter( lookahead, 0 );
							if( leftmost > Start )
							b_union( Entry[ leftmost ]->follow, lookahead );/* follow(X) */
						} /* end for match leftside */
					}  /* end while  closure     */
				}   /* end if    nonterm      */
			}    /* end if    dot < len    */
		}    /* end for   item-list    */
	}    /* end for   all states   */

	/**** Now, the lookaheads ****/

	for( curr = 1; curr < Last; ++curr )
	{
		for( item = State[ curr ]; item != NULL; item = item->i_next )
		{
			symbol = *( Productions[ item->i_prod ].p_rule + item->i_dot );
			symptr = Entry[ symbol ];
			empty_parent = b_in( item->i_ahead, 0 );
			left = Productions[ item->i_prod ].p_left;
			b_union( item->i_ahead, Entry[ left ]->follow ); /* spontaneous la */
			/* leave 0 in, if E in item's lookahead: */
			if( !empty_parent ) b_delete( item->i_ahead, 0 );
			if( item->i_dot < Productions[ item->i_prod ].p_length )
			{
				trsym = 0;
				b_clear( closure );
				if( symbol > Start ) b_assign( closure, symptr->closure );
				b_enter( closure, symbol );
				newitem = NULL;
				while(( trsym = b_next( closure, trsym+1 )) != ERROR )
				{
					trstate = find_trans( item, trsym );/* target item in Found_item */
					/* if ( other item's trans || recursive trans ): ignore */
					if( trstate == ERROR || newitem == Found_item ) continue;
					newitem = Found_item;

					if( trsym == symbol )
					{ /* always pass lookahead on parent symbol shift */
						push( &(newitem->i_stack), item->i_ahead ); /* parent to ch'n */
					}
					for( ;newitem != NULL; newitem = newitem->i_next )
					{ /* sym in closure: take target state's item list.. */
						subtarget = newitem;
						while( 1 ) /* work back from child 'newitem' to parent 'item'..*/
						{ 	 /* ..via leftsides of subparents 'subtarget' */
							left = Productions[ subtarget->i_prod ].p_left;
							if( b_in( Entry[ left ]->follow, 0 )) /* E in follow(left) */
							{
								if( left == symbol ) /* parent sym reached */
								{ /* E in all left->ahead: from parent to child */
									/*if ( empty_parent )*/ /* change made 9-4-2001 */
									push( &(newitem->i_stack), item->i_ahead );
									break;
								}
								substate = find_trans( item, left );
								if( substate == ERROR   /* other item's trans */
										|| substate == trstate  /* avoid loops */
										|| substate == curr ) break;
								subtarget = Found_item;
								/* E in all sub->ahead: from 'sub'parent to child */
								push( &(newitem->i_stack), subtarget->i_ahead );
							}
							else break;
						}
					}
				}
			}
		}
	}
	change = TRUE;
	while( change == TRUE )
	{
		change = FALSE;
		for( curr = 1; curr < Last; ++curr )
		{
			for( item = State[ curr ]; item != NULL; item = item->i_next )
			{
				top = item->i_stack; /* save ptr to top of stack */
				while(( lookahead = pull( &(item->i_stack))) != NULL )
				{
					if( unite( item->i_ahead, lookahead ) == TRUE )
					change = TRUE;
				}
				item->i_stack = top; /* pull() doesn't free or change the stack */
			}
		}
	}
}

/*
	#] items :
	#[ insert_Goto :        insert a 'goto' in the list
*/

void insert_Goto( short from, short sym, short to ) /* state-to-state Goto transitions */
{
	short ix, index, found = FALSE;
	short *goptr;

	sym -= Start;
	goptr = Go_to[ sym ];
	if( goptr == NULL )
	{
		Go_to[ sym ] = goptr = balloc( 32, sizeof( short ));
		Go_size[ sym ] = 32;
	}
	ix = Go_index[ sym ];
	for( index = 0; index < ix; index += 2 )
	if( goptr[ index ] == from && goptr[ index+1 ] == to )
	{ found = TRUE; break; }

	if( !found )
	{
		goptr[ ix++ ] = from;
		goptr[ ix++ ] = to;
		Go_index[ sym ] = ix;
		if( ix >= Go_size[ sym ] - 4 )
		{
			Go_size[ sym ] += 32;
			Go_to[ sym ] = realloc( Go_to[ sym ], Go_size[ sym ] * sizeof(short) );
		}
	}
}

void solve_rr( short i, short *action, short newact )
{
	short oldprod, newprod, old_prio, new_prio;

	oldprod = action[ 1 ] & ~REDUCT;
	newprod = newact & ~REDUCT;
	old_prio = Productions[ oldprod ].p_prior;
	new_prio = Productions[ newprod ].p_prior;
	if( old_prio > new_prio ) return;
	if( old_prio < new_prio ) { action[ 1 ] = newact; return; }
	/* equal: put earliest in */
	if( oldprod > newprod ) action[ 1 ] = newact;
	fprintf( Errfile, "on symbol:" );
	if( action[ 0 ] != END )print_symbol( action[ 0 ] );
	else fprintf( Errfile, " END" );
	fprintf( Errfile, "\nRR conflict on state %d,between:\n", i );
	if( oldprod < newprod ) fprintf( Errfile, "CHOICE:" );
	if( oldprod == 0 ) fprintf( Errfile, "ACCEPT\n" );
	else print_item( oldprod, -1 );
	if( oldprod > newprod ) fprintf( Errfile, "CHOICE:" );
	if( newprod == 0 ) fprintf( Errfile, "ACCEPT\n" );
	else print_item( newprod, -1 );
	fprintf( Errfile, "\n" );
	++RRconfl;
	return;
}

void solve_sr( short i, short *action, short newact )
{
	short prod, old_prio, new_prio, assoc;

	if( newact & REDUCT ) /* new = reduce, old = shift */
	{
		prod = newact & ~REDUCT;
		new_prio = Productions[ prod ].p_prior;
		old_prio = Entry[ action[ 0 ] ]->prior;
		assoc = Productions[ prod ].p_assoc;
	}
	else
	{
		prod = action[ 1 ] & ~REDUCT;
		old_prio = Productions[ prod ].p_prior;
		new_prio = Entry[ action[ 0 ] ]->prior;
		assoc = Productions[ prod ].p_assoc;
	}
	if( old_prio > new_prio ) return;
	if( old_prio < new_prio ) { action[ 1 ] = newact; return; }
	/* equal: check associativity of production */
	if( newact & REDUCT )
	{
		if( assoc == LEFT_ASS || assoc == NON_ASS )
		{ action[ 1 ] = newact; return; } /* reduce */
	}
	else
	{
		if( assoc == RIGHT_ASS )
		{ action[ 1 ] = newact; return; } /* shift */
	}
	if( assoc == 0 ) /* equal, no associativity */
	{
		if( !(newact & REDUCT) ) action[ 1 ] = newact; /* shift by default */
		fprintf( Errfile, "SR conflict on state %d,between:\n", i );
		fprintf( Errfile, "reduction:" );
		print_item( prod, -1 );
		fprintf( Errfile, "- or shift on '%s'\n", Entry[ action[ 0 ] ]->name );
		fprintf( Errfile, "\n" );
		++SRconfl;
	}
	return;
}

void insert_action( short i, short sym, short act )
{         /* i,sym,act = state nr, transition symbol, shift or reduce action */
	short *action;
	short index, ix, max, found;
	/*
IF { sym, * } is in Actions[ i ]
{
IF act is a REDUCE ** ACCEPT is also a reduce action ** {
	IF A in Actions{ S, A } is a REDUCE:act = RR-CONFLICT( i, sym, act, A )
	ELSE: act = SR-CONFLICT( i, sym, act, A ) }
ELSE ** act is a shift **   {
	IF A in Actions{ S, A } is a REDUCE:act = SR-CONFLICT( i, sym, act, A )
	ELSE : serious trouble (shift conflict) }
}
IF NOT FOUND ** new pair needed **
{
IF act is a REDUCE
	add { sym, act } to tail of Actions[ i ]
ELSE ** it's a shift **
	IF S is nonterminal
		insert transition( (symbol) sym, (state) i, (target) act )
}
	ELSE
	add { sym, act } to front of Actions[ i ]
*/

	if( sym == ENDset ) sym = END; /* renumber END to 0 */
	action = Action[ i ];
	if( action == NULL )
	{
		Action[ i ] = action = balloc( 32, sizeof( short ));
		Action_size[ i ] = 32;
		action[ 0 ] = action[ 1 ] = EOL;
	}
	max = Action_index[ i ];
	found = FALSE;
	for( index = 0; index < max ; index += 2 ) /* if max = 0 nothing happens */
	{    /* see if already in Action[ i ] : */
		if( action[ index ] != sym ) continue;
		if( action[ index+1 ] == act ) { found = TRUE; break; }
		if( action[ index+1 ] & REDUCT )
		{
			found = TRUE;
			if( act & REDUCT ) /* R-R conflict */
			solve_rr( i, &action[ index ], act );
			else /* S-R conflict */
			solve_sr( i, &action[ index ], act );
			break;
		}
		else /* we have a S-R conflict */
		{
			found = TRUE;
			if( act & REDUCT ) /* S-R conflict */
			solve_sr( i, &action[ index ], act );
			else
			fatal( "system err:shift confict!?!?" );
			break;
		}
	} /* end of Action[ i ] */
	if ( found == FALSE )
	{
		if( Action_index[ i ] >= Action_size[ i ] - 4 )
		{
			Action_size[ i ] += 32;
			Action[ i ] = action = realloc( Action[ i ],
			Action_size[ i ] * sizeof( short ));
		}
		if( act & REDUCT )
		{
			action[ max++ ] = sym;
			action[ max++ ] = act;
			Action_index[ i ] = max;
		}
		else
		{
			if( sym > Start ) insert_Goto( i, sym, act );
			else
			{   /* shifts added in front */
				for( ix = max-1; ix >= 0; --ix )
				action[ ix + 2 ] = action[ ix ];
				action[ 0 ] = sym;
				action[ 1 ] = act;
				Action_index[ i ] = max + 2;
			}
		}
	}
}

//****************************************************************************
//            CREATE ACTIONS TABLE
//****************************************************************************
void actions( void )
{
	short i;
	ITEM *item;
	SETPTR closure, ahead;
	SETPTR clofirst, follow;
	short term, shiftsym, prod, dotsym;
	short target, empty_parent;

	/* filled by insert_action() */
	Action = balloc( Last+2, sizeof( short * )); /* Action[ 0 ] unused */
	Action_index = balloc( Last+2, sizeof( short ));
	Action_size = balloc( Last+2, sizeof( short ));

	Go_to = balloc( Symnum+2, sizeof( short * ));
	Go_index = balloc( Symnum+2, sizeof( short ));
	Go_size = balloc( Symnum+2, sizeof( short ));

	closure = createset( N_OF_SYMBOLS );
	follow = createset( N_OF_SYMBOLS );

	/*
FOR all states:
FOR all items-in-a-state:
	IF dot-at-end:
	  FOR all terms A in lookahead( item ):
	    add-reduction [ state ][ A, production-index ] ** may be ACCEPT **
	ELSE ** dot not at end **
	  FOR all symbols A in closure( symbol-at-dot )
	    IF  A ==> Empty
	      FOR all terms B in first( sym-at-(dot+1) ):
		    add-reduction [ state ][ B , production-index(A ==> Empty) ]
	        IF lookahead( item ) ==> Empty ** (first,0) AND (follow,0) **
		      FOR all terms B in lookahead( item ):
		        add-reduction [ state ][ A , production-index(A ==> Empty) ]
	    ELSE ** not empty **
	      FOR all terms B in closure( A )
		    add-shift [ state ][ B , goto( state, B ) ]
*/
	for( i = 1; i < Last; ++i )
	{
		for( item = State[ i ]; item != NULL; item = item->i_next )
		{
			ahead =  item->i_ahead;
			empty_parent = b_delete( ahead, 0 );
			prod = item->i_prod;
			if( item->i_dot == Productions[ prod ].p_length )
			{ /* Reduce item found */
				term = 0;                /* 3: compare lookaheads: */
				while(( term = b_next( ahead, term + 1 )) != ERROR )
				{
					if( term > Start ) break;
					insert_action( i, term, prod | REDUCT );/* inserts Goto too */
				}
			} /* endif reduction-item */
			else /* shift or empty reduction item */
			{
				b_clear( closure );
				dotsym = *( Productions[ prod ].p_rule + item->i_dot );
				if( dotsym > Start ) b_union( closure, Entry[ dotsym ]->closure );
				b_enter( closure, dotsym ); /* make closure */
				shiftsym = 0; /* for all syms in closure: */
				while(( shiftsym = b_next( closure, shiftsym + 1 )) != ERROR )
				{
					if( shiftsym < Start ) /* shift on the terms in closure */
					{
						target = find_trans( item, shiftsym );
						if( target == ERROR ) fatal( "system transition error 1" );
						insert_action( i, shiftsym, target );
						continue;
					}
					clofirst = Entry[ shiftsym ]->closure;
					if( b_in( clofirst, 0 )) /* this sym in closure can be empty */
					{
						target = find_trans( item, shiftsym );
						if( target == ERROR ) fatal( "system transition error 2" );
						insert_Goto( i, shiftsym, target );
						/* E prod not in transition list */
						term = 0;
						follow = Entry[ shiftsym ]->follow;
						while(( term = b_next( follow, term + 1 )) != ERROR )
						{ /* reduce on its following terms (its shifts are in closure) */
							if( term > Start ) break;
							insert_action( i, term, Entry[ shiftsym ]->zero | REDUCT );
						}
						if( empty_parent && b_in( follow, 0 ))/* string may be empty */
						{
							term = 0;
							while(( term = b_next( ahead, term + 1 )) != ERROR )
							{ /* include item's lookahead */
								if( term > Start ) break;
								insert_action( i, term, Entry[ shiftsym ]->zero | REDUCT );
							}
						}
					}   /* endif empty symbol */
					else /* no E in shiftsym: shift on its l.a. terms */
					{
						b_enter( clofirst, shiftsym );
						term = 0;
						while(( term = b_next( clofirst, term + 1 )) != ERROR )
						{
							target = find_trans( item, term );
							if( target == ERROR ) fatal( "system transition error 3" );
							insert_action( i, term, target );
						} /* endwhile shift-terms */
					}  /* end if empty/non E */
				}   /* end closure */
			}    /* end if shift or reduce */
		}     /* endfor items of state */
	}      /* endfor all states */
}

/***** OPTIMIZE ACTIONS TABLE *****/

long optimize( short **oldtable, short *oldindex )
{   /* look for reductions matching 'ANY' & remove them */
	short *temp, i, moveindex, index, act;
	short max;
	long sum;

	sum = 0;
	for( i = 1; ( temp = oldtable[ i ] ) != NULL; i++ )
	{
		sum += oldindex[ i ];
		if( !( Option & NO_OPTIM ) && ( max = oldindex[ i ] ) > 0 )
		{
			act = temp[ max-1 ];
			if( !(act & REDUCT) ) continue; /* reducts at end */
			temp[ max-2 ] = ANY;
			index = 0;
			while( index < max-2 )
			{
				if( temp[ index+1 ] == act )
				{
					for( moveindex = index; moveindex < max-2; moveindex++ )
					temp[ moveindex ] = temp[ moveindex+2 ];
					max -= 2;
					oldindex[ i ] = max;
				}
				else index += 2;
			}
		}
	}
	sum += ( i * 2 ); /* voor EOL's */
	return( sum ); /* sum = number of ints in Action[] */
}

void optim_goto( short **table, short *index )
{
	short *temp, *tab;
	short i, inmax, ix;
	short temp_index, highest, count, obj, default_obj;

	for( i = 1; i <= Symnum; ++i )
	{
		tab = table[ i ];

		if( tab == NULL || index[ i ] == 0 )
		{
			printf( "no transitions on symbol: '%s'\n", Entry[ i+Start ]->name );
			++Errors;
			continue;
		}
		if( index[ i ] > 2 )
		{
			/* 1: create a temp list for all pairs of a row */
			temp = balloc( index[ i ] + 4, sizeof( short ));
			temp_index = 0;
			highest = 0;
			default_obj = 0;
			for( inmax = index[ i ] - 2; inmax >= 0; inmax -= 2 )
			{
				count = 0;
				obj = tab[ inmax + 1 ];
				for( ix = index[ i ] - 2; ix >= 0; ix -= 2 ) /* count occurrences */
				if( tab[ ix + 1 ] == obj ) ++count;
				if ( count > highest )
				{  /* most frequent one added at end of temp[] */
					default_obj = obj;
					highest = count;
					temp[ temp_index++ ] = tab[ inmax ];
					temp[ temp_index++ ] = obj;
				}
				else if( obj != default_obj ) /* add at front if not default */
				{
					if( temp_index > 0 )
					for( ix = temp_index-1; ix >= 0; ix-- )
					temp[ ix + 2 ] = temp[ ix ];
					temp[ 0 ] = tab[ inmax ];
					temp[ 1 ] = obj;
					temp_index += 2;
				}
			}
			table[ i ] = temp;
			index[ i ] = temp_index;
		}
		table[ i ][ index[ i ] - 2 ] = ANY;
	}
}

/*
	#] optim_goto :

	#] CREATE :
	#[ OUTPUT : 		output tables to disk

	#[ output_table :       send actions & goto tables to disk file
*/

void output_table( short **table, short *index, long sum ) /* stores new sum in 'Bytes' */
{
	short i, n, in, first, *temp, *tab;
	short temp_max, found;
	char c;
	/* concat all action lists into one string 'temp' */
	temp = malloc( sum * sizeof( short ) + 4L ); /* MS version */
	if( temp == NULL ) fatal( "out of memory" );
	temp_max = 0;
	for( i = 1; ( tab = table[ i ] ) != NULL; i++ )
	{
		found = FALSE;
		/* see if table[ i ] is already in temp ( equal substring ) */
		if( index[ i ] != 0 )
		{
			first = in = 0;
			while( (first + index[ i ]) < temp_max )
			{
				while( in < index[ i ] )
				{
					if( tab[ in ] == temp[ first + in ] ) { found = TRUE; ++in; }
					else { found = FALSE; break; }
				}
				if ( found && temp[ first + in ] == EOL ) break;
				found = FALSE;
				++first;
				in = 0;
			}
		}
		else
		{
			printf( "no actions in state %d\n", i );
			++Errors;
		}
		if ( !found )
		{
			first = temp_max;
			for( in = 0; in < index[ i ]; ++in ) temp[ temp_max + in ] = tab[ in ];
			temp_max += in;
			temp[ temp_max++ ] = EOL;
			temp[ temp_max++ ] = EOL;
		}
		index[ i ] = first;
		table[ i ] = &temp[ first ]; /* ptr into temp */
	}
	Bytes = 0;
	fprintf( Tabfile, "short Errtoken%s = %d;\n", Suffix, Errtoken );
	fprintf( Tabfile, "short Errstate%s[] = {", Suffix );
	for( i=0; i < Last; ++i )
	{
		if( !(i % 39) ) fprintf( Tabfile, "\n" );
		if( i < Last-1 ) c = ',';
		else c = ' ';
		fprintf( Tabfile, "%d%c", Errstate[ i ], c );
	}
	Bytes += (Last * sizeof( short ));
	fprintf( Tabfile, " };\n\nint Pstates%s[] = {\n", Suffix );
	for( i=0; i < temp_max; i += 2 )
	{
		if( i < temp_max-2 ) c = ',';
		else c = ' ';
		fprintf( Tabfile, "0x%x,0x%x%c", temp[ i ], temp[ i+1 ], c );
		if( temp[ i+1 ] == EOL ) fprintf( Tabfile, "\n" );
		Bytes += (2 * sizeof( short ));
	}
	fprintf( Tabfile, " };\n\nint *Pactions%s[] = {\n(short *)0L,\n", Suffix );
	for( i=1; table[ i ] != NULL; ++i )
	{
		if( table[ i+1 ] != NULL ) c = ',';
		else c = ' ';
		fprintf( Tabfile, "&Pstates%s[ %d ]%c\n", Suffix, index[ i ], c );
		Bytes += sizeof( short * );
	}
	/* print GOTO tab. re-use index[] */
	index = balloc( Symnum+2, sizeof( short ));
	index[ 1 ] = 2;
	fprintf( Tabfile, "\n };\n\nint Prow%s[]= { -1,-1 ,", Suffix );
	Bytes += (2 * sizeof( short ));
	for( i = 1; i <= Symnum; ++i )
	{
		tab = Go_to[ i ];
		in = Go_index[ i ];
		if( tab != NULL )
		for( n = 0; n < in; n += 2 )
		{
			fprintf( Tabfile, "%d,%d,", tab[ n ], tab[ n + 1 ] );
			Bytes += (2 * sizeof( short ));
		}
		if ( i == Symnum )
		{
			fprintf( Tabfile, " -1, -1 };\n" );
		}
		else
		{
			fprintf( Tabfile, " -1,-1,\n" );
			index[ i+1 ] = index[ i ] + in + 2;
		}
		Bytes += (2 * sizeof( short ));
	}
	fprintf( Tabfile, "\nint *Pgoto%s[] = {\n&Prow%s[ 0 ]", Suffix, Suffix );
	Bytes += sizeof( short * );
	for( i = 1; i <= Symnum; ++i )
	{
		fprintf( Tabfile, ",\n&Prow%s[ %d ]", Suffix, index[ i ] );
		Bytes += sizeof( short * );
	}

	fprintf( Tabfile, " };\n\nint Ptab%s[] = { -1,-1", Suffix );
	Bytes += (2 * sizeof( short ));

	for( i=1; i < Pr_index; ++i )
	{
		fprintf( Tabfile, ",%d,%d", Productions[ i ].p_left - Start,
		Productions[ i ].p_length );
		Bytes += (2 * sizeof( short ));
		if( !(i % 8) ) fprintf( Tabfile, "\n" );
	}
	fprintf( Tabfile, " };\n\nvoid Pattributes%s( p )\nint p; {\n", Suffix );
	fprintf( Tabfile, "switch( p ) {\n" );
	for( i=1; i < Pr_index; ++i )
	{
		(char *)temp = Productions[ i ].p_action;
		if( temp == NULL ) continue;
		fprintf( Tabfile, "case %d: %s\n\tbreak;\n", i, temp );
	}
	fprintf( Tabfile, "\t}\n}\n" );
}

void print_tab()    // print same tables to  .i file
{
	char *s, c;
	short a, i, n, *action, *go_to;

	fprintf( Optfile, "\n\nPARSING TABLE:\n" );
	for( i = 1; i < Last; ++i )
	{
		action = Action[ i ];
		if( Errstate[ i ] ) c = '*';
		else c = ' ';
		fprintf( Optfile, "%cState %d: ", c, i );
		if( action == NULL ) { fprintf( Optfile, "NO ACTIONS\n" ); continue; }
		for( n = 0; action[ n ] != EOL; n += 2 )
		{
			if( action[ n ] == ANY ) s = "ANY";
			else if( action[ n ] == END ) s = "END";
			else  s = Entry[ action[ n ] ]->name;
			a = action[ n + 1 ];
			if( a & REDUCT )
			{
				a &= ~REDUCT;
				if( a == 0 ) fprintf( Optfile, "%s,ACCEPT;", s );
				else fprintf( Optfile, "%s,R(%d);", s, a );
			}
			else fprintf( Optfile, "%s,%d;", s, a );
		}
		fprintf( Optfile, "\n" );
	}

	fprintf( Optfile, "\nGOTO-TABLE:\n" );
	for( i = 1; i <= Symnum; ++i )
	{
		fprintf( Optfile, "'%s' :", Entry[ i+Start ]->name );
		if( (go_to = Go_to[ i ]) == NULL )
		fprintf( Optfile, " :no GOTO transitions" );
		else
		{
			for( n = 0; n < Go_index[ i ]; n += 2 )
			{
				if ( go_to[ n ] == ANY )
				fprintf( Optfile, "[ANY:%d]", go_to[ n + 1 ] );
				else
				fprintf( Optfile, "[%d:%d]", go_to[ n ], go_to[ n + 1 ] );
				if( n && !( n % 20 ) ) fprintf( Optfile, "\n\t" );
			}
		}
		fprintf( Optfile, "\n" );
	}
	fprintf( Optfile, "\n" );
}
/*
main( argc, argv )
short argc;
char *argv[];
{
short argptr;
long sum;
short i, l, dot;
char *currpar, *filename, name[ 25 ];
static char errmsg[] = "USE: pg [ -i-l-n-pdigits -digits] filename";
TABPTR ptr;

Linenum = ERROR;
if ( argc<2 ) fatal( errmsg );

for ( argptr=1;( currpar = argv[ argptr ] ) != NULL; ++argptr )
{
if ( *currpar == '-' )
{
	if ( *( currpar+1 ) == 'i' )  Option |= PRINT;
	else if ( *( currpar+1 ) == 'l' )  Option |= LEX;
	else if ( *( currpar+1 ) == 'n' )  Option |= NO_OPTIM;
	else if ( *( currpar+1 ) == 'p' ) Prod_max = get_num( currpar+2 );
	else if ( isdigit( *( currpar+1 )))
	{
	strcpy( Suffix, currpar+1 );
	strcpy( Namebuf, "pgtab" );
	strcat( Namebuf, Suffix );
	strcat( Namebuf, ".c" );
	}
	else fatal( errmsg );
}
else break;
}

if( currpar == NULL )
fatal( "PG:missing input filename" );

l = strlen( currpar );
for( dot = i = 0; i < l ; ++i )
{
if( currpar[ i ] != '.' )
	name[ i ] = currpar[ i ];
else
{
	dot = TRUE;
	break;
}
}
name[ i ] = '\0';
if( dot ) filename = currpar;
else filename = strcat( name, ".p" );

if(( Infile = fopen( filename, "r" )) == NULL )
{
printf( "PG:input file '%s' ", filename );
fatal( "not found" );
}

if( Option & PRINT )
{
name[ i ] = '\0';
strcat( name, ".i" );
if(( Optfile = fopen( name, "w" )) == NULL )
	fatal( "PG:can't open .i file" );
Errfile = Optfile;
}
else Optfile = Errfile = stdout;

if( Option & LEX )
{
name[ i ] = '\0';
strcat( name, ".l" );
if (( Tabfile = fopen( name, "w" )) == NULL )
	fatal( "PG:can't open .l file" );
}
Linenum = 0;
Entry = balloc( Tabsize+1, sizeof( TABPTR ));
Text = balloc( Insize+1, sizeof( char ));
Productions = balloc( Prod_max+1, sizeof( struct prodtype ));

read_terms();

if( Option & LEX )
fclose( Tabfile );

if (( Tabfile = fopen( Namebuf, "w" )) == NULL )
fatal( "PG:can't open pgtab.c file" );

putstring( Text, "ERROR", 5 );
ptr = install();
Entry[ Start ] = ptr;
ptr->symbol = Errtoken = Start;
ptr->index = -1;

++Start;
*Firstprod = Start+1;
ptr = balloc( 1, sizeof( TABENTRY ));
Entry[ Start ] = ptr;
ptr->name = "{START}";// unique START sym: parent of all, child of none
Productions[ 0 ].p_left = ptr->symbol = Start;
Productions[ 0 ].p_rule = Firstprod;
Productions[ 0 ].p_length = 1;

read_nonterms();
fclose( Infile );

Linenum = ERROR;
Uniset = createset( N_OF_SYMBOLS );

printf( "PG:checking...\n" );
check_syms();

if( Errors == 0 )
{
first_syms();
printf( "items...\n" );
items();
printf( "actions...\n" );
actions();
printf( "tables...\n" );
sum = optimize( Action, Action_index );
optim_goto( Go_to, Go_index );
output_table( Action, Action_index, sum );
if( RRconfl ) printf( "%d R-R conflicts found.\n", RRconfl );
if( SRconfl ) printf( "%d S-R conflicts found.\n", SRconfl );
if( !SRconfl && !RRconfl && !Errors ) printf( "OK\n" );
printf( "You used %d terminals, %d nonterminals,\n", Start-2, Symnum );
printf( "%d out of %d productions.\n", Pr_index, Prod_max );
printf( "the parsing actions table has %d states.\n", Last );
if( Option & PRINT )
{
	dump();
	dump_pr();
	printstates();
	print_tab();
}
}
if( Errors ) printf( "%d errors in grammar\n", Errors );
if( Bytes ) printf( "Total size of tables:%ld bytes.\n", Bytes );
else printf( "No table created.\n" );
if( Option & PRINT ) fclose( Optfile );
fclose( Tabfile );
return( 1 );
}
*/


