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
#include <Log.h>

using namespace boost::program_options;
using namespace ReceptorMessages;

typedef enum {
    kDate = 0,
    kUsers = 1,
    kLogin = 2,
    kRegister = 3
} MessageType_t;

BaseMessage createMessage(
    const MessageType_t& iMsgType,
    const std::vector<std::string>& iArgs)
{
    BaseMessage aRetValue;
    std::string aSerializedRequest;

    switch (iMsgType) {
        case kDate:
        {
            aRetValue.set_messagetype("DATE");
            DateRequest aDateRequest;
            aDateRequest.set_format(iArgs[0]);
            aSerializedRequest = aDateRequest.SerializeAsString();
        }
            break;
        case kUsers:
        {
            aRetValue.set_messagetype("USERS");
            UsersRequest anUsersRequest;
            anUsersRequest.set_user(iArgs[0]);
            aSerializedRequest = anUsersRequest.SerializeAsString();
        }
            break;
        case kLogin:
        {
            aRetValue.set_messagetype("LOGIN");
            Login aLoginRequest;
            aLoginRequest.set_login(iArgs[0]);
            aLoginRequest.set_password(iArgs[1]);
            aSerializedRequest = aLoginRequest.SerializeAsString();
        }
            break;
        case kRegister:
        {
            aRetValue.set_messagetype("REGSTR");
            Registration aRegistrationRequest;
            aRegistrationRequest.set_login(iArgs[0]);
            aRegistrationRequest.set_password(iArgs[1]);
            aRegistrationRequest.set_email(iArgs[2]);
            aSerializedRequest = aRegistrationRequest.SerializeAsString();
        }
            break;
    }

    aRetValue.set_options(aSerializedRequest);
    return aRetValue;
}

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
        ("options,o", value<std::vector<std::string> >(), "Sets options (Mandatory)");
    
    std::string aRouterAddress;
    std::string aMsgType;
    uint16_t    anUserId = 1;
    std::string aPassword("blabla");
    std::vector<std::string> anOptions;
        
    variables_map aVariablesMap;
    store(parse_command_line(argc, argv, anOptionsDesc), aVariablesMap);
    
    if (!aVariablesMap.count("route")) exitWithMsg("route option is mandatory!", anOptionsDesc);
    else aRouterAddress = aVariablesMap["route"].as<std::string>();
    
    if (!aVariablesMap.count("message-type")) exitWithMsg("message-type option is mandatory!", anOptionsDesc);
    else aMsgType = aVariablesMap["message-type"].as<std::string>();
    
    if (!aVariablesMap.count("options")) exitWithMsg("options are mandatory!", anOptionsDesc);
    else anOptions = aVariablesMap["options"].as<std::vector<std::string> >();

    // Send message and gets reply
    zmq::context_t aZMQContext(1);
    zmq::socket_t aZMQSocket(aZMQContext, ZMQ_REQ);
    
    aZMQSocket.connect(aRouterAddress.c_str());
    
    BaseMessage aBaseMessage;
    if (aMsgType == "DATE") aBaseMessage = createMessage(kDate, anOptions);
    else if (aMsgType == "USERS") aBaseMessage = createMessage(kUsers, anOptions);
    else if (aMsgType == "LOGIN") aBaseMessage = createMessage(kLogin, anOptions);
    else if (aMsgType == "REGSTR") aBaseMessage = createMessage(kRegister, anOptions);
    else aBaseMessage.set_messagetype(aMsgType);

    aBaseMessage.set_userid(anUserId);
    aBaseMessage.set_shapassword(aPassword);
    
    std::string aSerializedMessage;
    aBaseMessage.SerializeToString(&aSerializedMessage);
    
    LOG_MSG("Sending message\n" + aBaseMessage.DebugString() + "\nto " + aRouterAddress);

    zmq::message_t aRequest(aSerializedMessage.size());
    memcpy(aRequest.data(), aSerializedMessage.c_str(), aSerializedMessage.size());
    aZMQSocket.send(aRequest);
    
    LOG_MSG("Message sent");
    
    // Get the reply
    zmq::message_t aReply;
    aZMQSocket.recv(&aReply);
    
    ResponseMessage aResponseMessage;
    aResponseMessage.ParseFromArray(aReply.data(), (uint32_t) aReply.size());

    LOG_MSG("Received reply:\n" + aResponseMessage.DebugString());
    
    return 0;
}
