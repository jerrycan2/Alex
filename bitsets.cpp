#include <iostream>

using namespace std;
#ifdef USE_DEBUG_NEW
#include "debug_new.h"
#endif

#include "bitsets.h"

/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* static utilities *********************************
////////////////////////////////////////////////////////////////////////////
==========================================================================*/

/************* set_bit(n) ********************//**
 * \brief set a bit in *bitarray. No range check.
 *
 * \param ptr UNIT* : ptr to bitarray data
 * \param n B_INDEX : bitindex
 * \return bool : rtn true if bit was set already
 ***********************************************/
inline bool set_bit( UNIT *ptr, B_INDEX m ) // rtns true if bit was set already
{
    UNIT mask;
    bool ret;
    unsigned n = m;

    ptr += n / BITS_PER_UNIT;
    mask = BIT0 << n % BITS_PER_UNIT;
    ret = ( ( *ptr & mask ) != 0 );
    *ptr |= mask;
    return( (bool)ret );
}

/************* set_bit(m,n) ******************//**
 * \brief set bits first..last (including).
 * If first > last nothing happens
 * \param ptr1 UNIT* : ptr to bitarray data
 * \param first B_INDEX : first
 * \param last B_INDEX : last
 * \return void
 ***********************************************/
void set_bit( UNIT *ptr1, B_INDEX first, B_INDEX last  )
{
    UNIT *ptr2, tmp;
    UNIT mask1, mask2;

    if( first > last ) return;
    ptr2 = ptr1;
    ptr1 += first / BITS_PER_UNIT;
    ptr2 += last / BITS_PER_UNIT;
    mask1 = -( BIT0 << first % BITS_PER_UNIT ); // ... 1 1 1 first [0 0 ...]
    mask2 = ( BIT0 << last % BITS_PER_UNIT );
    mask2 |= ( mask2 - 1 ); // [... 0 0 0] last 1 1 1 ...
    if( ptr1 == ptr2 )
    {
        tmp = ( mask1 & mask2 );
        *ptr1 |= tmp; // [... 0 0] last 1 1 1 first [0 0 ...]
    }
    else
    {
        *ptr1++ |= mask1;
        *ptr2 |= mask2;
        while( ptr1 < ptr2 )
        {
            *ptr1++ = MINUSONE;
        }
    }
}

/************* del_bit() ********************//**
 * \brief unset bit n;
 * Rtns true if bit was set, else false. no range check.
 * \param ptr UNIT* : ptr to bitarray data
 * \param n B_INDEX : bitindex
 * \return bool
 ***********************************************/
inline bool del_bit( UNIT *ptr, B_INDEX n ) // rtns true if bit was set already
{
    UNIT mask;
    bool ret;

    ptr += n / BITS_PER_UNIT;
    mask = BIT0 << n % BITS_PER_UNIT;
    ret = ( ( *ptr & mask ) != 0 );
    *ptr &= (~mask);
    return( ret );
}

/************ set_contains() ****************//**
 * \brief test bit n; no range check.
 *
 * \param ptr UNIT* : ptr to bitarray data
 * \param n B_INDEX : bitindex
 * \return bool
 ***********************************************/
inline bool set_contains( UNIT *ptr, B_INDEX n )
{
    return ( ( ptr[n / BITS_PER_UNIT] & ( BIT0 << n % BITS_PER_UNIT ) ) != 0 );
}

/************ set_equal() *******************//**
 * \brief rtn true if Bitset lhs == Bitset rhs.
 * Sets must be equal (no test).
 * \param ptr1 UNIT* : ptr to bitarray data lhs
 * \param ptr2 UNIT* : ptr to bitarray data rhs
 * \param m B_INDEX : Bitset maxindex
 * \return bool
 ***********************************************/
inline bool set_equal( UNIT *ptr1, UNIT *ptr2, B_INDEX m )
{
    m = m / BITS_PER_UNIT + 1;
    while( m-- )
    {
        if( *ptr1++ != *ptr2++ ) { return( false ); }
    }
    return( true );
}

/************ set_empty() *******************//**
 * \brief rtn true iff set empty.
 *  no size check here
 * \param ptr UNIT* : ptr to bitarray data
 * \param m B_INDEX : Bitset maxindex
 * \return bool
 ***********************************************/
