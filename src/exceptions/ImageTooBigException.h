#ifndef CAESIUM_IMAGE_COMPRESSOR_IMAGETOOBIGEXCEPTION_H
#define CAESIUM_IMAGE_COMPRESSOR_IMAGETOOBIGEXCEPTION_H

#include <exception>
#include <string>

class ImageTooBigException : public std::exception {
private:
    std::string message;

public:
    ImageTooBigException();
    const char* what() const noexcept override;
};

#endif //CAESIUM_IMAGE_COMPRESSOR_IMAGETOOBIGEXCEPTION_H