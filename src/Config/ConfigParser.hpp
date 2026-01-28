/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 21:51:21 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:57:00 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "Config.hpp"


class ConfigParser {
public:
    static std::vector<ServerConfig> parse(const std::string& path);

};