inline bool set_empty( UNIT *ptr, B_INDEX m )
{
    m = m / BITS_PER_UNIT + 1;
    while( m-- )
    {
        if( *ptr++ != 0 ) { return( false ); }
    }
    return( true );
}

//
/************ set_copy() ********************//**
 * \brief lhs = copy of rhs.
 *  sets must be equal. no check
 * \param ptr1 UNIT* : ptr to bitarray data lhs
 * \param ptr2 UNIT* : ptr to bitarray data rhs
 * \param m B_INDEX : Bitsets maxindex
 * \return void
 ***********************************************/
inline void set_copy( UNIT *ptr1, UNIT *ptr2, B_INDEX m )
{
    m = m / BITS_PER_UNIT + 1;
    while( m-- )
    {
        *ptr1++ = *ptr2++;
    }
}

/************ set_clear() *******************//**
 * \brief set all bits to zero.
 *
 * \param ptr UNIT* : ptr to bitarray data
 * \param m B_INDEX : Bitset maxindex
 * \return void
 ***********************************************/
inline void set_clear( UNIT *ptr, B_INDEX m )
{
    m = m / BITS_PER_UNIT + 1;
    while( m-- )
    {
        *ptr++ = 0;
    }
}

/************* set_and() ********************//**
 * \brief lhs = lhs AND rhs.
 *
 * \param ptr1 UNIT* : ptr to bitarray data lhs
 * \param ptr2 UNIT* : ptr to bitarray data rhs
 * \param m B_INDEX : Bitsets maxindex
 * \return void
 ***********************************************/
inline void set_and( UNIT *ptr1, UNIT *ptr2, B_INDEX m )
{
    m = m / BITS_PER_UNIT + 1;
    while( m-- )
    {
        *ptr1++ &= *ptr2++;
    }
}

/************* set_or() *********************//**
 * \brief lhs = lhs OR rhs.
 *
 * \param ptr1 UNIT* : ptr to bitarray data lhs
 * \param ptr2 UNIT* : ptr to bitarray data rhs
 * \param m B_INDEX : Bitsets maxindex
 * \return void
 ***********************************************/
inline void set_or( UNIT *ptr1, UNIT *ptr2, B_INDEX m )
{
    m = m / BITS_PER_UNIT + 1;
    while( m-- )
    {
        *ptr1++ |= *ptr2++;
    }
}

/************* set_xor() *********************//**
 * \brief lhs = lhs XOR rhs.
 *
 * \param ptr1 UNIT* : ptr to bitarray data lhs
 * \param ptr2 UNIT* : ptr to bitarray data rhs
 * \param m B_INDEX : Bitsets maxindex
 * \return void
 ***********************************************/
inline void set_xor( UNIT *ptr1, UNIT *ptr2, B_INDEX m )
{
    m = m / BITS_PER_UNIT + 1;
    while( m-- )
    {
        *ptr1++ ^= *ptr2++;
    }
}

/************* set_not() ********************//**
 * \brief complement all bits in set.
 *
 * \param ptr UNIT* : ptr to bitarray data
 * \param m B_INDEX : Bitsets maxindex
 * \return void
 ***********************************************/
inline void set_not( UNIT *ptr, B_INDEX m )
{
    m = m / BITS_PER_UNIT + 1;
    while( m-- )
    {
        *ptr = ~( *ptr );
        ptr++;
    }
}

/********************************************//**
 * \brief find index of next 1-bit, from index pointed to by ix.
 * No check, but rtns false if beyond length parameter.
 * \param ptr UNIT* : ptr to bitarray data
 * \param ix B_INDEX* : in: ptr to start-of-search index - 1, out: ptr to result
 * \param offset B_INDEX : Range offset (start)
 * \param length B_INDEX : Range length
 * \return bool : true if found, false otherwise.
 * *ix is undefined if the function rtns false
 ***********************************************/

