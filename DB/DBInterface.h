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
#ifndef DB_INTERFACE_H
#define DB_INTERFACE_H

#include "soci.h"

using soci::session;
using soci::row;
using soci::rowset;
using soci::statement;
using soci::use;
using soci::into;

class DBInterface {
public:
  virtual ~DBInterface();

  virtual void validate() = 0;
  virtual void clear() = 0;
  soci::session getSociSession();

  virtual int createUser( std::string uname, std::string password, int admin ) = 0;
  virtual int createChannel(std::string name, int community, int admin_id, int anonymous) = 0;
  virtual int createCommunity(std::string name, int admin, int pub) = 0;
  virtual rowset<row> getCommunityRowset(soci::session &sql) = 0;
  virtual rowset<row> getCommunityChannelsRowset(soci::session &sql, int community_id) = 0;
  virtual rowset<row> getChannelMessages(soci::session &sql, int channel_id) = 0;
  virtual std::unique_ptr<row> getChannelRow(soci::session &sql, int channel_id) = 0;

protected:
  DBInterface( const soci::backend_factory &backend, std::string conString );

  const soci::backend_factory &dbType_;
  std::string dbConnectString_;

};

#endif // DB_INTERFACE_H
