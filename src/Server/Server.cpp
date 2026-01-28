/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 20:41:18 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:48:38 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <cstdlib>

volatile sig_atomic_t g_shutdown_requested = 0;

Server::Server() {}

Server::~Server() {
    cleanup();
}

void Server::set_configs(const std::vector<ServerConfig> &configs) {
    _configs = configs;
}

void Server::cleanup() {
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    _clients.clear();

    for (size_t i = 0; i < _listen_fds.size(); ++i) {
        close(_listen_fds[i]);
    }
    _listen_fds.clear();
    _poll_fds.clear();
    _cgi_fds.clear();
    _listener_ports.clear();
}

void Server::update_poll_events(int fd, short events) {
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        if (_poll_fds[i].fd == fd) {
            _poll_fds[i].events = static_cast<short>(events | POLLIN);
            return;
        }
    }
}

bool Server::is_listener(int fd) {
    
    for (size_t i = 0; i < _listen_fds.size(); ++i) {
        if (_listen_fds[i] == fd)
            return true;
    }
    return false;
}

void Server::setup_server(int port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) throw std::runtime_error("Socket creation failed");

    // 1. Allow immediate reuse of the port
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Set to non-blocking
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("Bind failed");

    if (listen(listen_fd, 128) < 0)
        throw std::runtime_error("Listen failed");

    // Add to our poll list
    pollfd pfd = {listen_fd, POLLIN, 0};
    _poll_fds.push_back(pfd);
    _listen_fds.push_back(listen_fd);
    _listener_ports[listen_fd] = port;
    
    std::cout << "Server listening on port " << port << std::endl;
}

void Server::accept_new_connection(int listen_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) return;

    // VERY IMPORTANT: New client must also be non-blocking
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    // Create our state-tracking object
    int listen_port = _listener_ports.count(listen_fd) ? _listener_ports[listen_fd] : 0;
    _clients[client_fd] = new Client(client_fd, listen_port);

    // Add to poll list
    pollfd pfd = {client_fd, static_cast<short>(POLLIN | POLLOUT), 0};
    _poll_fds.push_back(pfd);
    
    std::cout << "New client connected on FD " << client_fd << std::endl;
}

