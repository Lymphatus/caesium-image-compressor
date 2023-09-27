#ifndef CAESIUM_IMAGE_COMPRESSOR_POSTCOMPRESSIONACTIONS_H
#define CAESIUM_IMAGE_COMPRESSOR_POSTCOMPRESSIONACTIONS_H

#include "utils/Utils.h"
class PostCompressionActions {

public:
    static void runAction(PostCompressionAction action);
    static void runAction(PostCompressionAction action, const QString& folder);

private:
    static void closeApplication();
    static void shutdownMachine();
    static void putMachineToSleep();
    static void openOutputFolder(const QString& folder);
};

#endif // CAESIUM_IMAGE_COMPRESSOR_POSTCOMPRESSIONACTIONS_H
