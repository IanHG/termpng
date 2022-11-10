#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "transform.h"


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
//



int main(int argc, char* argv[])
{  
   char* file_name = argv[argc - 1];
   image_t image;
   //image_t scaled;
   //image_t cropped;
   //char buffer[1024 * 1024 * 4];
   
   if(image_t_read_png(file_name, &image) != SUCCESS)
   {
      printf("Error\n");
   }

   color64_t term_background64 = { 0x3030, 0x0a0a, 0x2424, 0};
   color32_t term_background32;
   convert_color64_t_to_color32_t(&term_background64, &term_background32);

   transform_t* transform = (transform_t*) malloc(sizeof(transform_t));
   transform_t_init(transform);
   transform_parse_args(argc, argv, transform);
   
   //transform_t* next = transform_t_make_next(transform);
   //next->type    = SCALE;
   //transform_scale_t* transform_scale = (transform_scale_t*) malloc(sizeof(transform_scale_t));
   //transform_scale->percent = 0.1;
   //transform_scale->scale   = SCALE_SSAA;
   //next->options = transform_scale;
   //next = transform_t_make_next(next);
   //next->type = DRAW;
   transform_apply_pipeline(&image, transform);

   transform_t_destroy(transform);
   image_t_destroy(&image);
   
   //image_t_print(&image);
   //image_t_scale_percent(&image, &scaled, 0.1, SCALE_SSAA);
   //image_t_apply_background(&scaled, 255, 255, 255);
   ////image_t_crop_background(&scaled, &cropped, 255, 255, 255);
   //image_t_standardize_edge(&scaled, 255, 255, 255, term_background32.r, term_background32.g, term_background32.b);
   ////image_t_standardize_edge(&cropped, 255, 255, 255, term_background32.r, term_background32.g, term_background32.b);
   ////image_t_background(&scaled, 0x3030, 0x0a0a, 0x2424);
   ////image_t_scale(&image, &scaled, 200, 100);
   ////
   ////image_t_crop(&scaled, &cropped, 11, 10, scaled.width - 10, scaled.height - 10);
   ////
   ////puts("\e[?1049h"); // Go to alternate screen buffer (used for fullscreen applications)
   //draw_image(&scaled, buffer);
   ////puts("\e[?1049l"); // Go back to standard screen buffer
   ////draw_image(&cropped, buffer);
   //
   //image_t_destroy(&scaled);
   //image_t_destroy(&image);

   return 0;
}
