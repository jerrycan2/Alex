#include "stdio.h"

#define ANY    0x7fff
#define REDUCT 0x8000
#define ACCEPT 0x8000
#define PARSE_DEF dummy voor parse.h

#include "parse.h"

extern int *Pactions[];
extern int *Pgoto[];
extern int Ptab[];

int Stackptr;
int Pstack[ PGSTACKSIZE ];
ATYPE Astack[ PGSTACKSIZE ];
int Trace;
int Lexerr;

#include "pgtab.c"

int get_action( state, token )
int state, token;
{
    int *statelist, sym;

    statelist = Pactions[ state ];
    for( ; ( state = *(statelist+1) ) != ERROR; statelist += 2 )
      if(( sym = *statelist ) == token || sym == ANY ) break;
    return( state );
}

int nexttoken()
{
    int token;

    Lexerr = FALSE;
    if(( token = Alex()) == ERROR )
    {
      if( L_eof ) token = 0;
      else
      {
	Lexerr = TRUE;
	Ungettoken();
        while(( token = Alex()) == ERROR ) Eind = ++Begin;
	Ungettoken();
	token = Errtoken;
      } /* lexical error( unknown char): skip */
    }
    return( token );
}

int reduce( state )
int state;
{
        register int *statelist, *ptr, newstate, nterm;

        ptr = Ptab + 2*state;
        if(( nterm = *ptr ) == ERROR ) return( ERROR ); /* can't be !? */
        Stackptr -= *( ptr + 1 ); /* production length */
	Pattributes( state ); /* evaluate attributes */
        /* search in Pgoto */
        state = Pstack[ Stackptr++ ];
        statelist = Pgoto[ nterm ];
        for( ; ( newstate = *statelist ) != ANY; statelist += 2 )
          if( state == newstate ) break;
        Pstack[ Stackptr ] = *( statelist + 1 );
	return( 0 );
}

int parse()
{
    int token, target; /* target = target state of shift action */
    int currstate, errshift; /* current state on top of stack */

    errshift = FALSE; /* flag to make parser "skip until shift" after error */
    Stackptr = 0;
    Pstack[ 0 ] = 1;
    while( 1 )
    {
      currstate = Pstack[ Stackptr ];
      token = nexttoken();
      if( Lexerr ) target = ERROR;
      else target = get_action( currstate, token );
      /* search token in list for that state */

      if( Trace ) 
      {
	printf( "Stackptr %d, state %d, ", Stackptr, currstate );
        if( target & REDUCT )
	  printf( "R %d, " , target&0x7fff );
	else
	  printf( "S %d, " , target );
	printf( "token %d\n", token );
      }

      if( target == ACCEPT )
      {
	if( token == 0 ) return( 0 ); /* ok out */
	else 
	{
	  target = ERROR;
	}
      }
      if( target == ERROR )
      {
	if( errshift ) continue; /* error found, but no shift yet */
	while(( target = get_action( currstate, Errtoken )) & REDUCT )
	{ /* find a state with shift on Errtoken: (ERROR also & RED.) */
	  if( Stackptr == 0 ) return( ERROR );
	  currstate = Pstack[ --Stackptr ];
	}
        currstate = target & ~REDUCT; /* target state is new current state */
        Pstack[ ++Stackptr ] = currstate; /* shift virtual ERRTOKEN */

	while(( target = get_action( currstate, token )) == ERROR )
	{
	  token = nexttoken();
	}
	if( target & REDUCT ) 
	  errshift = TRUE;
	else
	  errshift = FALSE;
      }
      if( target & REDUCT ) /* reduce */
      {
        Ungettoken(); /* target minus REDUCT = production number index */
        if( reduce( target & ~REDUCT ) == ERROR )
	  return( ERROR );/* error in parsing table? */
      }
      else /* shift */
      {
	errshift = FALSE;
        Pstack[ ++Stackptr ] = target;
        Astack[ Stackptr ].p = Lexval;
      }
      if( Stackptr >= PGSTACKSIZE || Stackptr < 0 ) return( ERROR );
    }
}

