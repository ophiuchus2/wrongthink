class LoggingInterceptor : public grpc::experimental::Interceptor {
 public:
  LoggingInterceptor(grpc::experimental::ServerRpcInfo* info,
                     std::shared_ptr<DBInterface> db,
                     std::shared_ptr<spdlog::logger> logger) : info_{info},
                                                               db_{db},
                                                               logger_{logger}
                                                               { }

  void Intercept(grpc::experimental::InterceptorBatchMethods* methods) override {
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

 private:
  grpc::experimental::ServerRpcInfo* info_;
  std::shared_ptr<DBInterface> db_;
  std::shared_ptr<spdlog::logger> logger_;
};

class LoggingInterceptorFactory
    : public grpc::experimental::ServerInterceptorFactoryInterface {
 public:
   LoggingInterceptorFactory(std::shared_ptr<DBInterface> db,
                             std::shared_ptr<spdlog::logger> logger) : db_{db}, logger_{logger} {}
  virtual grpc::experimental::Interceptor* CreateServerInterceptor(
      grpc::experimental::ServerRpcInfo* info) override {
    return new LoggingInterceptor(info, db_, logger_);
  }
private:
  std::shared_ptr<DBInterface> db_;
  std::shared_ptr<spdlog::logger> logger_;
};
