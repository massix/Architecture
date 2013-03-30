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

#include "MessageQueue.h"
#include <protobuf/message.h>
#include <zmq.hpp>
#include <string>
#include <Log.h>

using zmq::message_t;

MessageQueue::MessageQueue()
{
    _messagesList.clear();
}

MessageQueue::~MessageQueue()
{

}

void MessageQueue::enqueueMessage(const std::string& iMessage)
{
    LOG_MSG("Enqueueing message: " + iMessage);
    _messagesList.push_back(iMessage);
}

void MessageQueue::enqueueHeader(const std::string &iHeader)
{
    LOG_MSG("Enqueueing header: " + iHeader);
    _headersList.push_back(iHeader);
}

std::string MessageQueue::dequeueMessage()
{
    std::string aRetValue("");
    if (hasMessages()) {
        aRetValue = _messagesList.back();
        _messagesList.pop_back();
    }
    return aRetValue;
}

std::string MessageQueue::dequeueHeader()
{
    std::string aRetValue("");
    if (!_headersList.empty()) {
        aRetValue = _headersList.back();
        _headersList.pop_back();
    }
    return aRetValue;
}

bool MessageQueue::hasMessages() const
{
    return (!_messagesList.empty());
}

void MessageQueue::clear()
{
    _messagesList.clear();
}

std::size_t MessageQueue::size() const
{
    return _messagesList.size();
}

bool MessageQueue::empty() const {
    return !hasMessages();
}
