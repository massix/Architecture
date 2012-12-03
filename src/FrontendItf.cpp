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

struct BackendReceptorThread
{
    BackendReceptorThread (
        boost::shared_ptr<BackendMap>& iMap,
        boost::shared_ptr<bool>& iStatus,
        std::vector<ReceptorMessages::ResponseMessage>& ioVector) :
        _map(iMap), _status(iStatus), _responses(ioVector) {};
    virtual ~BackendReceptorThread() {};
    void operator()() {
        zmq::context_t aContext(1);
        zmq::socket_t aSocket(aContext, ZMQ_REP);
        std::string aBEPort("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_port));
        aSocket.bind(aBEPort.c_str());
        
        LOG_MSG(_frontendId + " receiving Backends on: " + aBEPort);
        
        while (*_status) {
            zmq::pollitem_t aPollItems[] = {
                { aSocket, 0, ZMQ_POLLIN, 0 }
            };

            /* Expects address of first socket */
            zmq::poll(&aPollItems[0], 1, 3 * (1000 * 1000));
            
            if (aPollItems[0].revents & ZMQ_POLLIN) {
                LOG_MSG(_frontendId + " processing Backend request");
                zmq::message_t aMessage;
                aSocket.recv(&aMessage);
                
                /* FIXME: These two messages have to be distinguished */
                ReceptorMessages::BaseMessage aBaseMessage;
                ReceptorMessages::BackendRequestMessage aRequestMessage;
                ReceptorMessages::ResponseMessage aGotResponseMessage;
                
                aBaseMessage.ParseFromString((const char *) aMessage.data());
                
                // Backend asks for new messages
                if (aBaseMessage.messagetype() == "REQUEST") {
                    LOG_MSG(_frontendId + " backend is asking for new messages");
                    aRequestMessage.ParseFromString(aBaseMessage.options());
                    
                    // Prepare for reply
                    ReceptorMessages::BackendResponseMessage aResponseMessage;
                    aResponseMessage.set_messagetype("EMPTY");
                    aResponseMessage.set_serializedmessage("EMPTY");
                    
                    if (_map->find(aRequestMessage.messagetype()) != _map->end()) {
                        MessageQueue& aMsgQ = (*_map)[aRequestMessage.messagetype()];
                        std::string anEncodedMsg = aMsgQ.dequeueMessage();
                        if (aMsgQ.empty()) _map->erase(aRequestMessage.messagetype());
                        
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
                                if ((*_map)[aMessage].empty()) _map->erase(aMessage);
                                
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
                
                // Backend finished processing a message
                else {
                    aGotResponseMessage.ParseFromString(std::string((const char *) aMessage.data()));
                    LOG_MSG(_frontendId + " received response " + aGotResponseMessage.messagetype() + " from backend");
                    // TODO: and here?!
                    _responses.push_back(aGotResponseMessage);
                    LOG_MSG("Response put in vector, new size is: " + boost::lexical_cast<std::string>(_responses.size()));
                    
                    // Send fake response to BE
                    zmq::message_t aFakeResponseForBe(3);
                    memcpy(aFakeResponseForBe.data(), "OK", 2);
                    aSocket.send(aFakeResponseForBe);
                }
            }
        }
    }
    
    boost::shared_ptr<BackendMap>& _map;
    boost::shared_ptr<bool>& _status;
    std::string _frontendId;
    std::string _hostname;
    std::vector<ReceptorMessages::ResponseMessage>& _responses;
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
    LOG_MSG("Configuring FE " + _frontendId + " from " + _confXml);
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
    BackendReceptorThread aCallable(_map, _sonStatus, _responses);
    aCallable._hostname = _hostname;
    aCallable._port = _bePort;
    aCallable._frontendId = _frontendId;
    boost::thread aThread(aCallable);
}

void FrontendItf::start()
{
    std::string aZMQString("tcp://" + _hostname + ":" + boost::lexical_cast<std::string>(_port));
    
    LOG_MSG(_frontendId + " bound to: " + aZMQString);
        
    *_sonStatus = true;
    
    startBackendListener();
    
    _zmqSocket.bind(aZMQString.c_str());
    
    while (true) {
        zmq::message_t aZMQMessage;
        _zmqSocket.recv(&aZMQMessage);
        std::string aStringMessage((const char*) aZMQMessage.data(), aZMQMessage.size());

        ReceptorMessages::BaseMessage aRecvMessage;
        aRecvMessage.ParseFromString(aStringMessage);
        
        const std::string aMsgType(aRecvMessage.messagetype());

        std::size_t anOldSize = _responses.size();
        
        LOG_MSG(_frontendId + " storing message: " + aMsgType);
        MessageQueue& aMsgQ = (*_map)[aMsgType];
        aMsgQ.enqueueMessage(aRecvMessage.options());
        LOG_MSG(_frontendId + " enqueued message: " + aRecvMessage.messagetype());
        LOG_MSG(_frontendId + " waiting for response from backend..");

        while (_responses.size() == anOldSize) {/* do nothing */}
        // Send a Reply to the Receptor
        std::string aReply(_responses.back().SerializeAsString());
        _responses.pop_back();
        zmq::message_t aResponse(aReply.size());
        memcpy(aResponse.data(), aReply.c_str(), aReply.size());
        
        LOG_MSG(_frontendId + " sending reply");
        LOG_MSG(_frontendId + " _responses size: " + boost::lexical_cast<std::string>(_responses.size()));
        
        _zmqSocket.send(aResponse);
    }
    
    LOG_MSG("Frontend " + _frontendId + " stopping");
}

void FrontendItf::stop()
{
    
}
