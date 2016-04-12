/**
 *
 * This file is part of Caesium.
 *
 * Caesium - Caesium is an image compression software aimed at helping photographers,
 * bloggers, webmasters, businesses or casual users at storing, sending and sharing digital pictures.
 *
 * Copyright (C) 2016 - Matteo Paonessa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef CCLT_JPEG
#define CCLT_JPEG

#include <jpeglib.h>

#include "utils.h"

int cclt_jpeg_optimize(char* input_file, char* output_file, int progressive_flag, int exif_flag, char* exif_src);
struct jpeg_decompress_struct cclt_get_markers(char* input);
void cclt_jpeg_compress(char* output_file, unsigned char* image_buffer, cclt_compress_parameters* pars);
unsigned char* cclt_jpeg_decompress(char* fileName, cclt_compress_parameters* pars);
void jcopy_markers_execute (j_decompress_ptr srcinfo, j_compress_ptr dstinfo);


#endif
