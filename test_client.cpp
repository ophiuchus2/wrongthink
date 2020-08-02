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
#include <iostream>
#include <memory>
#include <string>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#include "wrongthink.grpc.pb.h"

std::unique_ptr<wrongthink::Stub> mstub;

void testUsers(){
  ClientContext context;
  CreateUserRequest req1, req2;
  WrongthinkUser resp;
  req1.set_uname("user1");
  req1.set_password("pass1");
  req1.set_admin(true);

  req2.set_uname("user2");
  req2.set_password("pass2");
  req2.set_admin(false);

  std::cout << "creating user1" << std::endl;
  Status status = mstub->CreateUser(&context, req1, &resp);
  if (!status.ok())
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;

  std::cout << "creating user2" << std::endl;
  status = mstub->CreateUser(&context, req2, &resp);
  if (!status.ok())
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;

}

void createChannels() {
  CreateWrongThinkChannelRequest mch, mch1, mch2;
  WrongthinkChannel resp;

  mch.set_name("channel 1");
  mch.set_communityid(678);
  mch.set_anonymous(true);
  mch.set_communityid(1);

  mch1.set_name("channel 2");
  mch1.set_communityid(99);
  mch1.set_anonymous(true);
  mch1.set_adminid(1);

  mch2.set_name("channel 3");
  mch2.set_communityid(99);
  mch2.set_anonymous(true);
  mch2.set_communityid(1);

  ClientContext context, context1, context2, context3;
  Status status = mstub->CreateWrongthinkChannel(&context, mch, &resp);
  // Act upon its status.
  if (!status.ok())
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
  status = mstub->CreateWrongthinkChannel(&context1, mch1, &resp);
  if (!status.ok())
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
  status = mstub->CreateWrongthinkChannel(&context2, mch2, &resp);
  if (!status.ok())
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
}

void getChannels() {
  ClientContext context;
  WrongthinkChannel resp;
  GetWrongthinkChannelsRequest channelRequest;
  channelRequest.set_communityid(1);
  auto reader = mstub->GetWrongthinkChannels(&context, channelRequest);
  while(reader->Read(&resp))
    std::cout << "got channel: " << resp.name() << std::endl;

}

void listen(std::unique_ptr<wrongthink::Stub> stub) {
  ClientContext context;
  ListenWrongthinkMessagesRequest request;
  WrongthinkMessage msg;
  request.set_channelname("channel 1");
  request.set_uname("bob");
  auto reader = stub->ListenWrongthinkMessages(&context, request);
  while (reader->Read(&msg))
    std::cout << msg.uname() << ": " << msg.text() << std::endl;

}

void send(std::unique_ptr<wrongthink::Stub> stub) {
  ClientContext context;
  WrongthinkMeta dummy;
  std::string text;
  auto writer = stub->SendWrongthinkMessage(&context, &dummy);
  while (true) {
    WrongthinkMessage msg;
    std::cin >> text;
    msg.set_text(text);
    msg.set_uname("alice");
    msg.set_channelname("channel 1");
    writer->Write(msg);
  }
}

int main(int argc, char** argv) {
  std::shared_ptr<Channel> mchannel = grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials());
  mstub = wrongthink::NewStub(mchannel);

  testUsers();
  createChannels();
  getChannels();

  if(argc > 1) {
    std::string arg(argv[1]);
    if(arg == "listen")
      listen(std::move(mstub));
    else if (arg == "send")
      send(std::move(mstub));
  }
  std::cout << "terminate" << std::endl;
  return 0;
}
