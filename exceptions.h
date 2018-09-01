#pragma once

class JpegLoadError : public std::exception {
private:
    std::string message;

public:
    JpegLoadError(std::string message) : message(message) {

    }

    const char* what() const throw()
    {
        return message.c_str();
    }
};
