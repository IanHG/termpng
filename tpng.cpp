#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>

#include <png.h>

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

typedef enum 
{  ERROR
,  SUCCESS
,  NOT_PNG 
} status_t;

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
    uint32_t b:8;
    uint32_t g:8;
    uint32_t r:8;
    uint32_t a:8;
} color32_t;

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
   fread(header, 1, 8, fp);
   if (png_sig_cmp(header, 0, 8))
      abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


   /* initialize stuff */
   png_structp png_ptr;
   png_infop info_ptr;
   int number_of_passes;

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

///* PNG_BYTES TO RGB VALUES */
//void process_file(void)
//{
//   if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB)
//      abort_("[process_file] input file is PNG_COLOR_TYPE_RGB but must be PNG_COLOR_TYPE_RGBA "
//            "(lacks the alpha channel)");
//
//   if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA)
//      abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) (is %d)",
//            PNG_COLOR_TYPE_RGBA, png_get_color_type(png_ptr, info_ptr));
//
//   for (y=0; y<height; y++) {
//      png_byte* row = row_pointers[y];
//      for (x=0; x<width; x++) {
//         png_byte* ptr = &(row[x*4]);
//         printf("Pixel at position [ %d - %d ] has RGBA values: %d - %d - %d - %d\n",
//               x, y, ptr[0], ptr[1], ptr[2], ptr[3]);
//
//         /* set red value to 0 and green value to the blue one */
//         ptr[0] = 0;
//         ptr[1] = ptr[2];
//      }
//   }
//}


void image_t_scale
   (  const image_t* const image
   ,  image_t* const       scaled
   ,  int                  scaled_width
   ,  int                  scaled_height
   )
{
   scaled->width  = scaled_width;
   scaled->height = scaled_height;
   scaled->color_type = image->color_type;
   scaled->bit_depth  = image->bit_depth;
   scaled->data   = malloc(scaled->width * scaled->height * sizeof(color32_t));

   printf("%p\n", scaled->data);

   int i;
   int scaled_size = scaled->width * scaled->height;  
   color32_t* scaled_data = (color32_t*) scaled->data;
   for(i = 0; i < scaled_size; ++i)
   {
      scaled_data[i] = color32_t{0, 0, 0, 0};
   }

   int y_block_size = ceil((double) image->width  / (double) scaled->width);
   int x_block_size = ceil((double) image->height / (double) scaled->height);
   double pixel_scale = (double) 1.0 / ((double) (y_block_size * x_block_size));
   printf("y_block_size = %d\n", y_block_size);
   printf("x_block_size = %d\n", x_block_size);
   printf("scale = %f\n", pixel_scale);

   color32_t* data       = (color32_t*) image->data;
   color32_t* scale_data = (color32_t*) scaled->data;
   printf("%p\n", scale_data);

   color32_t* scale_data_row;
   color32_t* data_row;

   int y_block, x_block;
   int y_scaled, x_scaled;
   for(y_scaled = 0; y_scaled < scaled->height; ++y_scaled)
   {  
      int y_block_min = min(y_block_size, image->height - y_scaled * y_block_size);
      for(y_block = 0; y_block < y_block_min; ++y_block)
      {
         scale_data_row = scale_data + y_scaled * scaled->width;
         data_row       = data       + ( y_scaled * y_block_size + y_block ) * image->width;
         for(x_scaled = 0; x_scaled < scaled->width; ++x_scaled)
         {
            int x_block_min = min(x_block_size, image->width - x_scaled * x_block_size);
            int r = 0;
            int g = 0;
            int b = 0;
            int a = 0;
            for(x_block = 0; x_block < x_block_min; ++x_block)
            {
               r += data_row->r;
               g += data_row->g;
               b += data_row->b;
               a += data_row->a;
               ++data_row;
            }

            //printf("r = %i\n", scale_data_row->r);
            //printf("r = %i\n", scale_data_row->g);
            //printf("r = %i\n", scale_data_row->b);
            //printf("r = %i\n", scale_data_row->a);
               
            //scale_data_row->r += ceil((double) data_row->r * pixel_scale);
            //scale_data_row->g += ceil((double) data_row->g * pixel_scale);
            //scale_data_row->b += ceil((double) data_row->b * pixel_scale);
            //scale_data_row->a += ceil((double) data_row->a * pixel_scale);
            scale_data_row->r = ceil((double) r * pixel_scale * 10);
            scale_data_row->g = ceil((double) g * pixel_scale * 10);
            scale_data_row->b = ceil((double) b * pixel_scale * 10);
            scale_data_row->a = ceil((double) a * pixel_scale * 10);

            //apply_gamma_correction(scale_data_row, 1.0, 2.2);

            //printf("r = %i\n", scale_data_row->r);
            //printf("r = %i\n", scale_data_row->g);
            //printf("r = %i\n", scale_data_row->b);
            //printf("r = %i\n", scale_data_row->a);

            //exit(2);

            ++scale_data_row;
         }
      }
   }
}

void image_t_scale_percent
   (  const image_t* const image
   ,  image_t* const       scaled
   ,  double               percent
   )
{
   int width  = ceil(image->width  * percent);
   int height = ceil(image->height * percent);
   height = (height % 2 == 0) ? height : height + 1;
   image_t_scale(image, scaled, width, height);
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
void draw_image(const image_t* const image, char* buffer)
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
   *buf++ = '\033'; *buf++ = '[';
   *buf++ = '1';    *buf++ = ';';
   *buf++ = '1';    *buf++ = 'H';

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
         char two_pixel_pr_char[] = { char(0xe2), char(0x96), char(0x84) };
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
	//*buf++ = '\033'; *buf++ = '[';
	//*buf++ = '0';
	//*buf++ = 'm';
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
   char buffer[1024 * 1024 * 4];
   
   if(read_png(file_name, &image) != SUCCESS)
   {
      printf("Error\n");
   }
   
   image_t_print(&image);
   image_t_scale_percent(&image, &scaled, 0.1);

   draw_image(&scaled, buffer);

   image_t_destroy(&image);

   return 0;
}