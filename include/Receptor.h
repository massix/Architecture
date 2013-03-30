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
    Receptor();
    
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
    
    bool _configured;
};
