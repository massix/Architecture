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
        zmq::socket_t& iMainSocket) :
        _map(iMap), _status(iStatus), _mainSocket(iMainSocket) {};
    virtual ~BackendReceptorThread() {};
    void operator()() {
        zmq::context_t aContext(1);
        zmq::socket_t aSocket(aContext, ZMQ_ROUTER);
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
                zmq::message_t anHeader;
                zmq::message_t aSeparator;
                zmq::message_t aMessage;
                aSocket.recv(&anHeader, ZMQ_RCVMORE); // HEADER, save it.
                aSocket.recv(&aMessage); // Actual Payload
                
                ReceptorMessages::BaseMessage aBaseMessage;
                ReceptorMessages::BackendRequestMessage aRequestMessage;
                ReceptorMessages::ResponseMessage aGotResponseMessage;
                
                aBaseMessage.ParseFromString((const char *) aMessage.data());
                
                // Backend asks for new messages
                if (aBaseMessage.messagetype() == "REQUEST") {
                    LOG_MSG(_frontendId + " backend is asking for new messages");
                    aRequestMessage.ParseFromString(aBaseMessage.options());
                    LOG_MSG(_frontendId + " received request:\n" + aRequestMessage.DebugString());
                    
                    // Prepare for reply
                    ReceptorMessages::BackendResponseMessage aResponseMessage;
                    aResponseMessage.set_messagetype("EMPTY");
                    aResponseMessage.set_serializedmessage("EMPTY");
                    
                    if (_map->find(aRequestMessage.messagetype()) != _map->end()) {
                        MessageQueue& aMsgQ = (*_map)[aRequestMessage.messagetype()];
                        std::string anEncodedMsg = aMsgQ.dequeueMessage();
                        std::string anHeader = aMsgQ.dequeueHeader();
                        
                        if (aMsgQ.empty()) _map->erase(aRequestMessage.messagetype());
                        
                        if (anEncodedMsg != "") {
                            aResponseMessage.set_messagetype(aRequestMessage.messagetype());
                            aResponseMessage.set_serializedmessage(anEncodedMsg);
                            aResponseMessage.set_serializedheader((void *) anHeader.data(), anHeader.size());
                        }
                    }
                    
                    // We might have other messages that BE is able to handle !
                    else if (aRequestMessage.othermessages().size() > 0) {
                        bforeach(const std::string& aMessage, aRequestMessage.othermessages()) {
                            if (_map->find(aMessage) != _map->end()) {
                                std::string anEncodedMsg = (*_map)[aMessage].dequeueMessage();
                                std::string anHeader = (*_map)[aMessage].dequeueHeader();
                                
                                if ((*_map)[aMessage].empty()) _map->erase(aMessage);
                                
                                if (anEncodedMsg != "") {
                                    aResponseMessage.set_messagetype(aMessage);
                                    aResponseMessage.set_serializedmessage(anEncodedMsg);
                                    aResponseMessage.set_serializedheader((void *) anHeader.data(), anHeader.size());
                                }
                                break;
                            }
                        }
                    }
                    
                    std::string aSerializedResponse = aResponseMessage.SerializeAsString();
                    
                    zmq::message_t aZmqResponse(aSerializedResponse.size());
                    memcpy((void *) aZmqResponse.data(), aSerializedResponse.data(), aSerializedResponse.size());
                    
                    LOG_MSG(_frontendId + " answering back to the backend (" + aResponseMessage.messagetype() + ")");
                    aSocket.send(anHeader, ZMQ_SNDMORE);
                    aSocket.send(aZmqResponse);
                }
                
                // Backend finished processing a message
                else {
                    aGotResponseMessage.ParseFromArray(aMessage.data(), (int32_t) aMessage.size());
                    std::string anHeader(
                        static_cast<const char*>(aGotResponseMessage.serializedresponseheader().data()),
                        aGotResponseMessage.serializedresponseheader().size());
                    LOG_MSG(_frontendId + " received response " + aGotResponseMessage.messagetype() + " from backend");
                    LOG_MSG("\n" + aGotResponseMessage.DebugString());
                    
                    zmq::message_t aResponseHeader(anHeader.size());
                    zmq::message_t aResponseEmptySeparator;
                    memcpy(aResponseHeader.data(), anHeader.data(), anHeader.size());
                    
                    // Reply using the right header 
                    _mainSocket.send(aResponseHeader, ZMQ_SNDMORE);
                    _mainSocket.send(aResponseEmptySeparator, ZMQ_SNDMORE);
                    _mainSocket.send(aMessage);
                }
            }
        }
    }
    
    boost::shared_ptr<BackendMap>& _map;
    boost::shared_ptr<bool>& _status;
    std::string _frontendId;
    std::string _hostname;
    uint16_t _port;
    zmq::socket_t& _mainSocket;
};

FrontendItf::FrontendItf(const std::string& iId) :
    _frontendId(iId),
    _confXml(_frontendId + "FE.xml"),
    _zmqContext(1),
    _zmqSocket(_zmqContext, ZMQ_ROUTER),
    _port(0),
    _bePort(0),
    _map(new BackendMap),
    _sonStatus(new bool(false))
{
    int64_t aVersionMajor = (__FRONTEND_VERSION__ >> 16);
    int64_t aVersionMinor = (__FRONTEND_VERSION__ >> 8) & 0x00FF;
    int64_t aVersionRelease = __FRONTEND_VERSION__ & 0x0000FF;
    LOG_MSG("Frontend library version: " +
            boost::lexical_cast<std::string>(aVersionMajor) + "." +
            boost::lexical_cast<std::string>(aVersionMinor) + "." +
            boost::lexical_cast<std::string>(aVersionRelease));
    LOG_MSG("Configuring FE " + _frontendId + " from " + _confXml);
}

FrontendItf::~FrontendItf()
{
    _zmqSocket.close();
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
    BackendReceptorThread aCallable(_map, _sonStatus, _zmqSocket);
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
        zmq::message_t anHeader;
        zmq::message_t anEmptySeparator;
        zmq::message_t aZMQMessage;

        _zmqSocket.recv(&anHeader);
        _zmqSocket.recv(&anEmptySeparator);
        _zmqSocket.recv(&aZMQMessage);

        std::string aStringHeader((const char *) anHeader.data(), anHeader.size());
        std::string aStringMessage((const char*) aZMQMessage.data(), aZMQMessage.size());

        ReceptorMessages::BaseMessage aRecvMessage;
        aRecvMessage.ParseFromString(aStringMessage);
        
        const std::string aMsgType(aRecvMessage.messagetype());
        
        LOG_MSG(_frontendId + " storing message:\n " + aRecvMessage.DebugString());
        MessageQueue& aMsgQ = (*_map)[aMsgType];
        aMsgQ.enqueueMessage(aRecvMessage.options());
        aMsgQ.enqueueHeader(aStringHeader);
    }
    
    LOG_MSG("Frontend " + _frontendId + " stopping");
}

void FrontendItf::stop()
{
    
}
