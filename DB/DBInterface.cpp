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
#include "DBInterface.h"

DBInterface::DBInterface( const soci::backend_factory *backend, const std::string conString ) :
  dbType_{backend}, dbConnectString_{conString}
{
};

soci::session DBInterface::getSociSession() {
  return soci::session(*dbType_, dbConnectString_);
}

DBInterface::~DBInterface(){
}
