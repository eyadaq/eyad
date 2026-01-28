#include "Response.hpp"
#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

Response::Response(const Request& req, const ServerConfig &config, const RouteConfig &route)
    : _content_type("text/html"), _config(config), _route(route) {
    std::string root = _route.root.empty() ? _config.root : _route.root;
    std::string index = _route.index.empty() ? _config.index : _route.index;
    bool autoindex = _route.autoindex_set ? _route.autoindex : _config.autoindex;

    // 1. Basic Method Validation
    if (req.get_method() != "GET" && req.get_method() != "DELETE") {
        _build_error_page(405, "Method Not Allowed");
    } else if (_route.redirect_code != 0 && !_route.redirect_target.empty()) {
        std::stringstream status;
        status << "HTTP/1.1 " << _route.redirect_code << " Redirect\r\n";
        _status_line = status.str();
        _headers = "Location: " + _route.redirect_target + "\r\n";
        _body.clear();
        _content_type = "text/plain";
    } else if (req.get_method() == "DELETE") {
        std::string full_path = root + req.get_path();
        if (remove(full_path.c_str()) == 0) {
            _status_line = "HTTP/1.1 204 No Content\r\n";
            _body.clear();
            _content_type = "text/plain";
        } else {
            _build_error_page(404, "Not Found");
        }
    } else {
        // 2. Map URL to local filesystem
        std::string request_path = (req.get_path().empty()) ? "/" : req.get_path();
        std::string full_path = root + request_path;

        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            if (request_path[request_path.size() - 1] != '/') {
                request_path += "/";
                full_path += "/";
            }
            std::string index_path = full_path + index;
            std::ifstream index_file(index_path.c_str(), std::ios::binary);
            if (index_file.is_open()) {
                _status_line = "HTTP/1.1 200 OK\r\n";
                std::stringstream ss;
                ss << index_file.rdbuf();
                _body = ss.str();
                _content_type = _detect_content_type(index_path);
                index_file.close();
            } else if (autoindex) {
                _status_line = "HTTP/1.1 200 OK\r\n";
                _build_autoindex(full_path, request_path);
                _content_type = "text/html";
            } else {
                _build_error_page(403, "Forbidden");
            }
        } else {
            // 3. Try to open the file
            std::ifstream file(full_path.c_str(), std::ios::binary);
            if (file.is_open()) {
                _status_line = "HTTP/1.1 200 OK\r\n";
                std::stringstream ss;
                ss << file.rdbuf();
                _body = ss.str();
                _content_type = _detect_content_type(full_path);
                file.close();
            } else {
                _build_error_page(404, "Not Found");
            }
        }
    }

    // 4. Assemble the final response
    std::stringstream res;
    res << _status_line;
    res << _headers;
    res << "Content-Type: " << _content_type << "\r\n";
    res << "Content-Length: " << _body.size() << "\r\n";
    res << "Connection: close\r\n";
    res << "\r\n";
    res << _body;

    _full_response = res.str();
}

Response::Response(int code, const std::string &message, const ServerConfig &config, const RouteConfig &route)
    : _content_type("text/html"), _config(config), _route(route) {
    _build_error_page(code, message);
    std::stringstream res;
    res << _status_line;
    res << "Content-Type: " << _content_type << "\r\n";
    res << "Content-Length: " << _body.size() << "\r\n";
    res << "Connection: close\r\n";
    res << "\r\n";
    res << _body;
    _full_response = res.str();
}

void Response::_build_error_page(int code, const std::string &message) {
    std::stringstream status;
    status << "HTTP/1.1 " << code << " " << message << "\r\n";
    _status_line = status.str();
    if (!_try_load_error_page(code)) {
        std::stringstream ss;
        ss << "<html><body><h1>" << code << " " << message << "</h1></body></html>";
        _body = ss.str();
        _content_type = "text/html";
    }
}

bool Response::_try_load_error_page(int code) {
    std::map<int, std::string>::const_iterator it = _config.error_pages.find(code);
    if (it == _config.error_pages.end()) {
        return false;
    }
    std::ifstream file(it->second.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    _body = ss.str();
    _content_type = _detect_content_type(it->second);
    file.close();
    return true;
}

void Response::_build_autoindex(const std::string &full_path, const std::string &request_path) {
    DIR *dir = opendir(full_path.c_str());
    if (!dir) {
        _build_error_page(404, "Not Found");
        return;
    }
    std::stringstream ss;
    ss << "<html><body><h1>Index of " << request_path << "</h1><ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        ss << "<li><a href=\"" << request_path << name << "\">" << name << "</a></li>";
    }
    closedir(dir);
    ss << "</ul></body></html>";
    _body = ss.str();
}

std::string Response::_detect_content_type(const std::string &path) const {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return "application/octet-stream";
    }
    std::string ext = path.substr(dot + 1);
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "ico") return "image/x-icon";
    if (ext == "txt") return "text/plain";
    return "application/octet-stream";
}
