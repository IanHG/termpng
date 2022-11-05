#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#include <png.h>

#define __termpng_attribute_unused__ __attribute__((unused))

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

struct
{
} global;

///**
// * Print usage help message.
// * Will exit the program.
// **/
//void print_usage (FILE* stream, int exit_code)
//{
//  fprintf (stream,
//           "  -h  --help         Display this usage information.\n"
//           "  -v  --verbose      Print verbose messages.\n"
//           "  -r  --rows <rows>  Set number of rows (default: 10).\n"
//           "  -c  --cols <cols>  Set number of cols (default: 10).\n"
//           "  --fps <fps>        The number of frames to simulate per second.\n"
//           "  --braille          Display using Braille characters.\n"
//           "  --random           Randomly fill the board.\n"
//           "  --fraction <fraction>\n"
//           "                     The fraction of cells that are populated when using --random.\n"
//           );
//  exit (exit_code);
//}

///**
// * Parse command line options.
// * Called on master thread.
// *
// * Based on this example:
// *    https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
// **/
//int parse_cmdl_args(int argc, char* argv[])
//{
//   int c;
//   
//   while (1)
//   {
//      /* Define command-line options to be parsed with getopt_long */
//      static struct option long_options[] =
//      {
//         /* These options set a flag. */
//         {"verbose", no_argument, &verbose, 1},
//         {"random",  no_argument, &random_fill,  1},
//         {"braille",  no_argument, &braille,  1},
//         /* These options don’t set a flag.
//            We distinguish them by their indices. */
//         {"help", no_argument,       0, 'h'},
//         {"rows", required_argument, 0, 'r'},
//         {"cols", required_argument, 0, 'c'},
//         {"fps", required_argument,  0, 'f'},
//         {"fraction", required_argument,  0, 'a'},
//         {0, 0, 0, 0}
//      };
//
//      /* getopt_long stores the option index here. */
//      int option_index = 0;
//
//      c = getopt_long (argc, argv, "hs:",
//            long_options, &option_index);
//
//      /* Detect the end of the options. */
//      if (c == -1)
//         break;
//
//      switch (c)
//      {
//         case 0:
//            /* If this option set a flag, do nothing else now. */
//            if (long_options[option_index].flag != 0)
//               break;
//            printf ("option %s", long_options[option_index].name);
//            if (optarg)
//               printf (" with arg %s", optarg);
//            printf ("\n");
//            break;
//
//         case 'h':
//            print_usage(stdout, 0);
//
//         case 'r':
//            rows = strtol(optarg, NULL, 10);
//            break;
//         
//         case 'c':
//            cols = strtol(optarg, NULL, 10);
//            break;
//
//         case 'f':
//            fps = strtof(optarg, NULL);
//            break;
//
//         case 'a':
//            random_fraction = strtof(optarg, NULL);
//            break;
//
//         case '?':
//            /* getopt_long already printed an error message. */
//            break;
//
//         default:
//            abort ();
//      }
//   }
//
//   /* Instead of reporting ‘--verbose’
//      and ‘--brief’ as they are encountered,
//      we report the final status resulting from them. */
//   if (verbose)
//      printf("verbose flag is set");
//
//   /* Print any remaining command line arguments (not options). */
//   if (optind < argc)
//   {
//      printf ("non-option ARGV-elements: ");
//      while (optind < argc)
//         printf ("%s ", argv[optind++]);
//      putchar ('\n');
//   }
//   
//   return 0;
//}

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

void abort_(const char * s, ...)
{
   va_list args;
   va_start(args, s);
   vfprintf(stderr, s, args);
   fprintf(stderr, "\n");
   va_end(args);
   abort();
}

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

int color32_t_is_equal_rgb
   (  const color32_t   color1
   ,  const color32_t   color2
   )
{
   if (  color1.r == color2.r
      && color1.g == color2.g
      && color1.b == color2.b
      )
   {
      return 1;
   }
   
   return 0;
}

void convert_color64_t_to_color32_t
   (  const color64_t* const color64
   ,        color32_t* const color32
   )
{
   int range64 = 65536;
   int range32 = 256;

   color32->r = (int) ((double) color64->r / (double) range64 * (double) range32);
   color32->g = (int) ((double) color64->g / (double) range64 * (double) range32);
   color32->b = (int) ((double) color64->b / (double) range64 * (double) range32);
   color32->a = (int) ((double) color64->a / (double) range64 * (double) range32);
}

