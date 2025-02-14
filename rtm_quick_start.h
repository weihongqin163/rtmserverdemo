

#include <iostream>
#include <memory>
#include <string>
#include <exception>
#include <thread>
#include <chrono>


#include "IAgoraRtmClient.h"
#include "IAgoraStreamChannel.h"
using namespace agora::rtm;

class RtmDemo;
class RtmEventHandler;
struct agora::rtm::IRtmEventHandler::MessageEvent;

class ServerBase
{
  public:
  virtual int doMessage(const agora::rtm::IRtmEventHandler::MessageEvent &event) = 0;
  virtual ~ServerBase(){};

};



class RtmEventHandler : public IRtmEventHandler {
  public:
  RtmEventHandler(ServerBase* inst);
public:
  // Add the event listener
  void onLoginResult(const uint64_t requestId, RTM_ERROR_CODE errorCode) override ;

  void onLogoutResult(const uint64_t requestId, RTM_ERROR_CODE errorCode);

  void onConnectionStateChanged(const char *channelName, RTM_CONNECTION_STATE state, RTM_CONNECTION_CHANGE_REASON reason) override ;

  void onLinkStateEvent(const LinkStateEvent& event) override ;
  void onPublishResult(const uint64_t requestId, RTM_ERROR_CODE errorCode) override ;

  void onMessageEvent(const MessageEvent &event) override ;

  void onSubscribeResult(const uint64_t requestId, const char *channelName, RTM_ERROR_CODE errorCode) override ;

  void onUnsubscribeResult(const uint64_t requestId, const char *channelName, RTM_ERROR_CODE errorCode) override ;

  // stream channel callback
  

void onJoinResult(const uint64_t requestId, const char* channelName, const char* userId,
RTM_ERROR_CODE errorCode) override {

}

void onJoinTopicResult(const uint64_t requestId, const char* channelName, const char* userId,
const char* topic, const char* meta, RTM_ERROR_CODE errorCode) override {

}

void onSubscribeTopicResult(const uint64_t requestId, const char* channelName, const char* userId, const char* topic, UserList succeedUsers, UserList failedUsers, RTM_ERROR_CODE errorCode) override {

}

private:
  ServerBase* rtminst_;

private:
  void cbPrint(const char* fmt, ...) {
    printf("\x1b[32m*** RTM async callback: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf(" \x1b[0m\n");
  }
};

// echo server
class EchoServer:public ServerBase
{
  public:
    EchoServer(std::string &appid, std::string& channel, std::string &userid);
  public:
    int init();
    void release();
  public:
    int  doMessage(const agora::rtm::IRtmEventHandler::MessageEvent &event) override;
  protected:
    int sub();
    int unSub();
    //stream channle related
    int streamchannel_init(const char *channel);
    int streamchannel_leave();
  private:
    IRtmEventHandler* eventHandler_;
    IRtmClient* rtmClient_;
    std::string channel_;
    std::string appid_;
    std::string userid_;
    IStreamChannel *streamChannel_;

};



