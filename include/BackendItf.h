/*
 * BackendItf.h
 *
 *  Created on: 21/nov/2012
 *      Author: massi
 */

#pragma once
#include <zmq.hpp>
#include <string>


class BackendItf
{
public:
	explicit BackendItf();
	virtual ~BackendItf();

	const bool isRegistered() const;

private:
	bool _status;
	zmq::context_t  _context;
	zmq::socket_t	_socket;
};
