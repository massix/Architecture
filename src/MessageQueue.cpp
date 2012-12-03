//
//  MessageQueue.cpp
//  Architecture
//
//  Created by Massimo Gengarelli on 17/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

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
    LOG_MSG("Enqueueing message");
    _messagesList.push_back(iMessage);
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
