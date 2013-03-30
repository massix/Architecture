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

        reply(aRegistrationResponse);
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
    sleep(getPollTimeout());
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
    return false;
}

int main()
{
    RegistrationBackend aBackend;
    aBackend.configure();
    aBackend.start();

    return 0;
}
