//
//  RegistrationBackend.cpp
//  Architecture
//
//  Created by Massimo Gengarelli on 24/03/13.
//
//

#include "RegistrationBackend.h"
#include <StandardMessage.pb.h>
#include <zmq.hpp>
#include <Log.h>
#include <iostream>
#include <istream>
#include <fstream>

#include <sqlite3.h>

using std::ifstream;

bool RegistrationBackend::handleMessage(const ReceptorMessages::BackendResponseMessage &iMessage)
{
    // Handle user registration
    if (iMessage.messagetype() == "REGSTR")
    {
        LOG_MSG("Handling REGISTER message");
        ReceptorMessages::Registration aRegistrationMsg;
        ReceptorMessages::RegistrationResponse aRegistrationResponse;
        std::string aError;
        aRegistrationMsg.ParseFromString(iMessage.serializedmessage());

        aRegistrationResponse.set_response(
            registerUser(aRegistrationMsg.login(), aRegistrationMsg.password(), aRegistrationMsg.email(), aError));
        aRegistrationResponse.set_error(aError);

        reply(aRegistrationMsg);
    }

    // Handle user login
    else if (iMessage.messagetype() == "LOGIN")
    {
        LOG_MSG("Handling LOGIN message");
        ReceptorMessages::Login aLoginMsg;
        ReceptorMessages::LoginResponse aLoginResponse;
        aLoginMsg.ParseFromString(iMessage.serializedmessage());
        LOG_MSG("Requested to login: " + aLoginMsg.login());
        aLoginResponse.set_token("NoTokenProvided");

        reply(aLoginResponse);
    }

    return true;
}

bool RegistrationBackend::handlePollTimeout()
{
    LOG_MSG("Handling poll timeout..");
    return true;
}

bool RegistrationBackend::handleNoMessages()
{
    LOG_MSG("Handling no messages..");
    return true;
}

bool RegistrationBackend::registerUser(const std::string &iLogin, const std::string &iPassword, const std::string &iEmail, std::string &oError)
{
    sqlite3 *db;

    LOG_MSG("Registering user " + iLogin);
    LOG_MSG("Registering password " + iPassword);
    LOG_MSG("Registering email " + iEmail);

    std::string aDbFile(getValueForVariable("FILE_PATH"));

    LOG_MSG("Using DB file " + aDbFile);

    int error = sqlite3_open(aDbFile.c_str(), &db);
    if (error)
    {
        oError = sqlite3_errmsg(db);
        LOG_MSG("Couldn't open file " + oError);
    }

    sqlite3_close(db);
    return true;
}

int main()
{
    RegistrationBackend aBackend;
    aBackend.configure();
    aBackend.start();

    return 0;
}