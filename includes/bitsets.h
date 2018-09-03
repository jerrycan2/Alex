/********************************************//**
 * include file for 'bitarray' and 'charset'
 * #defines, typedefs and constants are uppercase. Classes & structs are capitalized
 * Bitset: set of integers 0..maxindex (incl.)
 * bitarray: the data of a Bitset, an array of UNITs (in separate storage)
 ***********************************************/
#ifndef BITSETS_DEFINED
#define BITSETS_DEFINED

#include <string>
#include <iostream>
#include <iomanip>
#include <map>

using namespace std;
#include "defs.h"
#include "storage.h"

#define MAP_DEFAULTALLOC 4

typedef int POOL_PTR;
const UNIT BIT0 = 1;

struct Bitset_range_error
{
    Bitset_range_error( const wchar_t *str )
    {
        wcout << L"Bitset error: " << str << endl;
        exit( 1 );
    }
};

void hexdump( char * );
inline bool set_bit( UNIT *, B_INDEX );
void set_bit( UNIT *, B_INDEX, B_INDEX );
inline bool del_bit( UNIT *, B_INDEX );
inline bool set_contains( UNIT *, B_INDEX );
inline bool set_equal( UNIT *, UNIT *, B_INDEX );
inline bool set_empty( UNIT *, B_INDEX );
inline void set_copy( UNIT *, UNIT *, B_INDEX );
inline void set_clear( UNIT *, B_INDEX );
inline void set_and( UNIT *, UNIT *, B_INDEX );
inline void set_or( UNIT *, UNIT *, B_INDEX );
inline void set_not( UNIT *, B_INDEX );
bool nextbit( UNIT *, B_INDEX *, B_INDEX, B_INDEX );
void hexprint( UNIT *, B_INDEX );
void binprint( UNIT, unsigned );
void printset( UNIT *, B_INDEX, B_INDEX );

/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* Range ********************************************
////////////////////////////////////////////////////////////////////////////
==========================================================================*/

/************** class Range ***********************//**
 * Range: a pair<offset, length> repr. range offset..(offset+length),
 * implemented as bitarray 0..length (incl)
 * so the actual length of the range is Range.length + 1
 ***********************************************/
struct Range
{
    B_INDEX offset;
    B_INDEX length;
    Range(){ offset = length = 0; }
    Range( const Range& r ) : offset( r.offset ), length( r.length ) {}
    Range( B_INDEX fi, B_INDEX la );
    bool operator<(const Range &rhs) const
    {   // 'less' - operator for inserting in map. Throws exception if ranges overlap
        if( offset >= rhs.offset ){ return( false ); }
        if( offset + length >= rhs.offset ) { throw( Bitset_range_error( L"Uniset: can't unify overlapping ranges" ) ); }
        return( true );
    }
    bool operator==(const Range &rhs) const
    {
        if( offset != rhs.offset ) return false;
        if( length != rhs.length ) { throw( Bitset_range_error( L"Uniset: subsets with equal range-start must have equal length" ) ); }
        return( true );
    }
    bool operator!=(const Range &rhs) const { return( offset != rhs.offset || length != rhs.length ); }
    bool r_contains( const B_INDEX n ) { return( n >= offset && n <= offset+length ); }
    void printrange() const;
};

struct RangeNkey
{
    Range m_range; /**< bitarray range begin..end (implemented as offset,length) */
    POOL_PTR m_array_key; /**< key to bitarray data in MixedPool */
};


/********************************************//**
 * class Bitset
 * an ordered set of positive integers m..n where m >= 0 and n >= m,
 * a.k.a an array[ m..n ] of booleans implemented as a bitarray.
 * Max size 2^31 bits. Negative indexes not allowed.
 * Storage is not local but allocated, in a special way (see storage.cpp).
 * Suitable to represent UNICODE - ranges and codepoints (UTF-32 Little Endian)
 ***********************************************/
