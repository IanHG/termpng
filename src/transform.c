#include "transform.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "util.h"

int TRANSFORM_FAILLURE = 0;
int TRANSFORM_SUCCESS  = 1;

//! Forward declaration
void
transform_options_destroy
   (  transform_type_t  type
   ,  void*             options
   );

/**
 * transform_t helper functions.
 **/
//! Initialize transform_t
void 
transform_t_init
   (  transform_t* transform
   )
{
   transform->type      = NONE;
   transform->options   = NULL;
   transform->next      = NULL;
}

//! Allocate a new transform_t and add to the list. Will return pointer to next.
transform_t* 
transform_t_make_next
   (  transform_t* transform
   )
{
   transform->next = (transform_t*) malloc(sizeof(transform_t));
   transform_t_init(transform->next);
   return transform->next;
}

//! Destroy a transform_t. Will loop through the list and de-allocate all allocated transforms and options storage.
void 
transform_t_destroy
   (  transform_t* transform
   )
{
   while(transform)
   {
      // Get next pointer
      transform_t* prev = transform;
      transform         = transform->next;

      // Clean-up previous
      if(prev->options)
      {
         transform_options_destroy(prev->type, prev->options);
         free(prev->options);
      }
      free(prev);
   }
}

/**
 * All transform_parse_X functions must have the same function signature.
 * The functions signature is:
 *
 * int*           argn_ptr       [in/out] :  Pointer to an integer indicating the current argument to process.
 * int            argc           [in]     :  Total number of args in argv.
 * char**         argv           [in]     :  Array of argument strings.
 * transform_t**  transform_ptr  [in/out] :  Pointer to transform pointer.
 *                                           On output the transform pointer will point the next transform_t.
 *
 * Returns status as integer. 1 for success, all other values means failed.
 **/

/**
 * Parse "read".
 **/
typedef struct
{
   char* path;
}  transform_read_options_t;

static int
transform_parse_read
   (  int*           argn_ptr
   ,  int            argc
   ,  char**         argv
   ,  transform_t**  transform_ptr
   )
{
   *transform_ptr = transform_t_make_next(*transform_ptr);
   transform_t* transform = *transform_ptr;

   transform->type = READ;

   transform_read_options_t* transform_read_options = (transform_read_options_t*) malloc(sizeof(transform_read_options_t));

   int argn = *argn_ptr;
   assert(argn + 1 < argc);
   transform_read_options->path = string_allocate_and_copy(argv[argn + 1]); 
   
   transform->options = transform_read_options;

   argn += 2;
   *argn_ptr = argn;

   return 1;
}


/**
 * Parse "scale".
 **/
typedef struct
{
   int     width;
   int     height;
   double  percent;
   scale_t scale;
}  transform_scale_t;

static int 
transform_parse_scale
   (  int*           argn_ptr
   ,  int            argc
   ,  char**         argv
   ,  transform_t**  transform_ptr
   )
{
   *transform_ptr = transform_t_make_next(*transform_ptr);
   transform_t* transform = *transform_ptr;
   
   // Set type
   transform->type    = SCALE;
   
   // Create options with defaults
   transform_scale_t* transform_scale = (transform_scale_t*) malloc(sizeof(transform_scale_t));
   transform_scale->scale   = SCALE_SSAA;
   transform_scale->width   = 0;
   transform_scale->height  = 0;
   transform_scale->percent = 0.0;
   
   // Read options
   int argn = *argn_ptr;
   ++argn;
   while(argn < argc)
   {
      // Check if first char is a '-'
      if(argv[argn][0] != '-')
      {
         printf("Breaking on '%s' (first char: '%c').\n", argv[argn], argv[argn][0]);
         break;
      }
      
      // If first char is '-', we try to parse options
      if(strcmp(argv[argn], "--width") == 0)
      {
         assert(argn + 1 < argc);
         transform_scale->width = atoi(argv[argn + 1]);
         ++argn;
      }
      else if(strcmp(argv[argn], "--height") == 0)
      {
         assert(argn + 1 < argc);
         transform_scale->height = atoi(argv[argn + 1]);
         ++argn;
      }
      else if(strcmp(argv[argn], "--percent") == 0)
      {
         assert(argn + 1 < argc);
         transform_scale->percent = atof(argv[argn + 1]);
         ++argn;
      }
      else if(strcmp(argv[argn], "--type") == 0)
      {
         assert(argn + 1 < argc);
         if(strcmp(argv[argn + 1], "center") == 0)
         {
             transform_scale->scale = SCALE_CENTER;
         }
         else if(strcmp(argv[argn + 1], "first") == 0)
         {
            transform_scale->scale = SCALE_FIRST;
         }
         else if(strcmp(argv[argn + 1], "last") == 0)
         {
            transform_scale->scale = SCALE_LAST;
         }
         else if(strcmp(argv[argn + 1], "ssaa") == 0)
         {
            transform_scale->scale = SCALE_SSAA;
         }
         else
         {
            assert(0);
         }

         ++argn;
      }
      else
      {
         printf("Unknown option '%s'.\n", argv[argn]);
         assert(0);
      }

      ++argn;
   }
   *argn_ptr = argn;
   
   // Set options
   transform->options = transform_scale;

   return 1;
}