void Server::handle_client_read(int fd, Client &c) {
    if (c.state != STATE_READING_REQUEST) {
        return;
    }
    char buffer[4096];
    int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            std::cout << "Client FD " << fd << " disconnected." << std::endl;
        } else {
            std::cerr << "Recv error on FD " << fd << std::endl;
        }
        c.state = STATE_DONE; // Signal to clean up this client
        return;
    }

    buffer[bytes_read] = '\0';
    c.request_buffer.append(buffer, bytes_read);
    c.last_activity = time(NULL); // Reset timeout timer

    if (c.state != STATE_READING_REQUEST || c.request_complete) {
        return;
    }

    if (!c.header_parsed) {
        size_t header_end = c.request_buffer.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            return;
        }
        c.header_parsed = true;
        c.header_end = header_end + 4;
        c.chunk_parse_pos = c.header_end;
        if (!c.config_resolved) {
            Request req(c.request_buffer);
            const ServerConfig &config = select_config(req, c);
            RouteConfig route = select_route(req, config);
            c.max_body_size = route.max_body_size_set ? route.max_body_size : config.max_body_size;
            c.config_resolved = true;
        }

        std::string header_part = c.request_buffer.substr(0, header_end);
        std::stringstream ss(header_part);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty() && line[line.size() - 1] == '\r') {
                line.erase(line.size() - 1);
            }
            size_t colon_pos = line.find(':');
            if (colon_pos == std::string::npos) {
                continue;
            }
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            size_t first = value.find_first_not_of(' ');
            if (first != std::string::npos) {
                value = value.substr(first);
            }
            if (key == "Content-Length") {
                c.content_length = static_cast<size_t>(std::strtoul(value.c_str(), NULL, 10));
            }
            if (key == "Transfer-Encoding" && value.find("chunked") != std::string::npos) {
                c.chunked = true;
            }
        }
        if (c.max_body_size > 0 && c.content_length > c.max_body_size) {
            Request req(c.request_buffer);
            const ServerConfig &config = select_config(req, c);
            RouteConfig route = select_route(req, config);
            Response res(413, "Payload Too Large", config, route);
            c.response_buffer = res.get_raw_response();
            c.state = STATE_WRITING_RESPONSE;
            update_poll_events(c.fd, POLLIN | POLLOUT);
            return;
        }
    }

    if (c.chunked) {
        while (true) {
            size_t line_end = c.request_buffer.find("\r\n", c.chunk_parse_pos);
            if (line_end == std::string::npos) {
                return;
            }
            std::string size_str = c.request_buffer.substr(c.chunk_parse_pos, line_end - c.chunk_parse_pos);
            size_t semicolon = size_str.find(';');
            if (semicolon != std::string::npos) {
                size_str = size_str.substr(0, semicolon);
            }
            size_t chunk_size = static_cast<size_t>(std::strtoul(size_str.c_str(), NULL, 16));
            size_t data_start = line_end + 2;
            size_t data_end = data_start + chunk_size;
            if (c.request_buffer.size() < data_end + 2) {
                return;
            }
            if (chunk_size == 0) {
                std::string header_part = c.request_buffer.substr(0, c.header_end);
                c.request_buffer = header_part + c.decoded_body;
                c.request_complete = true;
                c.state = STATE_PROCESSING;
                return;
            }
            c.decoded_body.append(c.request_buffer.substr(data_start, chunk_size));
            if (c.max_body_size > 0 && c.decoded_body.size() > c.max_body_size) {
                Request req(c.request_buffer);
                const ServerConfig &config = select_config(req, c);
                RouteConfig route = select_route(req, config);
                Response res(413, "Payload Too Large", config, route);
                c.response_buffer = res.get_raw_response();
                c.state = STATE_WRITING_RESPONSE;
                update_poll_events(c.fd, POLLIN | POLLOUT);
                return;
            }
            c.chunk_parse_pos = data_end + 2;
        }
    } else if (c.content_length > 0) {
        if (c.request_buffer.size() >= c.header_end + c.content_length) {
            c.request_complete = true;
            c.state = STATE_PROCESSING;
        }
    } else if (c.header_parsed) {
        c.request_complete = true;
        c.state = STATE_PROCESSING;
    }
}

void Server::handle_client_write(int fd, Client &c) {
    if (c.response_buffer.empty()) return;

    // send() returns the number of bytes actually accepted by the kernel
    ssize_t bytes_sent = send(fd, c.response_buffer.c_str(), c.response_buffer.size(), 0);

    if (bytes_sent > 0) {
        // Remove the bytes that were successfully sent from the buffer
        c.response_buffer.erase(0, bytes_sent);
        c.last_activity = time(NULL);

        // If the buffer is now empty, we are finished with this response
        if (c.response_buffer.empty()) {
            std::cout << "Response fully sent to FD " << fd << std::endl;
            c.state = STATE_DONE; 
        }
    } else if (bytes_sent == -1) {
        std::cerr << "Send error on FD " << fd << std::endl;
        c.state = STATE_ERROR;
    }
}

