//===- llvm/System/ThreadLocal.h - Thread Local Data ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the llvm::sys::ThreadLocal class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SYSTEM_THREAD_LOCAL_H
#define LLVM_SYSTEM_THREAD_LOCAL_H

#include "llvm/System/Threading.h"
#include <cassert>
#include "yasmx/Config/export.h"

namespace llvm {
  namespace sys {
    // ThreadLocalImpl - Common base class of all ThreadLocal instantiations.
    // YOU SHOULD NEVER USE THIS DIRECTLY.
    class YASM_LIB_EXPORT ThreadLocalImpl {
      void* data;
    public:
      ThreadLocalImpl();
      virtual ~ThreadLocalImpl();
      void setInstance(const void* d);
      const void* getInstance();
      void removeInstance();
    };
    
    /// ThreadLocal - A class used to abstract thread-local storage.  It holds,
    /// for each thread, a pointer a single object of type T.
    template<class T>
    class ThreadLocal : public ThreadLocalImpl {
    public:
      ThreadLocal() : ThreadLocalImpl() { }
      
      /// get - Fetches a pointer to the object associated with the current
      /// thread.  If no object has yet been associated, it returns NULL;
      T* get() { return static_cast<T*>(getInstance()); }
      
      // set - Associates a pointer to an object with the current thread.
      void set(T* d) { setInstance(d); }
      
      // erase - Removes the pointer associated with the current thread.
      void erase() { removeInstance(); }
    };
  }
}

#endif