/**
 * Parse "draw" (takes no options).
 **/
static int 
transform_parse_draw
   (  int*           argn_ptr
   ,  int            argc
   ,  char**         argv
   ,  transform_t**  transform_ptr
   )
{
   *transform_ptr = transform_t_make_next(*transform_ptr);
   transform_t* transform = *transform_ptr;
   
   // Set type
   transform->type    = DRAW;

   ++(*argn_ptr);

   return 1;
}

/**
 * Parse "background"/"bg".
 **/
typedef struct
{
   color32_t color;
} transform_background_options_t;

static int 
transform_parse_background
   (  int*           argn_ptr
   ,  int            argc
   ,  char**         argv
   ,  transform_t**  transform_ptr
   )
{
   *transform_ptr = transform_t_make_next(*transform_ptr);
   transform_t* transform = *transform_ptr;
   
   // Set type
   transform->type    = BACKGROUND;

   transform_background_options_t* transform_background_options = (transform_background_options_t*) malloc(sizeof(transform_background_options_t));
   transform_background_options->color.r = 0;
   transform_background_options->color.g = 0;
   transform_background_options->color.b = 0;
   
   int argn = *argn_ptr;
   argn += 1;

   while(argn < argc)
   {
      // Check if first char is a '-'
      if(argv[argn][0] != '-')
      {
         printf("Breaking on '%s' (first char: '%c').\n", argv[argn], argv[argn][0]);
         break;
      }

      // If first char is '-', we try to parse options
      if(strcmp(argv[argn], "--color") == 0)
      {
         assert(argn + 3 < argc);
         transform_background_options->color.r = atoi(argv[argn + 1]);
         transform_background_options->color.g = atoi(argv[argn + 2]);
         transform_background_options->color.b = atoi(argv[argn + 3]);
         argn += 3;
      }
      else if(strcmp(argv[argn], "--red") == 0)
      {
         assert(argn + 1 < argc);
         transform_background_options->color.r = atoi(argv[argn + 1]);
         argn += 1;
      }
      else if(strcmp(argv[argn], "--green") == 0)
      {
         assert(argn + 1 < argc);
         transform_background_options->color.g = atoi(argv[argn + 1]);
         argn += 1;
      }
      else if(strcmp(argv[argn], "--blue") == 0)
      {
         assert(argn + 1 < argc);
         transform_background_options->color.b = atoi(argv[argn + 1]);
         argn += 1;
      }
      else if(strcmp(argv[argn], "--gray") == 0)
      {
         assert(argn + 1 < argc);
         transform_background_options->color.r = atoi(argv[argn + 1]);
         transform_background_options->color.g = atoi(argv[argn + 1]);
         transform_background_options->color.b = atoi(argv[argn + 1]);
         argn += 1;
      }
      else
      {
         printf("[transform:background] Unknown option '%s'.\n", argv[argn]);
         assert(0);
      }

      argn += 1;
   }

   transform->options = transform_background_options;
   
   *argn_ptr = argn;

   return 1;
}

/**
 * Parse "crop".
 **/
