#include "WrongthinkTokenAuthenticator.h"

namespace WrongthinkTokenAuth {

grpc::Status WrongthinkClientTokenPlugin::GetMetadata(
    grpc::string_ref service_url, grpc::string_ref method_name,
    const grpc::AuthContext& channel_auth_context,
    std::multimap<grpc::string, grpc::string>* metadata) {
  metadata->insert(std::make_pair(AUTH_UNAME_KEY, uname_));
  metadata->insert(std::make_pair(AUTH_TOKEN_KEY, token_));
  return grpc::Status::OK;
}

grpc::string WrongthinkClientTokenPlugin::DebugString() {
  return "uname: " + uname_ + ", token: " + token_;
}

// Interface implementation
bool WrongthinkAuthMetadataProcessor::IsBlocking() const { return is_blocking_; }

Status WrongthinkAuthMetadataProcessor::Process(const grpc::AuthMetadataProcessor::InputMetadata& auth_metadata, grpc::AuthContext* context,
                grpc::AuthMetadataProcessor::OutputMetadata* consumed_auth_metadata,
                grpc::AuthMetadataProcessor::OutputMetadata* response_metadata) {
  for (auto& p : auth_metadata) {
    std::string key(p.first.data(), p.first.length());
    std::string value(p.second.data(), p.second.length());
    context->AddProperty(key, value);
    consumed_auth_metadata->insert(std::make_pair(key, value));
  }
  return Status::OK;
}

bool hasCredentials(ServerContext* context) {
  auto cmeta = context->client_metadata();
  if (cmeta.count(AUTH_UNAME_KEY) == 0)
    return false;
  if (cmeta.count(AUTH_TOKEN_KEY) == 0)
    return false;
  return true;
}

std::pair<std::string, std::string> getCredentials(ServerContext* context) {
  auto cmeta = context->client_metadata();

  auto uname = cmeta.find(AUTH_UNAME_KEY);
  std::string s1(uname->second.data(), uname->second.length());
  auto token = cmeta.find(AUTH_TOKEN_KEY);
  std::string s2(token->second.data(), token->second.length());
  return std::make_pair(s1,s2);
}

void addCredentials(grpc::ClientContext* context, WrongthinkUser* user) {
  context->AddMetadata(AUTH_UNAME_KEY, user->uname());
  context->AddMetadata(AUTH_TOKEN_KEY, user->token());
}

} // namespace WrongthinkTokenAuth
