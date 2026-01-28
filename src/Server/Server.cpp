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

Server::Server() {}

Server::~Server() {
    // Clean up all clients
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    _clients.clear();

    // Close listeners
    for (size_t i = 0; i < _listen_fds.size(); ++i) {
        close(_listen_fds[i]);
    }
}

void Server::update_poll_events(int fd, short events) {
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        if (_poll_fds[i].fd == fd) {
            _poll_fds[i].events = events;
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
    _clients[client_fd] = new Client(client_fd);

    // Add to poll list
    pollfd pfd = {client_fd, POLLIN, 0};
    _poll_fds.push_back(pfd);
    
    std::cout << "New client connected on FD " << client_fd << std::endl;
}

void Server::handle_client_read(int fd, Client &c) {
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

    // State Machine Logic
    if (c.state == STATE_READING_REQUEST) {
        // Check if we have finished reading the header
        if (c.request_buffer.find("\r\n\r\n") != std::string::npos) {
            std::cout << "Header fully received for FD " << fd << std::endl;
            
            // In a real Webserv, you'd parse the header here to see 
            // if there is a Content-Length for a Body.
            // For now, let's move straight to processing.
            c.state = STATE_PROCESSING;
        }
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

    // CGI Handling
    if (req.get_path().find(".py") != std::string::npos) {
        CgiHandler cgi(req, "./www" + req.get_path());
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

    // Static Handling
    Response res(req);
    c.response_buffer = res.get_raw_response();
    c.state = STATE_WRITING_RESPONSE;
    
    // Switch from listening for data to waiting for the buffer to clear
    update_poll_events(c.fd, POLLOUT);
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
                _poll_fds[i].events = POLLOUT;
                break;
            }
        }
    }
}

void Server::run() {
    while (true) {
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
    }
}

