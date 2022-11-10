#pragma once
#ifndef TRANSFORM_H_INCLUDED
#define TRANSFORM_H_INCLUDED

#include "image.h"

typedef enum
{  NONE
,  READ
,  SCALE
,  CROP
,  DRAW
,  BACKGROUND
}  transform_type_t;

/**
 * Struct for holding transform type
 **/
typedef struct transform_t_struct
{
   transform_type_t           type;
   void*                      options;
   struct transform_t_struct* next;
}  transform_t;

void 
transform_t_init
   (  transform_t* transform
   );

transform_t* 
transform_t_make_next
   (  transform_t* transform
   );

void 
transform_t_destroy
   (  transform_t* transform
   );

/**
 * Parse transform input
 **/
int 
transform_parse_args
   (  int*           argn_ptr
   ,  int            argc
   ,  char*          argv[]
   ,  transform_t*   transform_ptr
   );

/**
 * Run tra
 **/
int 
transform_apply_pipeline
   (  image_t*             image
   ,  const transform_t*   transform
   );

#endif /* TRANSFORM_H_INCLUDED */
