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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lodepng.h"
#include <zopflipng/zopflipng_lib.h>
#include "png.h"

void cclt_png_optimize(char* input, char* output) {
	//TODO Error handling
	CZopfliPNGOptions png_options;

	CZopfliPNGSetDefaults(&png_options);

	unsigned char* orig_buffer;
	size_t orig_buffer_size;

	unsigned char* resultpng;
	size_t resultpng_size;

	png_options.num_iterations = 15;
	png_options.num_iterations_large = 5;
	png_options.block_split_strategy = 4;

	png_options.lossy_8bit = 1;
	png_options.lossy_transparent = 1;

	png_options.auto_filter_strategy = 1;

	if (lodepng_load_file(&orig_buffer, &orig_buffer_size, input) != 0) {
		fprintf(stderr, "Error while loading PNG. Aborting.\n");
		exit(-16);
	}

	if (CZopfliPNGOptimize(orig_buffer,
							orig_buffer_size,
							&png_options,
							0,
							&resultpng,
							&resultpng_size) != 0) {
		fprintf(stderr, "Error while optimizing PNG. Aborting.\n");
		exit(-17);
	}

	if (lodepng_save_file(resultpng, resultpng_size, output) != 0) {
		fprintf(stderr, "Error while writing PNG. Aborting.\n");
		exit(-18);
	}

	free(orig_buffer);
	free(resultpng);
}
