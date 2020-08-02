/*
This file is part of Wrongthink.

Wrongthink - Modern, open & performant chat protocol. Based on gRPC.
Copyright (C) 2020 Ophiuchus2

This program is free software: you can redistribute it and/or modify it under the
terms of the GNU Affero General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.
If not, see <https://www.gnu.org/licenses/>.
*/
#include "SynchronizedChannel.h"

SynchronizedChannel::SynchronizedChannel(const WrongthinkChannel& wtChannel):
  wtChannel_{wtChannel},
  lastMessage_{},
  msgVector_{},
  channelMutex_{},
  channelCondition_{}
{ }

SynchronizedChannel::SynchronizedChannel(int channelId,
                    const std::string& channelName): wtChannel_{} {
  wtChannel_.set_channelid(channelId);
  wtChannel_.set_name(channelName);
}

void SynchronizedChannel::sendMessage(const WrongthinkMessage& msg) {

}

void SynchronizedChannel::appendMessage(const WrongthinkMessage& msg) {
  {
    std::lock_guard<std::mutex> lock(channelMutex_);
    msgVector_.push_back(msg);
    lastMessage_ = msg;
  }
  channelCondition_.notify_all();
}

WrongthinkMessage SynchronizedChannel::lastMessage() {
  std::lock_guard<std::mutex> lock(channelMutex_);
  return lastMessage_;
}

std::vector<WrongthinkMessage> SynchronizedChannel::getMessages() {
  std::lock_guard<std::mutex> lock(channelMutex_);
  return msgVector_;
}

WrongthinkMessage SynchronizedChannel::waitMessage() {
  std::unique_lock<std::mutex> lock(channelMutex_);
  channelCondition_.wait(lock);
  return lastMessage_;
}

bool SynchronizedChannel::operator==(const SynchronizedChannel& sch) {
  return sch.getChannel().name() == wtChannel_.name();
}

bool SynchronizedChannel::operator==(const WrongthinkChannel& sch) {
  return sch.name() == wtChannel_.name();
}

bool SynchronizedChannel::operator<(const SynchronizedChannel& sch) {
  return wtChannel_.name() < sch.getChannel().name();
}

bool SynchronizedChannel::operator<(const WrongthinkChannel& sch) {
  return wtChannel_.name() < sch.name();
}
