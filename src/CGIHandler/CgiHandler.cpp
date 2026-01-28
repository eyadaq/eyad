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

#include "CgiHandler.hpp"
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