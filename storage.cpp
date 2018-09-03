#include <memory.h>                              // For memcpy and memset
#include <string.h>                              // For strlen
#include <stdlib.h>                              // For exit in Error function
#include <stdio.h>                               // Needed for example only
#include <iostream>
#ifdef USE_DEBUG_NEW
#include "debug_new.h"
#endif
#include "storage.h"

// Members of class MixedPool
MixedPool::MixedPool() {
   // Constructor
   buffer = 0;
   NumEntries = DataSize = BufferSize = 0;
}

MixedPool::~MixedPool() {
    std::wcout << "datasize: " << DataSize << std::endl;
   // Destructor
   ReserveSize(0);                     // De-allocate buffer
}

void MixedPool::ReserveSize(int newpoolsize) {
   // Allocate buffer of at least the specified newpoolsize
   // Setting newpoolsize > current BufferSize will allocate a larger buffer and
   // move all data to the new buffer.
   // Setting newpoolsize <= current BufferSize will do nothing. The buffer will
   // only grow, not shrink.
   // Setting newpoolsize = 0 will discard all data and de-allocate the buffer.
   if (newpoolsize <= BufferSize) {
      if (newpoolsize <= 0) {
         if (newpoolsize < 0) Error(1, newpoolsize);
         // newpoolsize = 0. Discard data and de-allocate buffer
         if (buffer) delete[] buffer;  // De-allocate buffer
         buffer = 0;
         NumEntries = DataSize = BufferSize = 0;
         return;
      }
      // Request to reduce newpoolsize. Ignore
      return;
   }
   newpoolsize = (newpoolsize + 15) & (-16);         // Round up newpoolsize to value divisible by 16
   char * buffer2 = 0;                 // New buffer
   buffer2 = new char[newpoolsize];           // Allocate new buffer
   if (buffer2 == 0) {Error(3,newpoolsize); return;} // Error can't allocate
   memset (buffer2, 0, newpoolsize);          // Initialize to all zeroes
   if (buffer) {
      // A smaller buffer is previously allocated
      memcpy (buffer2, buffer, BufferSize); // Copy contents of old buffer into new one
      delete[] buffer;                 // De-allocate old buffer
   }
   buffer = buffer2;                   // Save pointer to buffer
   BufferSize = newpoolsize;                  // Save newpoolsize
}

void MixedPool::SetDataSize(int newdatasize) {
   // Set the newdatasize of the data area that is considered used and valid.
   // DataSize is initially zero. It is increased by Push.
   // Setting newdatasize > DataSize is equivalent to pushing zeroes until
   // DataSize = newdatasize.
   // Setting newdatasize < DataSize will decrease DataSize so that all data
   // with offset >= newdatasize is erased.
   // NumEntries is not changed by SetDataSize(). Calls to GetNumEntries()
   // will be meaningless after calling SetDataSize().

   if (newdatasize < 0) Error(1, newdatasize);
   if (newdatasize > BufferSize) {
      // Allocate larger buffer. Add AllocateSpace and round up to divisible by 16
      ReserveSize((newdatasize + AllocateSpace + 15) & (-16));
   }
   else if (newdatasize < DataSize) {
      // Request to delete some data. Overwrite with zeroes
      memset(buffer + newdatasize, 0, DataSize - newdatasize);
   }
   // Set new DataSize
   DataSize = newdatasize;
}


