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
