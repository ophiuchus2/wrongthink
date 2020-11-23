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
#include "DBPostgres.h"

DBPostgres::DBPostgres(const std::string &user, const std::string &pass, const std::string &dbName) :
  DBInterface(soci::postgresql, "host=localhost dbname=" + dbName + " user=" + user + " password=" + pass)
{
}

DBPostgres::~DBPostgres() {
}

void DBPostgres::clear() {
  soci::session sql = getSociSession();
  sql << "drop table if exists message";
  sql << "drop table if exists control_message";
  sql << "drop table if exists channels";
  sql << "drop table if exists communities";
  sql << "drop table if exists banned_users";
  sql << "drop table if exists banned_ips";
  sql << "drop table if exists users";
}

void DBPostgres::validate() {
  // assume that the wrongthink database & user have already been created (manually)
  soci::session sql = getSociSession();
  // create tables if they don't already exist
  // create users table
  sql << "create table if not exists users ("
          "user_id           serial    primary key,"
          "uname             varchar(50) unique not null,"
          "password          varchar(50) not null,"
          "admin             boolean default false)";

  sql << "create table if not exists banned_users ("
         "entry_id          serial primary key,"
         "user_id           int references users,"
         "expire            date not null default CURRENT_DATE + 3)";

  sql << "create table if not exists banned_ips ("
         "entry_id          serial primary key,"
         "ip                varchar(50) unique not null,"
         "expire            date not null)";

  // create community table
  sql << "create table if not exists communities ("
          "community_id       serial   primary key,"
          "name               varchar(100) unique not null,"
          "admin              int references users,"
          "public             boolean default true)";

  // create channel table
  sql <<  "create table if not exists channels ("
          "channel_id      serial  primary key,"
          "name            varchar(100) unique not null,"
          "community       int references communities,"
          "admin              int references users,"
          "allow_anon       boolean default true)";

  // create message table
  sql <<  "create table if not exists message ("
          "msg_id         serial primary key,"
          "user_id          int references users,"
          "channel        int references channels,"
          "thread_id      int,"
          "thread_child   boolean not null default false,"
          "edited         boolean default false,"
          "mtext          text not null,"
          "mdate          timestamp with time zone not null default clock_timestamp())";

  // create control message table
  sql <<  "create table if not exists control_message ("
          "msg_id         serial primary key,"
          "user_id          int references users,"
          "channel        int references channels,"
          "type           varchar(50),"
          "mtext          text not null,"
          "mdate          timestamp with time zone not null default clock_timestamp())";
}

bool DBPostgres::isUserBanned(const std::string& uname, const std::string& ip) {
  soci::session sql = getSociSession();
  std::tm date;
  sql << "select expire from banned_users where uname = :uname", use(uname), into(date);
  if(!sql.got_data()) return false;
  std::time_t te = std::mktime(&date);
  std::time_t tc = std::time(nullptr);
  if(tc > te) {
    sql << "delete from banned_users where uname = :uname", use(uname);
    return false;
  }
  if (!this->isIPBanned(ip))
    sql << "insert into banned_ips (ip,expire) values (:ip,:date)", use(ip), use(date);
  return true;
}

bool DBPostgres::isIPBanned(const std::string& ip) {
  soci::session sql = getSociSession();
  std::tm date;
  sql << "select expire from banned_ips where ip = :ip", use(ip), into(date);
  if(!sql.got_data()) return false;
  std::time_t te = std::mktime(&date);
  std::time_t tc = std::time(nullptr);
  if(tc > te) {
    sql << "delete from banned_ips where ip = :ip", use(ip);
    return false;
  }
  return true;
}

void DBPostgres::banUser(const std::string& uname, int days) {
  soci::session sql = getSociSession();
  int uid;
  sql << "select user_id from users where uname = :uname", use(uname), into(uid);
  if (sql.got_data()) {
    sql << "select * from banned_users where user_id = :uid", use(uid);
    if(sql.got_data()) {
      sql << "update banned_users set expire = CURRENT_DATE + :days where user_id = :uid",
            use(days), use(uid);
    } else {
      sql << "insert into banned_users (user_id,expire) values(:uid,CURRENT_DATE + :days)",
             use(uid), use(days);
    }
  } else {
    throw soci::soci_error("user not found");
  }
}

int DBPostgres::createUser(const std::string uname, const std::string password, int admin) {
  soci::session sql = getSociSession();
  int uid = 0;

  sql << "insert into users (uname,password,admin) values(:uname,:password,:admin)",
        use(uname), use(password), use(admin);
  sql << "select user_id from users where uname = :uname", use(uname), into(uid);

  return uid;
}


int DBPostgres::createChannel(const std::string name, const int community, const int admin_id, const int anonymous) {
  soci::session sql = getSociSession();
  int channel_id = 0;

  sql << "insert into channels(name, "
      << "community, admin, allow_anon) "
      << "values(:name,:community,:admin,:anonymous)",
       use(name), use(community), use(admin_id), use(anonymous);
  sql << "select channel_id from channels where name = :name",
    use(name), into(channel_id);

  return channel_id;
}

int DBPostgres::createCommunity(const std::string name, const int admin, const int pub) {
  soci::session sql = getSociSession();
  int community_id;

  sql << "insert into communities (name, admin, public) "
      << "values(:name,:admin,:public)", use(name), use(admin), use(pub);
  sql << "select community_id from communities where name = :name",
    use(name), into(community_id);

  return community_id;
}

rowset<row> DBPostgres::getCommunityRowset(soci::session &sql) {
  rowset<row> rs = (sql.prepare << "select * from communities "
                                << "inner join users on "
                                << "communities.admin=users.user_id");

  return rs;
}


rowset<row> DBPostgres::getCommunityChannelsRowset(soci::session &sql, const int community_id) {
  rowset<row> rs = (sql.prepare << "select * from channels "
                                << "inner join users on channels.admin=users.user_id "
                                << "where community=:community order by channels.channel_id",
                                use(community_id));

  return rs;
}

rowset<row> DBPostgres::getChannelMessages(soci::session &sql, const int channel_id) {
  rowset<row> rs = (sql.prepare << "select * from message inner join users on "
              << "message.user_id = users.user_id where "
              << "message.channel = :channelid order by message.msg_id", use(channel_id));

  return rs;
}


std::unique_ptr<row> DBPostgres::getChannelRow(soci::session &sql, const int channel_id) {
  std::unique_ptr<row> r(new row());

  sql << "select name from channels where channel_id = :id",
        use(channel_id), into(*r);

  return r;
}
