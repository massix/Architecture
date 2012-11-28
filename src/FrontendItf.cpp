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
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <Log.h>

#define kMemBM   "BackendMap"
#define kMemST   "SonStatus"
#define bforeach BOOST_FOREACH

using namespace boost::interprocess;

FrontendItf::FrontendItf(const std::string& iId) :
    _frontendId(iId),
    _confXml(_frontendId + "FE.xml"),
    _zmqContext(1),
    _zmqSocket(_zmqContext, ZMQ_REP),
    _beSocket(_zmqContext, ZMQ_REP),
    _port(0),
    _bePort(0),
    _map(0),
    _sonStatus(0)
{
    shared_memory_object::remove(_frontendId.c_str());
    managed_shared_memory aSegment(create_only, _frontendId.c_str(), 65536);
    ShmemAllocator aMemAllocator(aSegment.get_segment_manager());
    
    aSegment.construct<BackendMap>(kMemBM)(std::less<std::string>(), aMemAllocator);
    aSegment.construct<bool>(kMemST)(false);
}

FrontendItf::~FrontendItf()
{
    shared_memory_object::remove(_frontendId.c_str());
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
    
    switch (fork())
    {
        // Child process
        case 0:
        {
            managed_shared_memory aSegment(open_only, _frontendId.c_str());
            _map = aSegment.find<BackendMap>(kMemBM).first;
            _sonStatus = aSegment.find<bool>(kMemST).first;
            
            while (*_sonStatus)
            {
                sleep(3);
                std::cout << " Map Size: " << _map->size() << std::endl;
                bforeach(const BackendMap::value_type& aPair, (*_map))
                {
                    std::cout << " Iterating ... " << aPair.first << std::endl;
                }
            }
        }
            break;
    }
}

void FrontendItf::start()
{
    std::string aZMQString("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_port));
    std::string aBEPort("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_bePort));
    
    LOG_MSG("Bound to: " + aZMQString);
    LOG_MSG("Receiving BEs: " + aBEPort);
    
    managed_shared_memory aSegmentForOpen(open_only, _frontendId.c_str());
    _map = aSegmentForOpen.find<BackendMap>(kMemBM).first;
    _sonStatus = aSegmentForOpen.find<bool>(kMemST).first;
    
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
