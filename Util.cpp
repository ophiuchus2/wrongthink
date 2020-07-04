#include "Util.h"
#include "soci.h"
#include "soci-postgresql.h"

namespace WrongthinkUtils {
  void validateDatabase(const std::string& user, const std::string& pass) {
    // assume that the wrongthink database & user have already been created (manually)
    soci::session sql(soci::postgresql, "host=localhost dbname=wrongthink user=" + user + " password=" + pass);
    // create tables if they don't already exist
    // create users table
    sql << "create table if not exists users ("
            "user_id           int    primary key,"
            "uname             varchar(50) unique not null,"
            "password          varchar(50) not null,"
            "admin             boolean default false)";

    // create community table
    sql << "create table if not exists communities ("
            "community_id       int   primary key,"
            "name               varchar(100) unique not null,"
            "admin              int references users,"
            "public             boolean)";

    // create channel table
    sql <<  "create table if not exists channels ("
            "channel_id      int  primary key,"
            "name            varchar(100) unique not null,"
            "community       int references communities,"
            "admin              int references users,"
            "allow_anon       boolean default true)";

    // create message table
    sql <<  "create table if not exists message ("
            "msg_id         int primary key,"
            "uname          int references users,"
            "channel        int references channels,"
            "thread_id      int,"
            "thread_child   boolean not null default false,"
            "edited         boolean default false,"
            "mtext          text not null,"
            "mdate          date not null default now())";

    // create control message table
    sql <<  "create table if not exists control_message ("
            "msg_id         int primary key,"
            "uname          int references users,"
            "channel        int references channels,"
            "type           varchar(50),"
            "mtext          text not null,"
            "mdate          date not null default now())";
  }
}
