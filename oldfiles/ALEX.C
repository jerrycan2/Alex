/*********************************************/
/*	ALEX.C v2.0 	J.Bakker     19-2-90 */
/*********************************************/

#include "stdio.h"
#define LEX_DEF dummy
#include "parse.h"

#define EMPTYTRANS	(Eind==Begin)
#define BUFSIZE		256
#define NOTHING		-1
#define NOTRANS		-1
#define CHARSET		0x100
#define RETRACT		0x200
#define UCHAR		unsigned char

FILE *Textfile; /* file ptr for gettext() */

int Begin,Eind,Token;
int EOF_flag = -1; /* gettext flag: if !-1, return EOF if text[eind]=flag */
static int Accept,Accend,*Retacc,*Retro;
static int Bufsize = 2 * BUFSIZE;
static int Bufflag;

char *realloc(), *malloc();
int b_in();  /* in libc.a */

char *Text; /* char*  buffer, filled by gettext(). */
int *Lexval;
int L_eof;
#include "lextab.h"     /* here comes the DFA table from pm.prg */
 
 /* lextab.h is made up of:
   int Tokens[] = list of tokens, indexed by 'state nr' i.
   unsigned *Setarray[] = array of ptrs to bitsets,accessed by b_in().
   int States[] =  list of {shift_char(set) , next_state} pairs. Divided into
		   i sublists, each ending with a {Tokens[i], -1 } pair .
		   The DFA will shift until such a pair is reached.
   int *Transitions[] = list of i ptrs to sublists (&States[ x ]).
   Plus any attribute code or definitions used by 'lexact.h'

*/


unsigned gettext()
{
	register char *text;
	register int eind, begin;

	eind = Eind;
	begin = Begin;
	if(( text = Text ) == NULL )
	{
	  Text = text = malloc( Bufsize+1 );
	  *text = '\0';
	}
	if( EOF_flag != ERROR )
	  if( EOF_flag == (unsigned)text[ eind ] ) 
		{ L_eof = TRUE; return( FALSE ); }

	if ( text[ eind ] == '\0' )
	{
	    if ( begin > 0 )
	      bcopy( text, text + begin, eind -= begin );
	    Eind = eind; /* move string to start of buffer */
	    Begin = 0;
	    if ( *Retacc ) *Retro = eind; /* Retract is going on */

	    if (( Bufsize - eind ) <= BUFSIZE ) /* check for overflow */
	    {
		Bufsize += BUFSIZE;	 /* make buffer large enough */
		Text = text = realloc( text, Bufsize );
		if ( text == NULL ) fatal( "out of memory" );
	    }
	    text += eind;
	    if ( fgets( text, BUFSIZE , Textfile ) == NULL ) /* fill buf */
		{ L_eof = EOF; return ( FALSE ); } /* EOF */
	    return( (unsigned)*text ); /* return char */
	}
	return( (unsigned)text[ eind ] ); /* return char */
}

int match( c )
register unsigned c;
{
	register unsigned t;

	if (( t = gettext()) == FALSE ) return( FALSE ); /* EOF */
	if ( c & CHARSET )
	 {
	    if ( !b_in( Setarray[ c&255 ], t )) return( ERROR );/* no match */
	 } 
	 else if ( c & RETRACT ) /* it can't happen here !? */
					fatal( "sorry:internal error" );
	 else if ( c != t ) return( ERROR ); /* no match */
			
	 ++Eind;  /* increase Eind only if matched */
	 return( TRUE );
}  /* returns FALSE if EOF, TRUE if matched, ERROR otherwise */

/******************************/
/*	LEXIN: get a token    */
/******************************/

int lexin( i ) /* get the longest match ( possibly empty ) */
register int i; /* i: starting state of DFA ( i>0 ) */
{
	register int *trans;
	int matched;
	int retacc,retro,rtemp;

	/* Transitions[] : array of ptrs to transitions */
	/* *(Transitions[m]+n) n = even: transition char */
	/* *(Transitions[m]+n+1) next state ( 1..end )  if -1(NOTRANS):
		 			*(Transitions[m]+n)= token  */
	/* No token: 0 ( You ain't seen NOTHING yet ) */

	retacc = NOTHING;
	retro = FALSE;
	if ( Retacc == NULL || *Retacc == NOTHING )
	{
	    Retacc = &retacc; /* set global ptrs to 'lookahead' results */
	    Retro = &retro;
	}

	if (( trans=Transitions[ i ] ) == NULL || *(trans + 1) == NOTRANS )
					 /* no trs-list */
	{ 
	    
	    return( gettext() == FALSE ? FALSE : ERROR );
	}

	Accend = Eind; 
	Accept = Tokens[ i ];

	while ( ( i = *( trans + 1 ) ) != NOTRANS )
	{

	    while ( *trans & RETRACT ) 
	    {  /* #1 if RETRACT, save Eind,token */
		if ( *( trans + 1 ) == NOTRANS ) break; /* safety catch */
		retro = Eind;
		retacc = *trans & 255;
		rtemp = lexin( *( trans+1 ));
		if ( rtemp != retacc ) /* could be EOF!? (FALSE) */
		  { 
			Eind = retro; 
			trans += 2; 
			continue; 
		  }
		Eind = retro;
		return( retacc );
	    }

	    if ( *( trans + 1 ) == NOTRANS ) break; /* dropped out of loop */

	    if (( matched = match( *trans )) == TRUE ) /* no retracts here */
	    {		/* if a match occurred, Eind has been increased */
		if ( retacc != NOTHING )
		{
		    Accend = Eind;   /* ...store it */
		    Accept = Tokens[ i ];
		}
		trans = *( Transitions + i );
	    }
	     else if ( matched == ERROR ) trans += 2;
	     else return( FALSE ); /* EOF */
	}
	if ( *trans != NOTHING )  return( *trans ); /* return token */
	
	Eind = Accend;  /* rewind */
	return( Accept );
	/* return previously accepted token ( possibly NOTHING ) */
}

int Alex() /* will be called by the parser, for instance */
{
        if( Bufflag == TRUE )
        {
            Bufflag = FALSE;
            return( Token );
        }

	while ( 1 )
	{
            Begin = Eind;
	    switch( Token = lexin( 1 ) )
	    {
		default: if( Token == 0 ) return( 0 ); /* EOF if token 0 */
                         break; /* otherwise do nothing */
	        case ERROR: return( ERROR );/* lexical error */

        #include "lexact.h"  /* enter the userdef "lexical actions" */

	    }
            if( Eind == Begin ) Eind = ++Begin; /* no empty patterns! */
	}
}

void Ungettoken()
{
    Bufflag = TRUE;
}

