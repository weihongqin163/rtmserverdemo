
#include <memory>
#include <string>
#include <exception>
#include <thread>
#include <chrono>
#include <csignal>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iomanip>
#include <ctime>
#include <errno.h>
#include <libgen.h> 
#include <sys/time.h>

#include "IAgoraRtmClient.h"
#include "rtm_quick_start.h"



using namespace agora::rtm;
class RtmDemo;
class RtmEventHandler;

/**
* global log funciton
 */
 //#define LOG_FILE_PATH "/var/log/my_daemon.log"
 #define LOG_FILE_PATH "rtm_log.log"
 static int log_fd = -1;
 static char *log_buffer = nullptr;

// 写入日志信息到文件
static int log_open() {
    // 以追加和创建模式打开日志文件，权限设置为 0644

    if (log_fd != -1)
      return 1; // 
    log_fd = open(LOG_FILE_PATH, O_WRONLY | O_APPEND | O_CREAT|O_TRUNC, 0644);
    if (log_fd == -1) {
      printf ("open log file = %d\n", errno);
        return -1;
    }
    // allocate log_buffer
    if (!log_buffer)
      log_buffer = new char [2048];
    return log_fd;
  }

static void cbPrint(const char* fmt, ...) {
  // validity check
  if (log_fd == -1)
    return ;
  // 获取当前时间（精确到毫秒）
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  // 获取当前时间的时间戳并格式化为字符串
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm tm = *std::localtime(&time);

  std::ostringstream timeStream;
  timeStream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  timeStream << '.' << std::setw(3) << std::setfill('0') << (millis % 1000); // 精确到毫秒

  std::string timestamp = timeStream.str();
  auto len = timestamp.size();

  // 使用 va_list 处理可变参数
  va_list args;
 

  // 创建一个足够大的缓冲区来存储格式化后的字符串
  char* buffer = log_buffer;

  // 将时间戳放到输出字符串的开头
  sprintf(buffer, "%s ", timestamp.c_str());

  // 再次使用 va_list 获取格式化后的字符串
  va_start(args, fmt);
  std::vsnprintf(buffer + len + 1, 2048, fmt, args); // 以时间戳为基础，追加格式化后的字符串
  va_end(args);

  // 输出最终的字符串
  len = strlen (log_buffer);
  
  auto wrtie_len = write(log_fd, log_buffer, len);

  // flush cache buffer 
  fsync(log_fd);
}
static void log_close()
{
  if (log_fd == -1)
    return;
  close(log_fd);
  log_fd = -1;

  // release
  if (log_buffer)
    delete []log_buffer;
  log_buffer = nullptr;
}



