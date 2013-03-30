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

#pragma once

#include <BackendItf.h>
#include <StandardMessage.pb.h>
#include <string>


class RegistrationBackend : public BackendItf
{
public:
    explicit RegistrationBackend() :
        BackendItf("RegistrationBackend") {};

    virtual ~RegistrationBackend() {};

protected:
    /* Deal with messages */
    virtual bool handleMessage(const ReceptorMessages::BackendResponseMessage& iMessage);

    /* Deal with poll timeout */
    virtual bool handlePollTimeout();

    /* Deal with no messages received */
    virtual bool handleNoMessages();

private:
    bool registerUser(std::string const & iLogin, std::string const & iPassword, std::string const & iEmail, std::string & oError);

};
