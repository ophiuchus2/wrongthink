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
#include "gtest/gtest.h"
#include "WrongthinkServiceImpl.h"
#include <grpcpp/grpcpp.h>
#include "wrongthink.grpc.pb.h"
#include <vector>
#include <iostream>
#include "DB/DBPostgres.h"

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

namespace {


  class RpcSuiteTest : public ::testing::Test {
  protected:
    void SetUp() override {
      db = std::make_shared<DBPostgres>( "wrongthink", "test", "testdb" );
      db->clear();
      db->validate();
      service = std::make_shared<WrongthinkServiceImpl>(db);
    }

    void TearDown() override {
      db->clear();
    }

    Status setupUser(WrongthinkUser& uresp, CreateUserRequest* req) {
      // create dummy user
      CreateUserRequest ureq;
      ureq.set_uname("user1");
      ureq.set_password("upass");
      ureq.set_admin(true);
      Status st = service->CreateUser(nullptr, (req) ? req : &ureq, &uresp);
      if (st.ok())
        users.push_back(uresp);
      return st;
    }

    Status setupCommunity(WrongthinkCommunity& communityResp,
      CreateWrongthinkCommunityRequest* req) {
      // create dummy communities
      CreateWrongthinkCommunityRequest mc;

      mc.set_name("com1");
      mc.set_adminid(users[0].userid());
      mc.set_public_(true);

      Status st = service->CreateWrongthinkCommunity(nullptr, (req) ? req : &mc,
        &communityResp);
      if (st.ok())
        communities.push_back(communityResp);
      return st;
    }

    Status setupChannel(WrongthinkChannel& resp,
      CreateWrongThinkChannelRequest* req) {
      // test create / get channels
      CreateWrongThinkChannelRequest mch;

      mch.set_name("channel 1");
      mch.set_communityid(communities[0].communityid());
      mch.set_anonymous(true);
      mch.set_adminid(users[0].userid());

      Status st = service->CreateWrongthinkChannel(nullptr,
                        (req) ? req : &mch, &resp);
      if (st.ok())
        channels.push_back(resp);
      return st;
    }

    std::shared_ptr<DBInterface> db;
    std::shared_ptr<WrongthinkServiceImpl> service;
    std::vector<WrongthinkUser> users;
    std::vector<WrongthinkCommunity> communities;
    std::vector<WrongthinkChannel> channels;
    WrongthinkUser mUResp;
  };

