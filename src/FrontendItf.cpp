//
//  FrontendItf.cpp
//  Architecture
//
//  Created by Massimo Gengarelli on 17/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

#include "FrontendItf.h"
#include "MessageQueue.h"
#include "StandardMessage.pb.h"

#include <string>
#include <zmq.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <Log.h>

#define kMemBM   "BackendMap"
#define kMemST   "SonStatus"

#define bforeach BOOST_FOREACH

class Callable
{
public:
    Callable (boost::shared_ptr<BackendMap>& iMap, boost::shared_ptr<bool>& iStatus) :
        _map(iMap), _status(iStatus) {};
    virtual ~Callable() {};
    void operator()() {
        while (*_status) {
            sleep(3);
            std::cout << " SON: Map size: " << _map->size() << std::endl;
            bforeach(const BackendMap::value_type& aPair, (*_map))
            {
                std::cout << " SON: Key  " << aPair.first << std::endl;
                std::cout << " SON: Size " << aPair.second.size() << std::endl;
            }
        }
    }
    
    boost::shared_ptr<BackendMap>& _map;
    boost::shared_ptr<bool>& _status;
};

FrontendItf::FrontendItf(const std::string& iId) :
    _frontendId(iId),
    _confXml(_frontendId + "FE.xml"),
    _zmqContext(1),
    _zmqSocket(_zmqContext, ZMQ_REP),
    _beSocket(_zmqContext, ZMQ_REP),
    _port(0),
    _bePort(0),
    _map(new BackendMap),
    _sonStatus(new bool(false))
{    
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

void FrontendItf::startBackendListener()
{
    Callable aCallable(_map, _sonStatus);
    boost::thread aThread(aCallable);
}

void FrontendItf::start()
{
    std::string aZMQString("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_port));
    std::string aBEPort("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_bePort));
    
    LOG_MSG("Bound to: " + aZMQString);
    LOG_MSG("Receiving BEs: " + aBEPort);
    
    std::cout << "_map: " << _map << std::endl;
    
    *_sonStatus = true;
    
    startBackendListener();
    
    _zmqSocket.bind(aZMQString.c_str());
    _beSocket.bind(aBEPort.c_str());
    
    while (true) {
        zmq::message_t aZMQMessage;
        _zmqSocket.recv(&aZMQMessage);
        std::string aStringMessage((const char*) aZMQMessage.data(), aZMQMessage.size());
        if ("QUIT" == aStringMessage) {
            *_sonStatus = false;
            break;
        }
        
        ReceptorMessages::BaseMessage aRecvMessage;
        aRecvMessage.ParseFromString(aStringMessage);
        
        const std::string aMsgType(aRecvMessage.messagetype());

        std::cout << "Storing message: " << aMsgType << std::endl;

        /* TODO: find out why this isn't working */
        MessageQueue& aMsgQ = (*_map)[aMsgType];
        aMsgQ.enqueueMessage(aStringMessage);
     
        std::cout << "Frontend enqueued message: " << aRecvMessage.messagetype() << std::endl;
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