bool nextbit( UNIT *ptr, B_INDEX *ix, B_INDEX offset, B_INDEX length )
{
    B_INDEX bitindex, res;
    int unitindex, maxunitindex;
    UNIT tmp;

    bitindex = *ix + 1 - offset;
    if( bitindex > length ) { return false; }
    maxunitindex = length / BITS_PER_UNIT + 1; //loop guard
    unitindex = bitindex / BITS_PER_UNIT;  //index
    //filter out bits with index < bitindex:
    tmp = ptr[ unitindex ] & -(BIT0 << bitindex % BITS_PER_UNIT);
    while( true )
    {
        if( tmp )
        { // get index of least sign. bit with asm instruction 'bit scan forward (quad word)'. tmp must not be 0.
#ifdef COMPILE_32
            asm ("bsfl %0, %0" : "=r" (tmp) : "0" (tmp));
#else
            asm ("bsfq %0, %0" : "=r" (tmp) : "0" (tmp));
#endif
            res = tmp + unitindex * BITS_PER_UNIT;
            if( res <= length ) { *ix = res + offset; return( true ); }
            else return false;
        }
        if( ++unitindex >= maxunitindex ) break;
        bitindex = 0;
        tmp = ptr[ unitindex ];
    }
    return( false );
}
/********************************************//**
 * \brief print bitarray in hex.
 *
 * \param ptr UNIT* : ptr to bitarray data
 * \param m B_INDEX : Bitsets maxindex
 * \return void
 ***********************************************/
void hexprint( UNIT *ptr, B_INDEX m )
{
    int sz;

    sz = m / BITS_PER_UNIT;
    for( ; sz >= 0; --sz )
    {
        wcout << setfill( L'0' ) << setw( BITS_PER_UNIT / 4 ) << hex << ptr[ sz ];
        if( m > 0 ) { wcout << " "; }
    }
    wcout << endl;
}

/********************************************//**
 * \brief print one UNIT in binary.
 *
 * \param n UNIT : unit to print
 * \param len UNIT : the 'valid' part of the unit. The rest is printed as '.'
 * Set len to 'BITS_PER_UNIT' if the whole unit is valid. (see b_binprint()).
 * \return void
 ***********************************************/
void binprint( UNIT n, unsigned len )
{   //
    for( unsigned i = 1; i <= BITS_PER_UNIT; ++i )
    {
        if( i % 8 == 1 ) { wcout << " "; }
        if( BITS_PER_UNIT - i <= len )
        {
            if( n & ( BIT0 << ( BITS_PER_UNIT - i ))) { wcout << "1"; }
            else { wcout << "0"; }
        }
        else
        {
            wcout << ".";
        }
    }
    wcout << endl;
}

/************ printset() ********************//**
 * \brief print set in decimal (the set, i.e. only the 1-bits of the bitarray).
 *
 * \param ptr UNIT* : ptr to bitarray data
 * \param offset B_INDEX
 * \param length B_INDEX
 * \return void
 ***********************************************/
void printset( UNIT *ptr, B_INDEX offset, B_INDEX length )
{
    B_INDEX result = offset - 1;

    while( nextbit( ptr, &result, offset, length ) )
    {
        wcout << dec << result << " ";
    }
    wcout << "{end}" << endl;
}

/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* class Bitset *************************************
////////////////////////////////////////////////////////////////////////////
==========================================================================*/

/************* Bitset ***********************//**
 * \brief Bitset copy constructor.
 *
 * \param ba const Bitset& : rhs to be copied
 ***********************************************/
Bitset::Bitset( const Bitset& ba )
{
    range.offset = ba.range.offset;
    range.length = ba.range.length;
    b_array_key = MyPool.Push( 0, ( range.length / BITS_PER_UNIT + 1 ) * sizeof( UNIT ) );
    UNIT *ptr1 = MyPool.Get<UNIT>( b_array_key );
    UNIT *ptr2 = MyPool.Get<UNIT>( ba.b_array_key );
    set_copy( ptr1, ptr2, range.length );
}

/************* Bitset ***********************//**
 * \brief construct copy of classless bitarray (for use in Uniset).
 *
 * \param rng const Range : Bitset range {offset, length}( rhs->m_range, rhs->m_array_key )
 * \param arr const POOL_PTR : data in MixedPool storage
 ***********************************************/
