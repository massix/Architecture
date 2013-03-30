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
#include "MessageQueue.h"

#include <iostream>
#include <string>
#include <zmq.hpp>
#include <StandardMessage.pb.h>
#include <map>
#include <vector>
#include <functional>
#include <utility>
#include <BackendItf.h>
#include <boost/shared_ptr.hpp>

typedef std::map<std::string, MessageQueue> BackendMap;

class FrontendItf
{
public:
    explicit FrontendItf(const std::string& iId);
    virtual ~FrontendItf();
    
    const std::string& getId() const;
    /* Return copy to avoid modifications */
    const std::string getConfXml() const;
    void configure();
    FrontendItf& reconfigure(const std::string iNewXml);
    void start();
    void startBackendListener();
    void stop();
      
private:
    // Avoid other construction methods
    FrontendItf();
    FrontendItf(const FrontendItf& iRight);
    FrontendItf& operator=(const FrontendItf& iRight);
    
protected:
    std::string _frontendId;
    std::string _confXml;
    zmq::context_t _zmqContext;
    zmq::socket_t _zmqSocket;
    
    std::string _hostname;
    uint16_t    _port;
    uint16_t    _bePort;
    boost::shared_ptr<BackendMap> _map;
    boost::shared_ptr<bool> _sonStatus;
};

