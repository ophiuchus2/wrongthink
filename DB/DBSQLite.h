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
#ifndef DB_SQLITE_H
#define DB_SQLITE_H

#include "DBInterface.h"
#include "DBPostgres.h"
#include "soci-sqlite3.h"

class SQLiteDB : public DBPostgres {
public:
  SQLiteDB(const std::string &filename);
  virtual ~SQLiteDB() {}
  virtual void validate() override;
};

#endif // DB_SQLITE_H
