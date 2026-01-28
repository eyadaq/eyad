/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 20:41:20 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:46:26 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <map>
#include <poll.h>
#include <netinet/in.h>

#include "Request.hpp"
#include "Response.hpp"
#include "CgiHandler.hpp"
#include "Client.hpp"

class Server {
private:
    std::vector<int>        _listen_fds;
    std::vector<pollfd>     _poll_fds;
    
    // Maps for tracking ownership
    std::map<int, Client*>  _clients;     // Key: socket_fd
    std::map<int, Client*>  _cgi_fds;      // Key: pipe_fd (maps pipe back to client)

public:
    Server();
    ~Server();

    // Core Engine
    void    setup_server(int port);
    void    run();
    
    // Event Handlers
    void    accept_new_connection(int listen_fd);
    void    handle_client_read(int fd, Client &c);
    void    handle_client_write(int fd, Client &c);
    void    handle_cgi_read(int pipe_fd, size_t &poll_idx);
    
    // Helpers
    bool    is_listener(int fd);
    void    process_request(Client &c);
    void    update_poll_events(int fd, short events);
};

#endif