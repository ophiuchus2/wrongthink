#include <iostream>
#include <memory>
#include <string>
#include <mutex>
#include <map>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "wrongthink.grpc.pb.h"
#include "WrongthinkConfig.h"

#include "SynchronizedChannel.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::Status;
using grpc::StatusCode;

std::map<std::string, SynchronizedChannel> channelMap;
std::mutex channelMapMutex;

class WrongthinkServiceImpl final : public wrongthink::Service {
  Status GetWrongthinkChannels(ServerContext* context, const WrongthinkChannel* request,
    ServerWriter<WrongthinkChannel>* writer) {
    (void)context;
    (void)request;
    std::lock_guard<std::mutex> lock(channelMapMutex);
    for (const auto& ch : channelMap) {
      writer->Write(ch.second.getChannel());
    }
    return Status::OK;
  }

  Status CreateWrongthinkChannel(ServerContext* context,
    const WrongthinkChannel* request, WrongthinkChannel* response) {
    (void)context;
    std::lock_guard<std::mutex> lock(channelMapMutex);
    if (channelMap.count(request->name()) == 1) {
      // channel already exists
      return Status(StatusCode::ALREADY_EXISTS, "");
    }
    channelMap.emplace(request->name(), *request);
    response->set_channelid(1);
    return Status::OK;
  }

  Status SendWrongthinkMessage(ServerContext* context,
    ServerReader< WrongthinkMessage>* reader, WrongthinkMeta* response) {
    (void) context;
    (void) reader;
    (void) response;
    return Status(StatusCode::UNIMPLEMENTED, "");
  }

  Status ListenWrongthinkMessages(ServerContext* context,
    const WrongthinkMeta* request, ServerWriter< WrongthinkMessage>* writer) {
    (void) context;
    (void) request;
    (void) writer;
    return Status(StatusCode::UNIMPLEMENTED, "");
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  WrongthinkServiceImpl service;

  grpc::EnableDefaultHealthCheckService(false);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  //RunServer();
  std::cout << "wrongthink version: "
    << Wrongthink_VERSION_MAJOR
    << "."
    << Wrongthink_VERSION_MINOR << std::endl;

  RunServer();

  return 0;
}
