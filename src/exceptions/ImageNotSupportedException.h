#ifndef CAESIUM_IMAGE_COMPRESSOR_IMAGENOTSUPPORTEDEXCEPTION_H
#define CAESIUM_IMAGE_COMPRESSOR_IMAGENOTSUPPORTEDEXCEPTION_H

#include <exception>
#include <string>

class ImageNotSupportedException : public std::exception {
private:
    std::string message;

public:
    ImageNotSupportedException();
    [[nodiscard]] const char* what() const noexcept override;
};

#endif // CAESIUM_IMAGE_COMPRESSOR_IMAGENOTSUPPORTEDEXCEPTION_H