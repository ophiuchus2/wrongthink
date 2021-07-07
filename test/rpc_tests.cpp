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
#include <thread>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "DB/DBPostgres.h"
#include "Authentication/WrongthinkTokenAuthenticator.h"
#include "DB/DBSQLite.h"

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

// include interceptor classes
#include "Interceptors/Interceptor.h"

namespace {


  class RpcSuiteTest : public ::testing::TestWithParam<std::shared_ptr<DBInterface>> {
  protected:

    RpcSuiteTest() {
      coinfigureLog();
      db = GetParam();
      service.reset(new WrongthinkServiceImpl(db, logger_));
      //db = std::make_shared<DBPostgres>( "wrongthink", "test", "testdb" );
      db->clear();
      db->validate();

      std::vector<
        std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>>
        creators;
      creators.push_back(
          std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>(
              new WrongthinkInterceptors::LoggingInterceptorFactory(db, logger_)));
      ServerBuilder builder;
      this->server_address_ = "localhost:50052";
      //auto server_creds = grpc::SslServerCredentials({});
      // add server credential processor
      //server_creds->SetAuthMetadataProcessor(
        //std::make_shared<WrongthinkTokenAuth::WrongthinkAuthMetadataProcessor>(true));
      builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
      builder.RegisterService(service.get());
      builder.experimental().SetInterceptorCreators(std::move(creators));
      this->server_ = builder.BuildAndStart();
      logger_->info("test server listening on {}", server_address_);
    }

    void coinfigureLog() {
      auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      console_sink->set_level(spdlog::level::trace);
      console_sink->set_pattern("[%H:%M:%S %z] [wrongthink] [%^%l%$] %v");

      auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/wrongthink_test.txt", true);
      file_sink->set_level(spdlog::level::trace);

      std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
      logger_.reset(new spdlog::logger("multi_sink", sinks.begin(), sinks.end()));
      logger_->set_level(spdlog::level::trace);
      logger_->info("logger started");
    }

    void SetUp() override {
      db->clear();
      db->validate();
    }

    void TearDown() override {
      db->clear();
      if(server_)
        server_->Shutdown();
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

    //std::shared_ptr<DBInterface> db{std::make_shared<DBPostgres>( "wrongthink", "test", "testdb" )};
    std::shared_ptr<DBInterface> db{std::make_shared<SQLiteDB>("sqlite.db")};
    std::vector<WrongthinkUser> users;
    std::vector<WrongthinkCommunity> communities;
    std::vector<WrongthinkChannel> channels;
    WrongthinkUser mUResp;
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<WrongthinkServiceImpl> service = std::make_shared<WrongthinkServiceImpl>(db, logger_);
    // server members
    std::string server_address_;
    std::unique_ptr<Server> server_;
  };

  TEST_P(RpcSuiteTest, TestBanUser) {

    // generate user
    WrongthinkUser admin;
    Status st = service->GenerateUser(nullptr, nullptr, &admin);
    ASSERT_TRUE(st.ok());
    ASSERT_TRUE(admin.admin());

    auto call_creds = grpc::MetadataCredentialsFromPlugin(
    std::unique_ptr<grpc::MetadataCredentialsPlugin>(
        new WrongthinkTokenAuth::WrongthinkClientTokenPlugin(admin.uname(), admin.token())));
    //auto channel_creds =  grpc::InsecureChannelCredentials();
    //auto combined_creds = grpc::CompositeChannelCredentials(channel_creds, call_creds);
    /*auto channel =
        grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());*/

    //grpc_impl::ChannelArguments &args
    auto channel = server_->InProcessChannel({});
    auto mstub = wrongthink::NewStub(channel);
    
    grpc::ClientContext ctx;
    WrongthinkTokenAuth::addCredentials(&ctx, &admin);
    //ctx.set_credentials(call_creds);

    WrongthinkUser banned;
    GenericRequest greq;
    st = mstub->GenerateUser(&ctx, greq, &banned);
    //logger_->info("generate user error message: {}", st.error_message());
    ASSERT_TRUE(st.ok());
    ASSERT_TRUE(!banned.admin());

    BanUserRequest req;
    req.set_uname(banned.uname());
    req.set_days(3);

    grpc::ClientContext ctx1;
    WrongthinkTokenAuth::addCredentials(&ctx1, &admin);
    //ctx1.set_credentials(call_creds);
    GenericResponse resp;
    st = mstub->BanUser(&ctx1, req, &resp);
    logger_->info("call_creds debug string: {}", call_creds->DebugString());
    logger_->info("ban user error message: {}", st.error_message());
    ASSERT_TRUE(st.ok());
    

    //TODO: check banned user list
    //...

    // generate new user
    WrongthinkUser b1;
    GenericRequest g1;
    grpc::ClientContext ctx2;
    st = mstub->GenerateUser(&ctx2, g1, &b1);
    ASSERT_TRUE(st.ok());
    ASSERT_TRUE(!b1.admin());

    // create a channel with bad credentials
    call_creds = grpc::MetadataCredentialsFromPlugin(
    std::unique_ptr<grpc::MetadataCredentialsPlugin>(
        new WrongthinkTokenAuth::WrongthinkClientTokenPlugin("abc", "def")));
    //channel_creds =  grpc::InsecureChannelCredentials();
    //combined_creds = grpc::CompositeChannelCredentials(channel_creds, call_creds);
    //channel = grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());
    //mstub = wrongthink::NewStub(channel);

    //dummy user
    WrongthinkUser u1;
    u1.set_uname("abc");
    u1.set_token("def");
    BanUserRequest r1;
    r1.set_uname(b1.uname());
    r1.set_days(3);
    grpc::ClientContext ctx3;
    WrongthinkTokenAuth::addCredentials(&ctx3, &u1);
    //ctx3.set_credentials(call_creds);
    st = mstub->BanUser(&ctx3, r1, nullptr);
    ASSERT_TRUE(!st.ok());
    ASSERT_EQ(st.error_code(), StatusCode::UNAUTHENTICATED);
    ASSERT_EQ(st.error_message(), "Invalid user");

    // create channel with no creds
    channel = grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());
    mstub = wrongthink::NewStub(channel);

