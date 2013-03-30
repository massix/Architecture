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

Receptor::Receptor() :
    _hostName(""),
    _port(-1),
    _status(false),
    _feConnectorThread(0),
    _context(1),
    _socket(_context, ZMQ_ROUTER),
    _configured(false)
{
    int64_t aVersionMajor = __RECEPTOR_VERSION__ >> 16;
    int64_t aVersionMinor = (__RECEPTOR_VERSION__ >> 8) & 0x00FF;
    int64_t aVersionRelease = __RECEPTOR_VERSION__ & 0x0000FF;
    
    LOG_MSG("Using receptor library version: " +
            boost::lexical_cast<std::string>(aVersionMajor) + "." +
            boost::lexical_cast<std::string>(aVersionMinor) + "." +
            boost::lexical_cast<std::string>(aVersionRelease));
}

Receptor& Receptor::GetInstance() {
    static Receptor _Instance;
    
    if (!_Instance._configured) {
        _Instance._routingMap.clear();
        _Instance.configure("RouterConfig.xml");
    }

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
            zmq::message_t anHeader;
            zmq::message_t anEmptySeparator;
            zmq::message_t aMessage;
            _socket.recv(&anHeader);
            _socket.recv(&anEmptySeparator);
            _socket.recv(&aMessage);
            
            ReceptorMessages::BaseMessage aBaseMessage;
            aBaseMessage.ParseFromString((const char*) aMessage.data());

            // Set the header if it doesn't already have one (client connected)
            if (!aBaseMessage.has_clientheader())
                aBaseMessage.set_clientheader(anHeader.data(), anHeader.size());
            
            LOG_MSG("Received\n" + aBaseMessage.DebugString());
            ReceptorMessages::ResponseMessage aResponse(routeMessage(&aBaseMessage));
            
            std::string aSerializedResponse(aResponse.SerializeAsString());
            zmq::message_t aReplyMessage(aSerializedResponse.size());
            memcpy((void*) aReplyMessage.data(), aSerializedResponse.c_str(), aSerializedResponse.size());
            _socket.send(anHeader, ZMQ_SNDMORE);
            _socket.send(anEmptySeparator, ZMQ_SNDMORE);
            _socket.send(aReplyMessage);
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    
    stopServer();
}

void Receptor::stopServer() throw(ReceptorException) {
    _socket.close();
}

ReceptorMessages::ResponseMessage Receptor::routeMessage(ReceptorMessages::BaseMessage *ioBaseMessage) {
    try {
        string& aDestination = _routingMap[ioBaseMessage->messagetype()];
        LOG_MSG("Routing message to: " + aDestination);
        
        // Send message to the frontend
        std::string aSerializedMessage(ioBaseMessage->SerializeAsString());
        zmq::message_t aMessage(aSerializedMessage.size());
        memcpy(aMessage.data(), aSerializedMessage.c_str(), aSerializedMessage.size());
        
        zmq::socket_t* aSocket = _feSockets.at(ioBaseMessage->messagetype());
        
        aSocket->send(aMessage);
        
        zmq::message_t aResponse;

        // Receive the reply
        aSocket->recv(&aResponse);
        ReceptorMessages::ResponseMessage aResponseMessage;
        aResponseMessage.ParseFromArray((const char *) aResponse.data(), (uint32_t) aResponse.size());
        LOG_MSG("Router response received:\n" + aResponseMessage.DebugString());
        return aResponseMessage;
    }
    catch (const std::exception& e) {
        ReceptorMessages::ResponseMessage anEmptyMsg;
        std::cerr << "Exception: " << e.what() << std::endl;
        return anEmptyMsg;
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
    
    _configured = true;
}
