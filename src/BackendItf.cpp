/*
 * BackendItf.cpp
 *
 *  Created on: 21/nov/2012
 *      Author: massi
 */

#include <BackendItf.h>
#include <zmq.hpp>
#include <string>

using std::string;
using zmq::context_t;
using zmq::socket_t;

BackendItf::BackendItf() :
	_status(false), _context(0), _socket(_context, ZMQ_REQ)
{

}

BackendItf::~BackendItf()
{

}

void BackendItf::registerToFrontend(const string& iFrontend)
{
	if (!iFrontend.empty()) {
		try {
			_socket.connect(iFrontend.c_str());
			_status = true;
		}
		catch (...) { _status = false; }
	}
}

const bool BackendItf::isRegistered() const
{
	return _status;
}