    grpc::ClientContext ctx4;
    st = mstub->BanUser(&ctx4, r1, nullptr);
    ASSERT_TRUE(!st.ok());
    ASSERT_EQ(st.error_code(), StatusCode::UNAUTHENTICATED);
    ASSERT_EQ(st.error_message(), "No credentials attached to the channel");

    // create new user
    // generate new user
    WrongthinkUser b2;
    grpc::ClientContext ctx5;
    st = mstub->GenerateUser(&ctx5, g1, &b2);
    ASSERT_TRUE(st.ok());
    ASSERT_TRUE(!b2.admin());

    // create a channel with b2's credentials
    call_creds = grpc::MetadataCredentialsFromPlugin(
    std::unique_ptr<grpc::MetadataCredentialsPlugin>(
        new WrongthinkTokenAuth::WrongthinkClientTokenPlugin(b2.uname(), b2.token())));
    //channel_creds =  grpc::InsecureChannelCredentials();
    //combined_creds = grpc::CompositeChannelCredentials(channel_creds, call_creds);
    channel = grpc::CreateChannel(server_address_, grpc::InsecureChannelCredentials());
    mstub = wrongthink::NewStub(channel);
    grpc::ClientContext ctx6;
    //ctx6.set_credentials(call_creds);
    WrongthinkTokenAuth::addCredentials(&ctx6, &b2);
    st = mstub->BanUser(&ctx6, r1, nullptr);
    ASSERT_TRUE(!st.ok());
    ASSERT_EQ(st.error_code(), StatusCode::UNAUTHENTICATED);
    // b2 is not an admin
    ASSERT_EQ(st.error_message(), "Invalid permission");

  }

  TEST_P(RpcSuiteTest, TestGenerateUser) {
    auto db = GetParam();
    WrongthinkUser resp;
    Status st = service->GenerateUser(nullptr, nullptr, &resp);
    std::cout << "TestGenerateUser: uname: " << resp.uname()
              << " token: " << resp.token() << std::endl;
    ASSERT_TRUE(st.ok());
    EXPECT_NE(resp.userid(), 0);
  }

  TEST_P(RpcSuiteTest, TestCreateUser) {
    WrongthinkUser resp;
    Status st = setupUser(resp, nullptr);
    ASSERT_TRUE(st.ok());
    EXPECT_NE(resp.userid(), 0);

    // negative case
    st = setupUser(resp, nullptr);
    ASSERT_TRUE(!st.ok());
  }

  TEST_P(RpcSuiteTest, TestCreateCommunity) {
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

  TEST_P(RpcSuiteTest, TestGetCommunity) {
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

  TEST_P(RpcSuiteTest, TestCreateChannel) {
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

  TEST_P(RpcSuiteTest, TestGetChannel) {
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

  TEST_P(RpcSuiteTest, TestMessages) {
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
    EXPECT_EQ(rMsg1.text(), "msg1");

    EXPECT_EQ(rMsg2.uname(), uresp.uname());
    //EXPECT_EQ(rMsg1.channelname(), "channel 1");
    EXPECT_EQ(rMsg2.channelid(), chresp.channelid());
    EXPECT_EQ(rMsg2.userid(), uresp.userid());
    EXPECT_EQ(rMsg2.text(), "msg2");
  }

  TEST_P(RpcSuiteTest, TestMessageWeb) {
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
    EXPECT_EQ(rMsg1.text(), "msg1");

    EXPECT_EQ(rMsg2.uname(), uresp.uname());
    //EXPECT_EQ(rMsg1.channelname(), "channel 1");
    EXPECT_EQ(rMsg2.channelid(), chresp.channelid());
    EXPECT_EQ(rMsg2.userid(), uresp.userid());
    EXPECT_EQ(rMsg2.text(), "msg2");
  }

  auto tValues = ::testing::Values(
                std::make_shared<SQLiteDB>("sqlite.db"), 
                std::make_shared<DBPostgres>( "wrongthink", "test", "testdb" )
        );

  INSTANTIATE_TEST_CASE_P(
        DBTEST,
        RpcSuiteTest,
        tValues
        );

  /*INSTANTIATE_TEST_CASE_P(
        TestGenerateUser,
        RpcSuiteTest,
        tValues
        );

  INSTANTIATE_TEST_CASE_P(
        TestCreateUser,
        RpcSuiteTest,
        tValues
        );

  INSTANTIATE_TEST_CASE_P(
        TestCreateCommunity,
        RpcSuiteTest,
        tValues
        );

  INSTANTIATE_TEST_CASE_P(
        TestGetCommunity,
        RpcSuiteTest,
        tValues
        );

  INSTANTIATE_TEST_CASE_P(
        TestCreateChannel,
        RpcSuiteTest,
        tValues
        );

  INSTANTIATE_TEST_CASE_P(
        TestGetChannel,
        RpcSuiteTest,
        tValues
        );

  INSTANTIATE_TEST_CASE_P(
        TestMessages,
        RpcSuiteTest,
        tValues
        );
  INSTANTIATE_TEST_CASE_P(
        TestMessagesWeb,
        RpcSuiteTest,
        tValues
        );*/
}
