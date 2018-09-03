//  Sample output:
//  There is       51 percent of memory in use.
//  There are 2029968 total KB of physical memory.
//  There are  987388 free  KB of physical memory.
//  There are 3884620 total KB of paging file.
//  There are 2799776 free  KB of paging file.
//  There are 2097024 total KB of virtual memory.
//  There are 2084876 free  KB of virtual memory.
//  There are       0 free  KB of extended memory.
//#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <tchar.h>
#include "defs.h"
using namespace std;
// Use to convert bytes to KB
#define DIV 1024

// Specify the width of the field in which to print the numbers.
// The asterisk in the format specifier "%*I64d" takes an integer
// argument and uses it to pad and right justify the number.
#define WIDTH 7
int testfilenames();

int main()
{
  MEMORYSTATUSEX statex;

  statex.dwLength = sizeof (statex);

  GlobalMemoryStatusEx (&statex);

  _tprintf (TEXT("There is  %*ld percent of memory in use.\n"),
            WIDTH, statex.dwMemoryLoad);
  _tprintf (TEXT("There are %*I64d total KB of physical memory.\n"),
            WIDTH, statex.ullTotalPhys/DIV);
  _tprintf (TEXT("There are %*I64d free  KB of physical memory.\n"),
            WIDTH, statex.ullAvailPhys/DIV);
  _tprintf (TEXT("There are %*I64d total KB of paging file.\n"),
            WIDTH, statex.ullTotalPageFile/DIV);
  _tprintf (TEXT("There are %*I64d free  KB of paging file.\n"),
            WIDTH, statex.ullAvailPageFile/DIV);
  _tprintf (TEXT("There are %*I64d total KB of virtual memory.\n"),
            WIDTH, statex.ullTotalVirtual/DIV);
  _tprintf (TEXT("There are %*I64d free  KB of virtual memory.\n"),
            WIDTH, statex.ullAvailVirtual/DIV);

  // Show the amount of extended memory available.

  _tprintf (TEXT("There are %*I64d free  KB of extended memory.\n"),
            WIDTH, statex.ullAvailExtendedVirtual/DIV);

    testfilenames();
}

int testfilenames()
{
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind;
   LPCTSTR  lpFileName = L"C:\\Users\\WinJeroen\\Documents\\*.*";
   ULONGLONG FileSize;
   DWORD attributes;
   string s;

   hFind = FindFirstFile(lpFileName , &FindFileData);
   if (hFind == INVALID_HANDLE_VALUE)
   {
      printf ("File not found (%d)\n", GetLastError());
      return -1;
   }
   else
   {
       do
       {
          attributes = FindFileData.dwFileAttributes;
          if( attributes & 0x10 ) continue; // directory
          FileSize = FindFileData.nFileSizeHigh;
          FileSize <<= sizeof( FindFileData.nFileSizeHigh ) * 8;
          FileSize |= FindFileData.nFileSizeLow;
          wcout << FindFileData.cFileName << L" :file size is: ";
          //if( FileSize / mByte ) { wcout << ( FileSize / mByte ) << "."; FileSize %= mByte; }
          //if( FileSize / kByte ) { wcout << ( FileSize / kByte ) << "."; FileSize %= kByte; }
          wcout << FileSize << std::endl;
       }
       while (FindNextFile( hFind, &FindFileData ));
      //_tprintf (, FileSize);
      FindClose(hFind);
   }
   return 0;

}

