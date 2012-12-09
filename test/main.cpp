//
//  main.cpp
//  Receptor
//
//  Created by Massimo Gengarelli on 10/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <zmq.hpp>
#include <zmq_utils.h>
#include <exception>
#include <pthread.h>
#include <stdint.h>
#include <signal.h>
#include <Log.h>
#include <ctime>
#include <protobuf/message.h>

#include "BackendItf.h"
#include "FrontendItf.h"
#include "Receptor.h"
#include "BackendItf.h"
#include "StandardMessage.pb.h"

void sendMessage(zmq::socket_t& ioSocket, const std::string& iMessage, ReceptorMessages::ResponseMessage& oMessage) {
    zmq::message_t aMessage(iMessage.size());
    memcpy(aMessage.data(), iMessage.c_str(), iMessage.size());
    ioSocket.send(aMessage);
    
    zmq::message_t aResponse;
    ioSocket.recv(&aResponse);
    
    oMessage.Clear();
    oMessage.ParseFromString((const char *) aResponse.data());
    
    LOG_MSG("Client finished conversation, receiving back: " + oMessage.messagetype());
}

void sendProtobufMessage(zmq::socket_t& ioSocket, const google::protobuf::Message* const iMessage, ReceptorMessages::ResponseMessage& oMessage) {
    std::string aSerializedMessage;
    iMessage->SerializeToString(&aSerializedMessage);
    
    sendMessage(ioSocket, aSerializedMessage, oMessage);
}

void* frontendThread(void* ioFeId)
{
    std::string aFrontendId((const char*) ioFeId);
    FrontendItf aFrontend(aFrontendId);
    aFrontend.configure();
    aFrontend.start();
    
    return 0;
}

void* receptorThread(void* ioArgs)
{
    Receptor& aReceptor = Receptor::GetInstance();
    aReceptor.printConfiguration();
    aReceptor.startServer();
    
    return 0;
}

void* clientThread(void *ioArgs)
{
    sleep(4);
    zmq::context_t aContext(1);
    zmq::socket_t aZMQSocket(aContext, ZMQ_REQ);
    sleep(6);
    aZMQSocket.connect("tcp://localhost:9000");
    
    zmq::message_t aZMQRequest(0);
    
    // Prepare a Global Message
    ReceptorMessages::BaseMessage aGlobalBaseMessage;
    ReceptorMessages::ResponseMessage aResponseMessage;

    aGlobalBaseMessage.set_userid(17);
    aGlobalBaseMessage.set_shapassword("Password");
    
    
    // Send DATE request
    ReceptorMessages::DateRequest aDateRequest;
    ReceptorMessages::DateResponse aDateResponse;
    aGlobalBaseMessage.set_messagetype("DATE");
    
    aDateRequest.set_format("YYYY-MM-DD");
    std::string aSerializedDateRequest = aDateRequest.SerializeAsString();
    aGlobalBaseMessage.set_options(aSerializedDateRequest);
    sendProtobufMessage(aZMQSocket, &aGlobalBaseMessage, aResponseMessage);
    aDateResponse.ParseFromString(aResponseMessage.serializedmessage());
    LOG_MSG("Client received response: " + aDateResponse.date());
    
    sleep(5);
    
    
    // Send USERS message
    ReceptorMessages::UsersRequest aUsersRequestMessage;
    ReceptorMessages::UsersResponse aUsersResponse;
    aGlobalBaseMessage.set_messagetype("USERS");
    
    aUsersRequestMessage.set_user("test");
    std::string aSerializedUsersRequest = aUsersRequestMessage.SerializeAsString();
    aGlobalBaseMessage.set_options(aSerializedUsersRequest);
    sendProtobufMessage(aZMQSocket, &aGlobalBaseMessage, aResponseMessage);
    aUsersResponse.ParseFromString(aResponseMessage.serializedmessage());
    LOG_MSG("Client received response: " + aUsersResponse.infos());
    
    sleep(5);

    
    aZMQSocket.close();
    
    return 0;
}

void* backendThread(void *ioArgs)
{
    std::string aBackendName((const char*) ioArgs);
    
    struct MyBackend : public BackendItf {
        MyBackend(const std::string& iBackendName) : BackendItf(iBackendName) {};
        virtual ~MyBackend() {};
        virtual bool handleMessage(const ReceptorMessages::BackendResponseMessage& iMessage) {
            if (iMessage.messagetype() == "DATE") {
                LOG_MSG("Handling DATE message");
                ReceptorMessages::DateRequest aDateRequestMsg;
                aDateRequestMsg.ParseFromString(iMessage.serializedmessage());
                LOG_MSG("Requested format: " + aDateRequestMsg.format());

                std::time_t aTime;
                aTime = time(0);

                std::string aResponse(ctime(&aTime));
                ReceptorMessages::DateResponse aDateResponseMsg;
                aDateResponseMsg.set_date(aResponse);
                
                reply(aDateResponseMsg);
            }
            
            if (iMessage.messagetype() == "USERS") {
                LOG_MSG("Handling USERS message");
                ReceptorMessages::UsersRequest anUsersRequestMsg;
                anUsersRequestMsg.ParseFromString(iMessage.serializedmessage());
                LOG_MSG("Requested user: " + anUsersRequestMsg.user());
                
                ReceptorMessages::UsersResponse aResponse;
                aResponse.set_infos("Not logged in");
                reply(aResponse);
            }

            return true;
        }
        virtual bool handlePollTimeout() {
            LOG_MSG("Handle PollTimeout called");
            return true;
        }
        virtual bool handleNoMessages() {
            LOG_MSG("HandleNoMessage called");
            sleep(getPollTimeout());
            return true;
        }
    } aBackend(aBackendName);
    
    aBackend.configure();
    aBackend.start();

    return 0;
}

int main(int argc, char *argv[])
{
    pthread_t aDateUsersFrontendThread;
    pthread_t aReceptorThread;
    pthread_t aClientThread;
    pthread_t aDateBackendThread;

    // Start the Frontend
    pthread_create(&aDateUsersFrontendThread, 0, &frontendThread, (void*) "DateUsers");
    
    sleep(3);
    
    // Start the Backend
    pthread_create(&aDateBackendThread, 0, &backendThread, (void*) "HandleDate");
    
    // Start the Receptor
    pthread_create(&aReceptorThread, 0, &receptorThread, 0);
    
    // Start the client
    pthread_create(&aClientThread, 0, &clientThread, 0);

    // Wait for the client to finish
    pthread_join(aClientThread, 0);
    
    // Kill the other processes
    pthread_kill(aReceptorThread, SIGINT);
    pthread_join(aReceptorThread, 0);
    
    pthread_kill(aDateUsersFrontendThread, SIGINT);
    pthread_join(aDateUsersFrontendThread, 0);
    
    pthread_kill(aDateBackendThread, SIGINT);
    pthread_join(aDateBackendThread, 0);

    return 0;
}