void Server::process_request(Client &c) {
    Request req(c.request_buffer);
    const ServerConfig &config = select_config(req, c);
    RouteConfig route = select_route(req, config);

    if (!is_method_allowed(req.get_method(), route)) {
        Response res(405, "Method Not Allowed", config, route);
        c.response_buffer = res.get_raw_response();
        c.state = STATE_WRITING_RESPONSE;
        update_poll_events(c.fd, POLLIN | POLLOUT);
        return;
    }

    // CGI Handling
    if (is_cgi_request(req.get_path(), route, config)) {
        std::string root = route.root.empty() ? config.root : route.root;
        CgiHandler cgi(req, root + req.get_path());
        int pipe_fd = cgi.launch();

        if (pipe_fd != -1) {
            c.cgi_pipe_fd = pipe_fd;
            c.cgi_pid = cgi.get_pid();
            c.state = STATE_WAITING_FOR_CGI;

            _cgi_fds[pipe_fd] = &c;
            pollfd pfd = {pipe_fd, POLLIN, 0};
            _poll_fds.push_back(pfd);
            return; 
        }
    }

    if (req.get_method() == "POST") {
        std::string upload_dir = route.upload_dir.empty() ? config.upload_dir : route.upload_dir;
        if (upload_dir.empty()) {
            Response res(403, "Forbidden", config, route);
            c.response_buffer = res.get_raw_response();
            c.state = STATE_WRITING_RESPONSE;
            update_poll_events(c.fd, POLLIN | POLLOUT);
            return;
        }
        std::stringstream path;
        path << upload_dir;
        if (upload_dir[upload_dir.size() - 1] != '/') {
            path << "/";
        }
        path << "upload_" << c.fd << "_" << time(NULL) << ".bin";
        std::ofstream out(path.str().c_str(), std::ios::binary);
        if (!out.is_open()) {
            Response res(500, "Internal Server Error", config, route);
            c.response_buffer = res.get_raw_response();
            c.state = STATE_WRITING_RESPONSE;
            update_poll_events(c.fd, POLLIN | POLLOUT);
            return;
        }
        out.write(req.get_body().c_str(), req.get_body().size());
        out.close();
        std::stringstream res;
        res << "HTTP/1.1 201 Created\r\n";
        res << "Content-Type: text/plain\r\n";
        res << "Content-Length: 0\r\n";
        res << "Connection: close\r\n\r\n";
        c.response_buffer = res.str();
        c.state = STATE_WRITING_RESPONSE;
        update_poll_events(c.fd, POLLIN | POLLOUT);
        return;
    }

    // Static Handling
    Response res(req, config, route);
    c.response_buffer = res.get_raw_response();
    c.state = STATE_WRITING_RESPONSE;
    
    // Switch from listening for data to waiting for the buffer to clear
    update_poll_events(c.fd, POLLIN | POLLOUT);
}

void Server::handle_cgi_read(int pipe_fd, size_t &poll_idx) {
    Client *c = _cgi_fds[pipe_fd];
    char buffer[4096];
    int bytes = read(pipe_fd, buffer, sizeof(buffer) - 1);

    if (bytes > 0) {
        buffer[bytes] = '\0';
        c->response_buffer.append(buffer, bytes);
        c->last_activity = time(NULL);
    } else {
        // Pipe closed, CGI is done
        c->state = STATE_WRITING_RESPONSE;
        close(pipe_fd);
        _cgi_fds.erase(pipe_fd);
        _poll_fds.erase(_poll_fds.begin() + poll_idx);
        poll_idx--; // Adjust loop index

        // Find client socket in poll_fds to switch to POLLOUT
        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].fd == c->fd) {
                _poll_fds[i].events = static_cast<short>(POLLIN | POLLOUT);
                break;
            }
        }
    }
}

void Server::run() {
    while (!g_shutdown_requested) {
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 1000);
        if (poll_count < 0) break;

        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            int fd = _poll_fds[i].fd;

            if (_poll_fds[i].revents & POLLIN) {
                if (is_listener(fd)) {
                    accept_new_connection(fd);
                } else if (_clients.count(fd)) {
                    handle_client_read(fd, *_clients[fd]);
                } else if (_cgi_fds.count(fd)) {
                    // This is a CGI pipe ready to be read
                    handle_cgi_read(fd, i);
                }
            }

            if (_poll_fds[i].revents & POLLOUT) {
                if (_clients.count(fd))
                    handle_client_write(fd, *_clients[fd]);
            }

            // --- State Transitions ---
            if (_clients.count(fd)) {
                Client *c = _clients[fd];
                if (c->state == STATE_PROCESSING) {
                    process_request(*c);
                }
                
                // Cleanup finished clients
                if (c->state == STATE_DONE || c->state == STATE_ERROR) {
                    std::cout << "Closing connection on FD " << fd << std::endl;
                    close(fd);
                    delete _clients[fd];
                    _clients.erase(fd);
                    _poll_fds.erase(_poll_fds.begin() + i);
                    --i;
                }
            }
        }
        // The Zombie Killer
        waitpid(-1, NULL, WNOHANG);
        apply_timeout_check();
    }
    cleanup();
}

