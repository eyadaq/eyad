/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 08:13:16 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:03:23 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

enum LogLevel {
    DEBUG,
    INFO,
    ERROR
};

class Logger
{
public:
	void static log(LogLevel level,const char* function , const std::string& message);
};

