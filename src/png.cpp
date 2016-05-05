#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lodepng.h"
#include <zopflipng/zopflipng_lib.h>
#include "png.h"

void cclt_png_optimize(char* input, char* output, cclt_png_parameters* pars) {
    //TODO Error handling
    CZopfliPNGOptions png_options;

    CZopfliPNGSetDefaults(&png_options);

    unsigned char* orig_buffer;
    size_t orig_buffer_size;

    unsigned char* resultpng;
    size_t resultpng_size;

    png_options.num_iterations = pars->iterations;
    png_options.num_iterations_large = pars->iterations_large;
    png_options.block_split_strategy = pars->block_split_strategy;

    png_options.lossy_8bit = pars->lossy_8;
    png_options.lossy_transparent = pars->transparent;

    png_options.auto_filter_strategy = pars->auto_filter_strategy;

    if (lodepng_load_file(&orig_buffer, &orig_buffer_size, input) != 0) {
        fprintf(stderr, "[ERROR] Error while loading PNG.\n");
        exit(-16);
    }

    if (CZopfliPNGOptimize(orig_buffer,
                            orig_buffer_size,
                            &png_options,
                            0,
                            &resultpng,
                            &resultpng_size) != 0) {
        fprintf(stderr, "[ERROR] Error while optimizing PNG.\n");
        exit(-17);
    }

    if (lodepng_save_file(resultpng, resultpng_size, output) != 0) {
        fprintf(stderr, "[ERROR] Error while writing PNG.\n");
        exit(-18);
    }

    free(orig_buffer);
    free(resultpng);
}
