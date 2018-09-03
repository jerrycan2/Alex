;	rxp.p door J.Bakker 16-4-92
;	syntax voor regular expressions
;	compile: 1. pg -l rxp
;			 2. pm -c -f -l rxp.l
;			 3. zie pp.prg
;
; 	#[ TERMINAL SYMBOLS : lexemes recognized by Alex()
;	\x25 = '%' (bug in pg)
;	==================== MACRO'S: (zie ook pm.mac) ==========================
						%:REST		{ ([^\r\n])* }
						%:ID  		{ [A-Za-z_]([0-9A-Za-z_])* }
						%:HEX 		{ [0-9A-Fa-f] }
						%:ANYCHAR 	{ [^ ' " { } \x25 ] }
						%:STR1 		{ " ([^"])* " }
						%:STR2 		{ ' ([^'])* ' }
						%:NM   		{ {SPC} ([0-9])+ {SPC} }
                       %:SPCS      { (\s|\t)+ }
;       ==================== TERMINAL SYMBOLS: ==================================
1		space			%1:{SPCS} 		 ::if( SpaceCounts ) RETURN;
;	backslashed characters (C-type):
		E8	  2 L		%\\0([0-7])*	 ::Lexval.i=getoct(P_TEXT+2);RETURN;
		E10   2 L		%\\[1-9]([0-9])* ::Lexval.i=getnum(P_TEXT+1);RETURN;
		E16   2 L		%\\(x|X){HEX}+	 ::Lexval.i=gethex(P_TEXT+2);RETURN;
		E1	  2 L		%\\[^\n]		 ::Lexval.i=backslash(P_TEXT+1);RETURN;
;	string: delim. " or '. No newlines in a string.
		STRING 2 L		%({STR1}|{STR2}) ::Lexval.p = P_TEXT; RETURN;
;
		MACDEF			%\${ID}{SPC}={REST} ::DefMacro(P_TEXT+1,P_LEN-1);\
													 RETURN;
;	macro substitution (invisible to parser)
		MACRO 			%${ID} 		 ::ExpandMacro( parstab );
;	COUNTER: return an index to a COUNTFIELD
		COUNT 3 N		%\{{NM}(,{NM})?\} ::Lexval.p=Counter(parstab,P_TEXT);\
													 RETURN;
;	Closure operators: (hoogste prioriteit)
		*	  3 N		%\* 			 ::RETURN;
		+	  3 N		%\+ 			 ::RETURN;
		?	  3 N		%\? 			 ::RETURN;
;
		OR	  1 L		%\| 			 ::RETURN;
		(	  2 L		%\( 			 ::RETURN;
		)	  2 L		%\) 			 ::RETURN;
		[	  2 L		%\[ 			 ::RETURN;
		]	  2 L		%\] 			 ::RETURN;
		-	  2 L		%\- 			 ::RETURN;
		.	  2 L		%\. 			 ::RETURN;
		^	  2 L		%\^ 			 ::RETURN;
;		$	  2 L		%\$ 			 ::RETURN;
		CHAR  2 L		%{ANYCHAR}		 ::Lexval.i = \
                               (IgnoreCase?tolower(*P_TEXT):*P_TEXT); RETURN;
; 	#] TERMINAL SYMBOLS :
%%
; 	#[ SYNTAX : parser syntax and attribute actions
line    =   expr            { S_S.p = build( S_1.p, NULL, NEWLINE, 0 ); }
       |   ^ expr           { S_S.p = build( S_2.p, NULL, NEWLINE, BOL ); }
		|	MACDEF			 { S_S.p = NULL; }
		|	ERROR			 { S_S.p = build( NULL, NULL, PERROR,  STACKERR ); }
		;
expr	=	symbol			 { S_S.p = build( NULL, NULL, NEWCHAR,	S_1.i ); }
       |   mkspace          { S_1.p = build( NULL, NULL, NEWCHAR,	S_1.i );\
                              S_S.p = build( S_1.p, NULL, ZERO_MORE, 0	 );  }
		|	STRING			 { S_S.p = build( S_1.p, NULL, STRING,	0	 ); }
2 L 	|	expr expr		 { S_S.p = build( S_1.p, S_2.p, CONCAT,	0	 ); }
		|	expr OR expr	 { S_S.p = build( S_1.p, S_3.p, UNIFY, 	0	 ); }
		|	( expr )		 { S_S.p = S_2.p; }
		|	expr *			 { S_S.p = build( S_1.p, NULL, ZERO_MORE, 0	 ); }
		|	expr +			 { S_S.p = build( S_1.p, NULL, ONE_MORE,	0	 ); }
		|	expr ?			 { S_S.p = build( S_1.p, NULL, ZERO_ONE,	0	 ); }
		|	expr COUNT		 { S_S.p = build( S_1.p, S_2.p, COUNTER,	0    ); }
		;
symbol	=	teken			 { S_S.i = S_1.i; }
		|	[ chars ]		 { S_S.i = new_charset( Newset ); }
		|	.				 { Newset = createset( CSETSIZE ); \
                              Complement_set = TRUE; \
                              S_S.i = new_charset( Newset ); }
		;
mkspace =  space            { Newset = createset( CSETSIZE ); \
                              b_enter( Newset, ' ' ); \
                              b_enter( Newset, '\t' ); \
                              S_S.i = new_charset( Newset ); }
       ;
teken	=	CHAR
		|	E1
		|	E8
		|	E10
		|	E16
		;
chars	=	chars crange
		|					 { Newset = createset( CSETSIZE ); }
		;
crange	=	teken - teken	 { AddToSet( Newset, S_1.i, S_3.i ); }
		|	teken			 { AddToSet( Newset, S_1.i, S_1.i ); }
		|	^				 { Complement_set = TRUE; }
		;
; 	#] SYNTAX : */
%%
/* code: zie pp.c */