typedef enum
{  CROP_DEFAULT
,  CROP_EDGE
}  crop_type_t;

typedef struct
{
   crop_type_t type;
   color32_t   bg_color; // Background color for CROP_EDGE
   int         x_crop_begin;
   int         y_crop_begin;
   int         x_crop_end;
   int         y_crop_end;
}  transform_crop_options_t;

static int 
transform_parse_crop
   (  int*           argn_ptr
   ,  int            argc
   ,  char**         argv
   ,  transform_t**  transform_ptr
   )
{
   *transform_ptr = transform_t_make_next(*transform_ptr);
   transform_t* transform = *transform_ptr;
   
   // Set type
   transform->type = CROP;

   transform_crop_options_t* transform_crop_options = (transform_crop_options_t*) malloc(sizeof(transform_crop_options_t));
   transform_crop_options->type           = CROP_DEFAULT;
   transform_crop_options->y_crop_begin   = 0;
   transform_crop_options->x_crop_begin   = 0;
   transform_crop_options->y_crop_end     = 10000; // should be max int
   transform_crop_options->x_crop_end     = 10000; // should be max int
   
   int argn = *argn_ptr;
   argn += 1;

   while(argn < argc)
   {
      // Check if first char is a '-'
      if(argv[argn][0] != '-')
      {
         printf("Breaking on '%s' (first char: '%c').\n", argv[argn], argv[argn][0]);
         break;
      }

      // If first char is '-', we try to parse options
      if(strcmp(argv[argn], "--edge") == 0)
      {
         assert(argn + 3 < argc);
         transform_crop_options->type = CROP_EDGE;
         transform_crop_options->bg_color.r = atoi(argv[argn + 1]);
         transform_crop_options->bg_color.g = atoi(argv[argn + 2]);
         transform_crop_options->bg_color.b = atoi(argv[argn + 3]);
         argn += 3;
      }
      else if(strcmp(argv[argn], "--define") == 0)
      {
         assert(argn + 4 < argc);
         transform_crop_options->x_crop_begin = atoi(argv[argn + 1]);
         transform_crop_options->y_crop_begin = atoi(argv[argn + 2]);
         transform_crop_options->x_crop_end   = atoi(argv[argn + 3]);
         transform_crop_options->y_crop_end   = atoi(argv[argn + 4]);
         argn += 4;
      }
      else if(strcmp(argv[argn], "--y_begin") == 0)
      {
         assert(argn + 1 < argc);
         transform_crop_options->y_crop_begin = atoi(argv[argn + 1]);
         argn += 1;
      }
      else if(strcmp(argv[argn], "--y_end") == 0)
      {
         assert(argn + 1 < argc);
         transform_crop_options->y_crop_end = atoi(argv[argn + 1]);
         argn += 1;
      }
      else if(strcmp(argv[argn], "--x_begin") == 0)
      {
         assert(argn + 1 < argc);
         transform_crop_options->x_crop_begin = atoi(argv[argn + 1]);
         argn += 1;
      }
      else if(strcmp(argv[argn], "--x_end") == 0)
      {
         assert(argn + 1 < argc);
         transform_crop_options->x_crop_end = atoi(argv[argn + 1]);
         argn += 1;
      }
      else
      {
         printf("[transform:crop] Unknown option '%s'.\n", argv[argn]);
         assert(0);
      }

      argn += 1;
   }

   transform->options = transform_crop_options;
   
   *argn_ptr = argn;

   return 1;
}

//!
void
transform_options_destroy
   (  const transform_type_t  type
   ,  void*                   options
   )
{
   switch(type)
   {
      case READ:
      {
         transform_read_options_t* options_read = (transform_read_options_t*) options;
         if(options_read->path)
            free(options_read->path);
         break;
      }
      case NONE:
      case SCALE:
      case CROP:
      case DRAW:
      case BACKGROUND:
         /* Do nothing */
         break;
   }
}

/**
 *
 **/
typedef struct
{
   const char* name;
   int (*func)(int*, int, char**, transform_t**);
}  transform_command_t;

