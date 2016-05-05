#ifndef CPNG_H
#define CPNG_H

#include "src/cimage.h"

class CPNG : public CImage
{
public:
    using CImage::CImage;

    int getIterations() const;
    void setIterations(int value);

    int getIterationsLarge() const;
    void setIterationsLarge(int value);

    int getBlockSplitStrategy() const;
    void setBlockSplitStrategy(int value);

    int getLossy8Bit() const;
    void setLossy8Bit(int value);

    int getTransparent() const;
    void setTransparent(int value);

    int getAutoFilterStrategy() const;
    void setAutoFilterStrategy(int value);

private:
    int iterations = 15;
    int iterationsLarge = 5;
    int blockSplitStrategy = 4;
    int lossy8Bit = 1;
    int transparent = 1;
    int autoFilterStrategy = 1;
};

#endif // CPNG_H