//Bitset::Bitset( const Range& rng, const POOL_PTR arr )
//{
//    range.offset = rng.offset;
//    range.length = rng.length;
//    b_array_key = MyPool.Push( 0, ( range.length / BITS_PER_UNIT + 1 ) * sizeof( UNIT ) );
//    UNIT *ptr1 = MyPool.Get<UNIT>( b_array_key );
//    UNIT *ptr2 = MyPool.Get<UNIT>( arr );
//    set_copy( ptr1, ptr2, range.length );
//}

/************* Bitset ***********************//**
 * \brief construct empty bitset with integer parameters for range. With range-check.
 *
 * \param first B_INDEX
 * \param last B_INDEX
 ***********************************************/
Bitset::Bitset( B_INDEX first, B_INDEX last )
{
    if( first < 0 || last < first ) { throw( Bitset_range_error( L"creating bitset with range < 1" ) ); }
    range.offset = first;
    range.length = last - first;
    b_array_key = MyPool.Push( 0, ( range.length / BITS_PER_UNIT + 1 ) * sizeof( UNIT ) );
}

/************* Bitset ***********************//**
 * \brief construct empty bitset with 'Range' parameter
 * errorcheck done by Range constructor.
 * \param r const Range&.
 ***********************************************/
Bitset::Bitset( const Range& r )
{   //
    range.offset = r.offset;
    range.length = r.length;
    b_array_key = MyPool.Push( 0, ( range.length / BITS_PER_UNIT + 1 ) * sizeof( UNIT ) );
}

/************* ~Bitset **********************//**
 * \brief Bitset destructor
 ***********************************************/
Bitset::~Bitset()
{
}

/************ b_insert(n) *******************//**
 * \brief set bit n. Has range check
 * \param n B_INDEX
 * \return bool : true iff bit was set already.
 ***********************************************/
bool Bitset::b_insert( B_INDEX n )
{
    n -= range.offset;
    if( n > range.length || n < 0 ) { throw( Bitset_range_error( L"b_insert(i) out of range" ) ); }
    UNIT *ptr = MyPool.Get<UNIT>( b_array_key );
    return( set_bit( ptr, n ) );
}

/************  b_insert(m,n) ****************//**
 * \brief set bits first..last (including).
 *
 * \param first B_INDEX : if first < 0 nothing happens.
 * \param last B_INDEX : if first > last nothing happens.
 * \return void
 ***********************************************/
void Bitset::b_insert( B_INDEX first, B_INDEX last )
{
    if( first > last || first < 0 ) { return; }
    first -= range.offset;
    last -= range.offset;
    if( first < 0 || last > range.length )
    {
        throw( Bitset_range_error( L"b_insert(i1,i2) out of range" ));
    }
    UNIT *ptr = MyPool.Get<UNIT>( b_array_key );
    return( set_bit( ptr, first, last ));
}

/************* b_delete *********************//**
 * \brief unset bit n.
 * ret. true if bit was set, else false. range check.
 * \param n B_INDEX
 * \return bool.
 ***********************************************/
bool Bitset::b_delete( B_INDEX n )
{
    n -= range.offset;
    if( n > range.length || n < 0 ) { throw( Bitset_range_error( L"b_delete(i) out of range" ) ); }
    UNIT *ptr = MyPool.Get<UNIT>( b_array_key );
    return( del_bit( ptr, n ) );
}

/************ b_contains ********************//**
 * \brief test bit n.
 * Has range check.
 * \param n B_INDEX
 * \return bool
 ***********************************************/
bool Bitset::b_contains( B_INDEX n ) const
{
    n -= range.offset;
    if( n > range.length || n < 0 ) { return( false ); }
    return ( set_contains( MyPool.Get<UNIT>( b_array_key ), n ));
}

/************ b_equal ***********************//**
 * \brief rtn true iff ranges equal AND Bitset lhs == Bitset rhs.
 *
 * \param ba const Bitset& : rhs
 * \return bool
 ***********************************************/
