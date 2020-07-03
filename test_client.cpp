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
  std::shared_ptr<Channel> mchannel(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  std::unique_ptr<wrongthink::Stub> mstub(wrongthink::NewStub(mchannel));

  WrongthinkChannel mch, mch1, mch2, resp, dummy;
  mch.set_name("channel 1");
  mch.set_channelid(678);
  mch.set_public_(true);

  mch1.set_name("channel 2");
  mch1.set_channelid(99);
  mch1.set_public_(true);

  mch2.set_name("channel 3");
  mch2.set_channelid(99);
  mch2.set_public_(true);

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

  std::unique_ptr< ::grpc::ClientReader< ::WrongthinkChannel>> reader(mstub->GetWrongthinkChannels(&context3, dummy));
  while(reader->Read(&resp))
    std::cout << "got channel: " << resp.name() << std::endl;

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
