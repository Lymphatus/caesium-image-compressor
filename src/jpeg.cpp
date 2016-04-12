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

#include <setjmp.h>
#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <turbojpeg.h>
#include <math.h>

#include "src/jpeg.h"

//TODO Error handling

struct jpeg_decompress_struct cclt_get_markers(char* input) {
	FILE* fp;
	struct jpeg_decompress_struct einfo;
	struct jpeg_error_mgr eerr;
	einfo.err = jpeg_std_error(&eerr);

	jpeg_create_decompress(&einfo);

  	//Open the input file
	fp = fopen(input, "r");

	//Check for errors
	//TODO Use UNIX error messages
	if (fp == NULL) {
		printf("INPUT: Failed to open exif file \"%s\"\n", input);
		exit(-13);
	}

	//Create the IO instance for the input file
	jpeg_stdio_src(&einfo, fp);

	//Save EXIF info
	for (int m = 0; m < 16; m++) {
		jpeg_save_markers(&einfo, JPEG_APP0 + m, 0xFFFF);
	}

	jpeg_read_header(&einfo, TRUE);

	fclose(fp);

	return einfo;
}

int cclt_jpeg_optimize(char* input_file, char* output_file, int progressive_flag, int exif_flag, char* exif_src) {
	//TODO Bug on normal compress: the input file is a bogus long string
	// Happened with a (bugged) server connection
	//File pointer for both input and output
	FILE* fp;

	//Those will hold the input/output structs
	struct jpeg_decompress_struct srcinfo;
	struct jpeg_compress_struct dstinfo;

	//Error handling
	struct jpeg_error_mgr jsrcerr, jdsterr;

	//Input/Output array coefficents
	jvirt_barray_ptr* src_coef_arrays;
	jvirt_barray_ptr* dst_coef_arrays;

	//Set errors and create the compress/decompress istances
	srcinfo.err = jpeg_std_error(&jsrcerr);
	jpeg_create_decompress(&srcinfo);
	dstinfo.err = jpeg_std_error(&jdsterr);
	jpeg_create_compress(&dstinfo);


	//Open the input file
	fp = fopen(input_file, "r");

	//Check for errors
	//TODO Use UNIX error messages
	if (fp == NULL) {
		printf("INPUT: Failed to open file \"%s\"\n", input_file);
		return -1;
	}

	//Create the IO istance for the input file
	jpeg_stdio_src(&srcinfo, fp);

	//Save EXIF info
	if (exif_flag == 1) {
		for (int m = 0; m < 16; m++) {
			jpeg_save_markers(&srcinfo, JPEG_APP0 + m, 0xFFFF);
		}
	}

	//Read the input headers
	(void) jpeg_read_header(&srcinfo, TRUE);
	

	//Read input coefficents
	src_coef_arrays = jpeg_read_coefficients(&srcinfo);
	//jcopy_markers_setup(&srcinfo, copyoption);

	//Copy parameters
	jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

	//Set coefficents array to be the same
	dst_coef_arrays = src_coef_arrays;

	//We don't need the input file anymore
	fclose(fp);

	//Open the output one instead
	fp = fopen(output_file, "w+");
	//Check for errors
	//TODO Use UNIX error messages
	if (fp == NULL) {
		printf("OUTPUT: Failed to open file \"%s\"\n", output_file);
		return -2;
	}

	//CRITICAL - This is the optimization step
	dstinfo.optimize_coding = TRUE;
	//Progressive
	jpeg_simple_progression(&dstinfo);

	//Set the output file parameters
	jpeg_stdio_dest(&dstinfo, fp);

	//Actually write the coefficents
	jpeg_write_coefficients(&dstinfo, dst_coef_arrays);

	//Write EXIF
	if (exif_flag == 1) {
		if (strcmp(input_file, exif_src) == 0) {
			jcopy_markers_execute(&srcinfo, &dstinfo);
		} else {
			//For standard compression EXIF data
			struct jpeg_decompress_struct einfo = cclt_get_markers(exif_src);
			jcopy_markers_execute(&einfo, &dstinfo);
			jpeg_destroy_decompress(&einfo);
		}
	}

	//Finish and free
	jpeg_finish_compress(&dstinfo);
	jpeg_destroy_compress(&dstinfo);
	(void) jpeg_finish_decompress(&srcinfo);
	jpeg_destroy_decompress(&srcinfo);

	//Close the output file
	fclose(fp);

	return 0;
}

