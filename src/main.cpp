/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 21:57:28 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 22:06:09 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "Server.hpp"


int main(int argc, char** argv) {
    std::string config_path = (argc == 2) ? argv[1] : "default.conf";

    try {
        std::vector<ServerConfig> configs = ConfigParser::parse(config_path);
        Server webserv;

        // Setup a listener for every config block
        for (size_t i = 0; i < configs.size(); ++i) {
            webserv.setup_server(configs[i].port);
        }

        webserv.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}