//stringlist *getLargestFilesize( wstring dirname )
//{
//    wstring all = L"\\*.*";
//    WIN32_FIND_DATA FindFileData;
//    HANDLE hFind;
//    LPCTSTR  lpFileName;
//    UNIT FileSize, largest;
//    DWORD attributes;
//    stringlist *names = new(stringlist);
//
//    if( dirname == L"" ) lpFileName = L"*.*";
//    else lpFileName = (dirname + all).c_str();
//    largest = 0;
//    hFind = FindFirstFile(dirname.c_str() , &FindFileData);
//    if (hFind != INVALID_HANDLE_VALUE)
//    {
//        do
//        {
//            attributes = FindFileData.dwFileAttributes;
//            if( attributes & 0x10 ) continue; // directory
//            FileSize = FindFileData.nFileSizeHigh;
//            FileSize <<= sizeof( FindFileData.nFileSizeHigh ) * 8;
//            FileSize |= FindFileData.nFileSizeLow;
//            if( FileSize > largest ) largest = FileSize;
//            names->push_back(wstring(FindFileData.cFileName));
//            //wcout << FileSize << ",";
//        }
//        while (FindNextFile( hFind, &FindFileData ));
//        //wcout << lpFileName << endl;
//        FindClose(hFind);
//    }
//    return names; // rtn 0 if nothing found
//}
