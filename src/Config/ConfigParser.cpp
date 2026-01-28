/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 21:50:53 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:56:37 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

std::vector<ServerConfig> ConfigParser::parse(const std::string& path) {
	std::vector<ServerConfig> configs;
	std::ifstream file(path.c_str());
	std::string line;

	if (!file.is_open())
		throw std::runtime_error("Could not open config file");

	ServerConfig current_config;
	// Default values
	current_config.port = 8080;
	current_config.root = "./www";

	while (std::getline(file, line)) {
		// Basic trimming and skipping comments
		size_t first = line.find_first_not_of(" \t");
		if (first == std::string::npos || line[first] == '#') continue;
		
		std::stringstream ss(line);
		std::string key;
		ss >> key;

		if (key == "listen") ss >> current_config.port;
		else if (key == "server_name") ss >> current_config.server_name;
		else if (key == "root") ss >> current_config.root;
		else if (key == "index") ss >> current_config.index;
		else if (key == "server") { /* Start of a new block logic */ }
		// Add a ';' check if your config format requires it
	}
	configs.push_back(current_config);
	return configs;
}