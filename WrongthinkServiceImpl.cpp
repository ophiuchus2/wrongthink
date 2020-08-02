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
#include "WrongthinkServiceImpl.h"

Status WrongthinkServiceImpl::GetWrongthinkChannels(ServerContext* context,
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

Status WrongthinkServiceImpl::CreateWrongthinkChannel(ServerContext* context,
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

Status WrongthinkServiceImpl::SendWrongthinkMessage(ServerContext* context,
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

Status WrongthinkServiceImpl::ListenWrongthinkMessages(ServerContext* context,
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

Status WrongthinkServiceImpl::GetWrongthinkMessages(ServerContext* context,
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

Status WrongthinkServiceImpl::CreateUser(ServerContext* context, const CreateUserRequest* request,
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

bool WrongthinkServiceImpl::checkForChannel(int channelid, soci::session& sql) {
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
