#include "Interceptor.h"

namespace WrongthinkInterceptors {

LoggingInterceptor::LoggingInterceptor(grpc::experimental::ServerRpcInfo* info,
                    std::shared_ptr<DBInterface> db,
                    std::shared_ptr<spdlog::logger> logger) : info_{info},
                                                              db_{db},
                                                              logger_{logger}
                                                              { }

void LoggingInterceptor::Intercept(grpc::experimental::InterceptorBatchMethods* methods) {
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::PRE_SEND_INITIAL_METADATA)) {
    /*logger->info("PRE_SEND_INITIAL_METADATA");
    auto* map = methods->GetSendInitialMetadata();
    for (const auto& pair : *map) {
      logger->info("{}, {}", to_string_view(pair.first), to_string_view(pair.second));
    }*/
  }
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE)) {
    // do nothing
  }
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::PRE_SEND_STATUS)) {
    // do nothing
  }
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::POST_RECV_INITIAL_METADATA)) {
    grpc_impl::ServerContextBase* serverContext = info_->server_context();
    // check if IP is in banned list, if so return error code
    if(db_->isIPBanned(serverContext->peer())) {
      serverContext->TryCancel();
    }
  }
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::POST_RECV_MESSAGE)) {
    //logger->info("POST_RECV_MESSAGE");
    grpc_impl::ServerContextBase* serverContext = info_->server_context();
    auto msg = static_cast<grpc::protobuf::Message*>(methods->GetRecvMessage());
    std::string data;
    //msg->SerializeToString(&data);
    const google::protobuf::Descriptor* descriptor = msg->GetDescriptor();
    logger_->info("RPC method: {}, peer: {}, request type: {}, request data: {}",
      info_->method(), serverContext->peer(), descriptor->name(), msg->ShortDebugString());
    auto auth = serverContext->auth_context();
    auto meta = serverContext->client_metadata();
    logger_->info("Client metadata:");
    for(auto& it : meta) {
      // grpc::string_ref, what a useful class
      auto strf = it.first;
      auto strf1 = it.second;
      std::string s1(strf.data(), strf.length());
      std::string s2(strf1.data(), strf1.length());
      logger_->debug("{}: {}", s1, s2);
    }
    // check for banned user
    const google::protobuf::FieldDescriptor* fDesc = descriptor->FindFieldByName("uname");
    if(fDesc) {
      const std::string uname = fDesc->default_value_string();
      if (db_->isUserBanned(uname, serverContext->peer())) {
        serverContext->TryCancel();
      }
    }
  }
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::POST_RECV_CLOSE)) {
    // Got nothing interesting to do here
  }
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::PRE_RECV_STATUS)) {
    // do nothing
  }
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::POST_RECV_STATUS)) {
    // do nothing
  }
  methods->Proceed();
}



LoggingInterceptorFactory::LoggingInterceptorFactory(std::shared_ptr<DBInterface> db,
  std::shared_ptr<spdlog::logger> logger) : db_{db}, logger_{logger} 
{ }
                          
  grpc::experimental::Interceptor* LoggingInterceptorFactory::CreateServerInterceptor(
    grpc::experimental::ServerRpcInfo* info)
{
  return new LoggingInterceptor(info, db_, logger_);
}

}