  TEST_F(RpcSuiteTest, TestCreateUser) {
    WrongthinkUser resp;
    Status st = setupUser(resp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(resp.userid(), 0);

    // negative case
    st = setupUser(resp, nullptr);
    ASSERT_TRUE(!st.ok());
  }

  TEST_F(RpcSuiteTest, TestCreateCommunity) {
    // create dummy user
    WrongthinkUser uresp;
    Status st = setupUser(uresp, nullptr);

    // create dummy community
    WrongthinkCommunity cresp;
    st = setupCommunity(cresp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(cresp.communityid(), 0);

    st = setupCommunity(cresp, nullptr);
    ASSERT_TRUE(!st.ok());
  }

  TEST_F(RpcSuiteTest, TestGetCommunity) {
    constexpr int COUNT = 10;
    // create dummy user
    WrongthinkUser uresp;
    Status st = setupUser(uresp, nullptr);
    // create some dummy communities
    std::string cname("com");
    for (int i = 0; i < COUNT; i++) {
      WrongthinkCommunity communityResp;
      CreateWrongthinkCommunityRequest mc;
      mc.set_name("com" + std::to_string(i));
      mc.set_adminid(uresp.userid());
      mc.set_public_(true);
      st = setupCommunity(communityResp, &mc);
      ASSERT_TRUE(st.ok());
      EXPECT_NE(communityResp.communityid(), 0);
    }

    ServerWriterWrapper<WrongthinkCommunity> wrapper;
    st = service->GetWrongthinkCommunitiesImpl(nullptr, &wrapper);
    ASSERT_TRUE(st.ok());

    std::vector<WrongthinkCommunity>& objList = wrapper.getObjList();
    ASSERT_EQ(objList.size(), 10);
    for (int i = 0; i < COUNT; i++) {
      WrongthinkCommunity& com = objList[i];
      EXPECT_TRUE(com.name() == ("com" + std::to_string(i)));
    }
  }

  TEST_F(RpcSuiteTest, TestCreateChannel) {
    // create dummy user
    WrongthinkUser uresp;
    Status st = setupUser(uresp, nullptr);
    ASSERT_TRUE(st.ok());

    // create dummy community
    WrongthinkCommunity cresp;
    st = setupCommunity(cresp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(cresp.communityid(), 0);

    WrongthinkChannel chresp;
    st = setupChannel(chresp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(chresp.channelid(), 0);

    st = setupChannel(chresp, nullptr);
    ASSERT_TRUE(!st.ok());
  }

  TEST_F(RpcSuiteTest, TestGetChannel) {
    constexpr int COUNT = 10;
    // create dummy user
    WrongthinkUser uresp;
    Status st = setupUser(uresp, nullptr);
    ASSERT_TRUE(st.ok());

    // create dummy community
    WrongthinkCommunity cresp;
    st = setupCommunity(cresp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(cresp.communityid(), 0);

    // create some dummy channels
    std::string cname("chan");
    for (int i = 0; i < COUNT; i++) {
      WrongthinkChannel chresp;
      CreateWrongThinkChannelRequest mch;
      mch.set_name(cname + std::to_string(i));
      mch.set_communityid(cresp.communityid());
      mch.set_anonymous(true);
      mch.set_adminid(uresp.userid());
      st = setupChannel(chresp, &mch);
      ASSERT_TRUE(st.ok());
      EXPECT_NE(chresp.channelid(), 0);
    }

    GetWrongthinkChannelsRequest req;
    ServerWriterWrapper<WrongthinkChannel> wrapper;

    req.set_communityid(cresp.communityid());
    st = service->GetWrongthinkChannelsImpl(&req, &wrapper);
    ASSERT_TRUE(st.ok());

    std::vector<WrongthinkChannel>& objList = wrapper.getObjList();
    for (int i = 0; i < COUNT; i++) {
      WrongthinkChannel& channel = objList[i];
      //std::cout << "channel name: " << channel.name() << std::endl;
      EXPECT_TRUE(channel.name() == (cname + std::to_string(i)));
    }
  }

  TEST_F(RpcSuiteTest, TestMessages) {
    // create dummy user
    WrongthinkUser uresp;
    Status st = setupUser(uresp, nullptr);
    ASSERT_TRUE(st.ok());

    // create dummy community
    WrongthinkCommunity cresp;
    st = setupCommunity(cresp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(cresp.communityid(), 0);

    // create dummy channel
    WrongthinkChannel chresp;
    st = setupChannel(chresp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(chresp.channelid(), 0);

    // send message test
    ServerReaderWrapper< WrongthinkMessage> sendWrapper;
    std::vector<WrongthinkMessage>& msgList = sendWrapper.getObjList();

    WrongthinkMessage msg1, msg2;
    msg1.set_uname(uresp.uname());
    msg1.set_channelname(chresp.name());
    msg1.set_channelid(chresp.channelid());
    msg1.set_userid(uresp.userid());
    msg1.set_text("msg1");

    msg2.set_uname(uresp.uname());
    msg2.set_channelname(chresp.name());
    msg2.set_channelid(chresp.channelid());
    msg2.set_userid(uresp.userid());
    msg2.set_text("msg2");

    msgList.push_back(msg1);
    msgList.push_back(msg2);

    st = service->SendWrongthinkMessageImpl(&sendWrapper, nullptr);
    ASSERT_TRUE(st.ok());

    // get message test
    ServerWriterWrapper< WrongthinkMessage> getMessageWrapper;
    GetWrongthinkMessagesRequest getMsgReq;

    getMsgReq.set_channelid(chresp.channelid());
    st = service->GetWrongthinkMessagesImpl(&getMsgReq, &getMessageWrapper);
    ASSERT_TRUE(st.ok());

    std::vector<WrongthinkMessage>& msgList1 = getMessageWrapper.getObjList();

    //expecting two messages
    EXPECT_EQ(msgList1.size(), 2);

    WrongthinkMessage rMsg1 = msgList1[0];
    WrongthinkMessage rMsg2 = msgList1[1];

    EXPECT_EQ(rMsg1.uname(), uresp.uname());
    //EXPECT_EQ(rMsg1.channelname(), "channel 1");
    EXPECT_EQ(rMsg1.channelid(), chresp.channelid());
    EXPECT_EQ(rMsg1.userid(), uresp.userid());
    EXPECT_EQ(rMsg1.text(), "msg2");

    EXPECT_EQ(rMsg2.uname(), uresp.uname());
    //EXPECT_EQ(rMsg1.channelname(), "channel 1");
    EXPECT_EQ(rMsg2.channelid(), chresp.channelid());
    EXPECT_EQ(rMsg2.userid(), uresp.userid());
    EXPECT_EQ(rMsg2.text(), "msg1");
  }

  TEST_F(RpcSuiteTest, TestMessageWeb) {
    // create dummy user
    WrongthinkUser uresp;
    Status st = setupUser(uresp, nullptr);
    ASSERT_TRUE(st.ok());

    // create dummy community
    WrongthinkCommunity cresp;
    st = setupCommunity(cresp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(cresp.communityid(), 0);

    // create dummy channel
    WrongthinkChannel chresp;
    st = setupChannel(chresp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(chresp.channelid(), 0);

    // send message test
    WrongthinkMessage msg1, msg2;
    msg1.set_uname(uresp.uname());
    msg1.set_channelname(chresp.name());
    msg1.set_channelid(chresp.channelid());
    msg1.set_userid(uresp.userid());
    msg1.set_text("msg1");

    msg2.set_uname(uresp.uname());
    msg2.set_channelname(chresp.name());
    msg2.set_channelid(chresp.channelid());
    msg2.set_userid(uresp.userid());
    msg2.set_text("msg2");

    st = service->SendWrongthinkMessageWeb(nullptr, &msg1, nullptr);
    ASSERT_TRUE(st.ok());

    st = service->SendWrongthinkMessageWeb(nullptr, &msg2, nullptr);
    ASSERT_TRUE(st.ok());

    // get message test
    ServerWriterWrapper< WrongthinkMessage> getMessageWrapper;
    GetWrongthinkMessagesRequest getMsgReq;

    getMsgReq.set_channelid(chresp.channelid());
    st = service->GetWrongthinkMessagesImpl(&getMsgReq, &getMessageWrapper);
    ASSERT_TRUE(st.ok());

    std::vector<WrongthinkMessage>& msgList1 = getMessageWrapper.getObjList();

    //expecting two messages
    EXPECT_EQ(msgList1.size(), 2);

    WrongthinkMessage rMsg1 = msgList1[0];
    WrongthinkMessage rMsg2 = msgList1[1];

    EXPECT_EQ(rMsg1.uname(), uresp.uname());
    //EXPECT_EQ(rMsg1.channelname(), "channel 1");
    EXPECT_EQ(rMsg1.channelid(), chresp.channelid());
    EXPECT_EQ(rMsg1.userid(), uresp.userid());
    EXPECT_EQ(rMsg1.text(), "msg2");

    EXPECT_EQ(rMsg2.uname(), uresp.uname());
    //EXPECT_EQ(rMsg1.channelname(), "channel 1");
    EXPECT_EQ(rMsg2.channelid(), chresp.channelid());
    EXPECT_EQ(rMsg2.userid(), uresp.userid());
    EXPECT_EQ(rMsg2.text(), "msg1");
  }

}