void apply_gamma_correction(color32_t* color, float exposure, float gamma)
{
   color->r = pow((exposure * color->r), gamma);
   color->g = pow((exposure * color->g), gamma);
   color->b = pow((exposure * color->b), gamma);
}

typedef struct
{
   int width, height;
   png_byte   color_type;
   png_byte   bit_depth;
   void*      data;
} image_t;

void image_t_destroy(image_t* image)
{
   if(image->data)
      free(image->data);
}

void image_t_print(const image_t* const image)
{
   printf("Image:\n");
   printf("   width     : %i\n", image->width);
   printf("   height    : %i\n", image->height);
   printf("   color_type: %i\n", image->color_type);
   printf("   bit_depth : %i\n", image->bit_depth);
}

status_t read_png(char* file_name, image_t* image)
{
   unsigned char header[8];    // 8 is the maximum size that can be checked

   /* open file and test for it being a png */
   FILE *fp = fopen(file_name, "rb");
   if (!fp)
      abort_("[read_png_file] File %s could not be opened for reading", file_name);
   if(fread(header, 1, 8, fp) != 8)
      abort_("[read_png_file] File %s, could not read header", file_name);
   if (png_sig_cmp(header, 0, 8))
      abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);

   /* initialize stuff */
   png_structp png_ptr;
   png_infop info_ptr;
   int number_of_passes __termpng_attribute_unused__;

   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if (!png_ptr)
      abort_("[read_png_file] png_create_read_struct failed");

   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
      abort_("[read_png_file] png_create_info_struct failed");

   if (setjmp(png_jmpbuf(png_ptr)))
      abort_("[read_png_file] Error during init_io");

   png_init_io(png_ptr, fp);
   png_set_sig_bytes(png_ptr, 8);

   png_read_info(png_ptr, info_ptr);

   image->width      = png_get_image_width(png_ptr, info_ptr);
   image->height     = png_get_image_height(png_ptr, info_ptr);
   image->color_type = png_get_color_type(png_ptr, info_ptr);
   image->bit_depth  = png_get_bit_depth(png_ptr, info_ptr);

   number_of_passes = png_set_interlace_handling(png_ptr);
   png_read_update_info(png_ptr, info_ptr);

   /* read file */
   if (setjmp(png_jmpbuf(png_ptr)))
      abort_("[read_png_file] Error during read_image");

   png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * image->height);
   int y;
   for (y = 0; y < image->height; ++y)
      row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

   png_read_image(png_ptr, row_pointers);

   image->data = malloc(image->width * image->height * sizeof(color32_t));
   color32_t* data = (color32_t*) image->data;
   
   int x;
   for(y = 0; y < image->height; ++y)
   {
      png_byte* row       = row_pointers[y];
      for(x = 0; x < image->width; ++x)
      {
         png_byte* ptr = &(row[x * 4]);
         (*data).r = ptr[0];
         (*data).g = ptr[1];
         (*data).b = ptr[2];
         (*data).a = ptr[3];
      
         ++data;
      }
   }
   
   /* Free row_pointers again */
   for (y = 0; y < image->height; ++y)
      free(row_pointers[y]);
   free(row_pointers);

   fclose(fp);

   return SUCCESS;
}

