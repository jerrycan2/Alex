#include "alex.h"
class AlexIn
{
private:
    UNIT allocedbufsize;
    UNIT filesize;
    char *bufferlimit;
    char *inputbuffer;
    UNIT setInputBuffer( const wstring ); //filename
protected:
public:
    AlexIn();
    ~AlexIn();
    //bool loadbuffer( const string ); // same filename
    bool openfile( const wstring );
    void hexdump( char * );

};

