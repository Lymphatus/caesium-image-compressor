#ifndef CAESIUM_IMAGE_COMPRESSOR_POSTCOMPRESSIONACTIONS_H
#define CAESIUM_IMAGE_COMPRESSOR_POSTCOMPRESSIONACTIONS_H

#include "utils/Utils.h"
class PostCompressionActions {

public:
    static void runAction(PostCompressionAction action);

private:
    static void closeApplication();
    static void shutdownMachine();
    static void putMachineToSleep();
};

#endif // CAESIUM_IMAGE_COMPRESSOR_POSTCOMPRESSIONACTIONS_H
