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
#include <map>
#include <ctime>

#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
//#include <grpcpp/health_check_service_interface.h>
#include "WrongthinkConfig.h"

#include "WrongthinkServiceImpl.h"

#include "DB/DBInterface.h"
#include "DB/DBPostgres.h"

static std::shared_ptr<DBInterface> db;

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  WrongthinkServiceImpl service( db );

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

    if( argc == 2 && strcmp(argv[1], "sqlite") == 0 ) {
      std::cout << "Using sqlite backend" << std::endl;
      std::cout << "Not currently implemented" << std::endl;
      return 1;
    } else {
      std::cout << "Using postgres backend" << std::endl;
      db = std::make_shared<DBPostgres>("wrongthink", "test", "wrongthink");
    }

    if (argc == 2 && strcmp(argv[1], "clear") == 0) {
      db->clear();
    }

    std::cout << "validating sql tables." << std::endl;

    db->validate();
  }
  catch (const std::exception& e) {
    // unexpecdted, terminate
    std::cout << e.what() << std::endl;
    return 0;
  }
  RunServer();

  return 0;
}
