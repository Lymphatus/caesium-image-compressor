#pragma once
#include <cstddef>
#include <cstdint>

enum class SupportedFileTypes {
    Jpeg,
    Png,
    Gif,
    WebP,
    Tiff,
    Unkn,
};

struct CCSResult {
    bool success;
    uint32_t code;
    const char *error_message;
};

struct CCSParameters {
    bool keep_metadata;
    uint32_t jpeg_quality;
    uint32_t jpeg_chroma_subsampling;
    bool jpeg_progressive;
    uint32_t png_quality;
    uint32_t png_optimization_level;
    bool png_force_zopfli;
    uint32_t gif_quality;
    uint32_t webp_quality;
    uint32_t tiff_compression;
    uint32_t tiff_deflate_level;
    bool optimize;
    uint32_t width;
    uint32_t height;
};


extern "C" {
CCSResult c_compress(const char *input_path, const char *output_path, CCSParameters params);

CCSResult c_compress_to_size(const char *input_path,
                             const char *output_path,
                             CCSParameters params,
                             size_t max_output_size,
                             bool return_smallest);

CCSResult c_convert(const char *input_path,
                    const char *output_path,
                    SupportedFileTypes format,
                    CCSParameters params);
} // extern "C"
