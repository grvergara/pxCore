#include <rtRemote.h>
#include <rtRemoteConfig.h>
#include <rtRemoteEnvironment.h>
#include <rtGuid.h>

#include <mutex>
#include <getopt.h>
#include <unistd.h>

static std::mutex shutdownMutex;
static bool testIsOver = false;

struct option longOptions[] = 
{
  { "test-id", required_argument, 0, 'i' },
  { "log-level", required_argument, 0, 'l' },
  { 0, 0, 0, 0 }
};

static FILE* logFile = nullptr;

void
logFileWriter(rtLogLevel level, const char* path, int line, int threadId, char* message)
{
  if (logFile)
  {
    char const* logLevel = rtLogLevelToString(level);
    fprintf(logFile, "%5s %s:%d -- Thread-%" RT_THREADID_FMT ": %s\n", logLevel, path, line,
        threadId, message);
  }
}

class rtTestObject : public rtObject
{
public:
  rtDeclareObject(rtTestObject, rtObject);
  rtProperty(num, num, setNum, uint32_t);
  rtMethodNoArgAndNoReturn("shutdown", shutdown);

  uint32_t num() const { return m_num; }
  rtError  num(uint32_t& n) const { n = m_num; return RT_OK; }
  rtError  setNum(uint32_t n) { m_num = n; return RT_OK; }

  rtError shutdown()
  {
    rtLogInfo("got shutdown signal");
    std::unique_lock<std::mutex> lock(shutdownMutex);
    testIsOver = true;
    return RT_OK;
  }

private:
  uint32_t m_num;
};

rtDefineObject(rtTestObject, rtObject);
rtDefineProperty(rtTestObject, num);
rtDefineMethod(rtTestObject, shutdown);

rtError
rtRemoteRunUntil(rtRemoteEnvironment* env, uint32_t millisecondsFromNow)
{
  rtError e = RT_OK;

  bool hasDipatchThread = env->Config->server_use_dispatch_thread();
  if (hasDipatchThread)
  {
    usleep(millisecondsFromNow * 1000);
    (void ) env;
  }
  else
  {
    auto endTime = std::chrono::milliseconds(millisecondsFromNow) + std::chrono::system_clock::now();
    while (endTime > std::chrono::system_clock::now())
    {
      {
        std::unique_lock<std::mutex> lock(shutdownMutex);
        if (testIsOver)
          return RT_OK;
      }

      e = rtRemoteRun(env, 16);
      if (e != RT_OK && e != RT_ERROR_QUEUE_EMPTY)
        return e;
    }
  }
  return e;
}

int main(int argc, char* argv[])
{
  int optionIndex;
  std::string testId;

  rtError e;
  rtLogLevel logLevel = RT_LOG_INFO;

  while (true)
  {
    int c = getopt_long(argc, argv, "i:", longOptions, &optionIndex);
    if (c == -1)
      break;

    switch (c)
    {
      case 'i':
        testId = optarg;
        break;
      case 'l':
        logLevel = rtLogLevelFromString(optarg);
        break;
    }
  }

  char logFileName[256];
  snprintf(logFileName, sizeof(logFileName), "%s.server.log", testId.c_str());
  logFile = fopen(logFileName, "w");

  rtLogSetLevel(logLevel);
  rtLogSetLogHandler(logFileWriter);

  rtRemoteEnvironment* env = rtEnvironmentGetGlobal();

  rtObjectRef obj(new rtTestObject());
  e = rtRemoteInit(env);
  RT_ASSERT(e == RT_OK);
  e = rtRemoteRegisterObject(env, testId.c_str(), obj);
  RT_ASSERT(e == RT_OK);

  bool running = true;
  while (running)
  {
    {
      std::unique_lock<std::mutex> lock(shutdownMutex);
      running = !testIsOver;
    }
    
    if (running)
    {
      e = rtRemoteRunUntil(env, 1000);
      rtLogInfo("[%s] rtRemoteRun:%s", testId.c_str(), rtStrError(e));
    }
  }

  rtLogInfo("server shutting down for %s", testId.c_str());

  if (logFile)
    fclose(logFile);

  return 0;
}
