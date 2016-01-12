#ifndef CCLT_JPEG
#define CCLT_JPEG

#include <jpeglib.h>

#include "utils.h"


int cclt_jpeg_optimize(char* input_file, char* output_file, int exif_flag, char* exif_src);
struct jpeg_decompress_struct cclt_get_markers(char* input);
void cclt_jpeg_compress(char* output_file, unsigned char* image_buffer, cclt_compress_parameters* pars);
unsigned char* cclt_jpeg_decompress(char* fileName, cclt_compress_parameters* pars);
void jcopy_markers_execute (j_decompress_ptr srcinfo, j_compress_ptr dstinfo);


#endif