class Bitset
{
    friend class Uniset;
    friend class U_iterator;
    private:
        Range range;
        POOL_PTR b_array_key; //reference (index) to bitarray in SetPool
    public:
        Bitset( const Bitset& );
        Bitset( const B_INDEX, const B_INDEX );
        Bitset( const Range& );
        //Bitset( const Range&, const POOL_PTR ); // copy bitset (from Uniset)
        ~Bitset();
        //void b_getrange( const Range * );
        bool b_insert( B_INDEX );
        void b_insert( B_INDEX, B_INDEX );
        bool b_delete( B_INDEX );
        bool b_contains( B_INDEX ) const;
        bool b_equal( const Bitset& ) const;
        bool b_empty() const;
        bool b_is_subset_of( const Bitset& ) const;
        Bitset& b_copy( const Bitset& );
        Bitset& b_clear();
        Bitset& b_intersect( const Bitset& );
        Bitset& b_unify( const Bitset& );
        Bitset& b_xor( const Bitset& );
        Bitset& b_complement();
        bool b_iterate( B_INDEX * ) const;
        void b_init( B_INDEX * ) const;
        void b_hexprint(); //print whole array in hex
        void b_binprint(); //print array in binary
        void b_printset(); // print items in set, in decimal
};

/********************************************//**
 * \brief M_map: sorted array of RangeNkey structs, mapping a Range
 * to its bitarray repr. a Bitset (Unicode-range)
 * Sorted by Map-key == Range.offset.
 * 'insert' and 'find' are implemented, plus a const iterator.
 ***********************************************/
class M_map
{
friend class M_iterator;
protected:
    POOL_PTR m_keymaps; /**< MixedPool key to (small) array of range_key_maps */
    short rkm_allocated;
    short rkm_count;
public:
    M_map()
    {
        m_keymaps = (POOL_PTR)MyPool.Push( 0, MAP_DEFAULTALLOC * sizeof(RangeNkey));
        rkm_allocated = MAP_DEFAULTALLOC;
        rkm_count = 0;
    };
    ~M_map(){};
    RangeNkey *m_insert( const Range&, const POOL_PTR ); //insert copy
    RangeNkey *m_find( const Range& );
};

class M_iterator
{
protected:
    unsigned m_index;
    const M_map *m_map;
public:
    M_iterator(){};
    M_iterator( const M_map *m )
    {
        m_index = (unsigned)-1;
        m_map = m;
    }
    RangeNkey *m_iterate();
    void m_iter_reset(){ m_index = (unsigned)-1; }
};

/************** Uniset **********************//**
 * Uniset: map of <Range, POOL_PTR> pairs. Key: the Range.offset of the set, POOL_PTR: -> the bitarray data.
 * Uniset is designed to collect and unify UNICODE-ranges (as Bitsets) into one object.
 * It maps a (UNICODE) Range to a MixedPool index where the bitarray is stored.
 * the Ranges of the subsets may not overlap.
 ***********************************************/

class Uniset
{
    friend class U_iterator;
    private:
        M_map included_ranges;
    public:
        Uniset() {};
        Uniset( const Uniset& );   // copy
        ~Uniset() {}; //the data entries in Mixed_Pool remain untouched
        Uniset& u_unify( const Bitset& );
        Uniset& u_unify( const Uniset& );
        void u_printset();
};

/********************************************//**
 * const Iterator for Uniset
 *
 *
 ***********************************************/

class U_iterator : public M_iterator
{
private:
    B_INDEX *result;
    Uniset *uniset;
    RangeNkey *m_iter;
public:
    // U_iterator(){};
    // use only this one (or the m_map will not be initialized):
    U_iterator( Uniset *u, B_INDEX *b ) : M_iterator( &(u->included_ranges) ){ u_reinit( u, b ); }; // init M_map iterator
    ~U_iterator(){};
    void u_reinit( Uniset *, B_INDEX * );
    bool u_iterate();
};



#endif // BITSETS_DEFINED


