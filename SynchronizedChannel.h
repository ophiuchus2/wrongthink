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
#include <mutex>
#include <string>
#include <vector>
#include <condition_variable>

#include "wrongthink.grpc.pb.h"

class SynchronizedChannel {
public:
  SynchronizedChannel(const WrongthinkChannel& wtChannel);
  SynchronizedChannel() { }
  SynchronizedChannel(int channelId,
                      const std::string& channelName);
  const WrongthinkChannel& getChannel() const { return wtChannel_; }
  void appendMessage(const WrongthinkMessage& msg);
  void sendMessage(const WrongthinkMessage& msg);
  WrongthinkMessage lastMessage();
  std::vector<WrongthinkMessage> getMessages();
  WrongthinkMessage waitMessage();
  bool operator==(const SynchronizedChannel& sch);
  bool operator==(const WrongthinkChannel& sch);
  bool operator<(const SynchronizedChannel& sch);
  bool operator<(const WrongthinkChannel& sch);

private:
  WrongthinkChannel wtChannel_;
  WrongthinkMessage lastMessage_;
  std::vector<WrongthinkMessage> msgVector_;
  std::mutex channelMutex_;
  std::condition_variable channelCondition_;
};
