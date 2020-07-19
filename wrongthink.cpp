#include <iostream>
#include <memory>
#include <string>
#include <mutex>
#include <map>
#include <ctime>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "wrongthink.grpc.pb.h"
#include "WrongthinkConfig.h"

#include "SynchronizedChannel.h"
#include "Util.h"

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

std::map<int, SynchronizedChannel> channelMap;
std::mutex channelMapMutex;

bool checkForChannel(int channelid, soci::session& sql) {
  if (channelMap.count(channelid) != 1) {
    row r;
    sql << "select name from channels where channel_id = :id",
          use(channelid), into(r);
    if(r.get_indicator(0) != soci::i_null)
      channelMap.emplace( std::piecewise_construct,
                          std::forward_as_tuple(channelid),
                          std::forward_as_tuple(channelid,
                                                r.get<std::string>(0)));
    else
      return false;
  }
  return true;
}

class WrongthinkServiceImpl final : public wrongthink::Service {
  Status GetWrongthinkChannels(ServerContext* context,
    const GetWrongthinkChannelsRequest* request,
    ServerWriter<WrongthinkChannel>* writer) {
    (void)context;
    try {
      int community = request->communityid();
      soci::session sql = WrongthinkUtils::getSociSession();
      rowset<row> rs = (sql.prepare << "select channel_id, name, "
                                    << "allow_anon from channels "
                                    << "where community_id = :community",
                                    use(community));
      for (rowset<row>::const_iterator it = rs.begin(); it != rs.end(); ++it) {
        WrongthinkChannel channel;
        row const& row = *it;
        channel.set_channelid(row.get<int>(0));
        channel.set_name(row.get<std::string>(1));
        channel.set_anonymous(row.get<bool>(2));
        writer->Write(channel);
      }
    } catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
      return Status(StatusCode::INTERNAL, "");
    }
    return Status::OK;
  }

  Status CreateWrongthinkChannel(ServerContext* context,
    const CreateWrongThinkChannelRequest* request, WrongthinkChannel* response) {
    (void)context;
    try {
      int channelid = 0;
      int community = request->communityid();
      char anonymous = request->anonymous();
      std::string name = request->name();
      int admin = request->adminid();
      soci::session sql = WrongthinkUtils::getSociSession();
      sql << "insert into channels(name, "
          << "community, admin, allow_anon) "
          << "values(:name,:community,:admin,:anonymous)",
           use(name), use(community), use(admin), use(anonymous);
      sql << "select channel_id from channels where name = :name",
        use(name), into(channelid);
      response->set_channelid(channelid);
    } catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
      return Status(StatusCode::INTERNAL, "");
    }
    return Status::OK;
  }

  Status SendWrongthinkMessage(ServerContext* context,
    ServerReader< WrongthinkMessage>* reader, WrongthinkMeta* response) {
    (void) context;
    (void) response;
    WrongthinkMessage msg;
    try {
      soci::session sql = WrongthinkUtils::getSociSession();
      int channelid = 0;
      int uname = 0;
      int thread_id = 0;
      char thread_child = 0;
      std::string text;
      statement st = (sql.prepare <<
                "insert into message(uname,channel,thread_id,thread_child, mtext)"
                <<" values(:uname,:channel,:thread_id,:thread_child)",
                use(uname), use(channelid), use(thread_id), use(thread_child),
                use(text));
      while (reader->Read(&msg)) {
        channelid = msg.channelid();
        uname = msg.userid();
        thread_id = msg.threadid();
        thread_child = msg.threadchild();
        text = msg.text();
        if(!checkForChannel(msg.channelid(), sql))
          return Status(StatusCode::INVALID_ARGUMENT, "");
        channelMap[msg.channelid()].appendMessage(msg);
        st.execute(true);
      }
    } catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
      return Status(StatusCode::INTERNAL, "");
    }
    return Status::OK;
  }

  Status ListenWrongthinkMessages(ServerContext* context,
    const ListenWrongthinkMessagesRequest* request,
    ServerWriter< WrongthinkMessage>* writer) {
    (void) context;
    (void) request;
    soci::session sql = WrongthinkUtils::getSociSession();
    int channelid = request->channelid();
    int count = 0;
    if (!checkForChannel(channelid, sql))
      return Status(StatusCode::INVALID_ARGUMENT, "");
    SynchronizedChannel& channel = channelMap[request->channelid()];
    while (true) {
      writer->Write(channel.waitMessage());
    }
    return Status::OK;
  }

  Status GetWrongthinkMessages(ServerContext* context,
    const GetWrongthinkMessagesRequest* request,
    ServerWriter< WrongthinkMessage>* writer) {
    int channelid = request->channelid();
    int limit = request->limit();
    int afterid = request->afterid();
    int afterdate = request->afterdate();
    try {
      soci::session sql = WrongthinkUtils::getSociSession();
      rowset<row> rs = (sql.prepare << "select * from message inner join users on "
                  << "message.uname = users.user_id where "
                  << "message.channel = :channelid", use(channelid));
      for (rowset<row>::const_iterator it = rs.begin(); it != rs.end(); ++it) {
        row const& row = *it;
        WrongthinkMessage msg;
        msg.set_uname(row.get<std::string>("users.uname"));
        msg.set_channelid(channelid);
        msg.set_userid(row.get<int>("user_id"));
        msg.set_threadid(row.get<int>("thread_id"));
        msg.set_threadchild(row.get<bool>("thread_child"));
        msg.set_edited(row.get<bool>("edited"));
        msg.set_text(row.get<std::string>("mtext"));
        msg.set_date(row.get<int>("mdate"));
        writer->Write(msg);
      }
    } catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
      return Status(StatusCode::INTERNAL, "");
    }
  }

  Status CreateUser(ServerContext* context, const CreateUserRequest* request,
    WrongthinkUser* response) {
    try {
      std::string uname = request->uname();
      std::string password = request->password();
      char admin = request->admin();
      int uid = 0;
      soci::session sql = WrongthinkUtils::getSociSession();
      sql << "insert into users (uname,password,admin) values(:uname,:password,:admin)",
            use(uname), use(password), use(admin);
      sql << "select user_id from users where uname = :uname", use(uname), into(uid);
      response->set_userid(uid);
    } catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
      return Status(StatusCode::INTERNAL, "");
    }
    return Status::OK;
  }

};

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
