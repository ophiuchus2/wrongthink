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
#ifndef DB_POSTGRES_H
#define DB_POSTGRES_H

#include "DBInterface.h"
#include "soci-postgresql.h"

class DBPostgres : public DBInterface {
public:
  DBPostgres(const std::string &user, const std::string &pass, const std::string &dbName);
  ~DBPostgres();
  virtual void validate() override;
  virtual void clear() override;

  virtual int createUser(std::string uname, std::string password, int admin) override;
  virtual int createChannel(std::string name, int community, int admin_id, int anonymous) override;
  virtual int createCommunity(std::string name, int admin, int pub) override;
  virtual rowset<row> getCommunityRowset(soci::session &sql) override;
  virtual rowset<row> getCommunityChannelsRowset(soci::session &sql, int community_id) override;
  virtual rowset<row> getChannelMessages(soci::session &sql, int channel_id) override;
  virtual std::unique_ptr<row> getChannelRow(soci::session &sql, int channel_id) override;
};

#endif // DB_POSTGRES_H
