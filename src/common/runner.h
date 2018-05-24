#ifndef RUNNER_H_
#define RUNNER_H_

#include <chrono>
#include <string>
#include <thread>

namespace rtsp {
namespace common {

enum class RunningStatus {
	running = 0,
	stop=1,
};

class Runner {
 public:
	 Runner(const std::string& runnerName);

	 void start();

	 RunningStatus getRunningStatus();

	 bool isRunning();

	 void wait();

	 const std::string& getRunnerName();

	 virtual void run();

	 virtual void stop();

	 virtual ~Runner();

 protected:
	 RunningStatus runStatus_;

 private:
	 std::string runnerName_;

	 std::thread thread_;
};

class TickTaskRunner :public Runner {
 public:
 private:
};

}
}

#endif // !RUNNER_H_
