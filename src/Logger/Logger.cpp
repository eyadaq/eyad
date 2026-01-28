/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 08:20:06 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/28 21:05:42 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"

void Logger::log(LogLevel level,const char* function ,const std::string& message) {
	std::string levelStr;
	switch (level) {
		case DEBUG:
			levelStr = "DEBUG";
			break;
		case INFO:
			levelStr = "INFO";
			break;
		case ERROR:
			levelStr = "ERROR";
			break;
		default:
			levelStr = "UNKNOWN";
	}
	std::cerr << "[" << levelStr << "] " << "[" << function << "] " << message << std::endl;
}
