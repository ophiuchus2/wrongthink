/**
* @file SynchronizedChannel.h
* @author Ophiuchus2
* @date 28 Jun 2020
* @copyright 2020 Wrongthink
* @brief <brief>
*/

#include <mutex>
#include <string>
#include <vector>
#include <condition_variable>

#include "wrongthink.grpc.pb.h"

class SynchronizedChannel {
public:
  SynchronizedChannel(const WrongthinkChannel& wtChannel);
  const WrongthinkChannel& getChannel() const { return wtChannel_; }
  void appendMessage(const WrongthinkMessage& msg);
  WrongthinkMessage lastMessage();
  std::vector<WrongthinkMessage> getMessages();
  WrongthinkMessage waitMessage();
  bool operator==(const SynchronizedChannel& sch);
  bool operator==(const WrongthinkChannel& sch);
  bool operator<(const SynchronizedChannel& sch);
  bool operator<(const WrongthinkChannel& sch);

private:
  const WrongthinkChannel wtChannel_;
  WrongthinkMessage lastMessage_;
  std::vector<WrongthinkMessage> msgVector_;
  std::mutex channelMutex_;
  std::condition_variable channelCondition_;
};
