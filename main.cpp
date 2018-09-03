#include <memory.h>                              // For memcpy and memset
#include <iostream>
#include <string>
#include <time.h>
#include <stdlib.h>
#ifdef USE_DEBUG_NEW
    #include "debug_new.h"
#endif

#include "bitsets.h"
#include "storage.h"
#include "alex.h"

using namespace std;
MixedPool MyPool;
void test();


int main(int argc, char* argv[])
{
//    wstring str;
//    wstring str1 = L"C:\\MinGW\\64\\projects\\test1proj\\alex.h";
//    wstring str2 = L"C:\\MinGW\\64\\projects\\test1proj\\bitsets.cpp.save";
//    wstring str3 = L"C:\\Users\\WinJeroen\\Documents\\RAD Studio\\Projects\\unilex2 - Copy\\bigE_1.txt";
//
//	Alex lex;
//
//	cout << lex.setInputBuffer(str1) << endl;
//    if( lex.openfile(str1)) lex.hexdump( lex.inputbuffer );
//	cout << endl;
//    wcin.get();
    Bitset a( 0, 10 );
    Bitset b( 11,20 );
    Bitset c( 21, 30 );
    Bitset c2( 21, 30 );
    Bitset d( 31, 40 );
    a.b_insert( 2 );
    b.b_insert( 12 );
    c.b_insert( 22 );
    c2.b_insert( 23 );
    d.b_insert( 32 );
    Uniset u;
    Uniset u2;
    u2.u_unify( a );
    u2.u_unify( b );
    u.u_unify( d );
    u.u_unify( c );
    u.u_unify( c2 );
    u2.u_unify( u );

    u.u_printset();
    u2.u_printset();

    return(0);
}


//void test()
//{
//	clock_t t1, t2;
//	int i;
//    Bitset b( 10,20 );
//
//	t1 = clock();
//
//    for (i=0; i<=100000000; ++i )
//    {
//        b.b_insert1(12);
//    }
//	t2 = clock();
//	wcout << endl << "time1: " << t2 - t1 << endl;
//}

