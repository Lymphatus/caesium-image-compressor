#include "ImageTooBigException.h"

ImageTooBigException::ImageTooBigException()
{
    message = "Image exceeds the maximum file size allowed.";
}

const char* ImageTooBigException::what() const noexcept
{
    return message.c_str();
}