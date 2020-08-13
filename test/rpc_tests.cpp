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
#include "Util.h"
#include <grpcpp/grpcpp.h>
#include "wrongthink.grpc.pb.h"
#include <vector>
#include <iostream>

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



  TEST(RpcSuite, TestRpc) {

    WrongthinkUtils::setCredentials("wrongthink", "test", "testdb");
    WrongthinkUtils::clearDatabase();
    WrongthinkUtils::validateDatabase();

    WrongthinkServiceImpl service;

    // create dummy user
    CreateUserRequest ureq;
    WrongthinkUser uresp;
    ureq.set_uname("user1");
    ureq.set_password("upass");
    ureq.set_admin(true);

    Status st = service.CreateUser(nullptr, &ureq, &uresp);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(uresp.userid(), 0);
    //std::cout << "userid: " << uresp.userid() << std::endl;

    // create dummy communities
    CreateWrongthinkCommunityRequest mc, mc1;
    WrongthinkCommunity communityResp, communityResp1;

    mc.set_name("com1");
    mc.set_adminid(uresp.userid());
    mc.set_public_(true);

    mc1.set_name("com2");
    mc1.set_adminid(uresp.userid());
    mc1.set_public_(true);

    st = service.CreateWrongthinkCommunity(nullptr, &mc, &communityResp);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(communityResp.communityid(), 0);

    st = service.CreateWrongthinkCommunity(nullptr, &mc1, &communityResp1);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(communityResp1.communityid(), 0);

    // test create / get channels
    CreateWrongThinkChannelRequest mch, mch1, mch2;
    WrongthinkChannel resp, resp1, resp2;

    mch.set_name("channel 1");
    mch.set_communityid(communityResp.communityid());
    mch.set_anonymous(true);
    mch.set_adminid(uresp.userid());

    mch1.set_name("channel 2");
    mch1.set_communityid(communityResp.communityid());
    mch1.set_anonymous(true);
    mch1.set_adminid(uresp.userid());

    mch2.set_name("channel 3");
    mch2.set_communityid(communityResp1.communityid());
    mch2.set_anonymous(true);
    mch2.set_adminid(uresp.userid());

    st = service.CreateWrongthinkChannel(nullptr,
                      &mch, &resp);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(resp.channelid(), 0);

    st = service.CreateWrongthinkChannel(nullptr,
                      &mch1, &resp1);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(resp1.channelid(), 0);

    st = service.CreateWrongthinkChannel(nullptr,
                      &mch2, &resp2);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(resp2.channelid(), 0);

    // get wrongthink channels
    ServerWriterWrapper<WrongthinkChannel> getChannelWrapper, getChannelWrapper1;
    GetWrongthinkChannelsRequest getChannelReq, getChannelReq1;

    getChannelReq.set_communityid(communityResp.communityid());
    st = service.GetWrongthinkChannelsImpl(&getChannelReq, &getChannelWrapper);

    EXPECT_TRUE(st.ok());

    std::vector<WrongthinkChannel>& channelList = getChannelWrapper.getObjList();
    //expecting two channels
    EXPECT_EQ(channelList.size(), 2);

    WrongthinkChannel ch = channelList[0];
    WrongthinkChannel ch1 = channelList[1];

    EXPECT_EQ(ch.name(), "channel 1");
    EXPECT_EQ(ch.channelid(), resp.channelid());
    EXPECT_EQ(ch.anonymous(), true);
    EXPECT_EQ(ch.communityid(), communityResp.communityid());

    EXPECT_EQ(ch1.name(), "channel 2");
    EXPECT_EQ(ch1.channelid(), resp1.channelid());
    EXPECT_EQ(ch1.anonymous(), true);
    EXPECT_EQ(ch1.communityid(), communityResp.communityid());

    getChannelReq1.set_communityid(communityResp1.communityid());
    st = service.GetWrongthinkChannelsImpl(&getChannelReq1, &getChannelWrapper1);

    EXPECT_TRUE(st.ok());

    channelList = getChannelWrapper1.getObjList();
    //expecting one channel
    EXPECT_EQ(channelList.size(), 1);

    WrongthinkChannel ch2 = channelList[0];

    EXPECT_EQ(ch2.name(), "channel 3");
    EXPECT_EQ(ch2.channelid(), resp2.channelid());
    EXPECT_EQ(ch2.anonymous(), true);
    EXPECT_EQ(ch2.communityid(), communityResp1.communityid());

    // send message test
    ServerReaderWrapper< WrongthinkMessage> sendWrapper;
    std::vector<WrongthinkMessage>& msgList = sendWrapper.getObjList();

    WrongthinkMessage msg1, msg2;
    msg1.set_uname(ureq.uname());
    msg1.set_channelname("channel 1");
    msg1.set_channelid(ch.channelid());
    msg1.set_userid(uresp.userid());
    msg1.set_text("msg1");

    msg2.set_uname(ureq.uname());
    msg2.set_channelname("channel 1");
    msg2.set_channelid(ch.channelid());
    msg2.set_userid(uresp.userid());
    msg2.set_text("msg2");

    msgList.push_back(msg1);
    msgList.push_back(msg2);

    st = service.SendWrongthinkMessageImpl(&sendWrapper, nullptr);
    ASSERT_TRUE(st.ok());

    // get message test
    ServerWriterWrapper< WrongthinkMessage> getMessageWrapper;
    GetWrongthinkMessagesRequest getMsgReq;

    getMsgReq.set_channelid(ch.channelid());
    st = service.GetWrongthinkMessagesImpl(&getMsgReq, &getMessageWrapper);
    ASSERT_TRUE(st.ok());

    std::vector<WrongthinkMessage>& msgList1 = getMessageWrapper.getObjList();

    //expecting two messages
    EXPECT_EQ(msgList.size(), 2);

    WrongthinkMessage rMsg1 = msgList1[0];
    WrongthinkMessage rMsg2 = msgList1[1];

    EXPECT_EQ(rMsg1.uname(), "user1");
    //EXPECT_EQ(rMsg1.channelname(), "channel 1");
    EXPECT_EQ(rMsg1.channelid(), ch.channelid());
    EXPECT_EQ(rMsg1.userid(), uresp.userid());
    EXPECT_EQ(rMsg1.text(), "msg2");

    EXPECT_EQ(rMsg2.uname(), "user1");
    //EXPECT_EQ(rMsg1.channelname(), "channel 1");
    EXPECT_EQ(rMsg2.channelid(), ch.channelid());
    EXPECT_EQ(rMsg2.userid(), uresp.userid());
    EXPECT_EQ(rMsg2.text(), "msg1");

  }

}
