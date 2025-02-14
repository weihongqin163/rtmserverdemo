#include <iostream>
#include <memory>
#include <string>
#include <exception>
#include <thread>
#include <chrono>
#include <csignal>

#include "IAgoraRtmClient.h"
#include "rtm_quick_start.h"



using namespace agora::rtm;
class RtmDemo;
class RtmEventHandler;



/*
RtmEventHandler IMPL
 */

  RtmEventHandler::RtmEventHandler(ServerBase* inst)
  {
    rtminst_ = inst;
  }

  // Add the event listener
  void RtmEventHandler::onLoginResult(const uint64_t requestId, RTM_ERROR_CODE errorCode)  {
    cbPrint("onLoginResult, request id: %lld, errorCode: %d", requestId, errorCode);
  }

  void RtmEventHandler::onLogoutResult(const uint64_t requestId, RTM_ERROR_CODE errorCode) {
    cbPrint("onLogoutResult, request id: %lld, errorCode: %d", requestId, errorCode);
  }

  void RtmEventHandler::onConnectionStateChanged(const char *channelName, RTM_CONNECTION_STATE state, RTM_CONNECTION_CHANGE_REASON reason)  {
    cbPrint("onConnectionStateChanged, channelName: %s, state: %d, reason: %d", channelName, state, reason);
  }

  void RtmEventHandler::onLinkStateEvent(const LinkStateEvent& event)  {
    cbPrint("onLinkStateEvent, state: %d -> %d, operation: %d, reason: %s", event.previousState, event.currentState, event.operation, event.reason);
  }

  void RtmEventHandler::onPublishResult(const uint64_t requestId, RTM_ERROR_CODE errorCode)  {
    cbPrint("onPublishResult request id: %lld result: %d", requestId, errorCode);
  }

  void RtmEventHandler::onMessageEvent(const MessageEvent &event)  {
    cbPrint("receive message from: %s, message: %s", event.publisher, event.message);
    if (rtminst_)
    {
      std::string strChannel(event.channelName);
      std::string msg(event.message);
      rtminst_->doMessage( event);
    }
  }

  void RtmEventHandler::onSubscribeResult(const uint64_t requestId, const char *channelName, RTM_ERROR_CODE errorCode)  {
    cbPrint("onSubscribeResult: channel:%s, request id: %lld result: %d, reason = %s", channelName, requestId, errorCode, getErrorReason(errorCode));
  }

  void RtmEventHandler::onUnsubscribeResult(const uint64_t requestId, const char *channelName, RTM_ERROR_CODE errorCode)  {
    cbPrint("onUnsubscribeResult: channel:%s, request id: %lld result: %d", channelName, requestId, errorCode);
  }


/*
EcohServer IMPL
 */
EchoServer::EchoServer(std::string &appid, std::string& channel, std::string& userid)
{
  appid_ = appid;
  channel_ = channel;
  userid_ = userid;
  eventHandler_ = new RtmEventHandler(this);
  rtmClient_ = nullptr;
  streamChannel_ = nullptr;

}
int EchoServer::init()
{
  int errCode = 0;
  //1. validith check
  if (rtmClient_) return 0;

  //2. init event handler
  
  //2. do init and create 


    RtmConfig config;
    config.appId = appid_.c_str();
    config.userId = userid_.c_str();
    config.eventHandler = eventHandler_;
    // Create an IRtmClient instance
    int errorCode = 0;
    rtmClient_ = createAgoraRtmClient(config, errorCode);
    if (!rtmClient_ || errorCode != 0) 
    {
      printf("init error: %d, reason = %s\n", errorCode, getErrorReason(errorCode));
      return -1;
    }
    // login
    uint64_t requestId = 0;
    rtmClient_->login(appid_.c_str(), requestId);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // then do sub
    sub();

    // get stream channle
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    errCode = streamchannel_init(channel_.c_str());
    
    if ( errorCode != 0)
    {
      printf ("stream channel error = %d, reason = %s\n", errCode, getErrorReason(errCode));
    }
    
  //3. sub
  return 1;
}
int EchoServer::sub()
{
  uint64_t requestId = 0;
  rtmClient_->subscribe(channel_.c_str(), SubscribeOptions(), requestId);
  return 0;
}
int EchoServer::unSub()
{
  uint64_t requestId = 0;
  rtmClient_->unsubscribe(channel_.c_str(), requestId);
  return 0;
}
// joinchannle, and jointopic, and sub topic
int EchoServer::streamchannel_init(const char *channel)
{
  int errCode = 0;
  uint64_t requestID = 0;
  if (!rtmClient_)
    return -1;

  streamChannel_ = rtmClient_->createStreamChannel(channel_.c_str(), errCode);
  if (!streamChannel_ || errCode != 0)
  {
    printf("create stream channel: err = %d, reason = %s\n", errCode, getErrorReason(errCode));
    return -1;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  // do join channel
  JoinChannelOptions joinpotion;
  joinpotion.token = appid_.c_str();

  streamChannel_->join(joinpotion, requestID);
  printf ("streamchannel-join\n");
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  // join topic
  JoinTopicOptions topicOption;

 streamChannel_->joinTopic(channel_.c_str(), topicOption, requestID);
  printf("streamchannel jointopic\n");
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

}
int EchoServer::doMessage(const agora::rtm::IRtmEventHandler::MessageEvent &event)
{
  if (!rtmClient_)
  {
    printf("invalid rtm\n");
    return -1;
  }
  PublishOptions options;
  options.messageType = RTM_MESSAGE_TYPE_STRING;
  options.channelType = RTM_CHANNEL_TYPE_MESSAGE;
  uint64_t requestId = 0;
  std::string message(event.message);
  rtmClient_->publish(channel_.c_str(), message.c_str(), message.size(), options, requestId);
    
  return 0;
}
void EchoServer::release()
{
  if (!rtmClient_)
    return;

  unSub();
  //sleep some time
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  uint64_t requestID = 0;
  rtmClient_->logout(requestID);
  // sleep some
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  rtmClient_->release();
  // sleep some
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  // release eventehandler
  if (eventHandler_)
  {
    delete eventHandler_;
  }
  // reassign
  rtmClient_ = nullptr;
  eventHandler_ = nullptr;
}

/**
=============================
===
=== main entry
=============================
 */

// usage: xx appid channel userid
const char usage[] = "usage: xx appid channle userid";


// global variable
int g_stop = 1;

// sig func process
// 信号处理函数
static void signalHandler(int signum) {
  if (signum == SIGINT)
  {
    g_stop = 0;
  }

}
//main functon
int main(int argc, const char *argv[])
{
  //1. get argc
  if (argc < 4)
  {
    printf ("argc = %d, usage = %s\n", argc, usage);
    return -1;
  }
  // 2.register sig
  signal(SIGINT, signalHandler);

  // 3. parse argv
  std::string appid(argv[1]), channel(argv[2]), userid(argv[3]);
  printf("input: appid = %s, channel = %s, userid = %s\n", appid.c_str(), channel.c_str(), userid.c_str());

  //4 start echo servcie and do loop
  EchoServer echoServer(appid, channel, userid);

  echoServer.init();
  while(g_stop)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  printf("sig handler now\n");

  //5. reelase resource
  echoServer.release();

  //6. exit now
  printf("rtm ecoh service exit now..\n");

  return 0;
}

