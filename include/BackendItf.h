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
#include <zmq.hpp>
#include <string>
#include <vector>
#include <map>
#include <StandardMessage.pb.h>
#include <protobuf/message.h>

class BackendItf;

class BackendConfiguration
{
    friend class BackendItf;
    std::string _backendName;
    std::string _backendOwner;
    std::string _backendOwnerMail;
    std::string _frontend;
    uint32_t _frontendPort;
    uint32_t _pollTimeoutSeconds;
    std::vector<std::string> _messagesHandled;
};

class BackendItf
{
public:
	explicit BackendItf(const std::string& iBackend);
	virtual ~BackendItf();

	const bool isConfigured() const;
    
    /* Inline accessors */
    const std::string& getBackendName() const { return _config._backendName; }
    const std::string& getBackendOwner() const { return _config._backendOwner; }
    const std::string& getBackendOwnerMail() const { return _config._backendOwnerMail; }
    const std::string& getFrontendHost() const { return _config._frontend; }
    const uint32_t& getFrontendPort() const { return _config._frontendPort; }
    const uint32_t& getPollTimeout() const { return _config._pollTimeoutSeconds; }
    const std::vector<std::string>& getMessagesHandled() const { return _config._messagesHandled; }

    void clientCall(const google::protobuf::Message& iRequest);
    void reply(const google::protobuf::Message& iReply);
    void configure();
    void start();
    
protected:    
    /* Deal with messages */
    virtual bool handleMessage(const ReceptorMessages::BackendResponseMessage& iMessage) = 0;
    
    /* Deal with poll timeout */
    virtual bool handlePollTimeout() = 0;
    
    /* Deal with no messages received */
    virtual bool handleNoMessages() = 0;

    /* This is not meant to be overwritten */
    const std::string getValueForVariable(const std::string & iVariable) const;

protected:
    std::string _backend;
    std::map<std::string, std::string> _variables;
	bool _configured;

	zmq::context_t  _context;
	zmq::socket_t	_socket;
    std::string _handledMessage;
    std::string _handledHeader;
    BackendConfiguration _config;
};