void cclt_jpeg_compress(char* output_file, unsigned char* image_buffer, cclt_compress_parameters* pars) {
	FILE* fp;
	tjhandle tjCompressHandle;
	unsigned char* output_buffer;
	unsigned long output_size = 0;

	fp = fopen(output_file, "wb");

	//Check for errors
	//TODO Use UNIX error messages
	if (fp == NULL) {
	   printf("OUTPUT: Failed to open output \"%s\"\n", output_file);
	   return;
   }

   output_buffer = NULL;
   tjCompressHandle = tjInitCompress();

   //TODO Error checks
   tjCompress2(tjCompressHandle,
	   image_buffer,
	   pars->width,
	   0,
	   pars->height,
	   pars->color_space,
	   &output_buffer,
	   &output_size,
	   pars->subsample,
	   pars->quality,
	   pars->dct_method);

   fwrite(output_buffer, output_size, 1, fp);

   fclose(fp);
   tjDestroy(tjCompressHandle);
   tjFree(output_buffer);

}

unsigned char* cclt_jpeg_decompress(char* fileName, cclt_compress_parameters* pars) {

	//TODO I/O Error handling

	FILE *file = NULL;
	int res = 0;
	long int sourceJpegBufferSize = 0;
	unsigned char* sourceJpegBuffer = NULL;
	tjhandle tjDecompressHandle;
	int fileWidth = 0, fileHeight = 0, jpegSubsamp = 0, colorSpace = 0;

	//TODO No error checks here
	file = fopen(fileName, "rb");
	res = fseek(file, 0, SEEK_END);
	sourceJpegBufferSize = ftell(file);
	sourceJpegBuffer = tjAlloc(sourceJpegBufferSize);

	res = fseek(file, 0, SEEK_SET);
	res = fread(sourceJpegBuffer, (long)sourceJpegBufferSize, 1, file);
	tjDecompressHandle = tjInitDecompress();
	res = tjDecompressHeader3(tjDecompressHandle, sourceJpegBuffer, sourceJpegBufferSize, &fileWidth, &fileHeight, &jpegSubsamp, &colorSpace);

	pars->width = fileWidth;
	pars->height = fileHeight;

	pars->subsample = jpegSubsamp;
	pars->color_space = colorSpace;

	unsigned char* temp = tjAlloc(pars->width * pars->height * tjPixelSize[pars->color_space]);

	res = tjDecompress2(tjDecompressHandle,
		sourceJpegBuffer,
		sourceJpegBufferSize,
		temp,
		pars->width,
		0,
		pars->height,
		pars->color_space,
		pars->dct_method);

	//fwrite(temp, pars->width * pars->height * tjPixelSize[pars->color_space], 1, fopen("/Users/lymphatus/Desktop/tmp/compresse/ccc", "w"));

	tjDestroy(tjDecompressHandle);

	return temp;
}

void jcopy_markers_execute (j_decompress_ptr srcinfo, j_compress_ptr dstinfo) {
  jpeg_saved_marker_ptr marker;

  for (marker = srcinfo->marker_list; marker != NULL; marker = marker->next) {
    if (dstinfo->write_JFIF_header &&
        marker->marker == JPEG_APP0 &&
        marker->data_length >= 5 &&
        GETJOCTET(marker->data[0]) == 0x4A &&
        GETJOCTET(marker->data[1]) == 0x46 &&
        GETJOCTET(marker->data[2]) == 0x49 &&
        GETJOCTET(marker->data[3]) == 0x46 &&
        GETJOCTET(marker->data[4]) == 0)
      continue;
    if (dstinfo->write_Adobe_marker &&
        marker->marker == JPEG_APP0+14 &&
        marker->data_length >= 5 &&
        GETJOCTET(marker->data[0]) == 0x41 &&
        GETJOCTET(marker->data[1]) == 0x64 &&
        GETJOCTET(marker->data[2]) == 0x6F &&
        GETJOCTET(marker->data[3]) == 0x62 &&
        GETJOCTET(marker->data[4]) == 0x65)
      continue;
    jpeg_write_marker(dstinfo, marker->marker,
                      marker->data, marker->data_length);
  }
}
