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
#include "Config.hpp"

class Response {
private:
    std::string _full_response;
    std::string _body;
    std::string _status_line;
    std::string _headers;
    std::string _content_type;
    ServerConfig _config;

    void _build_error_page(int code, const std::string &message);
    void _build_autoindex(const std::string &full_path, const std::string &request_path);
    std::string _detect_content_type(const std::string &path) const;
    bool _try_load_error_page(int code);

public:
    Response(const Request& req, const ServerConfig &config);
    ~Response() {}

    const std::string& get_raw_response() const { return _full_response; }
};

#endif
