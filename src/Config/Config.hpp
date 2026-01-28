/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 21:49:36 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:49:38 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct RouteConfig {
    std::string                 path;
    std::string                 root;
    std::string                 index;
    bool                        autoindex_set;
    bool                        autoindex;
    std::string                 upload_dir;
    std::vector<std::string>    allowed_methods;
    std::vector<std::string>    cgi_extensions;
    int                         redirect_code;
    std::string                 redirect_target;
    bool                        max_body_size_set;
    size_t                      max_body_size;

    RouteConfig()
        : autoindex_set(false),
          autoindex(false),
          redirect_code(0),
          max_body_size_set(false),
          max_body_size(0) {}
};

struct ServerConfig {
    int                         port;
    std::string                 host;
    std::string                 server_name;
    std::string                 root;
    std::string                 index;
    bool                        autoindex;
    std::string                 upload_dir;
    std::vector<std::string>    allowed_methods;
    std::vector<std::string>    cgi_extensions;
    size_t                      max_body_size;
    std::map<int, std::string>  error_pages;
    std::vector<RouteConfig>    routes;

    ServerConfig()
        : port(8080),
          autoindex(true),
          max_body_size(1024 * 1024) {}
};

#endif