void image_t_scale
   (  const image_t* const image
   ,  image_t* const       scaled
   ,  int                  scaled_width
   ,  int                  scaled_height
   ,  scale_t              scale
   )
{
   scaled_height = (scaled_height % 2 == 0) ? scaled_height : scaled_height + 1; /* make sure height is an even number */

   scaled->width  = scaled_width;
   scaled->height = scaled_height;
   scaled->color_type = image->color_type;
   scaled->bit_depth  = image->bit_depth;
   scaled->data   = malloc(scaled->width * scaled->height * sizeof(color32_t));

   int i;
   int scaled_size = scaled->width * scaled->height;  
   color32_t* scaled_data = (color32_t*) scaled->data;
   color32_t  black = {0, 0, 0, 0};
   for(i = 0; i < scaled_size; ++i)
   {
      scaled_data[i] = black;
   }

   int x_block_size_min = floor((double) image->width  / (double) scaled->width);
   int x_block_rest     = image->width % scaled->width;
   int y_block_size_min = floor((double) image->height / (double) scaled->height);
   int y_block_rest     = image->height % scaled->height;

   //printf("%i   %i   %i   %i\n", image->width,  scaled->width , y_block_size_min, y_block_rest);
   //printf("%i   %i   %i   %i\n", image->height, scaled->height, x_block_size_min, x_block_rest);
   //exit(2);

   color32_t* data       = (color32_t*) image->data;
   color32_t* scale_data = (color32_t*) scaled->data;
   int sum[scaled->width][5];

   color32_t* scale_data_row;
   color32_t* data_row;

   switch(scale)
   {
      case SCALE_FIRST:
      case SCALE_LAST:
      case SCALE_CENTER:
      {
         int y_scaled, x_scaled;
         for(y_scaled = 0; y_scaled < scaled->height; ++y_scaled)
         {
            int y_block_size = y_block_size_min + (y_scaled < y_block_rest ? 1 : 0);
            
            int y_block;
            if(scale == SCALE_FIRST)
               y_block = 0;
            else if(scale == SCALE_LAST)
               y_block = y_block_size - 1;
            else if(scale == SCALE_CENTER)
               y_block = ceil((double) y_block_size / 2.0);

            scale_data_row = scale_data + y_scaled * scaled->width;
            const int shift = ( y_scaled * y_block_size_min + y_block + min(y_scaled, y_block_rest)) * image->width;
            data_row = data + shift;
            for(x_scaled = 0; x_scaled < scaled->width; ++x_scaled)
            {
               scale_data_row->r = data_row->r;
               scale_data_row->g = data_row->g;
               scale_data_row->b = data_row->b;
               scale_data_row->a = data_row->a;
               data_row += y_block_size;
               ++scale_data_row;
            }
         }
         break;
      }
      case SCALE_SSAA:
      {
         int y_block, x_block;
         int y_scaled, x_scaled;
         for(y_scaled = 0; y_scaled < scaled->height; ++y_scaled)
         {  
            for(x_scaled = 0; x_scaled < scaled->width; ++x_scaled)
            {
               sum[x_scaled][0] = 0;  // r
               sum[x_scaled][1] = 0;  // g
               sum[x_scaled][2] = 0;  // b
               sum[x_scaled][3] = 0;  // a
               sum[x_scaled][4] = 0;  // #
            }

            int y_block_size = y_block_size_min + (y_scaled < y_block_rest ? 1 : 0);
            for(y_block = 0; y_block < y_block_size; ++y_block)
            {
               const int shift = ( y_scaled * y_block_size_min + y_block + min(y_scaled, y_block_rest)) * image->width;
               data_row = data + shift;
               for(x_scaled = 0; x_scaled < scaled->width; ++x_scaled)
               {
                  int x_block_size = x_block_size_min + (x_scaled < x_block_rest ? 1 : 0);
                  for(x_block = 0; x_block < x_block_size; ++x_block)
                  {
                     sum[x_scaled][0] += data_row->r;
                     sum[x_scaled][1] += data_row->g;
                     sum[x_scaled][2] += data_row->b;
                     sum[x_scaled][3] += data_row->a;
                     sum[x_scaled][4] += 1;
                     ++data_row;
                  }
               }
            }
            
            scale_data_row = scale_data + y_scaled * scaled->width;
            for(x_scaled = 0; x_scaled < scaled->width; ++x_scaled)
            {
               double pixel_scale = 1.0 / sum[x_scaled][4];
               scale_data_row[x_scaled].r = ceil((double) sum[x_scaled][0] * pixel_scale);
               scale_data_row[x_scaled].g = ceil((double) sum[x_scaled][1] * pixel_scale);
               scale_data_row[x_scaled].b = ceil((double) sum[x_scaled][2] * pixel_scale);
               scale_data_row[x_scaled].a = ceil((double) sum[x_scaled][3] * pixel_scale);
            }
         }
         break;
      }

      default:
      {
         abort_("[scale_image] Unknown SCALE type.");
         break;
      }
   }
}

void image_t_scale_percent
   (  const image_t* const image
   ,  image_t* const       scaled
   ,  double               percent
   ,  scale_t              scale
   )
{
   int width  = round(image->width  * percent);
   int height = round(image->height * percent);
   image_t_scale(image, scaled, width, height, scale);
}