bool Bitset::b_equal( const Bitset& ba ) const
{
    UNIT *ptr1, *ptr2;

    if( range != ba.range )
    {
        throw( Bitset_range_error( L"calling b_equal on sets with unequal ranges" ) );
    }
    ptr1 = MyPool.Get<UNIT>( b_array_key );
    ptr2 = MyPool.Get<UNIT>( ba.b_array_key );
    return( set_equal( ptr1, ptr2, range.length ) );
}

/************ b_empty ***********************//**
 * \brief test the emptiness of this set.
 *
 * \return bool : true iff empty.
 ***********************************************/
bool Bitset::b_empty() const
{
    return( set_empty( MyPool.Get<UNIT>( b_array_key ), range.length ) );
}

/************* b_is_subset_of ***************//**
 * \brief test if lhs is a subset of rhs, i.e every bit which is set in lhs is also set in rhs.
 * Has range check.
 * \param superset const Bitset& : rhs
 * \return bool
 ***********************************************/
bool Bitset::b_is_subset_of( const Bitset& superset ) const
{
    B_INDEX m = range.length;
    if( range != superset.range ) { throw( Bitset_range_error( L"function is_subset_of() only with equal-sized sets!" ) ); }

    UNIT *ptr1 = MyPool.Get<UNIT>( b_array_key );
    UNIT *ptr2 = MyPool.Get<UNIT>( superset.b_array_key );
    m = m / BITS_PER_UNIT + 1;
    while( m-- )
    {
        UNIT super = *ptr2;
        super |= *ptr1;
        if( super != *ptr2 ) return( false );
        ptr1++;
        ptr2++;
    }
    return( true );
}

/************* b_copy ***********************//**
 * \brief make lhs a copy of rhs.
 * Has range check.
 * \param ba const Bitset&
 * \return Bitset& : return(*this) for chaining.
 ***********************************************/
Bitset& Bitset::b_copy( const Bitset& ba )
{
    if( range != ba.range ) { throw( L"copying different-sized sets" ); }

    UNIT *ptr1 = MyPool.Get<UNIT>( b_array_key );
    UNIT *ptr2 = MyPool.Get<UNIT>( ba.b_array_key );
    set_copy( ptr1, ptr2, range.length );
    return( *this );
}

/*************** b_clear ********************//**
 * \brief set all bits in lhs to zero.
 *
 * \return Bitset& : return(*this) for chaining.
 ***********************************************/
Bitset& Bitset::b_clear()
{
    set_clear( MyPool.Get<UNIT>( b_array_key ), range.length );
    return( *this );
}

/************ b_intersect *******************//**
 * \brief lhs = lhs AND rhs..
 * Has range check.
 * \param ba const Bitset& : rhs.
 * \return Bitset& : return(*this) for chaining.
 ***********************************************/
Bitset& Bitset::b_intersect( const Bitset& ba )
{
    if( range != ba.range ) { throw( Bitset_range_error( L"intersecting different-sized sets" ) ); }

    UNIT *ptr1 = MyPool.Get<UNIT>( b_array_key );
    UNIT *ptr2 = MyPool.Get<UNIT>( ba.b_array_key );
    set_and( ptr1, ptr2, range.length );
    return( *this );
}

/************ b_unify ***********************//**
 * \brief lhs = lhs OR rhs.
 * Has range check.
 * \param ba const Bitset& : rhs.
 * \return Bitset& : return(*this) for chaining
 ***********************************************/
Bitset& Bitset::b_unify( const Bitset& ba )
{
    if( range != ba.range ) { throw( Bitset_range_error( L"unifying different-sized sets" ) ); }

    UNIT *ptr1 = MyPool.Get<UNIT>( b_array_key );
    UNIT *ptr2 = MyPool.Get<UNIT>( ba.b_array_key );
    set_or( ptr1, ptr2, range.length );
    return( *this );
}

/************ b_xor *************************//**
 * \brief lhs = lhs XOR rhs.
 * Has range check.
 * \param ba const Bitset& : rhs.
 * \return Bitset& : return(*this) for chaining.
 ***********************************************/
Bitset& Bitset::b_xor( const Bitset& ba )
{
    if( range != ba.range ) { throw( Bitset_range_error( L"XOR-ing different-sized sets" ) ); }

    UNIT *ptr1 = MyPool.Get<UNIT>( b_array_key );
    UNIT *ptr2 = MyPool.Get<UNIT>( ba.b_array_key );
    set_xor( ptr1, ptr2, range.length );
    return( *this );
}

