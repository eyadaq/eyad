#include "Response.hpp"
#include <fstream>
#include <sstream>

Response::Response(const Request& req) {
    // 1. Basic Method Validation
    if (req.get_method() != "GET") {
        _build_error_page("405", "Method Not Allowed");
    } 
    else {
        // 2. Map URL to local filesystem
        // In a real 42 project, the "www" folder would come from a config file
        std::string root = "./www";
        std::string path = (req.get_path() == "/") ? "/index.html" : req.get_path();
        std::string full_path = root + path;

        // 3. Try to open the file
        std::ifstream file(full_path.c_str(), std::ios::binary);
        if (file.is_open()) {
            _status_line = "HTTP/1.1 200 OK\r\n";
            std::stringstream ss;
            ss << file.rdbuf();
            _body = ss.str();
            file.close();
        } else {
            _build_error_page("404", "Not Found");
        }
    }

    // 4. Assemble the final response
    std::stringstream res;
    res << _status_line;
    res << "Content-Type: text/html\r\n";
    res << "Content-Length: " << _body.size() << "\r\n";
    res << "Connection: close\r\n";
    res << "\r\n";
    res << _body;

    _full_response = res.str();
}

void Response::_build_error_page(std::string code, std::string message) {
    _status_line = "HTTP/1.1 " + code + " " + message + "\r\n";
    _body = "<html><body><h1>" + code + " " + message + "</h1></body></html>";
}