void image_t_crop
   (  const image_t* const image
   ,  image_t*       const cropped
   ,  int x_crop_begin
   ,  int y_crop_begin
   ,  int x_crop_end
   ,  int y_crop_end
   )
{
   cropped->width  = x_crop_end - x_crop_begin;
   cropped->height = y_crop_end - y_crop_begin;
   cropped->color_type = image->color_type;
   cropped->bit_depth  = image->bit_depth;
   cropped->data   = malloc(cropped->width * cropped->height * sizeof(color32_t));

   color32_t* data_row;
   color32_t* cropped_data = (color32_t*) cropped->data;
   
   int y, x;
   for(y = y_crop_begin; y < y_crop_end; ++y)
   {
      data_row = (color32_t*) image->data + y * image->width + x_crop_begin;
      for(x = x_crop_begin; x < x_crop_end; ++x)
      {
         cropped_data->r = data_row->r;
         cropped_data->g = data_row->g;
         cropped_data->b = data_row->b;
         cropped_data->a = data_row->a;
         ++cropped_data;
         ++data_row;
      }
   }
}

void image_t_apply_background
   (  image_t* const  image
   ,  int r
   ,  int g
   ,  int b
   )
{
   int i;
   int size        = image->width * image->height;
   color32_t* data = image->data;
   for(i = 0; i < size; ++i)
   {
      double alpha = (double) data->a / (double)255;
      
      data->r = data->r * alpha + r * (1.0 - alpha);
      data->g = data->g * alpha + g * (1.0 - alpha);
      data->b = data->b * alpha + b * (1.0 - alpha);

      ++data;
   }
}

void image_t_crop_background
   (  const image_t* const image
   ,        image_t* const cropped
   ,  int r
   ,  int g
   ,  int b
   )
{
   color32_t background = {r, g, b, 0};
   
   // Find crop indeces
   int y, x;
   int crop_y_begin = image->height, crop_y_end = 0;
   int crop_x_begin = image->width,  crop_x_end = 0;

   color32_t* data = (color32_t*) image->data;

   for(y = 0; y < image->height; ++y)
   {
      for(x = 0; x < image->width; ++x)
      {
         if(!color32_t_is_equal_rgb(*data, background))
         {
            crop_y_begin = min(crop_y_begin, y);
            crop_y_end   = max(crop_y_end,   y);
            crop_x_begin = min(crop_x_begin, x);
            crop_x_end   = max(crop_x_end,   x);
         }
         ++data;
      }
   }
   
   // Crop the image based on the indices found
   if (  crop_y_begin != image->height
      && crop_x_begin != image->width
      && crop_y_end   != 0
      && crop_x_end   != 0
      )
   {
      image_t_crop(image, cropped, crop_x_begin, crop_y_begin, crop_x_end + 1, crop_y_end + 1);
   }
   else
   {
      // Just copy (or make function return a status, whether it did the crop or not)
      assert(0);
   }
}

void image_t_standardize_edge
   (  image_t* const image
   ,  int r
   ,  int g
   ,  int b
   ,  int r_new
   ,  int g_new
   ,  int b_new
   )
{
   color32_t background     = {r,     g,     b,     0};
   color32_t background_new = {r_new, g_new, b_new, 0};

   const int width  = image->width;
   const int height = image->height;
   int y, x;

   color32_t* data = (color32_t*) image->data;
   color32_t* data_row;

   for(y = 0; y < height; ++y)
   {
      data_row = data + y * width;
      for(x = 0; x < width; ++x)
      {
         if(color32_t_is_equal_rgb(data_row[x], background))
         {
            data_row[x].r = background_new.r;
            data_row[x].g = background_new.g;
            data_row[x].b = background_new.b;
         }
         else
         {
            if(y != 0 && y != height - 1)
               break;
         }
      }

      for(x = width - 1; x >= 0; --x)
      {
         if(color32_t_is_equal_rgb(data_row[x], background))
         {
            data_row[x].r = background_new.r;
            data_row[x].g = background_new.g;
            data_row[x].b = background_new.b;
         }
         else
         {
            if(y != 0 && y != height - 1)
               break;
         }
      }
   }
}


/**
 * Utilities for drawing image to terminal
 **/
