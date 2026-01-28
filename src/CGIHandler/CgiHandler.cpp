/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 21:08:15 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:13:01 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "CgiHandler.hpp"
#include <cstdlib>
#include <cstring>
#include <sstream>

CgiHandler::CgiHandler(const Request& req, std::string script_path)
    : _script_path(script_path), _body(req.get_body()), _cgi_pid(-1) {
    _init_env(req);
}

void CgiHandler::_init_env(const Request& req) {
    std::string path = req.get_path();
    std::string query;
    size_t qpos = path.find('?');
    if (qpos != std::string::npos) {
        query = path.substr(qpos + 1);
        path = path.substr(0, qpos);
    }
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env["REQUEST_METHOD"] = req.get_method();
    _env["SCRIPT_FILENAME"] = _script_path;
    _env["SCRIPT_NAME"] = path;
    _env["QUERY_STRING"] = query;
    _env["SERVER_PROTOCOL"] = "HTTP/1.1";
    _env["REDIRECT_STATUS"] = "200";

    std::string content_length = req.get_header("Content-Length");
    if (!content_length.empty()) {
        _env["CONTENT_LENGTH"] = content_length;
    }
    std::string content_type = req.get_header("Content-Type");
    if (!content_type.empty()) {
        _env["CONTENT_TYPE"] = content_type;
    }
}

char** CgiHandler::_get_env_char() {
    char** envp = new char*[_env.size() + 1];
    size_t idx = 0;
    for (std::map<std::string, std::string>::const_iterator it = _env.begin(); it != _env.end(); ++it) {
        std::string entry = it->first + "=" + it->second;
        char* data = new char[entry.size() + 1];
        std::strcpy(data, entry.c_str());
        envp[idx++] = data;
    }
    envp[idx] = NULL;
    return envp;
}
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int CgiHandler::launch() {
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) return -1;

    // 1. Set the read-end of the pipe to NON-BLOCKING
    // This is vital so the Server doesn't hang if the script is slow.
    fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK);

    _cgi_pid = fork();
    if (_cgi_pid == -1) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return -1;
    }

    if (_cgi_pid == 0) {
        // --- CHILD PROCESS ---
        // Redirect STDOUT to the write-end of the pipe
        dup2(pipe_fds[1], STDOUT_FILENO);
        close(pipe_fds[0]);
        close(pipe_fds[1]);

        char* args[] = { (char*)_script_path.c_str(), NULL };
        char** envp = _get_env_char();

        execve(args[0], args, envp);
        // If execve fails, exit immediately to prevent a zombie child
        exit(1); 
    }

    // --- PARENT PROCESS ---
    close(pipe_fds[1]); // Close the write-end in the parent
    return pipe_fds[0]; // Return the read-end to be added to poll()
}
