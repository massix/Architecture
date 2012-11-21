//
//  MessagesInjector.cpp
//  Architecture
//
//  Created by Massimo Gengarelli on 18/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

#include "StandardMessage.pb.h"

#include <string>
#include <iostream>

#include <boost/program_options.hpp>
#include <zmq.hpp>

using namespace boost::program_options;
using namespace ReceptorMessages;

void exitWithMsg(const std::string& iErrorMsg, const options_description& iOptsDesc)
{
    std::cerr << iErrorMsg << std::endl;
    std::cout << iOptsDesc << std::endl;
    
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    options_description anOptionsDesc("Allowed options");
    anOptionsDesc.add_options()
        ("help,h", "Produces help message")
        ("route,r", value<std::string>(), "Sets Receptor's address (Mandatory!)")
        ("message-type,m", value<std::string>(), "Sets message type (Mandatory)")
        ("user-id,u", value<uint16_t>(), "Sets user id (Mandatory)")
        ("password,p", value<std::string>(), "Sets sha password (Mandatory)")
        ("options,o", value<std::string>(), "Sets options");
    
    std::string aRouterAddress;
    std::string aMsgType;
    uint16_t    anUserId;
    std::string aPassword;
    std::string anOptions;
        
    variables_map aVariablesMap;
    store(parse_command_line(argc, argv, anOptionsDesc), aVariablesMap);
    
    if (!aVariablesMap.count("route")) exitWithMsg("route option is mandatory!", anOptionsDesc);
    else aRouterAddress = aVariablesMap["route"].as<std::string>();
    
    if (!aVariablesMap.count("message-type")) exitWithMsg("message-type option is mandatory!", anOptionsDesc);
    else aMsgType = aVariablesMap["message-type"].as<std::string>();
    
    if (!aVariablesMap.count("user-id")) exitWithMsg("user-id option is mandatory!", anOptionsDesc);
    else anUserId = aVariablesMap["user-id"].as<uint16_t>();
    
    if (!aVariablesMap.count("password")) exitWithMsg("password option is mandatory!", anOptionsDesc);
    else aPassword = aVariablesMap["password"].as<std::string>();
    
    if (aVariablesMap.count("options")) anOptions = aVariablesMap["options"].as<std::string>();
    
    std::cout << " Sending message: " << aMsgType << " to " << aRouterAddress << std::endl;
    
    // Send message and gets reply
    zmq::context_t aZMQContext(1);
    zmq::socket_t aZMQSocket(aZMQContext, ZMQ_REQ);
    
    aZMQSocket.connect(aRouterAddress.c_str());
    
    BaseMessage aBaseMessage;
    aBaseMessage.set_messagetype(aMsgType);
    aBaseMessage.set_userid(anUserId);
    aBaseMessage.set_shapassword(aPassword);
    aBaseMessage.set_options(anOptions);
    
    std::string aSerializedMessage;
    aBaseMessage.SerializeToString(&aSerializedMessage);
    
    zmq::message_t aRequest(aSerializedMessage.size());
    memcpy(aRequest.data(), aSerializedMessage.c_str(), aSerializedMessage.size());
    aZMQSocket.send(aRequest);
    
    std::cout << "Message sent" << std::endl;
    
    // Get the reply
    zmq::message_t aReply;
    aZMQSocket.recv(&aReply);
    
    std::string aResponseReceived((const char*) aReply.data());

    // This part will be completed once the Receptor will send back Protobuf messages
    std::cout << "Received reply: " << aResponseReceived << std::endl;
    
    return 0;
}
