/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 20:54:02 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:03:17 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>

class Request {
private:
    std::string                         _method;
    std::string                         _path;
    std::string                         _version;
    std::map<std::string, std::string>  _headers;
    std::string                         _body;

    void _parse_request_line(std::string line);
    void _parse_header_line(std::string line);

public:
    Request(const std::string& raw_data);
    ~Request() {}

    // Getters
    const std::string& get_method() const;
    const std::string& get_path() const;
    const std::string& get_header(const std::string& key) const;
    const std::string& get_body() const;
};

#endif