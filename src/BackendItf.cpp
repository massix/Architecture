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

#include "StandardMessage.pb.h"
#include <BackendItf.h>
#include <zmq.hpp>
#include <string>
#include <unistd.h>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <protobuf/message.h>
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
    _socket(_context, ZMQ_DEALER)
{
    int64_t aVersionMajor = __BACKEND_VERSION__ >> 16;
    int64_t aVersionMinor = (__BACKEND_VERSION__ >> 8) & 0x00FF;
    int64_t aVersionRelease = __BACKEND_VERSION__ & 0x0000FF;
    
    LOG_MSG("Using backend library version: " +
            boost::lexical_cast<std::string>(aVersionMajor) + "." +
            boost::lexical_cast<std::string>(aVersionMinor) + "." +
            boost::lexical_cast<std::string>(aVersionRelease));
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

    _variables.clear();

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

                    else if (aChildIterator->first == "variable") {
                        std::string aName(aChildIterator->second.get<std::string>("<xmlattr>.name"));
                        std::string aValue(aChildIterator->second.get<std::string>("<xmlattr>.value"));
                        _variables[aName] = aValue;
                        LOG_MSG("Found variable " + aName + " = " + aValue);aChildIterator->second.get<std::string>("<xmlattr>.value");
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

void BackendItf::clientCall(const google::protobuf::Message& iRequest)
{
    LOG_MSG("Serializing client request:\n" + iRequest.DebugString());
    std::string aSerializaedRequest = iRequest.SerializeAsString();
}

void BackendItf::reply(const google::protobuf::Message &iReply)
{
    ReceptorMessages::ResponseMessage aResponseMessage;
    aResponseMessage.set_messagetype(_handledMessage);
    aResponseMessage.set_serializedresponseheader(_handledHeader);
    aResponseMessage.set_serializedmessage(iReply.SerializeAsString());
    
    std::string aSerializedMessage(aResponseMessage.SerializeAsString());
    zmq::message_t aZMQResponse(aSerializedMessage.size());
    memcpy(aZMQResponse.data(), aSerializedMessage.data(), aSerializedMessage.size());
 
    LOG_MSG("Replying to the client\n" + aResponseMessage.DebugString());
    _socket.send(aZMQResponse);
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
                aRequestMessage.add_othermessages(_config._messagesHandled[i]);
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
                
                ReceptorMessages::BackendResponseMessage aResponseMessage;
                aResponseMessage.ParseFromArray(aResponse.data(), (int32_t) aResponse.size());
                _handledMessage = aResponseMessage.messagetype();
                _handledHeader = aResponseMessage.serializedheader();
                
                if (aResponseMessage.messagetype() == "EMPTY") handleNoMessages();
                else handleMessage(aResponseMessage);
                
                break;
            }
            else handlePollTimeout();
        }
    }
}

const std::string BackendItf::getValueForVariable(const std::string & iVariable) const
{
    std::string aReturnValue;
    std::map<std::string, std::string>::const_iterator anIterator;
    anIterator = _variables.find(iVariable);

    if (anIterator != _variables.end())
        aReturnValue = anIterator->second;

    return aReturnValue;
}
