/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* AlexIn methods *************************************compile with c++11
////////////////////////////////////////////////////////////////////////////
==========================================================================*/
#include <windows.h>
#include <fstream>
#include <iostream>
#include "inputmanager.h"

AlexIn::AlexIn()
{
    inputbuffer = 0;
    filesize = allocedbufsize = 0;
}

AlexIn::~AlexIn()
{
    if( inputbuffer ) delete[] inputbuffer;
    inputbuffer = 0;
}

UNIT AlexIn::setInputBuffer( const wstring inputfilepath )
{   //create a buffer fitting (if possible) the size of the input file
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx(&statex);
    UNIT freemem = statex.ullAvailPhys; // get available free RAM

    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    //DWORD attributes;
    UNIT fsize, alignedsize;

    hFind = FindFirstFile(inputfilepath.c_str() , &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        fsize = FindFileData.nFileSizeHigh;
        fsize <<= sizeof( FindFileData.nFileSizeHigh ) * 8;
        fsize |= FindFileData.nFileSizeLow;
    }
    else
    {
        return MINUSONE;// error "file not found"
    }
    filesize = fsize;
    fsize = (fsize + 31) & -32; //make divisible by 32 (so half is div. by 16)
    alignedsize = 4 * (freemem / 32); // claim 1/8th of free memory
    if( fsize > alignedsize ) fsize = alignedsize; //buffer smaller than file
    if( fsize > allocedbufsize ) //realloc if larger than existing buffer
    {
        allocedbufsize = fsize;
        if( inputbuffer ) delete[] inputbuffer;
        inputbuffer = new char[ allocedbufsize ];
        if( inputbuffer == 0 ) return 0; //alloc error
    }
    if( fsize > allocedbufsize )
    {
        bufferlimit = inputbuffer + allocedbufsize / 2;
    }
    else
    {
        bufferlimit = inputbuffer + allocedbufsize;
    }
    return fsize; // rtn buffersize or -1: no file, 0: no memory, stores total buffer size in allocedbufsize, address in inputbuffer
}

bool AlexIn::openfile( const wstring name )
{
    string str( name.begin(), name.end() );
    std::ifstream file1(str,ios::binary|ios::in);
	if (file1.is_open()==false)
    {
		cout << "Cannot open file\n";
        cout << "read:" << file1.gcount() << "; f-" << file1.fail()  << " b-" << file1.bad()  << " e-" << file1.eof()  << endl;
        return false;
    }
	else
    {
		file1.seekg(0, ios::end);
		unsigned len= file1.tellg();
		file1.seekg(0, ios::beg);

		file1.read(inputbuffer, len);

	}
	file1.close();
	return true;
}

