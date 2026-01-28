/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 19:39:45 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:41:11 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>
#include <unistd.h> // For pid_t

enum e_state {
    STATE_READING_REQUEST,
    STATE_WAITING_FOR_CGI,  // <--- Essential for non-blocking CGI
    STATE_PROCESSING,
    STATE_WRITING_RESPONSE,
    STATE_DONE,
    STATE_ERROR
};

class Client {
public:
    int             fd;
    int             cgi_pipe_fd;   // Read-end of the pipe from the CGI child
    pid_t           cgi_pid;       // Child process ID for waitpid()
    e_state         state;
    std::string     request_buffer;
    std::string     response_buffer;
    time_t          last_activity; // For handling timeouts

    // Constructor to initialize everything to safe defaults
    Client(int socket_fd);
};

#endif

