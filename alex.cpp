#include <iostream>
#include <fstream>

#include <windows.h>
#include <string>
#include <wchar.h>
#include "bitsets.h"
#include "alex.h"

using namespace std;

//==========================================================================
////////////////////////////////////////////////////////////////////////////
/************************ class Nfa ***************************************/
////////////////////////////////////////////////////////////////////////////
//==========================================================================
Nfa::Nfa()
{
    states = new Nfastate[ NFA_STATES_ALLOC ]();
    states_allocated = NFA_STATES_ALLOC;
    n_of_states = 1;
    initial_state = 0;
}

Nfa::~Nfa()
{
    delete[] states;
}

Nfastate *Nfa::getstate( const unsigned i ) // ptr valid only until the next newstate()
{
    if( i >= n_of_states ) throw( NFA_Error( L"nfastate[] index out of range" ) );
    else return( states + i );
}

unsigned Nfa::newstate( const Nfastate& state )
{
    if( n_of_states >= states_allocated )
    {
        Nfastate *newptr = new Nfastate[ states_allocated + NFA_STATES_ALLOC ];
        memcpy( newptr, states, n_of_states * sizeof(Nfastate));
        states_allocated += NFA_STATES_ALLOC;
        states = newptr;
    }
    states[ n_of_states++ ] = state;
    return( n_of_states - 1 ); //return index of new Nfastate
}

unsigned Nfa::newstate()
{
    return newstate( {0,0,0,tt_error,0} );
}






/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* Utilities (global) *******************************
////////////////////////////////////////////////////////////////////////////
==========================================================================*/
//void Alex::hexdump( char *address )
//{
//    string str;
//    const int width = 16;
//
//    cout << "\naddr: " <<  setfill( '0' ) << setw( 16 ) << hex << (unsigned long long)address << endl;
//    while( str != "q" )
//    {
//        for( int j = 0; j < 8; ++j )
//        {
//            cout << setw(4) <<  setfill( '0' ) << ".." << ((unsigned long long)address & 0xFFFF) << " ";
//            for( int i=0; i < width; ++i )
//            {
//                printf("%02X ", (address[ i ] & 0xFF)); //why do I need this FF?
//            }
//            cout << "  ";
//            for( int i=0; i < width; ++i )
//            {
//                cout << (isprint(address[ i ]) ? address[ i ] : '.');
//            }
//            address += width;
//            if(address >= bufferlimit)
//            {
//                cout << "end of file";
//                cin >> str;
//                return;
//            }
//            cout << endl;
//        }
//        cout << "(q=quit):";
//        cin >> str;
//    }
//}
//


