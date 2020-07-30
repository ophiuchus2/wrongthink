#include <grpcpp/grpcpp.h>

#include "wrongthink.grpc.pb.h"
#include "Util.h"
#include "SynchronizedChannel.h"

// grpc using statements
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::Status;
using grpc::StatusCode;

// soci using statements
using soci::session;
using soci::row;
using soci::rowset;
using soci::statement;
using soci::use;
using soci::into;

class WrongthinkServiceImpl final : public wrongthink::Service {
public:
  Status GetWrongthinkChannels(ServerContext* context,
    const GetWrongthinkChannelsRequest* request,
    ServerWriter<WrongthinkChannel>* writer) override;

  Status CreateWrongthinkChannel(ServerContext* context,
    const CreateWrongThinkChannelRequest* request,
    WrongthinkChannel* response) override;

  Status SendWrongthinkMessage(ServerContext* context,
    ServerReader< WrongthinkMessage>* reader, WrongthinkMeta* response) override;

  Status ListenWrongthinkMessages(ServerContext* context,
    const ListenWrongthinkMessagesRequest* request,
    ServerWriter< WrongthinkMessage>* writer) override;

  Status GetWrongthinkMessages(ServerContext* context,
    const GetWrongthinkMessagesRequest* request,
    ServerWriter< WrongthinkMessage>* writer) override;

  Status CreateUser(ServerContext* context, const CreateUserRequest* request,
    WrongthinkUser* response) override;
private:
  bool checkForChannel(int channelid, soci::session& sql);

  std::map<int, SynchronizedChannel> channelMap;
  std::mutex channelMapMutex;
};
