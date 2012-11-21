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

#include "FrontendItf.h"
#include "Receptor.h"
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
    std::cout << "Client received back: " << aResponse << std::endl;
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
    sendMessage(aBaseMessage, "MOVE", aZMQSocket, aZMQRequest);
    sendMessage(aBaseMessage, "CONTEXT", aZMQSocket, aZMQRequest);
    sendMessage(aBaseMessage, "QUIT", aZMQSocket, aZMQRequest);
    
    aZMQSocket.close();
    
    return 0;
}

int main(int argc, char *argv[])
{
    pthread_t aLoginFrontendThread;
    pthread_t aMoveFrontendThread;
    pthread_t aContextFrontendThread;
    pthread_t aReceptorThread;
    pthread_t aClientThread;
    
    // Start the Frontends
    pthread_create(&aLoginFrontendThread, 0, &frontendThread, (void*) "Login");
    pthread_create(&aMoveFrontendThread, 0, &frontendThread, (void*) "Context");
    pthread_create(&aContextFrontendThread, 0, &frontendThread, (void*) "Move");
    
    // Start the Receptor
    pthread_create(&aReceptorThread, 0, &receptorThread, 0);
    
    // Start the client
    pthread_create(&aClientThread, 0, &clientThread, 0);
    
    pthread_join(aClientThread, 0);
    pthread_join(aReceptorThread, 0);
    pthread_join(aContextFrontendThread, 0);
    pthread_join(aMoveFrontendThread, 0);
    pthread_join(aLoginFrontendThread, 0);
    
    return 0;
}