#define BYTE_TO_TEXT(buf_, byte_) do {\
	*(buf_)++ = '0' + (byte_) / 100u;\
	*(buf_)++ = '0' + (byte_) / 10u % 10u;\
	*(buf_)++ = '0' + (byte_) % 10u;\
} while (0)

/**
 * Draw image using a work buffer. 
 **/
void draw_image
   (  const image_t* const image
   ,  char* buffer
   )
{
	/* fill output buffer */
   int resx = image->width;
   int resy = image->height;

	uint32_t color_fg     = 0xFFFFFF00;
	uint32_t color_bg     = 0xFFFFFF00;
	color32_t *pixel_bg = (color32_t *) image->data;
	color32_t *pixel_fg = pixel_bg + resx;
	char *buf = buffer;
   
   // Set buffer to write from 1,1
   //*buf++ = '\033'; *buf++ = '[';
   //*buf++ = '1';    *buf++ = ';';
   //*buf++ = '1';    *buf++ = 'H';

	for (int row = 0; row < resy; row+=2) {
		for (int col = 0; col < resx; col++) {
         /* Handle foreground */
			if ((color_fg ^ *(uint32_t*)pixel_fg) & 0x00FFFFFF) {
				*buf++ = '\033'; *buf++ = '[';
				*buf++ = '3'; *buf++ = '8'; /* Set foreground color */
				*buf++ = ';'; *buf++ = '2';
				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_fg->r);
				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_fg->g);
				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_fg->b);
				*buf++ = 'm';
				color_fg = *(uint32_t*)pixel_fg;
			}
         /* Handle background */
			if ((color_bg ^ *(uint32_t*)pixel_bg) & 0x00FFFFFF) {
				*buf++ = '\033'; *buf++ = '[';
				*buf++ = '4'; *buf++ = '8'; /* Set background color */
				*buf++ = ';'; *buf++ = '2';
				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_bg->r);
				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_bg->g);
				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_bg->b);
				*buf++ = 'm';
				color_bg = *(uint32_t*)pixel_bg;
			}
         /* Write U+2584 (solid block in lower half of cell) */
         char two_pixel_pr_char[] = { (char)0xe2, (char)0x96, (char)0x84 };
			*buf++ = two_pixel_pr_char[0]; 
			*buf++ = two_pixel_pr_char[1]; 
			*buf++ = two_pixel_pr_char[2]; 
         /* Increment pointers */
			pixel_fg++;
			pixel_bg++;
		}

		*buf++ = '\n';

	   pixel_fg += resx;
	   pixel_bg += resx;
	}
   /* Reset char (not really needed, but also doesn't cost that much) and NULL terminate */
	*buf++ = '\033'; *buf++ = '[';
	*buf++ = '0';
	*buf++ = 'm';
	*buf = '\0'; /* NULL termination */

	/* flush output buffer */
	//CALL_STDOUT(fputs(buffer, stdout), "DG_DrawFrame: fputs error %d");
	fputs(buffer, stdout);

	/* clear output buffer */
	//memset(buffer, '\0', buf - buffer + 1u);
}

int main(int argc, char* argv[])
{  
   char* file_name = argv[1];
   image_t image;
   image_t scaled;
   image_t cropped;
   char buffer[1024 * 1024 * 4];
   
   if(read_png(file_name, &image) != SUCCESS)
   {
      printf("Error\n");
   }

   color64_t term_background64 = { 0x3030, 0x0a0a, 0x2424, 0};
   color32_t term_background32;
   convert_color64_t_to_color32_t(&term_background64, &term_background32);
   
   image_t_print(&image);
   image_t_scale_percent(&image, &scaled, 0.1, SCALE_SSAA);
   image_t_apply_background(&scaled, 255, 255, 255);
   image_t_crop_background(&scaled, &cropped, 255, 255, 255);
   image_t_standardize_edge(&cropped, 255, 255, 255, term_background32.r, term_background32.g, term_background32.b);
   //image_t_background(&scaled, 0x3030, 0x0a0a, 0x2424);
   //image_t_scale(&image, &scaled, 200, 100);
   //
   //image_t_crop(&scaled, &cropped, 11, 10, scaled.width - 10, scaled.height - 10);

   //draw_image(&scaled, buffer);
   draw_image(&cropped, buffer);
   
   image_t_destroy(&scaled);
   image_t_destroy(&image);

   return 0;
}
