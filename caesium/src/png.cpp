#include <string.h>

#include "lodepng.h"
#include <zopflipng/zopflipng_lib.h>
#include "png.h"

#include <QDebug>

/* LEGACY! Here for reference
 * void cclt_png_optimize(char* input, char* output, png_params* image) {
    //TODO Error handling
    CZopfliPNGOptions png_options;

    CZopfliPNGSetDefaults(&png_options);

    unsigned char* orig_buffer;
    size_t orig_buffer_size;

    unsigned char* resultpng;
    size_t resultpng_size;

    png_options.num_iterations = image->getIterations();
    png_options.num_iterations_large = image->getIterationsLarge();
    png_options.block_split_strategy = image->getBlockSplitStrategy();

    png_options.lossy_8bit = image->getLossy8Bit();
    png_options.lossy_transparent = image->getTransparent();

    png_options.auto_filter_strategy = image->getAutoFilterStrategy();

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
*/

void cclt_png_optimize(QString input, QString output, png_params *image) {
    std::string file = input.toStdString();
    std::string out_filename = output.toStdString();
    std::vector<unsigned char> origpng;
    std::vector<unsigned char> resultpng;

    ZopfliPNGOptions png_options;

    png_options.auto_filter_strategy = image->getAutoFilterStrategy();
    png_options.block_split_strategy = image->getBlockSplitStrategy();
    png_options.num_iterations = image->getIterations();
    png_options.num_iterations_large = image->getIterationsLarge();
    png_options.lossy_8bit = image->getLossy8Bit();
    png_options.lossy_transparent = image->getTransparent();

    lodepng::load_file(origpng, file);
    ZopfliPNGOptimize(origpng, png_options, false, &resultpng);
    lodepng::save_file(resultpng, out_filename);

}
