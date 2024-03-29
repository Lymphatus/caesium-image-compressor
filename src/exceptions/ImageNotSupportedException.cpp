#include "ImageNotSupportedException.h"

ImageNotSupportedException::ImageNotSupportedException()
{
    message = "Image format is not supported.";
}

const char* ImageNotSupportedException::what() const noexcept
{
    return message.c_str();
}