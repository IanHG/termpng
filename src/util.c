#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void 
abort_
   (  const char * s
   ,  ...
   )
{
   va_list args;
   va_start(args, s);
   vfprintf (stderr, s, args);
   fprintf  (stderr, "\n");
   va_end(args);
   abort();
}

/**
 * Copy a string by allocating a new char array, copying the data
 * and returning the copy. The copy will be null-terminated.
 *
 * The copy must be manually free'd to avoid leaks.
 **/
char* 
string_allocate_and_copy
   (  const char* const str
   )
{   
   int len = strlen(str);
   char* str_copy = (char*) malloc(len + 1);
   memcpy(str_copy, str, len);
   str_copy[len] = '\0';

   return str_copy;
}
