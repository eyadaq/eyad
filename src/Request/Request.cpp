/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 20:54:56 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:06:59 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

/*

Transport: Server class uses poll() to manage sockets.

State Machine: Client class tracks if we are reading or writing.

Parsing: Request class breaks down the HTTP syntax.

Logic: Response class finds the file and builds the output.

*/

Request::Request(const std::string& raw_data) {
    std::string data = raw_data;
    size_t pos = data.find("\r\n\r\n");
    
    // 1. Separate Headers from Body
    std::string head_part = (pos != std::string::npos) ? data.substr(0, pos) : data;
    if (pos != std::string::npos)
        _body = data.substr(pos + 4);

    // 2. Process Header Part line by line
    std::stringstream ss(head_part);
    std::string line;
    
    // First line is the Request Line (GET /index.html HTTP/1.1)
    if (std::getline(ss, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        _parse_request_line(line);
    }

    // Subsequent lines are Headers
    while (std::getline(ss, line) && line != "\r" && !line.empty()) {
        if (line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        _parse_header_line(line);
    }
}

void Request::_parse_request_line(std::string line) {
    std::stringstream ss(line);
    ss >> _method >> _path >> _version;
}

void Request::_parse_header_line(std::string line) {
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);
        
        // Trim leading space from value
        size_t first = value.find_first_not_of(' ');
        if (first != std::string::npos)
            value = value.substr(first);
            
        _headers[key] = value;
    }
}

// Getters implementation
const std::string& Request::get_method() const { return _method; }
const std::string& Request::get_path() const { return _path; }
const std::string& Request::get_body() const { return _body; }
const std::string& Request::get_header(const std::string& key) const {
    static std::string empty = "";
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    return (it != _headers.end()) ? it->second : empty;
}