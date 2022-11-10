#pragma once
#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#include <stdint.h>

#include <png.h>

typedef enum 
{  ERROR
,  SUCCESS
,  NOT_PNG 
} status_t;

typedef enum
{  SCALE_SSAA
,  SCALE_FIRST
,  SCALE_LAST
,  SCALE_CENTER
} scale_t;

typedef struct 
{
   uint32_t r:8;
   uint32_t g:8;
   uint32_t b:8;
   uint32_t a:8;
} color32_t;

typedef struct
{
   uint32_t r:16;
   uint32_t g:16;
   uint32_t b:16;
   uint32_t a:16;
} color64_t;

void 
convert_color64_t_to_color32_t
   (  const color64_t* const color64
   ,        color32_t* const color32
   );

typedef struct
{
   int width, height;
   png_byte   color_type;
   png_byte   bit_depth;
   void*      data;
} image_t;

void 
image_t_destroy
   (  image_t* image
   );

void 
image_t_swap
   (  image_t* image1
   ,  image_t* image2
   );

void 
image_t_print
   (  const image_t* const image
   );

status_t 
image_t_read_png
   (  const char* const file_name
   ,  image_t* image
   );

void 
image_t_scale
   (  const image_t* const image
   ,  image_t* const       scaled
   ,  int                  scaled_width
   ,  int                  scaled_height
   ,  scale_t              scale
   );

void 
image_t_scale_percent
   (  const image_t* const image
   ,  image_t* const       scaled
   ,  double               percent
   ,  scale_t              scale
   );

void image_t_draw
   (  const image_t* const image
   ,  char* buffer
   );

void 
image_t_apply_background
   (  image_t* const  image
   ,  int r
   ,  int g
   ,  int b
   );

#endif /* IMAGE_H_INCLUDED */
