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
#include <zmq_utils.h>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <Log.h>

#define bforeach BOOST_FOREACH

struct Callable
{
    Callable (boost::shared_ptr<BackendMap>& iMap, boost::shared_ptr<bool>& iStatus) :
        _map(iMap), _status(iStatus) {};
    virtual ~Callable() {};
    void operator()() {
        zmq::context_t aContext(1);
        zmq::socket_t aSocket(aContext, ZMQ_REP);
        std::string aBEPort("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_port));
        aSocket.bind(aBEPort.c_str());
        
        LOG_MSG("Receiving Backends on: " + aBEPort);
        
        while (*_status) {
            zmq::pollitem_t aPollItems[] = {
                {aSocket, 0, ZMQ_POLLIN, 0 }
            };
            
            /* Expects address of first socket */
            zmq::poll(&aPollItems[0], 1, 3 * (1000 * 1000));
            
            if (aPollItems[0].revents & ZMQ_POLLIN) {
                LOG_MSG("Processing Backend request");
                zmq::message_t aMessage;
                aSocket.recv(&aMessage);
                
                ReceptorMessages::BackendRequestMessage aRequestMessage;
                aRequestMessage.ParseFromString(std::string((const char *) aMessage.data()));
                
                // Prepare for reply
                ReceptorMessages::BackendResponseMessage aResponseMessage;
                aResponseMessage.set_messagetype("EMPTY");
                aResponseMessage.set_serializedmessage("EMPTY");
                
                if (_map->find(aRequestMessage.messagetype()) != _map->end()) {
                    MessageQueue& aMsgQ = (*_map)[aRequestMessage.messagetype()];
                    std::string anEncodedMsg = aMsgQ.dequeueMessage();
                    
                    if (anEncodedMsg != "") {
                        aResponseMessage.set_messagetype(aRequestMessage.messagetype());
                        aResponseMessage.set_serializedmessage(anEncodedMsg);
                    }
                }
                
                // We might have other messages that BE is able to handle !
                else if (aRequestMessage.othermessages().size() > 0) {
                    bforeach(const std::string& aMessage, aRequestMessage.othermessages()) {
                        if (_map->find(aMessage) != _map->end()) {
                            std::string anEncodedMsg = (*_map)[aMessage].dequeueMessage();
                            
                            if (anEncodedMsg != "") {
                                aResponseMessage.set_messagetype(aMessage);
                                aResponseMessage.set_serializedmessage(anEncodedMsg);
                            }
                            break;
                        }
                    }
                }
                
                std::string aSerializedResponse;
                aResponseMessage.SerializeToString(&aSerializedResponse);
                
                zmq::message_t aZmqResponse(aSerializedResponse.size());
                memcpy(aZmqResponse.data(), aSerializedResponse.c_str(), aSerializedResponse.size());
                aSocket.send(aZmqResponse);
            }
        }
    }
    
    boost::shared_ptr<BackendMap>& _map;
    boost::shared_ptr<bool>& _status;
    std::string _hostname;
    uint16_t _port;
};

FrontendItf::FrontendItf(const std::string& iId) :
    _frontendId(iId),
    _confXml(_frontendId + "FE.xml"),
    _zmqContext(1),
    _zmqSocket(_zmqContext, ZMQ_REP),
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
    aCallable._hostname = _hostname;
    aCallable._port = _bePort;
    boost::thread aThread(aCallable);
}

void FrontendItf::start()
{
    std::string aZMQString("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_port));
    
    LOG_MSG("Bound to: " + aZMQString);
        
    *_sonStatus = true;
    
    startBackendListener();
    
    _zmqSocket.bind(aZMQString.c_str());
    
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
