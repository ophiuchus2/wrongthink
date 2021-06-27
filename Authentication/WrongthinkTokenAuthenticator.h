#ifndef WRONGTHINK_TOKENAUTH_H_
#define WRONGTHINK_TOKENAUTH_H_

#include <grpcpp/security/credentials.h>
#include <grpcpp/security/auth_metadata_processor.h>

namespace WrongthinkTokenAuth {

const std::string AUTH_UNAME_KEY = "auth-uname";
const std::string AUTH_TOKEN_KEY = "auth-token";

class WrongthinkClientTokenPlugin : public grpc::MetadataCredentialsPlugin {
 public:
  WrongthinkClientTokenPlugin(const grpc::string& uname,
                               const grpc::string& token) : uname_{uname}, token_{token} { }

  grpc::Status GetMetadata(
      grpc::string_ref service_url, grpc::string_ref method_name,
      const grpc::AuthContext& channel_auth_context,
      std::multimap<grpc::string, grpc::string>* metadata) override {
    metadata->insert(std::make_pair(AUTH_UNAME_KEY, uname_));
    metadata->insert(std::make_pair(AUTH_TOKEN_KEY, token_));
    return grpc::Status::OK;
  }

  grpc::string DebugString() override {
    return "uname: " + uname_ + ", token: " + token_;
  }

 private:
  grpc::string uname_;
  grpc::string token_;
};

class WrongthinkAuthMetadataProcessor : public grpc::AuthMetadataProcessor {
 public:

  WrongthinkAuthMetadataProcessor (bool is_blocking) : is_blocking_(is_blocking) {}

  // Interface implementation
  bool IsBlocking() const override { return is_blocking_; }

  Status Process(const grpc::AuthMetadataProcessor::InputMetadata& auth_metadata, grpc::AuthContext* context,
                 grpc::AuthMetadataProcessor::OutputMetadata* consumed_auth_metadata,
                 grpc::AuthMetadataProcessor::OutputMetadata* response_metadata) override {
    for (auto& p : auth_metadata) {
      std::string key(p.first.data(), p.first.length());
      std::string value(p.second.data(), p.second.length());
      context->AddProperty(key, value);
      consumed_auth_metadata->insert(std::make_pair(key, value));
    }
    return Status::OK;
  }

 private:
  bool is_blocking_;
};

static bool hasCredentials(ServerContext* context) {
  auto cmeta = context->client_metadata();
  if (cmeta.count(AUTH_UNAME_KEY) == 0)
    return false;
  if (cmeta.count(AUTH_TOKEN_KEY) == 0)
    return false;
  return true;
}

static std::pair<std::string, std::string> getCredentials(ServerContext* context) {
  auto cmeta = context->client_metadata();

  auto uname = cmeta.find(AUTH_UNAME_KEY);
  std::string s1(uname->second.data(), uname->second.length());
  auto token = cmeta.find(AUTH_TOKEN_KEY);
  std::string s2(token->second.data(), token->second.length());
  return std::make_pair(s1,s2);
}

static void addCredentials(grpc::ClientContext* context, WrongthinkUser* user) {
  context->AddMetadata(AUTH_UNAME_KEY, user->uname());
  context->AddMetadata(AUTH_TOKEN_KEY, user->token());
}

}

/*
class WrongthinkAuthService {
public:

  using AuthFunc = std::

  WrongthinkAuthService(std::shared_ptr<DBInterface> db) : db_{db} {

  }

  bool Authenticate(const std::string& rpc, ServerContext* context) {

  }

  static bool isAdmin_(ServerContext* context) {

  }
private:



  std::shared_ptr<DBInterface> db_;
}*/

#endif