/*
 * BackendItf.cpp
 *
 *  Created on: 21/nov/2012
 *      Author: massi
 */

#include "StandardMessage.pb.h"
#include <BackendItf.h>
#include <zmq.hpp>
#include <string>
#include <unistd.h>

using std::string;
using zmq::context_t;
using zmq::socket_t;
using zmq::message_t;

BackendItf::BackendItf() :
	_status(false), _context(0), _socket(_context, ZMQ_REQ)
{

}

BackendItf::~BackendItf()
{

}

const bool BackendItf::isRegistered() const
{
	return _status;
}
