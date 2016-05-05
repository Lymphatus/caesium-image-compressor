#include "cpng.h"

int CPNG::getIterations() const
{
    return iterations;
}

void CPNG::setIterations(int value)
{
    iterations = value;
}

int CPNG::getIterationsLarge() const
{
    return iterationsLarge;
}

void CPNG::setIterationsLarge(int value)
{
    iterationsLarge = value;
}

int CPNG::getBlockSplitStrategy() const
{
    return blockSplitStrategy;
}

void CPNG::setBlockSplitStrategy(int value)
{
    blockSplitStrategy = value;
}

int CPNG::getLossy8Bit() const
{
    return lossy8Bit;
}

void CPNG::setLossy8Bit(int value)
{
    lossy8Bit = value;
}

int CPNG::getTransparent() const
{
    return transparent;
}

void CPNG::setTransparent(int value)
{
    transparent = value;
}

int CPNG::getAutoFilterStrategy() const
{
    return autoFilterStrategy;
}

void CPNG::setAutoFilterStrategy(int value)
{
    autoFilterStrategy = value;
}