/*
RtmEventHandler IMPL
 */

  RtmEventHandler::RtmEventHandler(ServerBase* inst)
  {
    rtminst_ = inst;
  }

  // Add the event listener
  void RtmEventHandler::onLoginResult( const uint64_t requestId, RTM_ERROR_CODE errorCode)  {
    cbPrint("onLoginResult, request id: %lld, errorCode: %d\n", requestId, errorCode);
  }

  void RtmEventHandler::onLogoutResult(const uint64_t requestId, RTM_ERROR_CODE errorCode) {
    cbPrint("onLogoutResult, request id: %lld, errorCode: %d\n", requestId, errorCode);
  }

  void RtmEventHandler::onConnectionStateChanged(const char *channelName, RTM_CONNECTION_STATE state, RTM_CONNECTION_CHANGE_REASON reason)  {
    cbPrint("onConnectionStateChanged, channelName: %s, state: %d, reason: %d\n", channelName, state, reason);
  }



  void RtmEventHandler::onPublishResult(const uint64_t requestId, RTM_ERROR_CODE errorCode)  {
    cbPrint("onPublishResult request id: %lld result: %d\n", requestId, errorCode);
  }

  void RtmEventHandler::onMessageEvent(const MessageEvent &event)  {
    int msglen = strlen(event.message);
    if (msglen > 1024)
    {
      //only print first 10 
      char szmsg[18];
      memcpy(szmsg, event.message, 17);
      szmsg[17] = '\0';
      cbPrint("receive message from: %s, message: %s, type: %d\n", event.publisher, szmsg, int(event.channelType));
    }
    else
      cbPrint("receive message from: %s, message: %s, type: %d\n", event.publisher, event.message, int(event.channelType));
    if (rtminst_)
    {
      rtminst_->doMessage( event);
    }
  }
  void RtmEventHandler::onTopicEvent(const TopicEvent& event)
  {
    if (event.type == RTM_TOPIC_EVENT_TYPE_REMOTE_JOIN_TOPIC ||
    event.type == RTM_TOPIC_EVENT_TYPE_SNAPSHOT)
    {
      rtminst_->doSubTopic();
    }

  }

  void RtmEventHandler::onSubscribeResult(const uint64_t requestId, const char *channelName, RTM_ERROR_CODE errorCode)  {
    cbPrint("onSubscribeResult: channel:%s, request id: %lld result: %d, reason = %s\n", channelName, requestId, errorCode, getErrorReason(errorCode));
  }

  void RtmEventHandler::onUnsubscribeResult(const uint64_t requestId, const char *channelName, RTM_ERROR_CODE errorCode)  {
    cbPrint("onUnsubscribeResult: channel:%s, request id: %lld result: %d\n", channelName, requestId, errorCode);
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

    // get current file path
    char currentPath[2048];
    std::string strPath;
    char path[2048];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0'; // 确保字符串以 null 结尾
        strPath = dirname(path);
    }
    strPath += "/rtm_sdk.log";
    config.logConfig.filePath = strPath.c_str();
    cbPrint("current log path: %s, %s, %d\n", strPath.c_str(), currentPath, errno);
   
    
    // Create an IRtmClient instance
    int errorCode = 0;
    rtmClient_ = createAgoraRtmClient(config, errCode);
    if (!rtmClient_ ) 
    {
      cbPrint("create error: %d, reason = %s\n", errorCode, getErrorReason(errorCode));
      return -1;
    }
    //then do init
    /* 
    errorCode = rtmClient_->initialize(config);
    if (errCode != 0)
    {
      cbPrint("init error: %d, reason = %s\n", errorCode, getErrorReason(errorCode));
      return -1;
    }*/
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
      cbPrint ("stream channel error = %d, reason = %s\n", errCode, getErrorReason(errCode));
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
  if (!streamChannel_ )
  {
    cbPrint("create stream channel: err = %d, reason = %s\n", errCode, getErrorReason(errCode));
    return -1;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  // do join channel
  JoinChannelOptions joinpotion;
  joinpotion.token = appid_.c_str();

  streamChannel_->join(joinpotion, requestID);
  cbPrint ("streamchannel-join\n");
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  // join topic
  JoinTopicOptions topicOption;

 streamChannel_->joinTopic(channel_.c_str(), topicOption, requestID);
 cbPrint("streamchannel jointopic\n");
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  TopicOptions subTopicInfo;
  requestID = 0;
  streamChannel_->subscribeTopic(channel_.c_str(), subTopicInfo, requestID);
  return 0;

}
int EchoServer::doMessage(const agora::rtm::IRtmEventHandler::MessageEvent &event)
{
  //1. validith check
  if (!rtmClient_)
  {
    cbPrint("invalid rtm\n");
    return -1;
  }
  uint64_t requestId = 0;
  std::string message(event.message);
  // added by wei on 2025-03-25, to add extra info to message
  static char extraInfo[128];
  static uint64_t echo_server_id = 0;
  sprintf(extraInfo, "echo server: %lld", echo_server_id++);
  message =  message + extraInfo;
  if (event.channelType == RTM_CHANNEL_TYPE_MESSAGE )
  {
    //echo back to rtmclient_ 
    PublishOptions options;
    options.messageType = event.messageType;
    options.channelType = RTM_CHANNEL_TYPE_MESSAGE;
    requestId = 0;
    
    rtmClient_->publish(channel_.c_str(), message.c_str(), message.size(), options, requestId);
  
  }
  if (event.channelType == RTM_CHANNEL_TYPE_STREAM && streamChannel_)
  {
    // echo back to stream channel
    TopicMessageOptions topicOptions;
    requestId = 0;
     
    streamChannel_->publishTopicMessage(channel_.c_str(), message.c_str(), message.size(), topicOptions,requestId);
  
  }
  
  return 0;
}
int EchoServer::doSubTopic()
{
  if (streamChannel_)
  {
    TopicOptions subTopicInfo;
    uint64_t requestID = 0;
    streamChannel_->subscribeTopic(channel_.c_str(), subTopicInfo, requestID);
  }
  return 1;
}
void EchoServer::release()
{
  if (!rtmClient_)
    return;

  unSub();
  //sleep some time
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  uint64_t requestId = 0;
  rtmClient_->logout(requestId);
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
int EchoServer::renewToken()
{
  uint64_t requestId = 0;
  if (rtmClient_)
    rtmClient_->renewToken(appid_.c_str(), requestId);
  return 0;
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
  if (signum == SIGINT || signum == SIGTERM)
  {
    g_stop = 0;
  }

}
//main functon
int main(int argc, const char *argv[])
{
  signal(SIGTERM, signalHandler);  // 捕获 SIGTERM 信号（kill -15 命令默认发送的信号）
  signal(SIGINT, signalHandler); 
   // 1.2 start log
  log_open();

 

     //0. start daemon
  if (daemon(0, 0) == -1) {
    printf("daemon");
    return -1;
  } 
  //1. get argc
  if (argc < 4)
  {
    cbPrint ("argc = %d, usage = %s\n", argc, usage);
    return -1;
  }
 

  // 3. parse argv
  std::string appid(argv[1]), channel(argv[2]), userid(argv[3]);
  cbPrint("input: appid = %s, channel = %s, userid = %s,pid = %d\n", appid.c_str(), channel.c_str(), userid.c_str(), getpid());

  //4 start echo servcie and do loop
  EchoServer echoServer(appid, channel, userid);

  echoServer.init();
  struct timeval starttv, curtv;
  gettimeofday(&starttv, NULL);
  uint64_t tick = 0;
  while(g_stop)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tick++;
    // every 10s
    if (tick % 1000 == 0)
    {
      gettimeofday(&curtv, NULL);
      if (curtv.tv_sec - starttv.tv_sec > 20*60*60)
      {
        echoServer.renewToken();
        starttv.tv_sec = curtv.tv_sec;
      }
    }

  }
  cbPrint("sig handler now\n");


  //5. reelase resource
  echoServer.release();
 

  //6. exit now
  cbPrint("rtm ecoh service exit now..\n");
  log_close();

  return 0;
}

