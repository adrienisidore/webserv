#include "webserv.hpp"

AHTTPtransfer::AHTTPtransfer() : _code(0) {}

AHTTPtransfer::AHTTPtransfer(int & code) : _code(code) {}

//response.copyFrom(request);
//Peut etre recoder dans les heritants si besoin
void	AHTTPtransfer::copyFrom(const AHTTPtransfer& other) {
        _code = other._code;
        _method = other._method;
        _target = other._target;
}

AHTTPtransfer::~AHTTPtransfer() {}