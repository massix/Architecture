//
//  Receptor.h
//  Receptor
//
//  Created by Massimo Gengarelli on 10/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <map>
#include <zmq.hpp>
#include <boost/thread.hpp>
#include "StandardMessage.pb.h"

class ReceptorException : public std::runtime_error
{
public:
    explicit ReceptorException(const std::string& iError) :
        std::runtime_error(iError) {};
    virtual ~ReceptorException() throw() {};
};

class Receptor
{
public:
    static Receptor& GetInstance();
    virtual ~Receptor();
    
    const std::string& getHostName() const;
    const uint16_t& getPort() const;
    
    void startServer() throw(ReceptorException);
    void stopServer() throw(ReceptorException);
    
    const bool isRunning() const;
    
    void printConfiguration() const;
    
private:
    Receptor() :
        _hostName(""),
        _port(-1),
        _status(false),
        _feConnectorThread(0),
        _context(1),
        _socket(_context, ZMQ_ROUTER) {};
    
    // Not implemented on purpose
    Receptor(const Receptor& iRight);
    Receptor& operator=(const Receptor& iRight);
    ReceptorMessages::ResponseMessage routeMessage(
        ReceptorMessages::BaseMessage* ioBaseMessage);
    void configure(const std::string& iConfFile);
    
    std::string _hostName;
    uint16_t    _port;
    bool        _status;
    std::map<std::string, std::string> _routingMap;
    
    boost::thread* _feConnectorThread;
    
    zmq::context_t _context;
    zmq::socket_t  _socket;
    
    std::map<std::string, zmq::socket_t*> _feSockets;
};
