//---------------------------------------------------------------------------
//#define _UNICODE
#include <vcl.h>
#pragma hdrstop

#include <tchar.h>
#include <wchar.h>
#include "Unit1.h"
#include "Main.h"
#include "Unit2.h"
#include "optionsdialog.h"
#include "ConfDialog.h"
#include "filexdialog.h"
#include "Bar.h"
#include "KeyForm.h"
#include "ArrayForm.h"
#include "dock.h"
#include <stdio.h>
#include <ctype.h>
//#include <istream>
#include "fstream.h"
#include "iostream.h"
#include <mbstring.h>
#include "definitions.h"
//#include "WideStrings.hpp"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

extern "C" void chrout( char );
extern *PatFile;
extern TFileStream* ResFile;
//extern FILE *OutFile;
extern TStringList *OutFile;
extern int IgnoreCase;
extern int Parserror;
extern int BreakOff;
extern unsigned long *CharMap;  // maps uppercase <-> lowercase
extern char *Utf8, *Utf16LE, *Utf16BE;

unsigned short NonBolToken;
PARSTAB *Alextab;
TForm1 *Form1;
int RegelNummer = 1;

//---------------------------------------------------------------------------

class FastString
{
  private:
    wchar_t *fs;
    unsigned long ActualLength;
    unsigned long Size; // both in wchar_t!
  public:
    FastString(){ Size = 256 * 128; ActualLength = 0;
    	fs = (wchar_t*)malloc( Size * sizeof(wchar_t) ); 
        if( fs == 0 ) throw( "out of memory error" );
        else fs[ 0 ] = 0;
    }
    FastString( unsigned long sz ){ Size = sz; ActualLength = 0;
    	fs = (wchar_t*)malloc( sz * sizeof(wchar_t) ); 
        if( fs == 0 ) throw( "out of memory error" );
        else fs[ 0 ] = 0;
    }
    ~FastString(){ free( fs ); }
    void AddChar( wchar_t c ){
    	if( ActualLength >= Size-1 ) {
            Size = 2 * Size;
        	fs = (wchar_t*)realloc( fs, Size * sizeof(wchar_t));
        }
        if( fs == 0 ) throw( "out of memory error" );
        else { fs[ ActualLength++ ] = c; fs[ ActualLength ] = 0; }
    }
	void AddString( UnicodeString s ){
    	int i=0;
        wchar_t *buf;
        if( s == "" ) return;
		buf = s.c_str();
        while( 1 ) {      
			if((fs[ ActualLength++ ] = buf[ i++ ]) == 0 ) break;
			if( ActualLength >= Size ) {
				Size = 2 * Size;
				fs = (wchar_t*)realloc( fs, Size * sizeof(wchar_t));
				if( fs == 0 ) throw( "out of memory error" );
			}
		}
		--ActualLength;
	}
	UnicodeString GetString( void ){
		return UnicodeString( fs );
	}
	void Clear( void ) { ActualLength = 0; fs[ 0 ] = 0; }
};

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
: TForm(Owner)
{
	Width = MainForm->Width;
	FileList = new TStringList;
	RichEdit1->PlainText = false;
	RichEdit1->WordWrap = true;
	RichEdit1->HideSelection = false;
	RichEdit1->Font->Name = MainForm->DefaultFont;
	FBox1->Font->Name = MainForm->DefaultFont;
	TargetBox1->Font->Name = MainForm->DefaultFont;

	TileMode = tbVertical;

	//AnsiString s = AnsiString(MainForm->DefaultFont);
	//FontDialog1->Font->Name = s;
	//FontDialog1->Font->Size = MainForm->DefaultFontSize;
	RichEdit1->Font->Size = MainForm->DefaultFontSize;

	FindOnly = true;
	Mode->ItemIndex = 0;
	//SaveDialog1->Options << ofOverwritePrompt;
	FormResize(NULL);
	SendMessage(RichEdit1->Handle, EM_FMTLINES ,false ,0) ;
	DragAcceptFiles(Handle, true); //tell Windows this form accepts drag files
	//SendMessage(      // returns LRESULT in lResult
	//(HWND) RichEdit1->Handle,      // handle to destination control
	//(UINT) EM_LIMITTEXT,      // message ID
	//(WPARAM) 0x7FFFFFFF,      // maxnum of tchar's Richedit1 will accept
	//(LPARAM) 0      // = 0; not used, must be zero
	//);
	RichEdit1->MaxLength = 0x7FFFFFFF;

	ManualDock( DockForm->Panel3, NULL, alClient );
	Form1->Visible = true;
	DockForm->Panel3->UndockHeight = Height;
	DockForm->Panel3->UndockWidth = Width;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormResize(TObject *Sender)
{
	unsigned n;
	int fsize;

	Label1->Left = 10;
	Label1->Width = (ClientWidth - 20)/4;
	Label6->Width = Label1->Width;
	Label7->Width = Label1->Width;
	Label8->Width = Label1->Width;
	Label6->Left = Label1->Left + Label1->Width;
	Label7->Left = Label6->Left + Label1->Width;
	Label8->Left = Label7->Left + Label1->Width;
	Label4->Height = (-RichEdit1->Font->Height) + 4;
	Label2->Height = Label4->Height;
	Label5->Height = Label4->Height;
	Label3->Height = Label4->Height;
#define OFF 4
#define RUIMTEBOVEN 55
	RichEdit1->Left = 10;
	RichEdit1->Top = RUIMTEBOVEN;
	RichEdit1->Width = ((ClientWidth-30)*3)/4;
	RichEdit1->Height = ClientHeight - RUIMTEBOVEN;
	//fsize = RichEdit1->Font->Size  * RichEdit1->Font->PixelsPerInch / 72;
	//n = RichEdit1->Height % fsize;
	//if( n > 0 ) RichEdit1->Height = RichEdit1->Height - n;

	linelabel->Left = Label1->Left;
	LineNum->Left = Label1->Left + linelabel->Width;
	linelabel->Top = clear1->Top;
	LineNum->Top = linelabel->Top;
	LineNum->Height = linelabel->Height;
	//hex1->Top = Label9->Top;
	//hex1->Height = Label9->Height;

	Label4->Left = RichEdit1->Width + 20;
	Label4->Top = RichEdit1->Top;
	Label4->Width = (ClientWidth - 30)/4;
	Label2->Left = Label4->Left;
	Label2->Width = Label4->Width;
	Label2->Top = Label4->Top + Label4->Height + OFF;
	FBox1->Top = Label2->Top + Label4->Height + OFF;
	FBox1->Height = (RichEdit1->Height - (Label4->Height *4 + Mode->Height
		+ bombox->Height + Types->Height + 6*OFF )) / 2;
	FBox1->Left = Label4->Left;
	FBox1->Width = Label4->Width;
	Label5->Left = Label4->Left;
	Label5->Top = FBox1->Top + FBox1->Height + OFF;
	Label5->Top = FBox1->Top + FBox1->Height + OFF;
	Label5->Width = Label4->Width;
	Label3->Left = Label4->Left;
	Label3->Width = Label4->Width;
	Label3->Top = Label5->Top + Label5->Height + OFF;
	TargetBox1->Left = Label4->Left;
	TargetBox1->Top = Label3->Top + Label3->Height + OFF;
	TargetBox1->Height = FBox1->Height;
	TargetBox1->Width = FBox1->Width;
	Mode->Left = Label4->Left;
	Mode->Top = TargetBox1->Top + TargetBox1->Height + OFF;
	Mode->Width = FBox1->Width;
	Types->Top = Mode->Top + Mode->Height + OFF;
	Types->Left = Label4->Left;
	Types->Width = FBox1->Width;
	bombox->Top = RichEdit1->Top + RichEdit1->Height - bombox->Height;
	bombox->Left = Label4->Left;
	bombox->Width = FBox1->Width;
	//RichEdit1->Refresh();
	//RichEdit1SelectionChange( NULL );
}
//---------------------------------------------------------------------------
wchar_t __fastcall TForm1::Backslash( ACHAR *s )
{
	wchar_t c;

	c = *s;

	switch ( c )
	{
        case 's': c = L' ';    break;	/* SPACE */
        case 't': c = L'\t';   break;	/* TAB	 */
        case 'r': c = L'\x0d'; break;	/* CR	 */
        case 'n': c = L'\x0a'; break;	/* LF	 */
        case 'f': c = L'\x0c'; break;	/* FF	 */
	}
	return( c );
}

//---------------------------------------------------------------------------
int __fastcall TForm1::alexin( void )
{  // zoals lexin() maar voor de zoekroutine
	PARSTAB *tab;
	short unsigned state;
	int nextchar, f, l, tmp;
    UnicodeString s;
	int accept, accend;

	tab = Alextab;
	accept = accend = 0;
	state = 1;
	while( 1 )
	{
		if(( nextchar = tab->lex[ tab->len ] ) == '\0' ) break;
		if( nextchar == '\n' ) ++RegelNummer;
		if( IgnoreCase != 0 ) {
			tmp = CharMap[ nextchar ]; // only 16-bit codepoints are loaded!
            if(tmp & 0xC0000000) nextchar = (unsigned short)tmp;
        }   // 2 top bits set for lowercase characters
		tab->len += 1;
        f = tab->firsts[ state ];
        l = tab->lasts[ state ];
        if (nextchar < f || nextchar > l) break;
		if( (state = tab->tr_tab[ state ][ nextchar-f ]) == 0 )
		{
			break;
		}
		if( tab->tokens[ state ] != 0 )
		{
			accept = tab->tokens[ state ];
			accend = tab->len;
			NonBolToken = tab->nonbols[ state ];
		}
	}
	if( tab->len == 0 && nextchar == L'\0' )
    	return EOL;
	tab->len = accend;
	return accept;
}
//---------------------------------------------------------------------------
int __fastcall TForm1::search( void )
{
	int n, token, begin, butt, len, ok;
	unsigned maxlen, nr;
	int count = 0;
	UnicodeString origstr,temp2;
	FastString *tempstr, *targetstr; // zie definitions.h
	int i;
	wchar_t c[ 2 ] = L"x";
	wchar_t ch;
	maxlen = RichEdit1->Text.Length();
	tempstr = new FastString();
	targetstr = new FastString( 128 );

	BarForm->ProgressBar->Position = 0;
	len = 0;
	if( !FindOnly) Alextab->len = 0;
	while (( token = alexin()) != EOL )
	{
		Application->ProcessMessages();
		begin = Alextab->lex - Alextab->buf;
		if( BreakOff )
		{
			BreakOff = false;
			ok = Application->MessageBox(
				L"(yes = stop, no = continue)", L"Quit this job?",
					MB_YESNO | MB_ICONQUESTION );
			if( ok == IDYES )
			{
				RichEdit1->SelStart = begin;
				RichEdit1->SelLength = len;
				count = EOL;
				BarForm->Hide();
				break;
			}
		}
		if( BarForm->Visible )
		{
			BarForm->ProgressBar->Max = maxlen;
			BarForm->ProgressBar->Position = begin;
		}
		if( token & BOL )
		{
			if( !( begin == 0 || Alextab->buf[ begin - 1 ] == '\n') )
			{
				token = NonBolToken;
			}
		}
		if( token == 0 )
		{
			if( !MainForm->Confirm )
			{
            	tempstr->AddChar(Alextab->lex[ 0 ]);
			}
			Alextab->lex += 1;
			Alextab->len = 0;
		}
		else
		{
			if( MainForm->Confirm )
			{
            	RichEdit1->SelStart = begin;
        		RichEdit1->SelLength = Alextab->len;
            }
			if( FindOnly )
			{
				if( MainForm->Confirm ) {count = 1; break; }// findfirst
				WriteResultsFile( begin+1 );
				continue; // find & report
			}
//----------replace: --------------
			ReplText = UnicodeString( Alextab->output[ token&0xFF ] );
			if( ReplText == (ACHAR *)NULL ) len = 0;
			else len = ReplText.Length();
			origstr = RichEdit1->Text.SubString( begin + 1, Alextab->len );
            targetstr->Clear();
			for( i = 1; i <= len; ++i )
			{
				if( i<len && ReplText[ i ] == '\\' )
				{
					c[ 0 ] = ReplText[ i+1 ];
					ch = Backslash( (ACHAR *)c );
					targetstr->AddChar( ch );
					i += 1;
				}
				else if ( ReplText[ i ] == '@' )
				{
					if( i<len && ReplText[ i+1 ] == 'U' )
					{
						targetstr->AddString( AnsiUpperCase( origstr ));
						++i;
					}
					else if( i<len && ReplText[ i+1 ] == 'L' )
					{
						targetstr->AddString( AnsiLowerCase( origstr ));
						++i;
					}
					else if( i<len && ReplText[ i+1 ] == 'C' )
					{
						targetstr->AddString(AnsiUpperCase(origstr.SubString( 1,1 )));

						targetstr->AddString(
                        	AnsiLowerCase(origstr.SubString( 2, origstr.Length()-1)));
						++i;
					}
					else targetstr->AddString( origstr );
				}
				else
				{
					targetstr->AddChar(ReplText[ i ]);
				}
			}
			ReplText = targetstr->GetString();
			if( MainForm->changingfilenames && Optionsform->IncrCheck->Checked )
			{
				ReplText += Suffix;
				ReplText += UnicodeString( NumToAdd++ );
			}
			if( MainForm->Confirm )
			{
				butt = ConfDial->ShowModal();
				if( butt == mrCancel ) { break; }
				else if( butt == mrIgnore ) /*skip*/
				{
					ReplText = origstr;
				}
				else if( butt == mrAbort ) {count = EOL; break; }
				else if( butt == mrAll || butt == mrYes)
                {
                	MainForm->Confirm = false;
                    if( butt == mrYes ) DoOneFile = true;
                    tempstr->AddString(RichEdit1->Text.SubString(1, begin));
                }
			}
			len = ReplText.Length();
			if( MainForm->Confirm )
			{
				RichEdit1->SelText = ReplText; // replace

				Alextab->buf = RichEdit1->Text.c_str(); // update searchbuffer
                Alextab->lex = Alextab->buf;
				Alextab->lex += (begin+len);
           		Alextab->len = 0;

			}
			else
			{
				tempstr->AddString(ReplText);  //replace without confirm:
				Alextab->lex += Alextab->len;  // build up tempstr
				Alextab->len = 0;
			}
			++count;
		}
	}
	if( !MainForm->Confirm )
	{
		RichEdit1->Text = tempstr->GetString(); // done: put it in editor
	}
	delete tempstr;
	delete targetstr;
	if( FindOnly && token == EOL )	return EOL;
	return count;
}
//---------------------------------------------------------------------------
int __fastcall TForm1::PrepareSearch( void )
{   // load filelist, set search button (=F3) function
	int i, index, len;
	int ok = 0; // return val: 1 = files selected
	UnicodeString Suffix;
	wchar_t t;
	int mr;
	GETTEXTEX *gtxtx;

	Textline = 1; Prevline = 0;
	if( ((MainForm->DoRename == true && Optionsform->FilenamEdit->Text == "") ||
				MainForm->DoRename == false) &&
			MainForm->TargetDir == MainForm->SourceDir )
	{
		ok = Application->MessageBox(
		L"Destination files will overwrite\r\nsource files!",L"Watch Out!",
		MB_OKCANCEL | MB_ICONEXCLAMATION );
		if( ok == IDCANCEL ) return 0;
	}
	if( FindOnly )
	{
		Mode->ItemIndex = 0;
		Label1->OnClick = FindFirstButtonClick;
	}
	else
	{
		Mode->ItemIndex = 1;
		Label1->OnClick = SearchButtonClick;
	}
	MyOpenDialog->DirectoryListBox1->Directory = MainForm->SourceDir;

	Label1->Enabled = true;
	mr = MyOpenDialog->ShowModal();
	if( mr == mrOk )
	{
		FileList->Clear();
		for( i=0; i < MyOpenDialog->FileListBox1->Items->Count; ++i )
		{
			if(MyOpenDialog->FileListBox1->Selected[ i ])
				FileList->Add(MyOpenDialog->FileListBox1->Items->Strings[ i ]);
		}

		MainForm->SourceDir = MyOpenDialog->DirectoryListBox1->Directory;
		Form1->Caption = UnicodeString( L"Searching in " ) + MainForm->SourceDir
			+ L"\\" + FileList->Strings[ 0 ];

		//SaveDialog1->InitialDir = MainForm->SourceDir;
		if(MainForm->OutisInDir) MainForm->TargetDir = MainForm->SourceDir;
		if( !FindOnly) Label3->Caption = MainForm->TargetDir;
		Label2->Caption = MainForm->SourceDir;
		Label2->Hint = Label2->Caption;
		Label3->Hint = Label3->Caption;
		ok = 1;
	}
	if( ok ) {  // else ?
        ok = LoadFiles();  
        /*len = RichEdit1->Text.Length() + 1;
        //Alextab->buf = RichEdit1->Text;
        Alextab->buf = (wchar_t *)calloc( len, sizeof( wchar_t ));
        gtxtx = new GETTEXTEX;
        gtxtx->codepage = 1200; // unicode
        gtxtx->cb = len * sizeof( wchar_t );
        gtxtx->lpDefaultChar = NULL;
        gtxtx->lpUsedDefChar = NULL;
        gtxtx->flags = GT_USECRLF;

        unsigned lResult = SendMessage(      // returns LRESULT in lResult
        RichEdit1->Handle,      // handle to destination control
		(UINT) EM_GETTEXTEX,      // message ID
        (WPARAM)gtxtx,      // = (WPARAM) () wParam;
        (LPARAM)Alextab->buf      // = (LPARAM) () lParam;
		);
        Alextab->lex = Alextab->buf;
		delete gtxtx; */
	}
	if( MainForm->Confirm && MainForm->showscr ) Show();
	return ok;
}
//----------------------------------------------------------------------------
int __fastcall TForm1::LoadFiles( void )
{   // by preparesearch or by wmdropfiles
	int i, index;
	UnicodeString ws;

	//Mode->Enabled = true;
	FBox1->Items->Clear();
	TargetBox1->Items->Clear();
	FileIndex = 0;
	for( i=0; i < FileList->Count; ++i )
	{
		tempname = UnicodeString(FileList->Strings[ i ]);
        //OpenDialog1->Files->Strings[ i ];
		index = LastDelimiter( "\\", tempname );
		tempname = tempname.SubString( index, tempname.Length()-index + 1 );
		FBox1->Items->Add( tempname );
	}
//	tempname = FileList->Items->Strings[ 0 ];
//	Form1->Caption = WideString( "Searching in " ) + tempname;
//	index = LastDelimiter( "\\", tempname );
//	tempname = tempname.SubString( 1, index - 1 );
//	MainForm->SourceDir = tempname;
//	MyOpenDialog->DirectoryListBox1->Directory = tempname;
	Oldfile = UnicodeString(FileList->Strings[ 0 ]);
//	FileList->Assign( MyOpenDialog->FileListBox1->Items );
	FBox1->ItemIndex = 0;
//	if( !CheckTextFile( Oldfile ))
//	{
//		Application->MessageBox( "cannot read from file!", "error",
//		MB_OK | MB_ICONEXCLAMATION );
//		return( -1 );
//	}
	ws = LoadAndConvert( Oldfile );
	RichEdit1->Text = ws;  // assignment to RichEdit is a COPY?
	Alextab->lex = Alextab->buf = RichEdit1->Text.c_str();
    // ass to Alextab ptrs is a pointer only ?

	return( 1 );
}
//-----------------------------------------------------------------------------

void __fastcall TForm1::SearchButtonClick(TObject *Sender )
{
	int i, n, index, ok;
	int count;
	bool found;
	bool OverwriteAll = false;
	UnicodeString ws, s, temp;

	if( Alextab->tr_tab == NULL )
	{
		MainForm->clrgrid( false );
		if( !MainForm->compile()) return;
	}
	OutFile = new TStringList;
	if( Optionsform->IncrCheck->Checked )
	{
		n = 1;
		while( !isdigit( Optionsform->FilenamEdit->Text[ n ] )) ++n;
		NumToAdd = Optionsform->FilenamEdit->Text.SubString(
			n, Optionsform->FilenamEdit->Text.Length()).ToInt();
		Suffix = Optionsform->FilenamEdit->Text.SubString( 1,n-1 );
	}
	for (i = 0; i < FileList->Count; i++ )
	{
		if( DoOneFile )
		{
			DoOneFile = false;
			MainForm->Confirm = true;
		}
		FBox1->ItemIndex = i;
		Oldfile = FileList->Strings[ i ];
		ws = LoadAndConvert( Oldfile );
		RichEdit1->Text = Alextab->lex = Alextab->buf = ws.c_str();
		//RichEdit1->Text.SetLength(UnicodeString(Alextab->buf).Length());
		//if( MainForm->Confirm) RichEdit1->Text = UnicodeString(Alextab->buf).Copy();
		Alextab->len = 0;
		Alextab->lex += RichEdit1->SelStart;
		Form1->Caption = UnicodeString( L"Searching in " ) + Oldfile;

//------- zoekactie-------------
		count = search();
//------------------------------

		if ( count == EOL )
		{
			break;
		}
        s = UnicodeString( count ) + UnicodeString( L" changes made in file " ) +
			  Oldfile + UnicodeString( L", \n" );
    	OutFile->Add( s );
		if( MainForm->DoRename == true )
		{
			index = LastDelimiter(L".", Oldfile );
			if( Optionsform->IncrCheck->Checked )
			{
				if( index == 0 )
				Oldfile += (Suffix + UnicodeString( NumToAdd++ ));
				else
				Oldfile.Insert( Suffix + UnicodeString( NumToAdd++ ), index );
			}
			else
			{
				if( index == 0 ) Oldfile += MainForm->Appendix;
				else Oldfile.Insert( MainForm->Appendix, index );
			}
		}
		index = LastDelimiter( "\\", Oldfile ) - 1;
		tempname = Oldfile.SubString( index+1,Oldfile.Length()- index );
		Oldfile = UnicodeString(
        	Optionsform->DirectoryListBox1->Directory) + L"\\" + tempname;
		ok = mrOk;
		if( FileExists( Oldfile ))
		{
			FileExistsDialog->filename->Caption = tempname;
			if( OverwriteAll ) ok = mrAll;
			else ok = FileExistsDialog->ShowModal();  // ask
		}
		if( ok == mrRetry )
		{
			CurrFileName = Oldfile;
			SaveButtonClick( this );
			//Oldfile = SaveDialog1->FileName;
			index = LastDelimiter( "\\", Oldfile ) - 1;
			tempname = Oldfile.SubString( index+1,Oldfile.Length()- index );
			ok = mrOk;
		}
		else if( ok == mrAll )
		{
			OverwriteAll = true;
		}
		else if( ok == mrAbort )
		{
			//fwprintf( OutFile, L"\nChanges NOT saved!\n" );
            OutFile->Add(L"\nChanges NOT saved!\n");
			break;
		}

		if( ok == mrOk || ok == mrAll )
		{
            //RichEdit1->WordWrap = false;
			SendMessage(RichEdit1->Handle, EM_FMTLINES ,false ,0);
            ConvertAndSave(RichEdit1->Text, Oldfile);
			//RichEdit1->Lines->SaveToFile( Oldfile );
            //RichEdit1->WordWrap = CheckBox1->Checked;

			found = false;
			for( n=0; n < TargetBox1->Items->Count; ++n )
			{
				if( TargetBox1->Items->Strings[ n ] == tempname )
				{ found = true; break; }
			}
			if( !found )
			{
				TargetBox1->Items->Add( tempname );
				TargetBox1->ItemIndex = i;
			}
			//fwprintf( OutFile, L"file saved as %ls\n\n", Oldfile.c_str() );
            s = UnicodeString( L"file saved as " ) + Oldfile +
                  UnicodeString( "\n\n" );
            OutFile->Add( s );

		}
		RichEdit1->Lines->Clear();
	}
	FBox1->ItemIndex = -1;
	TargetBox1->ItemIndex = -1;

	//fclose( OutFile );
    SaveAsUTF16LEBOM( OutFile->Text, MainForm->OutDir );
    //OutFile->SaveToFile( MainForm->OutDir );
    delete OutFile;
    OutFile = 0;
	MainForm->Confirm = true;
	Form1->Caption = "Search / Edit";
	if( count != EOL )
	{
		RichEdit1->Lines->LoadFromFile( MainForm->OutDir );
		Oldfile = MainForm->OutDir;
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FindFirstButtonClick(TObject *Sender )
{
	int count, len;
	UnicodeString temp;

	if( Alextab->tr_tab == NULL )
	{
		MainForm->clrgrid( false );
		if( !MainForm->compile()) return;
	}
	if( FileIndex == FileList->Count &&
			RichEdit1->SelStart < RichEdit1->Text.Length() )
	{
		--FileIndex;
	}

again: Alextab->len = 0;
	RegelNummer = 1;
	Alextab->lex = Alextab->buf + (RichEdit1->SelStart + RichEdit1->SelLength);
	if ( FileIndex < FileList->Count )
	{
		FBox1->ItemIndex = FileIndex;
		Form1->Caption = UnicodeString( "Searching in " ) + Oldfile;
//------zoekactie:
		count = search();
//----------------
		if( count == EOL ) //&& MainForm->Confirm )
		{
			if( ++FileIndex == FileList->Count )
			{
				if(MainForm->Confirm )
					Application->MessageBox(L"no (more) items found",L"finished",
						MB_ICONASTERISK | MB_OK );
				Form1->Caption = L"Search";
				return;
			}
			Oldfile =
				UnicodeString(FileList->Strings[ FileIndex ]);
			temp = LoadAndConvert( Oldfile );
			RichEdit1->Text = temp;
			Alextab->lex = Alextab->buf = RichEdit1->Text.c_str();
			goto again;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OptionsButtonClick(TObject *Sender)
{
	Optionsform->Show();             
}                                    
//---------------------------------------------------------------------------


void __fastcall TForm1::FormShow(TObject *Sender)
{
	if( MainForm->WindowState == wsMaximized ) WindowState = wsMaximized;
    if( FindOnly ) Mode->ItemIndex = 0; // dit is nodig!
	else Mode->ItemIndex = 1;
	
	Top = MainForm->Height;
	Left = MainForm->Left;
	DockForm->showEditor1->Caption = "hide Editor";
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Testclick(TObject *Sender)
{
	Label1->Enabled = true;
	hex1->Checked = false;
	Optionsform->OKbuttonClick( this );
	if( Mode->ItemIndex == 0 ) FindOnly = true;
	else FindOnly = false;
	MainForm->Confirm = true;
	MainForm->showscr = true;
	PrepareSearch();
//unsigned lResult = SendMessage(      // returns LRESULT in lResult
//	(HWND) RichEdit1->Handle,      // handle to destination control
//    (UINT) EM_GETLIMITTEXT,      // message ID
//    (WPARAM) 0,      // = 0; not used, must be zero
//    (LPARAM) 0      // = 0; not used, must be zero
//    );

}
//---------------------------------------------------------------------------



void __fastcall TForm1::TargetBox1aClick(TObject *Sender)
{
	FBox1->ItemIndex = -1;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FBox10Click(TObject *Sender)
{
	TargetBox1->ItemIndex = -1;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FBox10DblClick(TObject *Sender)
{
	UnicodeString temp;

	MainForm->Confirm = true;
	TargetBox1->ItemIndex = -1;
	CurrFileName = FBox1->Items->Strings[ FBox1->ItemIndex ];
	if( CurrFileName == "" ) return;
	CurrFileName = MainForm->SourceDir + "\\" + CurrFileName;
	if( FileExists( CurrFileName ))
	{
    	temp = LoadAndConvert( CurrFileName );
        RichEdit1->Text = temp;
        Alextab->lex = Alextab->buf = RichEdit1->Text.c_str();

		FileList->Clear();
		FileList->Add( CurrFileName );
		Oldfile = FileList->Strings[ 0 ];
		Form1->Caption = UnicodeString( L"Searching " ) + Oldfile;
	}
	else
	Application->MessageBox( L"no such file!", L"error",
	MB_OK | MB_ICONEXCLAMATION );
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TargetBox1aDblClick(TObject *Sender)
{
	UnicodeString temp;

	FBox1->ItemIndex = -1;
	MainForm->Confirm = true;
	CurrFileName = TargetBox1->Items->Strings[ TargetBox1->ItemIndex ];
	if( CurrFileName == "" ) return;
	CurrFileName = MainForm->TargetDir + "\\" + CurrFileName;
	if( FileExists( CurrFileName ))
	{
    	temp = LoadAndConvert( CurrFileName );
        RichEdit1->Text = temp;
		Alextab->lex = Alextab->buf = temp.c_str();

		FileList->Clear();
		FileList->Add( CurrFileName );
		Oldfile = FileList->Strings[ 0 ];
		Form1->Caption = UnicodeString( L"Searching " ) + Oldfile;
	}
	else
	Application->MessageBox( L"no such file!", L"error",
	MB_OK | MB_ICONEXCLAMATION );
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TargetBox1aKeyUp(TObject *Sender, WORD &Key,
TShiftState Shift)
{
	if( Key == '\r' ) TargetBox1aDblClick(NULL);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FBox10KeyUp(TObject *Sender, WORD &Key,
TShiftState Shift)
{
	if( Key == '\r' ) FBox10DblClick(NULL);
}
//---------------------------------------------------------------------------


void __fastcall TForm1::SaveButtonClick(TObject *Sender)
{
	wchar_t *txt;
	unsigned long n;
	//T *file;

	/*SaveDialog1->FileName = CurrFileName;
	if( SaveDialog1->Execute() )
	{    //SaveToFile laat soft returns erin staan!
		SendMessage(RichEdit1->Handle, EM_FMTLINES ,false ,0) ; // nodig?
		try
		{
			ConvertAndSave( RichEdit1->Text, SaveDialog1->FileName );
		}
		catch(...)
		{
			ShowMessage( "save file not succeeded!" );
		}
	}   */
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Label1Click(TObject *Sender)
{
	SearchButtonClick(NULL );
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormKeyUp(TObject *Sender, WORD &Key,
TShiftState Shift)
{
	if( Key == 114 )
	{
		if( !Label1->Enabled ) return;
		if( FindOnly) FindFirstButtonClick( this );
		else  SearchButtonClick( this );
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ModeClick(TObject *Sender)
{

	if( Mode->ItemIndex == 1 )
	{
		FindOnly = false;
		Label1->OnClick = SearchButtonClick;
	}
	else
	{
		FindOnly = true;
		Label1->OnClick = FindFirstButtonClick;
	}
	Label3->Caption = MainForm->TargetDir;
    Label3->Hint = MainForm->TargetDir;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Loadfile1Click(TObject *Sender)
{
	if( FBox1->Focused()) FBox10DblClick( this );
	else if( TargetBox1->Focused()) TargetBox1aDblClick( this );
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Deletefile1Click(TObject *Sender)
{
	if( FBox1->Focused() && FBox1->ItemIndex >= 0 )
	{
		TargetBox1->ItemIndex = -1;
		CurrFileName = FBox1->Items->Strings[ FBox1->ItemIndex ];
		if( CurrFileName == "" ) return;
		CurrFileName = MainForm->SourceDir + CurrFileName;
		if( FileExists( CurrFileName ))
		{
			FBox1->Items->Delete( FBox1->ItemIndex );
			DeleteFile( CurrFileName );
		}                                                         
	}
	else if( TargetBox1->Focused() && TargetBox1->ItemIndex >= 0 )
	{
		FBox1->ItemIndex = -1;
		CurrFileName = TargetBox1->Items->Strings[ TargetBox1->ItemIndex ];
		if( CurrFileName == "" ) return;
		CurrFileName = MainForm->TargetDir + CurrFileName;
		if( FileExists( CurrFileName ))
		{
			TargetBox1->Items->Delete( TargetBox1->ItemIndex );
			DeleteFile( CurrFileName );
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Copy1Click(TObject *Sender)
{
	RichEdit1->CopyToClipboard();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Cut1Click(TObject *Sender)
{
	RichEdit1->CutToClipboard();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Paste1Click(TObject *Sender)
{
	RichEdit1->PasteFromClipboard();
}
//---------------------------------------------------------------------------


void __fastcall TForm1::Print1Click(TObject *Sender)
{
	RichEdit1->PageRect.Left = 12 * 20;
	RichEdit1->PageRect.Top = 12 * 20;
	RichEdit1->PageRect.Right = 128 * 20;
	RichEdit1->PageRect.Bottom = 160 * 20;
	RichEdit1->Print( "" );
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RichEdit1SelectionChange(TObject *Sender)
{
	int iy;
    wchar_t c;

	iy = SendMessage(RichEdit1->Handle, EM_LINEFROMCHAR, -1 ,0) ;
    RegelNummer = iy; // voor gebruik dblclick routine
	LineNum->Caption = IntToStr(iy + 1);// + ':' + IntToStr(ix ) ;
	if( CharData->Checked && RichEdit1->SelLength == 0) {
		if( !Arrayinput->Visible ) Arrayinput->Visible = true;
		Arrayinput->Enabled = true;
		c = RichEdit1->Text[ RichEdit1->SelStart+1 ];
		//ShowMessage( AnsiString( c ));
		Arrayinput->LookupChar( c );
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::WriteResultsFile(int bg)
{
	//char *ptr;
	int line, begin;
	UnicodeString s, fnaam;

	Textline = RegelNummer;
	fnaam = MainForm->SourceDir + "\\" + FBox1->Items->Strings[ FBox1->ItemIndex ];
	//fnaam = fnaam.SubString( 2, fnaam.Length()-1 );
	if ( Textline == Prevline ) return;
	Prevline = Textline;
	begin = line = bg;
	while( begin > 1 && RichEdit1->Text[ begin ] != L'\n' ) --begin;
	if( begin > 1 ) ++begin;
	do
	{
		while( RichEdit1->Text[ line ] != L'\n' ) ++line;
		++line;
	}while( line < bg + RichEdit1->SelLength );
    //ShowMessage( Alextab->lex );
	s = RichEdit1->Text.SubString( begin, line - begin );
	s = UnicodeString("(") + fnaam + UnicodeString(" line ") + UnicodeString( Textline )
    		+ UnicodeString("):\r\n") + s;
	ResFile->WriteBuffer( s.c_str(), s.Length()*sizeof(wchar_t) );
}

//---------------------------------------------------------------------------
void __fastcall TForm1::hex1Click(TObject *Sender)
{
	unsigned m, n, line, save_ss, save_sl, len, count;
    int regels, rest;
	unsigned ssn, ssm, sln, slm, sltot, s_start, s_end;
	UnicodeString s, t;
	static AnsiString fnt;
	static int fntsize = 10;
	unsigned char *chr;
	wchar_t chars[ 9 ], *chars2;
	wchar_t hexval[ 5 ] = { 0, 0, 0, 0, 0 };

	if( hex1->Checked )
	{
		Label1->Enabled = false;
		fnt = RichEdit1->Font->Name;
		fntsize = RichEdit1->Font->Size;
		RichEdit1->Font->Name = "Courier New";
		RichEdit1->Font->Size = 10;
		save_ss = RichEdit1->SelStart;
		save_sl = RichEdit1->SelLength;
		RichEdit1->SelStart = 0;
		RichEdit1->SelLength = 0;
		REtmp = RichEdit1->Text;
        len = REtmp.Length();
        regels = len / 8; // aantal regels van 8
        rest = len % 8;
		RichEdit1->Clear();
		s = L"";
		chr = (char *)REtmp.c_str();
		n = 0;
        while( regels >= 0 )
        {
            if( regels > 0 ) count = 8;
            else count = rest;
			chars2 = (wchar_t *)chr;
            for( m=0; m < count; ++m )   // characters first
            {
                if( iswprint(chars2[ m ]))  chars[ m ] = chars2[ m ];
                else chars[ m ] = L'.';
            }
            chars[ m ] = 0;
            for (n = 0; n < count; n++) {
                t = _itow( *chr, hexval, 16 );
                if( t.Length() == 1 ) t = UnicodeString( L"0" ) + t;
                s = s + t + L" ";
                ++chr;
                t = _itow( *chr, hexval, 16 );
                if( t.Length() == 1 ) t = UnicodeString( L"0" ) + t;
                s = s + t + L" ";
                ++chr;
            }
            if( regels == 0 ) for( ; n < 8; ++n ) { s = s + L"      "; }
            s = s + UnicodeString( chars );
            RichEdit1->Lines->Add( s );
            s = L"";
            --regels;
        }
		ssn = save_ss / 8;      // zet selectie
		ssm = save_ss % 8;      // 8 dubbelbytes per hexregel
		RichEdit1->SelStart = ssn * 58 + ssm * 6;
		sln = save_sl / 8;
		slm = save_sl % 8;
		sltot = sln * 58 + slm * 6;
		if(( ssm + slm ) > 8 ) sltot += 10;
		RichEdit1->SelLength = sltot;
	}
	else
	{
		Label1->Enabled = true;
		save_ss = RichEdit1->SelStart;
		save_sl = RichEdit1->SelLength;
		RichEdit1->Font->Name = fnt;
		RichEdit1->Font->Size = fntsize;
		RichEdit1->Clear();
		RichEdit1->Text = REtmp;
		ssm = save_ss % 58;   // 16*2 hexbytes + 16 spaties + 8 tekens + \r\n
		m = ssm % 6;
		save_ss -= m;  // corrigeer onnauwkeurige selstart in hexfile
		save_sl += m;
		ssn = save_ss / 58;  // regel
		ssm = save_ss % 58;  // positie in regel
		RichEdit1->SelStart = ssn * 8 + ssm / 6; // selstart in textfile
		s_end = save_ss + save_sl; // eindpositie in hexregel
		m = (s_end % 58) % 6;   // wchar positie
		s_end  += (m==0?0:6-m); // corrigeer naar voren
		sln = (s_end - save_ss) / 58; // hoeveel regels
		slm = (s_end - save_ss) % 58; // hoeveel tekens
		sltot = sln * 8 + slm / 6; // hoeveel tekens in textfile
		RichEdit1->SelLength = sltot;
	}

}
//---------------------------------------------------------------------------



void __fastcall TForm1::xxRichEdit1KeyDown(TObject *Sender, WORD &Key,
TShiftState Shift)
{
	wchar_t *buf, *val2;
	unsigned char val[ 18 ];
	long i, x, st, selst, valstart, ln;
	UnicodeString s, hex;

	if( hex1->Checked && Key == VK_RETURN && Shift.Contains( ssCtrl ))
	{
		Key = 0;
		buf = RichEdit1->Text.c_str()																					;
		selst = st = RichEdit1->SelStart;
		buf += st;
		while( st > 0 )
		{
			--buf; --st;
			if( *buf == L'\n' ) break;
		}
		if( st > 0 ) ++st;
		valstart = (st / 58) * 8;
		RichEdit1->SelStart = st;
		RichEdit1->SelLength = 48;
		// read hex line:
		hex = RichEdit1->SelText;
		if( hex.Length() < 48 ){ ShowMessage( L"carriage return in line!" ); return; }
		for( x=i=0; i<48; i += 3 )
		{
			s = hex.SubString( i+1, 2 );
			s = UnicodeString( L"x" ) + s;
			val[ x++ ] = StrToInt( s );
		}
		val[ 16 ] = 0; val[ 17 ] = 0;
		val2 = (wchar_t *)val;
		for( i=0; i < 8; ++i )
		{
			if( val2[ i ] == 0 )
			Application->MessageBox( L"no null characters!", L"error",
			MB_OK | MB_ICONEXCLAMATION );
			else continue;
			RichEdit1->Undo();
			RichEdit1->SelStart = selst;
			RichEdit1->SelLength = 0;
			return;
		}
		s = UnicodeString( (wchar_t *)val );
		ln = REtmp.Length();
		val2 = REtmp.c_str();
		val2[ ln ] = 0;
		for( i=0; i < 8; ++i )
		{   // put it in
			val2[ valstart + i ] = s[ i + 1 ];
		}
		REtmp = UnicodeString( val2 );

		for( i=1; i<9; ++i)
		{
			if( !iswprint( s[ i ] )) s[ i ] = L'.';
		}
		RichEdit1->SelStart = (st/58)*58 + 48;
		RichEdit1->SelLength = 8;
		RichEdit1->SelText = s;
		RichEdit1->SelStart = selst;
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::undo1Click(TObject *Sender)
{
	RichEdit1->Undo();	
}
//---------------------------------------------------------------------------


void __fastcall TForm1::FormHide(TObject *Sender)
{
	hex1->Checked = false;
	DockForm->showEditor1->Caption = "show Editor";
}
//---------------------------------------------------------------------------

void __fastcall TForm1::fontbtnClick(TObject *Sender)
{
	//if( FontDialog1->Execute() )
	//{
	//	RichEdit1->Font->Name = UnicodeString(FontDialog1->Font->Name);
	//	RichEdit1->Font->Size = FontDialog1->Font->Size;
	//}
}
//---------------------------------------------------------------------------

unsigned short __fastcall TForm1::GetUTF16CharWidth( wchar_t *text )
{   // get lengte van UTF-16 unicode char in wchar_t's: 1 of 2
	// text moet gegarandeerd UTF 16 zijn & lo-hi word order (little E)
	wchar_t w1, w2;

	w1 = text[ 0 ];
	w2 = text[ 1 ];
	if( (w1 & 0xDC00) == 0xD800 ) // first word of two
	{
		if( (w2 & 0xDC00) != 0xDC00 ) // error
		{
			ShowMessage( "not well-formed utf-16 !" );
			return 0;
		}
		return 2;
	}
	return 1;
}
//---------------------------------------------------------------------------

unsigned long __fastcall TForm1::GetUTF16Codepoint( wchar_t *text )
{   // get UTF-16 unicode codepoint. check of 1 of 2 words
	// text moet gegarandeerd UTF 16 zijn & lo-hi word order (little E)
	wchar_t w1, w2;
	unsigned long cp = 0L;

	w1 = text[ 0 ];
	w2 = text[ 1 ];
	if( (w1 & 0xDC00) == 0xD800 ) // first word of two
	{
		if( (w2 & 0xDC00) != 0xDC00 ) // error
		{
			ShowMessage( "not well-formed utf-16 !" );
			return 0L;
		}
		cp = (w1 & 0x3FF) + 0x40;
		cp = (cp << 10) | (w2 & 0x3FF);
	}
	else cp = w1;

	return cp;
}
//---------------------------------------------------------------------------

UnicodeString __fastcall TForm1::GetUniCodeCharDescription( unsigned long cp )
{   // file "UnicodeData.txt" moet aanwezig zijn
	ACHAR *str;
	ACHAR *bg;
	int n, db;
	ACHAR c, *hex;
	bool found;
	//filebuf fb;
	unsigned dblines;
	unsigned cval;

	//fb.open ("UnicodeData.txt",ios::in); // zie file  NB write in infofile
	//istream file( &fb );
	hex = (IntToHex( (int)cp, 4 )).c_str();
	dblines = MainForm->UCDB->Count;
	for( db = 1; db < dblines; ++db ) // get 1 line
	{
		str = MainForm->UCDB->Strings[ db ].c_str();
		found = true;
		n = 0;
		cval = MainForm->HexStringToInt( str, 4 );
		if( cval > cp )
			{ return( UnicodeString( L"not found" )); }
		else if( cval < cp )
			continue;
		else
		{
			bg = str + 5;
			return( UnicodeString( bg ));
		}
	}
	return( UnicodeString( "not found" ));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::hex2Click(TObject *Sender)
{
	hex1->Checked = !hex1->Checked;
	//hex1Click( NULL );
}
//---------------------------------------------------------------------------

void __fastcall TForm1::conv1Click(TObject *Sender)
{
	wchar_t conv;
	unsigned ss, sl;
	unsigned long pos;
    int n;
	union {
		char c[4];
		unsigned long l;
	} entry;
	TFileStream *f;
	UnicodeString s;

	ss = RichEdit1->SelStart;
	if( RichEdit1->SelLength == 0 ) RichEdit1->SelLength = 1;
	sl = RichEdit1->SelLength;
	s = RichEdit1->SelText;
	try
	{
		f = new TFileStream( MainForm->HomeDir + "\\mapfile.dat",
			fmOpenRead );
	}
	catch(...)
	{
		Application->MessageBox( L"cannot open file 'mapfile.dat'", L"file error",
			MB_OK | MB_ICONEXCLAMATION );
		return;
	}
	for( n=1; n <= s.Length(); ++n )
	{
		pos = (unsigned long)s[ n ];
		f->Position = pos * 4;
		f->ReadBuffer( (void *)entry.c, 4 );
		conv = entry.l & 0xFFFF;
		if( conv != 0 ) s[ n ] = conv;
	}
	RichEdit1->SelText = s;
	RichEdit1->SelStart = ss;
	RichEdit1->SelLength = sl;
	delete f;

}
//---------------------------------------------------------------------------

void __fastcall TForm1::WmDropFiles(TWMDropFiles& Message)// message handler:
 { // read filenames of dropped files. Dropped text doesn't come here
	wchar_t buff[MAX_PATH]; // max = 260
	int i;
	HDROP hDrop = (HDROP)Message.Drop;
	int numFiles =
	  DragQueryFile(hDrop, -1, NULL, NULL);

	if( numFiles > 0 )
	{
		MyOpenDialog->FileListBox1->Items->Clear();
		DragQueryFile(hDrop, 0, buff, sizeof(buff));
		MyOpenDialog->FileListBox1->Items->Add( UnicodeString(buff) );
		MyOpenDialog->FileListBox1->FileName = UnicodeString( buff );
		for ( i=1;i < numFiles;i++) {
			DragQueryFile(hDrop, i, buff, sizeof(buff));
			MyOpenDialog->FileListBox1->Items->Add( UnicodeString(buff) );
		}
		LoadFiles();
	}
	DragFinish(hDrop);
 }
//----------------------------------------------------------------------------
UnicodeString __fastcall TForm1::LoadAndConvert( UnicodeString filename )
{
	UnicodeString s;
	UnicodeString result;
	unsigned long filesize, needed, i, err;
	TFileStream *f;
	unsigned char *buf;
	int off;
	wchar_t *widebuf;
	UINT codepage;

	if( !CheckTextFile( filename ))
	{
		s = UnicodeString( L"cannot read: " ) + filename;
		Application->MessageBox( s.c_str() , L"File Error",
			MB_OK | MB_ICONSTOP );
		return( NULL );
	}
	f = new TFileStream( filename, fmOpenRead );
	filesize = f->Size;
	buf = new char[ filesize+2 ];
	f->ReadBuffer( buf, filesize );
	delete f;
	buf[ filesize ] = '\0';
	buf[ filesize+1 ] = '\0';
	if( Types->ItemIndex == 2 && bom1->Checked == true ) off = 3;
	else off = 0;
	switch( Types->ItemIndex )
	{
		case 0: // utf16 Big Endian
			widebuf = new wchar_t[ (filesize/2)+1 ];
			for( i=0; i < filesize/2; ++i )
			{
				widebuf[ i ] = (buf[ 2 * i ] << 8) | buf[ 2 * i + 1 ];
			}
			widebuf[ i ] = 0;
			delete[] buf;
			result = UnicodeString( widebuf + 1 );
			delete[] widebuf;
			return result;
		case 1: // Little Endian: doe niets
			if( bom1->Checked ) off = 2;
			else off = 0;
			result = UnicodeString((wchar_t *)(buf+off)); // skip BOM
			delete[] buf;
			return result;
		case 2: // utf-8   65001 = utf8 codepage
			codepage = CP_UTF8;
			break;
		case 3: // other, load anyway
		case 4: // utf-7
			codepage = CP_UTF7;
			break;
	}
	needed = MultiByteToWideChar(
	   codepage,
	   0, //MB_ERR_INVALID_CHARS,
	   buf+off,
	   -1,
	   NULL,
	   0  );

	if( needed > 0 ) widebuf = new wchar_t[ needed+1 ];
	else //error
	{
		err = GetLastError();
		if( err == ERROR_NO_UNICODE_TRANSLATION )
		Application->MessageBox( L"not a legal UTF-8 file", L"File Error",
			MB_OK | MB_ICONSTOP );
		delete[] buf;
		return( NULL );
	}
	MultiByteToWideChar(
	   codepage,
	   0, //MB_ERR_INVALID_CHARS,
	   buf+off,
	   -1,
	   widebuf,
	   needed );
	result = UnicodeString( widebuf );
	delete[] buf;
	delete[] widebuf;
	return result;
}
//-----------------------------------------------------------------------------
bool __fastcall TForm1::ConvertAndSave( UnicodeString text, UnicodeString filename )
{
	unsigned long n, len, utf8len; //, err;
	unsigned char *buf;
	int off;
	wchar_t *widebuf;
	TFileStream *f;

	len = text.Length();
	//if( len == 0 ) return false;
	if( DockForm->m_sameasinput->Checked == false ) // menuitem: force output format
	{
		if( DockForm->mBOM->Checked ) bom1->Checked = true;
		if( DockForm->mUTF16LE->Checked ) {
			Types->ItemIndex = 1;
		}
		else if( DockForm->mUTF16BE->Checked ){
			Types->ItemIndex = 0;
		}
		else if( DockForm->mUTF8->Checked ){
			Types->ItemIndex = 2;
		}
		else if( DockForm->mUTF7->Checked ){
			Types->ItemIndex = 4;
			bom1->Checked = false;
		}
	}
	switch( Types->ItemIndex )
	{
		case 0: // utf16 Big Endian
			f = new TFileStream( filename, fmCreate );
			widebuf = new wchar_t[ len ];
			if( bom1->Checked == true )	f->WriteBuffer( &Utf16BE, 2 );
			for( n = 0; n < len; ++n )
			{
				widebuf[ n ] = (text[ n + 1 ] << 8) | (text[ n + 1 ] >> 8);
			}
			f->WriteBuffer( widebuf, len * sizeof( wchar_t ));
			delete f;
			delete[] widebuf;
			return true;
		case 1: // Little Endian
			widebuf = text.c_str();
			f = new TFileStream( filename, fmCreate );
			if( bom1->Checked == true )	f->WriteBuffer( &Utf16LE, 2 );
			f->WriteBuffer( widebuf, len * sizeof( wchar_t ));
			delete f;
			return true;
		case 2: // utf-8
		case 3: // unknown - problem?
		case 4: // utf-7
			widebuf = text.c_str();
			f = new TFileStream( filename, fmCreate );
			if( len == 0 ) {delete f; return true;}
			if( bom1->Checked == true )	f->WriteBuffer( &Utf8, 3 );
			utf8len = WideCharToMultiByte(  // get lengte
				CP_UTF8,			//UINT CodePage,
				0,					//DWORD dwFlags,
				widebuf,			//LPCWSTR lpWideCharStr,
				len,				//int cchWideChar,
				NULL,				//LPSTR lpMultiByteStr,
				0,					//int cbMultiByte,
				NULL,				//LPCSTR lpDefaultChar,
				NULL				//LPBOOL lpUsedDefaultChar
			);
			if( utf8len == 0 )
			{
				//err = GetLastError();
				//if( err == ERROR_NO_UNICODE_TRANSLATION )
				Application->MessageBox( L"not a legal UTF-8 file",
					L"Save Error", MB_OK | MB_ICONSTOP );
				delete f;
				return( false );
			}
			buf = new char[ utf8len+1 ];
			WideCharToMultiByte(
				CP_UTF8,
				0,
				widebuf,
				len,
				buf,
				utf8len,
				NULL,
				NULL
			);
//            if (utf8len == 0) utf8len = 1;
			f->WriteBuffer( buf, utf8len );
			delete f;
			delete[] buf;
			return true;
		default:
			return false;
	}
}
//----------------------------------------------------------------------------
bool __fastcall TForm1::SaveAsUTF16LEBOM( UnicodeString text, UnicodeString filename )
{
	bool t1, t2, res;
	int ti;

	t1 = bom1->Checked;
	t2 = DockForm->m_sameasinput->Checked;
	ti = Types->ItemIndex;
	bom1->Checked = true;
	DockForm->m_sameasinput->Checked = true;
	Types->ItemIndex = 1;
	res = ConvertAndSave( text, filename );
	bom1->Checked = t1;
	DockForm->m_sameasinput->Checked = t2;
	Types->ItemIndex = ti;
	return res;
}

//-----------------------------------------------------------------------------
bool __fastcall TForm1::CheckTextFile( UnicodeString name )
{   // set Types
	unsigned char buf[ 4 ] = {0,0,0,0};
	TFileStream *f;
	int r;
	unsigned long n, size;
	//	unsigned char lo[ 256 ];
	//	unsigned char hi[ 256 ];
	unsigned char chrs[ 514 ];
	bool NotUTF7 = false;
	unsigned LoNotNul = 0, HiNotNul = 0;
	try
	{
		f = new TFileStream( name, fmOpenRead );
		r = f->Read( buf, 4 );
		delete f;
	}
	catch(...)
	{
		delete f;
		return false;
	}
	bom1->Checked = true;
	if( buf[ 0 ] == 0 && buf[ 1 ] == 0  // utf32 big E
			&& buf[ 2 ] == 0xFE && buf[ 3 ] == 0xFF )
	{ ShowMessage( L"UTF-32 Big Endian file. Can\'t edit it." ); return false; }
	else if( buf[ 0 ] == 0 && buf[ 1 ] == 0  // utf32 little E
			&& buf[ 2 ] == 0xFF && buf[ 3 ] == 0xFE )
	{ ShowMessage( L"UTF-32 Little Endian file. Can\'t edit it." ); return false;}
	else if( buf[ 0 ] == 0xFE && buf[ 1 ] == 0xFF )   // utf16 big E
	Types->ItemIndex = 0;
	else if( buf[ 0 ] == 0xFF && buf[ 1 ] == 0xFE )   // utf16 little E
	Types->ItemIndex = 1;
	else if( buf[ 0 ] == 0xEF && buf[ 1 ] == 0xBB  // utf8
			&& buf[ 2 ] == 0xBF ) Types->ItemIndex = 2;
	else if( r == 0 ) { Types->ItemIndex = 4; return true; }
	else //Types->ItemIndex = 3; // other
	{   // no BOM: try to discover filetype (RichEdit knows, but I don't)
		nobom1->Checked = true;
		for( n=0; n < 514; ++n ) { chrs[ n ] = '\0'; }
		f = new TFileStream( name, fmOpenRead );
		size = f->Size;
		if( size > 512 ) size = 512;
		f->ReadBuffer( chrs, size );
		delete f;
		for( n=0; n < size; n += 2 )
		{   // kijk wat voor soort chars, en hoeveel
			//lo[ chrs[ n ] ] += 1;
			//hi[ chrs[ n+1 ] ] += 1;
			if( chrs[ n ] > '\x7F' || chrs[ n+1 ] > '\x7F' ) NotUTF7 = true;
			if( chrs[ n ] != '\0' ) ++LoNotNul;
			if( chrs[ n+1 ] != '\0' ) ++HiNotNul;
		}
		if( LoNotNul < (HiNotNul / 2) )
		{
			Types->ItemIndex = 0; // unicode big E (educated guess)
		}
		else if( HiNotNul < (LoNotNul / 2) )
		{
			Types->ItemIndex = 1; // unicode little E
		}
		else if( NotUTF7 == false )
		{
			Types->ItemIndex = 4; // utf-7
		}
		else
		{
			if(( LoNotNul + HiNotNul ) == size )  // utf8 or other encoding
			{
				n = 0;
				while( *(chrs+n) != '\0' )
				{
					r = is_utf8( chrs + n );
					if( r > 0 ) n += r;
					else if( r == 0 ) break;
					else if( r < 0 ) break;
				}
				if( r >= 0 ) Types->ItemIndex = 2; // utf8
				else Types->ItemIndex = 3; // other
			}
			else Types->ItemIndex = 5; // binary?
		}
	}

	return true;
}
//---------------------------------------------------------------------------
int __fastcall TForm1::is_utf8( unsigned char *buf )
{   // return 0 if nulbyte, 1-4 if utf8 byte (=lengte), -1 if not utf8
	unsigned n;    // no 100% guarantee

	if( *buf == 0 ) return 0;
	if( *buf > 0 && *buf < 0x80 ) return 1;
	if( (*buf & 0xE0) == 0xC0 && (*(buf+1) & 0xC0) == 0x80 ) return 2;
	if( (*buf & 0xF0) == 0xE0 && (*(buf+1) & 0xC0) == 0x80
							  && (*(buf+2) & 0xC0) == 0x80 ) return 3;
	if( (*buf & 0xF8) == 0xF0 && (*(buf+1) & 0xC0) == 0x80
							  && (*(buf+2) & 0xC0) == 0x80
							  && (*(buf+3) & 0xC0) == 0x80 ) return 4;
	return -1;
}
void __fastcall TForm1::clear1Click(TObject *Sender)
{
	RichEdit1->Lines->Clear();
	Form1->Caption = L"Search / Edit";
	Types->ItemIndex = 2;
	bom1->Checked = true;
}
//---------------------------------------------------------------------------


void __fastcall TForm1::xxRichEdit1KeyUp(TObject *Sender, WORD &Key,
	  TShiftState Shift)
{
	RichEdit1SelectionChange( NULL );
}
//---------------------------------------------------------------------------



void __fastcall TForm1::xxRichEdit1DblClick(TObject *Sender)
{
	ShowMessage( RichEdit1->Lines->Strings[ RegelNummer ] );
}
//---------------------------------------------------------------------------
char * __fastcall TForm1::GetUniCodeRange( unsigned long cp )
{    // cp = codepoint
	unsigned from = 0, to = 0;
	UnicodeString s;
	TStreamReader* reader;

	/* Create a new stream writer directly. */
	try
	{
		reader = new TStreamReader(MainForm->HomeDir + L"\\unicode_subranges.txt").c_str(),
			TEncoding::UTF8);  // format: xxxx;yyyy;description
	}
	catch( EFOpenError &e )
	{
		Application->MessageBox( L"Unicode subranges file missing", L"error", MB_OK );
		Application->Terminate();
	}

	while( !reader.EndOfStream ) // get 1 line
	{
		s = reader.ReadLine();
		from = MainForm->HexStringToInt( s.c_str(), 4 );
		to =  MainForm->HexStringToInt( s.c_str() + 5, 4 );

		if( cp >= from ) Form1->RangeBegin = from;
		if( cp <= to ) Form1->RangeEnd = to;
		if( cp >= from && cp <= to ) {
			fb.close();
			return( str + 10 );
		}
	}
	fb.close();
	strcpy( str, "unknown range" );
	return( str ); // str = static stringbuf
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FormDestroy(TObject *Sender)
{
	delete FileList;
	//delete CurrentFile;
}
//---------------------------------------------------------------------------

