#ifndef BASE_PROCESS_H_
#define BASE_PROCESS_H_

#include <atomic>
#include <iostream>
#include <map>
#include <thread>

#include "boost/thread/locks.hpp"
#include "boost/thread/shared_mutex.hpp"
#include "common/runner.h"

using rtsp::common::Runner;

namespace rtsp {
namespace common {

class BaseProcess{
 public:
	 using rLock = boost::shared_lock<boost::shared_mutex>;
	 using wLock = boost::unique_lock<boost::shared_mutex>;

	 using runnerPtr = std::shared_ptr<Runner>;

	 void registeRunnerMap(const std::string& runnerName, runnerPtr runnerObj);

	 void unRegisteRunnerMap(const std::string& runnerName);

	 static BaseProcess& instance();

	 void start();

	 void stop();

 private:
	 BaseProcess();

	 ~BaseProcess();

	 void signalHandle();

	 void installSignal();

	 void waitAll();

	 boost::shared_mutex mtx_;
	 std::map<std::string, runnerPtr> runnerMap_;

	 std::atomic<bool> signalRunning_;
	 std::thread signalHandleThread_;
};
}
}

#endif // !BASE_PROCESS_H_
