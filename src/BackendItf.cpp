/*
 * BackendItf.cpp
 *
 *  Created on: 21/nov/2012
 *      Author: massi
 */

#include "StandardMessage.pb.h"
#include <BackendItf.h>
#include <zmq.hpp>
#include <string>
#include <unistd.h>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <Log.h>

#define bforeach BOOST_FOREACH
#define seconds * 1000 * 1000

using std::string;
using zmq::context_t;
using zmq::socket_t;
using zmq::message_t;

BackendItf::BackendItf(const std::string& iBackend) :
    _backend(iBackend),
	_configured(false),
    _context(1),
    _socket(_context, ZMQ_REQ)
{

}

BackendItf::~BackendItf()
{

}

const bool BackendItf::isConfigured() const
{
	return _configured;
}

void BackendItf::configure() {
    using boost::property_tree::ptree;
    using namespace boost::property_tree::xml_parser;
    
    std::string anXmlFile(_backend + "BE.xml");
    try {
        LOG_MSG("Starting BE configuration at " + anXmlFile);
        ptree aPtree;
        read_xml(anXmlFile, aPtree);
        ptree::const_iterator aPtreeIterator = aPtree.begin();
        for (; aPtreeIterator != aPtree.end(); ++aPtreeIterator) {
            if (aPtreeIterator->first == "Backend") {
                _config._backendName = aPtreeIterator->second.get<std::string>("<xmlattr>.name");
                _config._backendOwner = aPtreeIterator->second.get<std::string>("<xmlattr>.owner");
                _config._backendOwnerMail = aPtreeIterator->second.get<std::string>("<xmlattr>.email");
                
                ptree aChild = aPtreeIterator->second;
                ptree::const_iterator aChildIterator = aChild.begin();
                for (; aChildIterator != aChild.end(); ++aChildIterator) {
                    if (aChildIterator->first == "frontend") {
                        _config._frontend = aChildIterator->second.get<std::string>("<xmlattr>.host");
                        _config._frontendPort = aChildIterator->second.get<uint32_t>("<xmlattr>.port");
                        
                        LOG_MSG("Frontend: " + _config._frontend + ":" +
                                boost::lexical_cast<std::string>(_config._frontendPort));
                    }
                    
                    else if (aChildIterator->first == "poll") {
                        _config._pollTimeoutSeconds = aChildIterator->second.get<uint32_t>("<xmlattr>.timeout");
                        LOG_MSG("Timeout: " + boost::lexical_cast<std::string>(_config._pollTimeoutSeconds));
                    }
                    
                    else if (aChildIterator->first == "message") {
                        _config._messagesHandled.push_back(aChildIterator->second.get<std::string>("<xmlattr>.type"));
                        LOG_MSG("Inserting message");
                    }
                }
            }
        }
        
        LOG_MSG("Be " + _config._backendName + " configured");
        _configured = true;
        
    }
    catch(...) {
        LOG_MSG("Error while configuring BE");
        _configured = false;
    }
    
}

void BackendItf::start()
{
    std::string aFrontendString("tcp://" + _config._frontend + ":" +
                                boost::lexical_cast<std::string>(_config._frontendPort));
    _socket.connect(aFrontendString.c_str());
    
    while (true) {
        LOG_MSG("Polling frontend");
        
        zmq::pollitem_t aPollItems [] = {
            { _socket, 0, ZMQ_POLLIN, 0 }
        };
        
        ReceptorMessages::BaseMessage aBaseRequestMessage;
        aBaseRequestMessage.set_messagetype("REQUEST");
        
        ReceptorMessages::BackendRequestMessage aRequestMessage;
        aRequestMessage.set_messagetype(_config._messagesHandled[0]);
        
        if (_config._messagesHandled.size() >= 2) {
            for (int i = 1; i < _config._messagesHandled.size(); i++) {
                aRequestMessage.set_othermessages(i-1, _config._messagesHandled[i]);
            }
        }
        
        std::string aSerializedRequest;
        aRequestMessage.SerializeToString(&aSerializedRequest);
        aBaseRequestMessage.set_options(aSerializedRequest);
        
        std::string aSerializedBaseRequestMessage;
        aBaseRequestMessage.SerializeToString(&aSerializedBaseRequestMessage);
        
        zmq::message_t aZmqRequest(aSerializedBaseRequestMessage.size());
        memcpy(aZmqRequest.data(), aSerializedBaseRequestMessage.c_str(), aSerializedBaseRequestMessage.size());
        
        _socket.send(aZmqRequest);
        
        while (true) {
            zmq::poll(&aPollItems[0], 1, _config._pollTimeoutSeconds seconds);
            if (aPollItems[0].revents & ZMQ_POLLIN) {
                LOG_MSG("Received something");
                zmq::message_t aResponse;
                _socket.recv(&aResponse);
                
                std::string aSerializedResponse((const char*) aResponse.data());
                ReceptorMessages::BackendResponseMessage aResponseMessage;
                aResponseMessage.ParseFromString(aSerializedResponse);

                std::string aBackendResponse;
                
                if (aResponseMessage.messagetype() == "EMPTY") handleNoMessages();
                else {
                    ReceptorMessages::ResponseMessage aBEResponseMessage;
                    aBEResponseMessage.set_messagetype("ERROR");

                    if (handleMessage(aResponseMessage.serializedmessage(), aBackendResponse)) {
                        aBEResponseMessage.set_messagetype(aResponseMessage.messagetype());
                        aBEResponseMessage.set_serializedmessage(aBackendResponse);
                    }

                    LOG_MSG("Sending " + aBEResponseMessage.messagetype() + " back to the FE");

                    // Send response back to the frontend
                    std::string aSerializedResponseForFrontend;
                    aBEResponseMessage.SerializeToString(&aSerializedResponseForFrontend);
                    zmq::message_t aZmqMessageForFrontend(aSerializedResponseForFrontend.size());
                    memcpy(aZmqMessageForFrontend.data(), aSerializedResponseForFrontend.c_str(), aSerializedResponseForFrontend.size());

                    _socket.send(aZmqMessageForFrontend);

                    // This is done just to keep the conversation alive !
                    zmq::message_t aFakeResponse;
                    _socket.recv(&aFakeResponse);
                }
                
                break;
            }
            else handlePollTimeout();
        }
    }
}
