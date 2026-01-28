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
#include <cstdlib>

static std::vector<std::string> tokenize_config(const std::string &content) {
    std::vector<std::string> tokens;
    std::string current;
    for (size_t i = 0; i < content.size(); ++i) {
        char ch = content[i];
        if (ch == '#') {
            while (i < content.size() && content[i] != '\n') {
                ++i;
            }
            continue;
        }
        if (ch == '{' || ch == '}' || ch == ';') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            std::string token(1, ch);
            tokens.push_back(token);
            continue;
        }
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current += ch;
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

static void apply_default_methods(std::vector<std::string> &methods) {
    if (methods.empty()) {
        methods.push_back("GET");
        methods.push_back("POST");
        methods.push_back("DELETE");
    }
}

static RouteConfig parse_location_block(const std::vector<std::string> &tokens, size_t &i) {
    RouteConfig route;
    if (i >= tokens.size()) {
        return route;
    }
    route.path = tokens[i++];
    if (i >= tokens.size() || tokens[i] != "{") {
        throw std::runtime_error("Expected '{' after location path");
    }
    ++i;
    while (i < tokens.size() && tokens[i] != "}") {
        std::string key = tokens[i++];
        if (key == "root") {
            route.root = tokens[i++];
        } else if (key == "index") {
            route.index = tokens[i++];
        } else if (key == "autoindex") {
            std::string value = tokens[i++];
            route.autoindex_set = true;
            route.autoindex = (value == "on");
        } else if (key == "upload_dir") {
            route.upload_dir = tokens[i++];
        } else if (key == "methods") {
            route.allowed_methods.clear();
            while (i < tokens.size() && tokens[i] != ";") {
                route.allowed_methods.push_back(tokens[i++]);
            }
        } else if (key == "cgi_ext") {
            route.cgi_extensions.clear();
            while (i < tokens.size() && tokens[i] != ";") {
                route.cgi_extensions.push_back(tokens[i++]);
            }
        } else if (key == "return") {
            route.redirect_code = std::atoi(tokens[i++].c_str());
            route.redirect_target = tokens[i++];
        } else if (key == "client_max_body_size") {
            route.max_body_size_set = true;
            route.max_body_size = static_cast<size_t>(std::strtoul(tokens[i++].c_str(), NULL, 10));
        }
        if (i < tokens.size() && tokens[i] == ";") {
            ++i;
        }
    }
    if (i < tokens.size() && tokens[i] == "}") {
        ++i;
    }
    apply_default_methods(route.allowed_methods);
    return route;
}

static ServerConfig parse_server_block(const std::vector<std::string> &tokens, size_t &i) {
    ServerConfig config;
    config.root = "./www";
    config.index = "index.html";
    config.autoindex = true;
    apply_default_methods(config.allowed_methods);

    if (i >= tokens.size() || tokens[i] != "{") {
        throw std::runtime_error("Expected '{' after server");
    }
    ++i;
    while (i < tokens.size() && tokens[i] != "}") {
        std::string key = tokens[i++];
        if (key == "listen") {
            config.port = std::atoi(tokens[i++].c_str());
        } else if (key == "server_name") {
            config.server_name = tokens[i++];
        } else if (key == "root") {
            config.root = tokens[i++];
        } else if (key == "index") {
            config.index = tokens[i++];
        } else if (key == "autoindex") {
            std::string value = tokens[i++];
            config.autoindex = (value == "on");
        } else if (key == "upload_dir") {
            config.upload_dir = tokens[i++];
        } else if (key == "methods") {
            config.allowed_methods.clear();
            while (i < tokens.size() && tokens[i] != ";") {
                config.allowed_methods.push_back(tokens[i++]);
            }
        } else if (key == "cgi_ext") {
            config.cgi_extensions.clear();
            while (i < tokens.size() && tokens[i] != ";") {
                config.cgi_extensions.push_back(tokens[i++]);
            }
        } else if (key == "client_max_body_size") {
            config.max_body_size = static_cast<size_t>(std::strtoul(tokens[i++].c_str(), NULL, 10));
        } else if (key == "error_page") {
            int code = std::atoi(tokens[i++].c_str());
            std::string path_value = tokens[i++];
            config.error_pages[code] = path_value;
        } else if (key == "location") {
            RouteConfig route = parse_location_block(tokens, i);
            config.routes.push_back(route);
            continue;
        }
        if (i < tokens.size() && tokens[i] == ";") {
            ++i;
        }
    }
    if (i < tokens.size() && tokens[i] == "}") {
        ++i;
    }
    return config;
}

std::vector<ServerConfig> ConfigParser::parse(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::vector<std::string> tokens = tokenize_config(buffer.str());

    std::vector<ServerConfig> configs;
    size_t i = 0;
    while (i < tokens.size()) {
        if (tokens[i] == "server") {
            ++i;
            ServerConfig config = parse_server_block(tokens, i);
            configs.push_back(config);
        } else {
            ++i;
        }
    }
    if (configs.empty()) {
        ServerConfig config;
        config.root = "./www";
        config.index = "index.html";
        config.autoindex = true;
        apply_default_methods(config.allowed_methods);
        configs.push_back(config);
    }
    return configs;
}