int MixedPool::Push(void const * obj, int objsize) {
   // Add object to buffer, return offset
   // Parameters:
   // obj = pointer to object, 0 if fill with zeroes
   // objsize = objsize of object to push

   // Old offset will be offset to new object
   int OldOffset = DataSize;
#ifndef COMPILE_32
    objsize = (objsize+15)&(-16); //align by 16 bytes
#endif
    //std::cout << "allocating " << objsize << std::endl;
   // New data objsize will be old data objsize plus objsize of new object
   int NewOffset = DataSize + objsize;

   if (NewOffset > BufferSize) {
      // Buffer too small, allocate more space.
      // We can use SetSize for this only if it is certain that obj is not
      // pointing to an object previously allocated in the old buffer
      // because it would be deallocated before copied into the new buffer.

      // Allocate more space without using SetSize:
      // Double the objsize + AllocateSpace, and round up objsize to value divisible by 16
      int NewSize = (NewOffset * 2 + AllocateSpace + 15) & (-16);
      char * buffer2 = 0;              // New buffer
      buffer2 = new char[NewSize];     // Allocate new buffer
      if (buffer2 == 0) {
         Error(3, NewSize); return 0;  // Error can't allocate
      }
      // Initialize to all zeroes
      memset (buffer2, 0, NewSize);
      if (buffer) {
         // A smaller buffer is previously allocated
         // Copy contents of old buffer into new
         memcpy (buffer2, buffer, BufferSize);
      }
      BufferSize = NewSize;                      // Save objsize
      if (obj && objsize) {
         // Copy object to new buffer
         memcpy (buffer2 + OldOffset, obj, objsize);
         obj = 0;                                // Prevent copying once more
      }
      // Delete old buffer after copying object
      if (buffer) delete[] buffer;

      // Save pointer to new buffer
      buffer = buffer2;
   }
   // Copy object to buffer if nonzero
   if (obj && objsize) {
      memcpy (buffer + OldOffset, obj, objsize);
   }
   if (objsize) {
      // Adjust new offset
      DataSize = NewOffset;
      NumEntries++;
   }
   // Return offset to allocated object
   return OldOffset;
}

void *MixedPool::Alloc(size_t objsize) {
   int OldOffset = DataSize;
#ifndef COMPILE_32
    objsize = (objsize+15)&(-16);
#endif
    //std::cout << "allocating2 " << objsize << std::endl;
   int NewOffset = DataSize + objsize;

   if (NewOffset > BufferSize) {
      int NewSize = (NewOffset * 2 + AllocateSpace + 15) & (-16);
      char * buffer2 = 0;              // New buffer
      buffer2 = new char[NewSize];     // Allocate new buffer
      if (buffer2 == 0) {
         Error(3, NewSize); return 0;  // Error can't allocate
      }
      if (buffer) {
         // A smaller buffer is previously allocated
         // Copy contents of old buffer into new
         memcpy (buffer2, buffer, BufferSize);
      }
      BufferSize = NewSize;                      // Save objsize
      if (buffer) delete[] buffer;
      buffer = buffer2;
   }
   if (objsize) {
      // Adjust new offset
      DataSize = NewOffset;
      NumEntries++;
   }
   // Return ptr to allocated object
   return buffer + OldOffset;
}

void MixedPool::Align(int a) {
   // Align next entry to address divisible by a, relative to buffer start.
   // If a is not sure to be a power of 2:
   //int NewOffset = (DataSize + a - 1) / a * a;
   // If a is sure to be a power of 2:
   int NewOffset = (DataSize + 15) & (-16);
   if (NewOffset > BufferSize) {
      // Allocate more space
      ReserveSize (NewOffset * 2 + AllocateSpace);
   }
   // Set DataSize to after alignment space
   DataSize = NewOffset;
}


// Produce fatal error message. Used internally and by StringElement.
// Note: If your program has a graphical user interface (GUI) then you
// must rewrite this function to produce a message box with the error message.
void MixedPool::Error(int e, int n) {
   // Define error texts
   static const char * ErrorTexts[] = {
      "Unknown error",                 // 0
      "Offset out of range",           // 1
      "Size out of range",             // 2
      "Memory allocation failed"       // 3
   };
   // Number of texts in ErrorTexts
   const unsigned int NumErrorTexts = sizeof(ErrorTexts) / sizeof(*ErrorTexts);

   // check that index is within range
   if ((unsigned int)e >= NumErrorTexts) e = 0;

   // Replace this with your own error routine, possibly with a message box:
   fprintf(stderr, "\nMixedPool error: %s (%i)\n", ErrorTexts[e], n);

   // Terminate execution
   exit(1);
}

