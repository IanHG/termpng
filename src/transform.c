#include "transform.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 *
 **/
int TRANSFORM_FAILLURE = 0;
int TRANSFORM_SUCCESS  = 1;



void 
transform_t_init
   (  transform_t* transform
   )
{
   transform->type      = NONE;
   transform->options   = NULL;
   transform->next      = NULL;
}

transform_t* 
transform_t_make_next
   (  transform_t* transform
   )
{
   transform->next = (transform_t*) malloc(sizeof(transform_t));
   transform_t_init(transform->next);
   return transform->next;
}

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
         free(prev->options);
      free(prev);
   }
}

typedef struct
{
   int     width;
   int     height;
   double  percent;
   scale_t scale;
}  transform_scale_t;

/**
 *
 * @return
 *    status
 **/
static int 
transform_command_scale
   (  int*           argn_ptr
   ,  int            argc
   ,  char**         argv
   ,  transform_t**  transform_ptr
   )
{
   *transform_ptr = transform_t_make_next(*transform_ptr);
   
   // Set type
   transform_t* transform = *transform_ptr;
   transform->type    = SCALE;
   
   // Create options with defaults
   transform_scale_t* transform_scale = (transform_scale_t*) malloc(sizeof(transform_scale_t));
   transform_scale->scale   = SCALE_SSAA;
   transform_scale->width   = 0;
   transform_scale->height  = 0;
   transform_scale->percent = 0.0;
   
   // Read options
   int argn = *argn_ptr;
   while(argn < argc)
   {
      // Check if first char is a '-'
      if(argv[argn][0] == '-')
         break;
      
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

      ++argn;
   }
   *argn_ptr = argn;
   
   // Set options
   transform->options = transform_scale;

   return 1;
}

typedef struct
{
   const char* name;
   int (*func)(int*, int, char**, transform_t**);
}  transform_command_t;

static transform_command_t command_table [] =
{  {  "scale", transform_command_scale }
};

static int 
transform_command_index
   (  const char* const name
   )
{
   int i;
   for(i = 0; i < sizeof(command_table); ++i)
   {
      if(strcmp(command_table[i].name, name) == 0)
      {
         return i;
      }
   }
   return -1;
}

int 
transform_parse_args
   (  int            argc
   ,  char*          argv[]
   ,  transform_t*   transform
   )
{
   int argn = 1;
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
         assert(0);
         break;
      }
   }

   return 1;
}


int 
transform_apply_scale
   (  image_t*                        image
   ,  const transform_scale_t* const  options
   )
{
   image_t scaled;
   
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
transform_apply_pipeline
   (  image_t*       image
   ,  transform_t*   transform
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
         case SCALE:
         {
            printf("SCALE\n");
            status = transform_apply_scale(image, (transform_scale_t*) transform->options);
            break;
         }
         case CROP:
         {
            printf("CROP\n");
         }
         case DRAW:
         {
            printf("DRAW\n");
            char buffer[1024 * 1024 * 4];
            image_t_draw(image, buffer);
            break;
         }
      }

      if(status == TRANSFORM_FAILLURE)
         break;

      transform = transform->next; // Go to next transform
   }

   return status;
}
