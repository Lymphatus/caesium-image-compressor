#ifndef CPNG_H
#define CPNG_H

#include "src/cimage.h"

class CPNG : public CImage
{
public:
    using CImage::CImage;

private:
    int iterations = 15;
    int iterations_large = 5;
    int block_split_strategy = 4;
    int lossy_8 = 1;
    int transparent = 1;
    int auto_filter_strategy = 1;
};

#endif // CPNG_H