const ServerConfig& Server::select_config(const Request &req, const Client &c) const {
    const ServerConfig *fallback = NULL;
    std::string host = req.get_header("Host");
    size_t colon = host.find(':');
    if (colon != std::string::npos) {
        host = host.substr(0, colon);
    }

    for (size_t i = 0; i < _configs.size(); ++i) {
        if (_configs[i].port != c.listen_port) {
            continue;
        }
        if (!fallback) {
            fallback = &_configs[i];
        }
        if (!host.empty() && _configs[i].server_name == host) {
            return _configs[i];
        }
    }
    if (fallback) {
        return *fallback;
    }
    static ServerConfig default_config;
    default_config.root = "./www";
    default_config.index = "index.html";
    return default_config;
}

RouteConfig Server::select_route(const Request &req, const ServerConfig &config) const {
    RouteConfig best_match;
    bool found = false;
    size_t best_len = 0;
    for (size_t i = 0; i < config.routes.size(); ++i) {
        const RouteConfig &route = config.routes[i];
        if (req.get_path().find(route.path) == 0 && route.path.size() >= best_len) {
            best_match = route;
            best_len = route.path.size();
            found = true;
        }
    }
    if (!found) {
        best_match.path = "/";
        best_match.autoindex_set = true;
        best_match.autoindex = config.autoindex;
        best_match.root = config.root;
        best_match.index = config.index;
        best_match.upload_dir = config.upload_dir;
        best_match.allowed_methods = config.allowed_methods;
        best_match.cgi_extensions = config.cgi_extensions;
        best_match.max_body_size = config.max_body_size;
        best_match.max_body_size_set = true;
    } else {
        if (!best_match.autoindex_set) {
            best_match.autoindex = config.autoindex;
            best_match.autoindex_set = true;
        }
        if (best_match.root.empty()) {
            best_match.root = config.root;
        }
        if (best_match.index.empty()) {
            best_match.index = config.index;
        }
        if (best_match.upload_dir.empty()) {
            best_match.upload_dir = config.upload_dir;
        }
        if (best_match.allowed_methods.empty()) {
            best_match.allowed_methods = config.allowed_methods;
        }
        if (best_match.cgi_extensions.empty()) {
            best_match.cgi_extensions = config.cgi_extensions;
        }
        if (!best_match.max_body_size_set) {
            best_match.max_body_size = config.max_body_size;
            best_match.max_body_size_set = true;
        }
    }
    return best_match;
}

bool Server::is_method_allowed(const std::string &method, const RouteConfig &route) const {
    for (size_t i = 0; i < route.allowed_methods.size(); ++i) {
        if (route.allowed_methods[i] == method) {
            return true;
        }
    }
    return false;
}

bool Server::is_cgi_request(const std::string &path, const RouteConfig &route, const ServerConfig &config) const {
    std::vector<std::string> extensions = route.cgi_extensions.empty() ? config.cgi_extensions : route.cgi_extensions;
    if (extensions.empty()) {
        return false;
    }
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return false;
    }
    std::string ext = path.substr(dot);
    for (size_t i = 0; i < extensions.size(); ++i) {
        if (extensions[i] == ext) {
            return true;
        }
    }
    return false;
}

void Server::apply_timeout_check() {
    const time_t kClientTimeout = 30;
    time_t now = time(NULL);
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        Client *c = it->second;
        if (c->state == STATE_DONE || c->state == STATE_ERROR || c->state == STATE_WRITING_RESPONSE) {
            continue;
        }
        if (now - c->last_activity > kClientTimeout) {
            Request req("GET / HTTP/1.1\r\n\r\n");
            const ServerConfig &config = select_config(req, *c);
            RouteConfig route = select_route(req, config);
            Response res(408, "Request Timeout", config, route);
            c->response_buffer = res.get_raw_response();
            c->state = STATE_WRITING_RESPONSE;
            update_poll_events(c->fd, POLLIN | POLLOUT);
        }
    }
}
