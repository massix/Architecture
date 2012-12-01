//
//  FrontendItf.h
//  Architecture
//
//  Created by Massimo Gengarelli on 17/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

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
    
    // Debugging
    friend std::ostream& operator<<(std::ostream& ostream, FrontendItf& iFrontend) {
        ostream << "Frontend: ";
        ostream << iFrontend._frontendId;
        ostream << " running on: ";
        ostream << iFrontend._hostname << ":" << iFrontend._port;
        return ostream;
    };
    
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
    zmq::socket_t _beSocket;
    
    std::string _hostname;
    uint16_t    _port;
    uint16_t    _bePort;
    boost::shared_ptr<BackendMap> _map;
    boost::shared_ptr<bool> _sonStatus;
        
};

