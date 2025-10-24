//g++ -std=c++98 test2.cpp -o test2.cgi
//chmod +x test.cgi

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string body;
    std::getline(std::cin, body); // lit le corps POST

    std::cout << "Content-Type: text/plain\n\n";
    std::cout << "Corps POST reçu : " << body << "\n";

    std::cout << "Arguments :\n";
    for (int i = 0; i < argc; ++i)
        std::cout << "  argv[" << i << "] = " << argv[i] << "\n";

    return 0;
}