#ifndef WRONGTHINK_INTERCEPTOR_H_
#define WRONGTHINK_INTERCEPTOR_H_

#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/support/server_interceptor.h>
#include "wrongthink.grpc.pb.h"
#include "spdlog/spdlog.h"
#include "../DB/DBInterface.h"

namespace WrongthinkInterceptors {

class LoggingInterceptor : public grpc::experimental::Interceptor {
 public:
  LoggingInterceptor(grpc::experimental::ServerRpcInfo* info,
                     std::shared_ptr<DBInterface> db,
                     std::shared_ptr<spdlog::logger> logger);

  void Intercept(grpc::experimental::InterceptorBatchMethods* methods) override;

 private:
  grpc::experimental::ServerRpcInfo* info_;
  std::shared_ptr<DBInterface> db_;
  std::shared_ptr<spdlog::logger> logger_;
};

class LoggingInterceptorFactory
    : public grpc::experimental::ServerInterceptorFactoryInterface {
 public:
   LoggingInterceptorFactory(std::shared_ptr<DBInterface> db,
                             std::shared_ptr<spdlog::logger> logger);
                             
  virtual grpc::experimental::Interceptor* CreateServerInterceptor(
      grpc::experimental::ServerRpcInfo* info) override;
private:
  std::shared_ptr<DBInterface> db_;
  std::shared_ptr<spdlog::logger> logger_;
};

}// namespace WrongthinkInterceptors
#endif
