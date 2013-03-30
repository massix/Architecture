/*
 *  Architecture - A simple (yet not working) architecture for cloud computing
 *  Copyright (C) 2013 Massimo Gengarelli <massimo.gengarelli@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <exception>
#include <string>
#include <zmq.hpp>

class MessageQueueException : public std::exception
{
public:
    MessageQueueException(const std::string& iMessage) : _exception(iMessage) {};
    virtual ~MessageQueueException() throw() {};
    const char* what() const throw() { return _exception.c_str(); };
    
private:
    std::string _exception;
};


class MessageQueue
{
public:
    explicit MessageQueue();
    virtual ~MessageQueue();
    
    void enqueueMessage(const std::string& iMessage);
    std::string dequeueMessage();
    
    void enqueueHeader(const std::string& iHeader);
    std::string dequeueHeader();
    
    bool hasMessages() const;
    std::size_t size() const;
    
    void clear();
    bool empty() const;
    
protected:
    std::list<std::string> _messagesList;
    std::list<std::string> _headersList;
};