/************ b_complement ******************//**
 * \brief complement Bitset lhs.
 *
 * \return Bitset& : return(*this) for chaining.
 ***********************************************/
Bitset& Bitset::b_complement()
{
    UNIT *ptr = MyPool.Get<UNIT>( b_array_key );
    set_not( ptr, range.length );
    return( *this );
}

/************* b_init ***********************//**
 * \brief set iterator to start-of-range - 1.
 * b_init need not be called, ix can also be set by user (but not lower than start-1).
 * \param ix B_INDEX* : the address of a user-def int variable, ptr to init/result index.
 * \return void
 ***********************************************/
void Bitset::b_init( B_INDEX *ix ) const
{
    *ix = range.offset - 1;
}

/************ b_iterate *********************//**
 * \brief auto-increment iterator, starts at *ix + 1 (see b_init).
 *
 * \param ix B_INDEX* :ptr to initial/result index.
 * \return bool : if true: result in *ix, false otherwise.
 ***********************************************/
bool Bitset::b_iterate( B_INDEX *ix ) const
{
    if( (*ix) < range.offset-1 ) return( false );
    UNIT *ptr = MyPool.Get<UNIT>( b_array_key );
    return( nextbit( ptr, ix, range.offset, range.length ) );
}

//int bitarray::b_next( b_index ix ) const
//{
//	unit tmp;
//	size_t m, i, nxt;
//
//	if( ix >= maxindex ) return( MINUSONE );
//	m = maxindex / BITS_PER_UNIT + 1; //loop guard
//	i = ix / BITS_PER_UNIT;  //bitarray::array_key index
//	nxt = 0;
//	// get rid of bits with lower index than pos:
//	tmp = array_key[ i ] & -(BIT0 << ix % BITS_PER_UNIT);
//	while( i < m ) {
//		if( tmp != 0 ) { //|| () {
//			nxt += bitpos( tmp );
//			return( i * BITS_PER_UNIT + nxt );
//		}
//		tmp = array_key[ ++i ];
//	}
//	return( MINUSONE );
//}


/************* b_hexprint *******************//**
 * \brief print Bitset in hex.
 *
 * \return void
 ***********************************************/
void Bitset::b_hexprint()
{
    UNIT *ptr = MyPool.Get<UNIT>( b_array_key );
    hexprint( ptr, range.length );
}

/************* b_binprint *******************//**
 * \brief print whole Bitset (plus its range) in binary.
 *
 * \return void
 ***********************************************/
void Bitset::b_binprint()
{
    UNIT *ptr = MyPool.Get<UNIT>( b_array_key );
    int len;
    int m = range.length / BITS_PER_UNIT;
    len = range.length % BITS_PER_UNIT;
    range.printrange();
    do {
        binprint( ptr[ m-- ], len );
        len = BITS_PER_UNIT;
    } while( m >= 0 );

}

/************* b_printset *******************//**
 * \brief print Bitset in decimal, only the indexes of the 1-bits.
 *
 * \return void
 ***********************************************/
void Bitset::b_printset()
{
    UNIT *ptr = MyPool.Get<UNIT>( b_array_key );
    printset( ptr, range.offset, range.length );
}
/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* class RANGE **************************************
////////////////////////////////////////////////////////////////////////////
==========================================================================*/

/*********** Range constructor **************//**
 * \brief create Range first..last (incl).
 *
 * \param first B_INDEX : first >= 0
 * \param last B_INDEX : last >= first (max 0x7FFFFFFF)
 ***********************************************/
Range::Range( B_INDEX first, B_INDEX last ) // not offset & length, but first & last!
{
    if( first < 0 || last < first ) { throw( Bitset_range_error( L"creating Range with illegal params" ) ); }
    offset = first;
    length = last - first;
}

/********************************************//**
 * \brief Print start (offset) and length (last+1) of a range
 *
 * \return void
 ***********************************************/
