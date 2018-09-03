#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED

/**************************  MixedPool.cpp   **********************************
* Author:        Agner Fog
* Date created:  2008-06-12
* Last modified: 2008-06-12
* Description:
* Defines memory pool for storing data of mixed type and size.
*
* The latest version of this file is available at:
* www.agner.org/optimize/cppexamples.zip
* (c) 2008 GNU General Public License www.gnu.org/copyleft/gpl.html
*******************************************************************************
*
* MixedPool is a container class defining a memory pool that can contain
* objects of any type and size. It is possible to store objects of different
* types in the same MixedPool.
*
* MixedPool is useful for storing objects of any type and size in a memory
* buffer of dynamic size. This is often more efficient that allocating a
* separate memory buffer for each object.
*
* MixedPool is also useful for reading and writing binary files containing
* mixed data structures.
*
* The objects stored can be of any type that do not require a constructor
* or destructor. The size of the memory pool can grow, but not shrink.
* Objects cannot be removed randomly. Objects can only be removed by
* calling SetDataSize, which removes all subsequently stored objects as well.
* While this restriction can cause some memory space to be wasted, it has
* the advantage that no time-consuming garbage collection is needed.
*
* MixedPool is not thread safe if shared between threads. If your program
* needs storage in multiple threads then each thread must have its own
* private instance of MixedPool, or you must prevent other threads from
* accessing MixedPool while you are adding objects.
*
* Note that you should never store a pointer to an object in the memory
* pool because the pointer will become invalid in case the subsequent
* addition of another object causes the memory to be re-allocated.
* All objects in the pool should be identified by the offset returned
* by the Push function, not by pointers.
*
* Attempts to read an object with an offset beyond the current DataSize
* will cause an error message to the standard error output. You may change
* the MixedPool::Error function to produce a message box if the program has
* a graphical user interface.
*
* It is possible, but not necessary, to allocate a certain amount of
* memory before adding any objects. This can reduce the risk of having
* to re-allocate memory if the first allocated memory block turns out
* to be too small. Use the function ReserveSize to set the desired size
* of memory if it is possible to make a reasonable estimate of how much
* memory will be needed.
*
* MixedPool does not remember the type, size and offset of each object
* stored. It is the responsibility of the programmer to make sure that
* the correct offset and type is specified when retrieving a previously
* stored object from MixedPool.
*
* At the end of this file you will find a working example of how to use
* MixedPool.
*
* The first part of this file containing declarations may be placed in a
* header file. The second part containing function definitions should be in
* a .cpp file. The third part containing examples should be removed from your
* final application.
*
******************************************************************************/


// Class MixedPool makes a dynamic array which can grow as new data are
// added. Data can be of mixed type and size
class MixedPool {
public:
   MixedPool();                                  // Constructor
   ~MixedPool();                                 // Destructor
   void ReserveSize(int newpoolsize);               // Allocate buffer of specified size
   void SetDataSize(int newdatasize);               // Set the size of the data area that is considered used
   int GetDataSize()  {return DataSize;};        // Size of data stored so far
   int GetBufferSize(){return BufferSize;};      // Size of allocated buffer
   int GetNumEntries(){return NumEntries;};      // Get number of items pushed
   int Push(void const * obj, int objsize);     // Add object to buffer, return offset
   template<typename TX> int Push(TX const & x){ // Same, size detected automatically
      return Push(&x, sizeof(TX));}
   void *Alloc(size_t objsize);                     // rtn ptr to uninitialized space
//   int PushString(char * s) {                    // Add zero-terminated string to buffer, return offset
//      return Push(s, strlen(s) + 1);}
   void Align(int a);                            // Align next entry to address divisible by a (relative to buffer start)
   char * Buf() const {return buffer;};          // Access to buffer
   template <typename TX> TX * Get(int Offset) { // Get object of arbitrary type at specified offset in buffer
      if (Offset >= DataSize) {Error(1,Offset); Offset = 0;} // Offset out of range
      return (TX*)(buffer + Offset);}
   // Define desired allocation size
   enum DefineSize {
      AllocateSpace  = 4096};                    // Minimum size of allocated memory block
private:
   char * buffer;                                // Buffer containing binary data. To be modified only by SetSize
   int BufferSize;                               // Size of allocated buffer ( > DataSize)
   MixedPool(MixedPool const&){};                // Make private copy constructor to prevent copying
   void operator = (MixedPool const&){};         // Make private assignment operator to prevent copying
protected:
   int NumEntries;                               // Number of objects pushed
   int DataSize;                                 // Size of data, offset to vacant space
   void Error(int e, int n);                     // Make fatal error message
};

extern MixedPool MyPool;



#endif // STORAGE_H_INCLUDED
