//
//  RegistrationBackend.h
//  Architecture
//
//  Created by Massimo Gengarelli on 24/03/13.
//
//

#pragma once

#include <BackendItf.h>
#include <StandardMessage.pb.h>
#include <string>


class RegistrationBackend : public BackendItf
{
public:
    explicit RegistrationBackend() :
        BackendItf("RegistrationBackend"),
        _usersDb("./usersdb.txt") {};

    virtual ~RegistrationBackend() {};

protected:
    /* Deal with messages */
    virtual bool handleMessage(const ReceptorMessages::BackendResponseMessage& iMessage) = 0;

    /* Deal with poll timeout */
    virtual bool handlePollTimeout() = 0;

    /* Deal with no messages received */
    virtual bool handleNoMessages() = 0;

private:
    bool registerUser(std::string const & iLogin, std::string const & iPassword, std::string const & iEmail, std::string & oError);
    const std::string _usersDb;

};