void Range::printrange() const
{
    wcout << "offset:" << offset << "(0x" << setfill( L'0' ) <<  setw(8) << hex << offset;
    wcout << "; length: " << dec << length + 1 << hex << "(0x" << length + 1<<")\n";
}

/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* class M_map **************************************
////////////////////////////////////////////////////////////////////////////
==========================================================================*/

/*************** m_insert *******************//**
 * \brief Insert a new tuple {Range, Bitset::b_array_key(=POOL_PTR)} in the RangeNkey array.
 * i.e: add a Bitset to a Uniset;
 * \param Range : Bitset::range {offset, length}.
 * \param POOL_PTR : Bitset::b_array_key
 * \return POOL_PTR : ptr into MixedPool.
 *
 ***********************************************/
RangeNkey *M_map::m_insert( const Range& incoming, const POOL_PTR inptr )
{
    RangeNkey *rkm, *p1, *p2;
    rkm = MyPool.Get<RangeNkey>( m_keymaps );
    int i, j;

    i = 0;
    while( i < rkm_count )
    {
        if( rkm[ i ].m_range < incoming ) ++i;  // Range operator '<' and '==' overloaded. Only the ranges are compared
        else if( rkm[ i ].m_range == incoming )
        {
            UNIT *lhs = MyPool.Get<UNIT>( rkm[ i ].m_array_key );
            UNIT *rhs = MyPool.Get<UNIT>( inptr );
            set_or( lhs, rhs, (rkm[ i ].m_range).length ); // now that we're here, OR them
            return rkm;
        }
        else
        {
            for ( j = rkm_count - 1; j >= i; --j )
            {
                rkm[ j + 1 ] = rkm[ j ];
            }
            break;
        }
    }
    rkm_count += 1;
    if( rkm_count == rkm_allocated ) //realloc
    {
        rkm_allocated += MAP_DEFAULTALLOC; //go in small steps, not many subsets expected
        POOL_PTR nw = MyPool.Push( 0, rkm_allocated * sizeof(RangeNkey) );
        p1 = MyPool.Get<RangeNkey>(m_keymaps); // old
        p2 = MyPool.Get<RangeNkey>(nw);
        if( p1 != p2 )
        {
            for ( j = 0; j < rkm_count; ++j ) // copy to newly alloc. space
            {
                p2[ j ] = p1[ j ];
            }
            rkm = p2;
            m_keymaps = nw;
        }
    }
    // insert new rkm
    rkm[ i ].m_range = incoming;
    rkm[ i ].m_array_key = MyPool.Push( MyPool.Get<UNIT>(inptr),
                                       (1 + incoming.length / BITS_PER_UNIT) * sizeof(UNIT)); // copy in
    return rkm + i;
}

RangeNkey *M_map::m_find( const Range& incoming )
{
    RangeNkey *rkm;
    rkm = MyPool.Get<RangeNkey>( m_keymaps );
    int i;

    i = 0;
    while( i < rkm_count )
    {
        if( rkm[ i ].m_range.offset == incoming.offset ) return rkm + i;
        ++i;
    }   // check offset only, though both offset & length must be equal
    return 0; // (an error is caught by Range::operator==)
}

/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* M_map iterator ***********************************
////////////////////////////////////////////////////////////////////////////
==========================================================================*/

RangeNkey *M_iterator::m_iterate()
{
    int i = ++m_index;
    if( i >= m_map->rkm_count ) { m_index = (unsigned)-1; return 0; }
    else return MyPool.Get<RangeNkey>(m_map->m_keymaps) + i;
}

/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* class Uniset *************************************
////////////////////////////////////////////////////////////////////////////
==========================================================================*/

/************ u_unify ( Bitset ) ************//**
 * \brief unify a Uniset and a Bitset.
 * If a set with the same Range is already present, their data are OR-ed,
 * otherwise a copy of the Bitset is inserted.
 * \param incoming const Bitset& : incoming Bitset
 * \return Uniset& : return( *this ) for chaining.
 * Uniset ranges cannot overlap, and ranges with the same offset must have the same length.
 ***********************************************/
