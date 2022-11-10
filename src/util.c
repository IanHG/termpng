#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
