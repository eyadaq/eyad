/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 21:01:08 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:01:09 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include "Request.hpp"

class Response {
private:
    std::string _full_response;
    std::string _body;
    std::string _status_line;
    std::string _headers;

    void _build_error_page(std::string code, std::string message);
    void _file_to_body(std::string path);

public:
    Response(const Request& req);
    ~Response() {}

    const std::string& get_raw_response() const { return _full_response; }
};

#endif