Uniset& Uniset::u_unify( const Bitset& incoming )
{
//    RangeNkey *here = included_ranges.m_find( incoming.range );
//    if( here != 0 )
//    {
//        UNIT *ptr1 = MyPool.Get<UNIT>( here->m_array_key );
//        UNIT *ptr2 = MyPool.Get<UNIT>( incoming.b_array_key );
//        int m = here->m_range.length / BITS_PER_UNIT;
//        while( m-- >= 0 )
//        {
//            *ptr1++ |= *ptr2++;
//        }
//    }
//    else if( !incoming.b_empty() )
//    {
        included_ranges.m_insert( incoming.range, incoming.b_array_key ); // insert
//    }
    return( *this );

}

/************ u_unify ( Uniset ) ************//**
 * \brief unify a Uniset and a Uniset.
 * if a subset with the same Range is already present, their data are OR-ed,
 * otherwise a copy is inserted into the lhs map.
 * \param incoming const Uniset& : incoming Uniset
 * \return Uniset& : return( *this ) for chaining.
 * Uniset ranges cannot overlap, and ranges with the same offset must have the same length.
 ***********************************************/
Uniset& Uniset::u_unify( const Uniset& incoming )
{
    RangeNkey /* *lhs, */ *rhs;

    M_iterator there( &(incoming.included_ranges) );

    while( (rhs = there.m_iterate()) != 0 )
    {
//        lhs = included_ranges.m_find( rhs->m_range );
//        if( lhs != 0 ) // found one
//        {
//            UNIT *ptr1 = MyPool.Get<UNIT>( lhs->m_array_key );
//            UNIT *ptr2 = MyPool.Get<UNIT>( rhs->m_array_key );
//            int m = lhs->m_range.length / BITS_PER_UNIT;
//            while( m-- >= 0 )
//            {
//                *ptr1++ |= *ptr2++;
//            }
//        }
//        else
//        {
            included_ranges.m_insert( rhs->m_range, rhs->m_array_key ); // insert copy of incoming
//        }
    }
    return( *this );
}

/*************** u_printset *****************//**
 * \brief print all subsets of a Uniset.
 *
 * \return void
 ***********************************************/
void Uniset::u_printset()
{
    B_INDEX result = (unsigned)MINUSONE;
    RangeNkey *rkp;

    U_iterator here( this, &result );
    while( (rkp = here.m_iterate()) != 0 ) // iterate through subsets
    {
        int off = rkp->m_range.offset;
        int len = rkp->m_range.length;
        wcout << "0x" << setfill( L'0' ) << setw( 4 ) << hex << off << ": " << dec;
        UNIT *ptr = MyPool.Get<UNIT>( rkp->m_array_key );
        printset( ptr, off, len );
        //wcout << endl;
    }
}

/*==========================================================================
////////////////////////////////////////////////////////////////////////////
************************* class U_iterator *********************************
////////////////////////////////////////////////////////////////////////////
==========================================================================*/

/************* U_init ***********************//**
 * \brief initialize Uniset iterator.
 *
 * \param us Uniset* : set to iterate.
 * \param res B_INDEX* : pointer to result.
 * \return void :
 * Iterator must be initialized before use.
 ***********************************************/
void U_iterator::u_reinit( Uniset *us , B_INDEX *res )
{
    uniset = us;
    result = res;
    m_iter_reset();
}

/************* U_iterate() ******************//**
 * \brief Uniset iterator, auto increment.
 * Must be u_reinit-ed but iteration start can be set in 'result' : set result = start-1;
 * \return bool : true if found next entry, false if no more entries.
 * Found entry in *result, which is undef if returning false.
 ***********************************************/
bool U_iterator::u_iterate()
{
    if ( m_index == MINUSONE )
    {
        m_iter = m_iterate();
        if( m_iter ) *result = m_iter->m_range.offset - 1;
    }

    while( m_iter )
    {

        UNIT *ptr = MyPool.Get<UNIT>( m_iter->m_array_key );
        if( nextbit( ptr, result, m_iter->m_range.offset, m_iter->m_range.length )) return( true );
        else
        {
            m_iter = m_iterate();
            if( m_iter ) *result = m_iter->m_range.offset - 1;
        }
    }
    m_iter_reset();
    *result = (unsigned)-1;
    return( false );
}
