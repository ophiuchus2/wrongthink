#ifndef WRONGTHINK_TOKENAUTH_H_
#define WRONGTHINK_TOKENAUTH_H_

#include <grpcpp/security/credentials.h>
#include <grpcpp/security/auth_metadata_processor.h>
#include <grpcpp/grpcpp.h>
#include "spdlog/spdlog.h"
#include "wrongthink.grpc.pb.h"

// grpc using statements
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::Status;
using grpc::StatusCode;

namespace WrongthinkTokenAuth {

class WrongthinkClientTokenPlugin : public grpc::MetadataCredentialsPlugin {
 public:
  WrongthinkClientTokenPlugin(const grpc::string& uname,
                               const grpc::string& token) : uname_{uname}, token_{token} { }

  grpc::Status GetMetadata(
      grpc::string_ref service_url, grpc::string_ref method_name,
      const grpc::AuthContext& channel_auth_context,
      std::multimap<grpc::string, grpc::string>* metadata) override;

  grpc::string DebugString() override ;

 private:
  grpc::string uname_;
  grpc::string token_;
};

class WrongthinkAuthMetadataProcessor : public grpc::AuthMetadataProcessor {
 public:

  WrongthinkAuthMetadataProcessor (bool is_blocking) : is_blocking_(is_blocking) {}

  // Interface implementation
  bool IsBlocking() const override;

  Status Process(const grpc::AuthMetadataProcessor::InputMetadata& auth_metadata, grpc::AuthContext* context,
                 grpc::AuthMetadataProcessor::OutputMetadata* consumed_auth_metadata,
                 grpc::AuthMetadataProcessor::OutputMetadata* response_metadata) override;

 private:
  bool is_blocking_;
};

bool hasCredentials(ServerContext* context);
std::pair<std::string, std::string> getCredentials(ServerContext* context);
void addCredentials(grpc::ClientContext* context, WrongthinkUser* user);

}

#endif