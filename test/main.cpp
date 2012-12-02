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
#include <exception>
#include <pthread.h>
#include <stdint.h>
#include <Log.h>

#include "BackendItf.h"
#include "FrontendItf.h"
#include "Receptor.h"
#include "BackendItf.h"
#include "StandardMessage.pb.h"

void craftMessage(ReceptorMessages::BaseMessage& ioMessage, const std::string& iMessage)
{
    ioMessage.Clear();

    ioMessage.set_messagetype(iMessage);
    ioMessage.set_userid(17);
    ioMessage.set_shapassword("shaPassword");
    ioMessage.set_options("MVT+1'GEN+14:123");
}

void encapsulateMessage(zmq::message_t& ioMessage, const ReceptorMessages::BaseMessage& iMessage)
{
    std::string aSerializedMessage;
    iMessage.SerializeToString(&aSerializedMessage);
    ioMessage.rebuild(aSerializedMessage.size());
    memcpy((void *) ioMessage.data(), aSerializedMessage.c_str(), aSerializedMessage.size());
}

void sendAndReceive(zmq::socket_t& ioSocket, zmq::message_t& ioMessage)
{
    ioSocket.send(ioMessage);
    ioSocket.recv(&ioMessage);
    
    std::string aResponse((const char*)ioMessage.data(), 3);
    LOG_MSG("Client received back: " + aResponse);
    assert(aResponse == "ACK");
}

void sendMessage(
    ReceptorMessages::BaseMessage& ioMessage,
    const std::string& iMessage,
    zmq::socket_t& ioSocket,
    zmq::message_t& ioZMessage)
{
    craftMessage(ioMessage, iMessage);
    encapsulateMessage(ioZMessage, ioMessage);
    sendAndReceive(ioSocket, ioZMessage);
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
    
    ReceptorMessages::BaseMessage aBaseMessage;
    zmq::message_t aZMQRequest(0);
    
    sendMessage(aBaseMessage, "LOGIN", aZMQSocket, aZMQRequest);
    sleep(1);
    
    sendMessage(aBaseMessage, "MOVE", aZMQSocket, aZMQRequest);
    sleep(1);
    
    sendMessage(aBaseMessage, "DATE", aZMQSocket, aZMQRequest);
    sleep(5);
    
    sendMessage(aBaseMessage, "QUIT", aZMQSocket, aZMQRequest);
    sleep(1);
    
    aZMQSocket.close();
    
    return 0;
}

void* backendThread(void *ioArgs)
{
    std::string aBackendName((const char*) ioArgs);
    
    struct MyBackend : public BackendItf {
        MyBackend(const std::string& iBackendName) : BackendItf(iBackendName) {};
        virtual ~MyBackend() {};
        virtual bool handleMessage(const std::string& iSerializedMessage) {
            LOG_MSG("MyBackend received a message");
            sleep(getPollTimeout());
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
    pthread_t aLoginFrontendThread;
    pthread_t aMoveFrontendThread;
    pthread_t aDateUsersFrontendThread;
    pthread_t aReceptorThread;
    pthread_t aClientThread;
    pthread_t aDateBackendThread;

    // Start the Frontends
    pthread_create(&aLoginFrontendThread, 0, &frontendThread, (void*) "Login");
    pthread_create(&aMoveFrontendThread, 0, &frontendThread, (void*) "Move");
    pthread_create(&aDateUsersFrontendThread, 0, &frontendThread, (void*) "DateUsers");
    
    sleep(3);
    
    // Start the Backend
    pthread_create(&aDateBackendThread, 0, &backendThread, (void*) "HandleDate");
    
    // Start the Receptor
    pthread_create(&aReceptorThread, 0, &receptorThread, 0);
    
    // Start the client
    pthread_create(&aClientThread, 0, &clientThread, 0);
    
    pthread_join(aClientThread, 0);
    pthread_join(aReceptorThread, 0);
    pthread_join(aDateUsersFrontendThread, 0);
    pthread_join(aMoveFrontendThread, 0);
    pthread_join(aLoginFrontendThread, 0);
    pthread_join(aDateBackendThread, 0);
    
    return 0;
}
