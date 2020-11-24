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
#include <csignal>
#include <string_view>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/support/server_interceptor.h>
//#include <grpcpp/health_check_service_interface.h>
#include "WrongthinkConfig.h"
#include "DB/DBInterface.h"
#include "DB/DBPostgres.h"
#include "WrongthinkServiceImpl.h"

// include interceptor classes
#include "Interceptors/Interceptor.h"

std::shared_ptr<spdlog::logger> logger;

static std::shared_ptr<DBInterface> db;

inline std::string_view to_string_view(const grpc::string_ref& s) {
  return {s.data(), s.length()};
}

void sigHandler(int num) {
  logger->info("received signal: {}", num);
  logger->info("terminating");
  exit(num);
}

void coinfigureLog() {
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::trace);
  console_sink->set_pattern("[%H:%M:%S %z] [wrongthink] [%^%l%$] %v");

  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/wrongthink.txt", true);
  file_sink->set_level(spdlog::level::trace);

  std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
  logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
  //logger->set_level(spdlog::level::trace);
  logger->info("logger started");
}

void RunServer() {
  std::vector<
      std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>>
      creators;
  creators.push_back(
      std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>(
          new LoggingInterceptorFactory(db, logger)));
  std::string server_address("0.0.0.0:50051");
  WrongthinkServiceImpl service( db, logger );

  grpc::EnableDefaultHealthCheckService(false);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);

  logger->info("register interceptors");
  builder.experimental().SetInterceptorCreators(std::move(creators));
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  logger->info("Server listening on {}", server_address);

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  coinfigureLog();
  logger->info("wrongthink version: {}.{} ", Wrongthink_VERSION_MAJOR, Wrongthink_VERSION_MINOR);

  signal(SIGINT, sigHandler);
  try {

    if( argc == 2 && strcmp(argv[1], "sqlite") == 0 ) {
      logger->info("Using sqlite backend");
      logger->info("Not currently implemented");
      return 1;
    } else {
      logger->info("Using postgres backend");
      db = std::make_shared<DBPostgres>("wrongthink", "test", "wrongthink");
    }

    if (argc == 2 && strcmp(argv[1], "clear") == 0) {
      db->clear();
    }

    logger->info("validating sql tables.");

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
