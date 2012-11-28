//
	//  MessageQueue.h
//  Architecture
//
//  Created by Massimo Gengarelli on 17/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

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
    bool hasMessages() const;
    std::size_t size() const;
    
    void clear();
    
protected:
    std::list<std::string> _messagesList;
};
