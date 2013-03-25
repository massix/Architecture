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
