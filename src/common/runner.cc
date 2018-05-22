#include "common/runner.h"

rtsp::common::Runner::Runner(const std::string& runnerName) :
							runnerName_(runnerName),
							runStatus_(RunningStatus::stop) {

}

void rtsp::common::Runner::start() {
	runStatus_ = RunningStatus::running;
	thread_ = std::thread(&Runner::run, this);
}

void rtsp::common::Runner::run() {
	while (RunningStatus::running == runStatus_) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

rtsp::common::RunningStatus rtsp::common::Runner::getRunningStatus() {
	return runStatus_;
}

void rtsp::common::Runner::stop() {
	runStatus_ = RunningStatus::stop;
}

rtsp::common::Runner::~Runner() {
	if (thread_.joinable()) {
		thread_.join();
	}
}

