//
//  FrontendItf.cpp
//  Architecture
//
//  Created by Massimo Gengarelli on 17/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

#include "FrontendItf.h"
#include <string>
#include "MessageQueue.h"

#include <zmq.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

FrontendItf::FrontendItf(const std::string& iId) :
    _frontendId(iId),
    _confXml(_frontendId + "FE.xml"),
    _zmqContext(1),
    _zmqSocket(_zmqContext, ZMQ_REP)
{
    std::cout << "Initializing FE " << _frontendId << std::endl;
}

FrontendItf::~FrontendItf()
{
    _zmqSocket.close();
}

const MessageQueue& FrontendItf::getMsgQ() const
{
    return _msgQ;
}

const std::string& FrontendItf::getId() const
{
    return _frontendId;
}

void FrontendItf::configure()
{
    using boost::property_tree::ptree;
    using namespace boost::property_tree::xml_parser;
    
    try {
        ptree aPtree;
        read_xml(_confXml, aPtree);
        
        ptree::const_iterator aPtreeIterator;
        for (aPtreeIterator = aPtree.begin(); aPtreeIterator != aPtree.end(); ++aPtreeIterator) {
            if (aPtreeIterator->first == "Frontend") {
                _hostname = aPtreeIterator->second.get<std::string>("<xmlattr>.host");
                _port = aPtreeIterator->second.get<uint16_t>("<xmlattr>.port");
            }
        }
    }
    catch (...) {
        
    }
    
    std::cout << *this << std::endl;
}

FrontendItf& FrontendItf::reconfigure(const std::string iNewXml)
{
    _hostname.clear();
    _port = 0;
    _confXml = iNewXml;
    configure();
    
    _msgQ.clear();
    return *this;
}

const std::string FrontendItf::getConfXml() const
{
    return _confXml;
}

void FrontendItf::start()
{
    std::string aZMQString("tcp://" + _hostname + ":");
    aZMQString += boost::lexical_cast<std::string>(_port);
    std::cout << *this << " bound to: " << aZMQString << std::endl;
    _zmqSocket.bind(aZMQString.c_str());
    
    while (true) {
        zmq::message_t aZMQMessage;
        _zmqSocket.recv(&aZMQMessage);
        std::string aStringMessage((const char*) aZMQMessage.data(), aZMQMessage.size());
        if ("QUIT" == aStringMessage) break;
        _msgQ.enqueueMessage(aStringMessage);
        
        std::cout << "Frontend enqueued message" << std::endl;
        
        // Send a Reply to the Receptor
        std::string aReply("ENQ:1");
        zmq::message_t aResponse(aReply.size());
        memcpy(aResponse.data(), aReply.c_str(), aReply.size());
        
        std::cout << "Frontend sending reply" << std::endl;
        
        _zmqSocket.send(aResponse);
    }
    
    std::cout << " Frontend " << _frontendId << " stopping." << std::endl;
}

void FrontendItf::stop()
{
    
}