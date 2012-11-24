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
#include <Log.h>

FrontendItf::FrontendItf(const std::string& iId) :
    _frontendId(iId),
    _confXml(_frontendId + "FE.xml"),
    _zmqContext(1),
    _zmqSocket(_zmqContext, ZMQ_REP),
    _beSocket(_zmqContext, ZMQ_ROUTER),
    _port(0),
    _bePort(0)
{
	LOG_MSG("Initializing FE " + _frontendId);
}

FrontendItf::~FrontendItf()
{
    _zmqSocket.close();
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
                _bePort = aPtreeIterator->second.get<uint16_t>("<xmlattr>.recv");
            }
        }
    }
    catch (...) {
        
    }
}

FrontendItf& FrontendItf::reconfigure(const std::string iNewXml)
{
    _hostname.clear();
    _port = 0;
    _confXml = iNewXml;
    configure();
    return *this;
}

const std::string FrontendItf::getConfXml() const
{
    return _confXml;
}

void FrontendItf::start()
{
    std::string aZMQString("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_port));
    std::string aBEPort("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_bePort));

    LOG_MSG("Bound to: " + aZMQString);
    LOG_MSG("Receiving BEs: " + aBEPort);

    _zmqSocket.bind(aZMQString.c_str());
    _beSocket.bind(aBEPort.c_str());
    
    while (true) {
        zmq::message_t aZMQMessage;
        _zmqSocket.recv(&aZMQMessage);
        std::string aStringMessage((const char*) aZMQMessage.data(), aZMQMessage.size());
        if ("QUIT" == aStringMessage) break;
        
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