static transform_command_t command_table [] =
{  {  "scale"     , transform_parse_scale }
,  {  "read"      , transform_parse_read  }
,  {  "draw"      , transform_parse_draw  }
,  {  "bg"        , transform_parse_background  }
,  {  "background", transform_parse_background  }
,  {  "crop"      , transform_parse_crop  }
};

//! Get index of command with name, if it exists, otherwise returns -1.
static int 
transform_command_index
   (  const char* const name
   )
{
   int i;
   for(i = 0; i < 6; ++i)
   {
      printf("Testing keyword '%s'.\n", command_table[i].name);
      if(strcmp(command_table[i].name, name) == 0)
      {
         return i;
      }
   }
   return -1;
}

/**
 * Parse transform command line
 **/
int 
transform_parse_args
   (  int*           argn_ptr
   ,  int            argc
   ,  char*          argv[]
   ,  transform_t*   transform
   )
{
   int argn = *argn_ptr;
   int i;
   transform_t** transform_ptr = &transform;
   while(argn < argc)
   {
      if((i = transform_command_index(argv[argn])) != -1)
      {
         if(!command_table[i].func(&argn, argc, argv, transform_ptr))
         {
            // Command failed
            assert(0);
         }
      }
      else
      {
         // Unknown command
         printf("Unknown command '%s'.\n", argv[argn]);
         assert(0);
         break;
      }
   }

   *argn_ptr = argn;

   return 1;
}

int
transform_apply_read
   (  image_t* image
   ,  const void* const options
   )
{
   transform_read_options_t* options_read = (transform_read_options_t*) options;
   
   printf("Filename '%s'.\n", options_read->path);
   fflush(stdout);

   image_t_read_png(options_read->path, image);
   
   return TRANSFORM_SUCCESS;
}

int 
transform_apply_scale
   (  image_t*          image
   ,  const void* const options_ptr
   )
{
   image_t scaled;
   transform_scale_t* options = (transform_scale_t*) options_ptr;
   
   // Scale
   if(options->percent)
   {
      image_t_scale_percent(image, &scaled, options->percent, options->scale);
   }
   else
   {
      image_t_scale(image, &scaled, options->width, options->height, options->scale);
   }

   // Clean-up
   image_t_swap(image, &scaled);
   image_t_destroy(&scaled);

   return TRANSFORM_SUCCESS;
}

int 
transform_apply_crop
   (  image_t*          image
   ,  const void* const options_ptr
   )
{
   image_t modified;
   transform_crop_options_t* options = (transform_crop_options_t*) options_ptr;
   
   // Do operation
   switch(options->type)
   {
      case CROP_DEFAULT:
         image_t_crop(image, &modified, options->x_crop_begin, options->y_crop_begin, options->x_crop_end, options->y_crop_end);
         break;
      case CROP_EDGE:
         image_t_crop_background(image, &modified, options->bg_color.r, options->bg_color.g, options->bg_color.b);
         break;
   }

   // Clean-up
   image_t_swap(image, &modified);
   image_t_destroy(&modified);

   return TRANSFORM_SUCCESS;
}

/**
 * Apply a transform pipeline to an image.
 **/
int 
transform_apply_pipeline
   (  image_t*             image
   ,  const transform_t*   transform
   )
{
   int status = TRANSFORM_SUCCESS;

   while(transform)
   {
      switch(transform->type)
      {
         case NONE:
         {
            printf("NONE\n");
            break;
         }
         case READ:
         {
            printf("READ");
            status = transform_apply_read(image, transform->options);
            break;
         }
         case SCALE:
         {
            printf("SCALE\n");
            status = transform_apply_scale(image, transform->options);
            break;
         }
         case CROP:
         {
            printf("CROP\n");
            status = transform_apply_crop(image, transform->options);
            break;
         }
         case DRAW:
         {
            printf("DRAW\n");
            char buffer[1024 * 1024 * 4];
            image_t_draw(image, buffer);
            break;
         }
         case BACKGROUND:
         {
            printf("BACKGROUND\n");
            color32_t* color = &((transform_background_options_t*) transform->options)->color;
            image_t_apply_background(image, color->r, color->g, color->b);
            break;
         }
      }

      if(status == TRANSFORM_FAILLURE)
         break;

      transform = transform->next; // Go to next transform
   }

   return status;
}
