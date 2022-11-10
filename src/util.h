#pragma once
#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#define __termpng_attribute_unused__ __attribute__((unused))

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

//! Abort execution and print custom message (printf style)
void 
abort_
   (  const char * s
   ,  ...
   );

#endif /* UTIL_H_INCLUDED */
