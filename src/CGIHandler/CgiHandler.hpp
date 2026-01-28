/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 21:08:01 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:14:55 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <map>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <cstdio>
#include "Request.hpp"

class CgiHandler {
private:
    std::string                         _script_path;
    std::string                         _body;
    std::map<std::string, std::string>  _env;
    pid_t                               _cgi_pid;

    void    _init_env(const Request& req);
    char** _get_env_char(); // Remember to free this memory in the .cpp!

public:
    CgiHandler(const Request& req, std::string script_path);
    ~CgiHandler() {}

    // Changes 'execute' (blocking) to 'launch' (non-blocking)
    // Returns the read-end of the pipe to the Server
    int     launch();

    // Getter for the PID so the Server can call waitpid(pid, ...)
    pid_t   get_pid() const { return _cgi_pid; }
};

#endif
