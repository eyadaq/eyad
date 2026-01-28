/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/28 19:43:19 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:41:00 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

 
#include "Client.hpp"
  
Client::Client(int socket_fd) : 
        fd(socket_fd), 
        cgi_pipe_fd(-1), 
        cgi_pid(-1), 
        state(STATE_READING_REQUEST),
        last_activity(time(NULL)) {}