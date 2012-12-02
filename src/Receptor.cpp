//
//  Receptor.cpp
//  Receptor
//
//  Created by Massimo Gengarelli on 10/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

#include "Receptor.h"

#include <string>
#include <iostream>
#include <exception>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <Log.h>

#include "StandardMessage.pb.h"

using std::string;
using std::map;

using boost::thread;

Receptor& Receptor::GetInstance() {
    static Receptor _Instance;
    
    // clear the routing map
    _Instance._routingMap.clear();
    _Instance.configure("RouterConfig.xml");
    
    return _Instance;
}

Receptor::~Receptor() {
    stopServer();
}

const string& Receptor::getHostName() const {
    return _hostName;
}

const uint16_t& Receptor::getPort() const {
    return _port;
}

const bool Receptor::isRunning() const {
    return _status;
}

void Receptor::startServer() throw(ReceptorException) {
    if (_status) throw ReceptorException("Server already started?");
    
    string aZMQString(_hostName);
    aZMQString += ":" + boost::lexical_cast<string>(_port);
    _socket.bind(aZMQString.c_str());
    _status = true;
    
    while (true) {
        try {
            zmq::message_t aMessage;
            _socket.recv(&aMessage);
            
            ReceptorMessages::BaseMessage aBaseMessage;
            aBaseMessage.ParseFromString((const char*) aMessage.data());
            
            LOG_MSG("Received " + aBaseMessage.messagetype());
            routeMessage(&aBaseMessage);
            
            // Send a fake reply
            zmq::message_t aReplyMessage(4);
            memcpy((void*) aReplyMessage.data(), "ACK", 3);
            _socket.send(aReplyMessage);
            
            if (aBaseMessage.messagetype() == "QUIT") break;
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    stopServer();
}

void Receptor::stopServer() throw(ReceptorException) {
    _socket.close();
}

void Receptor::routeMessage(ReceptorMessages::BaseMessage *ioBaseMessage) {
    // QUIT message terminates all the FEs
    if (ioBaseMessage->messagetype() == "QUIT") {
        std::map<std::string, zmq::socket_t*>::iterator anIte;
        for (anIte = _feSockets.begin(); anIte != _feSockets.end(); ++anIte) {
            zmq::message_t aQuitMessage(4);
            memcpy(aQuitMessage.data(), "QUIT", 4);
            anIte->second->send(aQuitMessage);
            LOG_MSG("Asking FE " + anIte->first + " to terminate");
            anIte->second->close();
        }
        
        return;
    }
    
    try {
        string& aDestination = _routingMap[ioBaseMessage->messagetype()];
        LOG_MSG("Routing message to: " + aDestination);
        
        // Send message to the frontend
        std::string aSerializedMessage;
        ioBaseMessage->SerializeToString(&aSerializedMessage);
        zmq::message_t aMessage(aSerializedMessage.size());
        memcpy(aMessage.data(), aSerializedMessage.c_str(), aSerializedMessage.size());
        
        zmq::socket_t* aSocket = _feSockets.at(ioBaseMessage->messagetype());
        
        aSocket->send(aMessage);
        
        zmq::message_t aResponse;

        // Receive the reply
        aSocket->recv(&aResponse);
        LOG_MSG("Router response received");
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void Receptor::printConfiguration() const {
    std::cout << "Router - " << _hostName << ":" << _port << std::endl;
    map<string, string>::const_iterator anIte = _routingMap.begin();
    for (; anIte != _routingMap.end(); ++anIte) {
        std::cout << " MESSAGE " << anIte->first << " ROUTED TO " << anIte->second << std::endl;
    }
}

void Receptor::configure(const std::string &iConfFile) {
    using boost::property_tree::ptree;
    using namespace boost::property_tree::xml_parser;
    using std::pair;
    
    try {
        ptree aPropertyTree;
        read_xml(iConfFile, aPropertyTree);
    
        ptree::const_iterator aTreeIterator;
        for (aTreeIterator = aPropertyTree.begin(); aTreeIterator != aPropertyTree.end(); ++aTreeIterator) {
            if (aTreeIterator->first == "RouterConfig") {
                _hostName = aTreeIterator->second.get<string>("<xmlattr>.hostname");
                _port = aTreeIterator->second.get<uint16_t>("<xmlattr>.port");
                
                // Get the messages
                ptree aChild = aTreeIterator->second;
                ptree::const_iterator aChildIterator = aChild.begin();
                for (; aChildIterator != aChild.end(); ++aChildIterator) {
                    if (aChildIterator->first == "message") {
                        string aKey = aChildIterator->second.get<string>("<xmlattr>.type");
                        string aValue = aChildIterator->second.get<string>("<xmlattr>.routing");
                        _routingMap.insert(make_pair(aKey, aValue));
                        _feSockets[aKey] = new zmq::socket_t(_context, ZMQ_REQ);
                        _feSockets[aKey]->connect(aValue.c_str());
                    }
                }
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Caught exception while configuring" << std::endl;
    }
}