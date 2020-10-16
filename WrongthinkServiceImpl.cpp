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
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "WrongthinkServiceImpl.h"
#include <memory>

WrongthinkServiceImpl::WrongthinkServiceImpl( const std::shared_ptr<DBInterface> db,
                                              const std::shared_ptr<spdlog::logger> logger) :
  db{ db }, logger{ logger }
{

}

Status WrongthinkServiceImpl::GenerateUser(ServerContext* context, const GenericRequest* request,
  WrongthinkUser* response) {
  try {
    (void)request;
    // generate two uuids
    boost::uuids::random_generator gen;
    std::string id = boost::uuids::to_string(gen());
    int idx = 0;
    while((idx = id.find('-')) != std::string::npos) {
      id.erase(idx, 1);
    }
    std::string id2 = boost::uuids::to_string(gen());
    while((idx = id2.find('-')) != std::string::npos) {
      id2.erase(idx, 1);
    }

    int uid = db->createUser( id, id2, false );

    response->set_uname(id);
    response->set_token(id2);
    response->set_admin(false);
    response->set_userid(uid);
  }catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return Status(StatusCode::INTERNAL, "");
  }
  return Status::OK;
}

/* needed to make the rpc function testable */
Status WrongthinkServiceImpl::GetWrongthinkCommunitiesImpl(const GetWrongthinkCommunitiesRequest* request,
  ServerWriterWrapper<WrongthinkCommunity>* writer) {
  try {
    // not using request data yet
    (void)request;

    soci::session sql = db->getSociSession();
    rowset<row> rs = db->getCommunityRowset( sql );

    for (rowset<row>::const_iterator it = rs.begin(); it != rs.end(); ++it) {
      WrongthinkCommunity community;
      row const& row = *it;
      community.set_communityid(row.get<int>(0));
      community.set_name(row.get<std::string>(1));
      community.set_unameadmin(row.get<std::string>(5));
      writer->Write(community);
    }
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return Status(StatusCode::INTERNAL, "");
  }
  return Status::OK;
}

/* needed to make the rpc function testable */
Status WrongthinkServiceImpl::GetWrongthinkCommunities(ServerContext* context,
  const GetWrongthinkCommunitiesRequest* request,
  ServerWriter<WrongthinkCommunity>* writer) {
    (void)context;
    ServerWriterWrapper<WrongthinkCommunity> wrapper(writer);
    return GetWrongthinkCommunitiesImpl(request, &wrapper);
}

Status WrongthinkServiceImpl::GetWrongthinkChannels(ServerContext* context,
  const GetWrongthinkChannelsRequest* request,
  ServerWriter<WrongthinkChannel>* writer) {
  (void)context;
  ServerWriterWrapper<WrongthinkChannel> wrapper(writer);
  return GetWrongthinkChannelsImpl(request, &wrapper);
}

Status WrongthinkServiceImpl::GetWrongthinkChannelsImpl(const GetWrongthinkChannelsRequest* request,
  ServerWriterWrapper<WrongthinkChannel>* writer) {
  try {
    int community = request->communityid();

    soci::session sql = db->getSociSession();
    rowset<row> rs = db->getCommunityChannelsRowset(sql, community);

    for (rowset<row>::const_iterator it = rs.begin(); it != rs.end(); ++it) {
      WrongthinkChannel channel;
      row const& row = *it;
      channel.set_channelid(row.get<int>(0));
      channel.set_name(row.get<std::string>(1));
      channel.set_anonymous(row.get<int>(2));
      channel.set_communityid(community);
      channel.set_unameadmin(row.get<std::string>(6));
      writer->Write(channel);
    }
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return Status(StatusCode::INTERNAL, "");
  }
  return Status::OK;
}

Status WrongthinkServiceImpl::CreateWrongthinkChannel(ServerContext* context,
  const CreateWrongThinkChannelRequest* request, WrongthinkChannel* response) {
  (void)context;
  try {
    int channelid = 0;
    int community = request->communityid();
    int anonymous = request->anonymous();
    std::string name = request->name();
    int admin = request->adminid();

    channelid = db->createChannel( name, community, admin, anonymous );

    response->set_channelid(channelid);
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return Status(StatusCode::INTERNAL, "");
  }
  return Status::OK;
}

Status WrongthinkServiceImpl::CreateWrongthinkCommunity(ServerContext* context,
  const CreateWrongthinkCommunityRequest* request, WrongthinkCommunity* response) {
  try {
    int communityid = 0;
    std::string name = request->name();
    int admin = request->adminid();
    int pub = request->public_();

    communityid = db->createCommunity( name, admin, pub );

    response->set_communityid(communityid);
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return Status(StatusCode::INTERNAL, "");
  }
  return Status::OK;
}

