#include <iostream>
#include <memory>
#include <string>
#include <mutex>
#include <map>
#include <ctime>

#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
//#include <grpcpp/health_check_service_interface.h>
#include "WrongthinkConfig.h"

#include "Util.h"

#include "WrongthinkServiceImpl.h"

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

  try {
    std::cout << "validating sql tables." << std::endl;
    WrongthinkUtils::setCredentials("wrongthink", "test");
    WrongthinkUtils::validateDatabase();
  }
  catch (const std::exception& e) {
    // unexpecdted, terminate
    std::cout << e.what() << std::endl;
    return 0;
  }
  RunServer();

  return 0;
}
