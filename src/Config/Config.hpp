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

struct ServerConfig {
    int                         port;
    std::string                 host;
    std::string                 server_name;
    std::string                 root;
    std::string                 index;
    size_t                      max_body_size;
    std::map<int, std::string>  error_pages;
    // Add more as needed (e.g., allowed methods)
};

#endif