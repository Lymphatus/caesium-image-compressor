#ifndef LIB_CAESIUM_H
#define LIB_CAESIUM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum SupportedFileTypes {
    Jpeg = 0,
    Png = 1,
    Gif = 2,
    WebP = 3,
    Tiff = 4,
    Unkn = 5
} SupportedFileTypes;

typedef struct CCSResult {
    bool success;
    uint32_t code;
    char* error_message;
} CCSResult;

typedef struct CCSParameters {
    bool keep_metadata;
    uint32_t jpeg_quality;
    uint32_t jpeg_chroma_subsampling; // support 444, 422, 420, 411
    bool jpeg_progressive;
    uint32_t png_quality;
    uint32_t png_optimization_level;
    bool png_force_zopfli;
    uint32_t gif_quality;
    uint32_t webp_quality;
    uint32_t tiff_compression; // support 1:Lzw 2:Deflate 3:Packbits Other Int:Uncompressed
    uint32_t tiff_deflate_level; // support 1:Fast 6:Balanced Other Int:Best
    bool optimize;
    uint32_t width;
    uint32_t height;
} CCSParameters;

CCSResult c_compress(const char* input_path, const char* output_path, CCSParameters* params);

CCSResult c_compress_to_size(const char* input_path, const char* output_path, CCSParameters* params, uint64_t max_output_size, bool return_smallest);

CCSResult c_convert(const char* input_path, const char* output_path, SupportedFileTypes format, CCSParameters* params);

#ifdef __cplusplus
}
#endif

#endif