Status WrongthinkServiceImpl::SendWrongthinkMessageWeb(ServerContext* context,
  const WrongthinkMessage* msg, WrongthinkMeta* response) {
  try {
    int channelid = msg->channelid();
    int user_id = msg->userid();
    int thread_id = msg->threadid();
    int thread_child = msg->threadchild();
    std::string text = msg->text();
    soci::session sql = db->getSociSession();
    if(!checkForChannel(msg->channelid(), sql))
      return Status(StatusCode::INVALID_ARGUMENT, "");
    channelMap[msg->channelid()].appendMessage(*msg);
    sql << "insert into message(user_id,channel,thread_id,thread_child, mtext)"
        << " values(:user_id,:channel,:thread_id,:thread_child,:text)",
        use(user_id), use(channelid), use(thread_id), use(thread_child),
        use(text);
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return Status(StatusCode::INTERNAL, "");
  }
  return Status::OK;
}

Status WrongthinkServiceImpl::SendWrongthinkMessage(ServerContext* context,
  ServerReader< WrongthinkMessage>* reader, WrongthinkMeta* response) {
  (void) context;
  ServerReaderWrapper< WrongthinkMessage> wrapper(reader);
  return SendWrongthinkMessageImpl(&wrapper, response);
}

Status WrongthinkServiceImpl::SendWrongthinkMessageImpl(ServerReaderWrapper< WrongthinkMessage>* reader,
  WrongthinkMeta* response) {
  (void) response;
  WrongthinkMessage msg;
  try {
    soci::session sql = db->getSociSession();
    int channelid = 0;
    int user_id = 0;
    int thread_id = 0;
    int thread_child = 0;
    std::string text;
    statement st = (sql.prepare <<
              "insert into message(user_id,channel,thread_id,thread_child, mtext)"
              <<" values(:user_id,:channel,:thread_id,:thread_child,:text)",
              use(user_id), use(channelid), use(thread_id), use(thread_child),
              use(text));
    while (reader->Read(&msg)) {
      channelid = msg.channelid();
      user_id = msg.userid();
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

Status WrongthinkServiceImpl::ListenWrongthinkMessages(ServerContext* context,
  const ListenWrongthinkMessagesRequest* request,
  ServerWriter< WrongthinkMessage>* writer) {
  (void) context;
  ServerWriterWrapper< WrongthinkMessage> wrapper(writer);
  return ListenWrongthinkMessagesImpl(request, &wrapper);
}

/* needed to make the rpc function testable */
Status WrongthinkServiceImpl::ListenWrongthinkMessagesImpl(const ListenWrongthinkMessagesRequest* request,
  ServerWriterWrapper< WrongthinkMessage>* writer) {
  soci::session sql = db->getSociSession();
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

Status WrongthinkServiceImpl::GetWrongthinkMessages(ServerContext* context,
  const GetWrongthinkMessagesRequest* request,
  ServerWriter< WrongthinkMessage>* writer) {
  (void)context;
  ServerWriterWrapper< WrongthinkMessage> wrapper(writer);
  return GetWrongthinkMessagesImpl(request, &wrapper);
}

/* needed to make the rpc function testable */
Status WrongthinkServiceImpl::GetWrongthinkMessagesImpl(const GetWrongthinkMessagesRequest* request,
  ServerWriterWrapper< WrongthinkMessage>* writer) {
  int channelid = request->channelid();
  int limit = request->limit();
  int afterid = request->afterid();
  int afterdate = request->afterdate();
  try {

    soci::session sql = db->getSociSession();
    rowset<row> rs = db->getChannelMessages(sql, channelid);

    for (rowset<row>::const_iterator it = rs.begin(); it != rs.end(); ++it) {
      row const& row = *it;
      WrongthinkMessage msg;
      msg.set_uname(row.get<std::string>("uname"));
      msg.set_channelid(channelid);
      msg.set_userid(row.get<int>("user_id"));
      msg.set_threadid(row.get<int>("thread_id"));
      msg.set_threadchild(row.get<int>("thread_child"));
      msg.set_edited(row.get<int>("edited"));
      msg.set_text(row.get<std::string>("mtext"));
      std::tm tm = row.get<std::tm>("mdate");
      msg.set_date(mktime(&tm));
      msg.set_messageid(row.get<int>("msg_id"));
      writer->Write(msg);
    }
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return Status(StatusCode::INTERNAL, "");
  }
  return Status::OK;
}

Status WrongthinkServiceImpl::CreateUser(ServerContext* context, const CreateUserRequest* request,
  WrongthinkUser* response) {
  try {
    std::string uname = request->uname();
    std::string password = request->password();
    int admin = request->admin();
    int uid = 0;

    uid = db->createUser( uname, password, admin );

    response->set_userid(uid);
    response->set_uname(request->uname());
    response->set_admin(admin);
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return Status(StatusCode::INTERNAL, "");
  }
  return Status::OK;
}

bool WrongthinkServiceImpl::checkForChannel(int channelid, soci::session &sql) {
  if (channelMap.count(channelid) != 1) {

    auto r = db->getChannelRow( sql, channelid );

    if(r->get_indicator(0) != soci::i_null)
      channelMap.emplace( std::piecewise_construct,
                          std::forward_as_tuple(channelid),
                          std::forward_as_tuple(channelid,
                                                r->get<std::string>(0)));
    else
      return false;
  }
  return true;
}
