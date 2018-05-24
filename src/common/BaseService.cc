#include "common/BaseService.h"

#include "signal.h"

using rtsp::common::BaseProcess;
using rtsp::common::RunningStatus;

void BaseProcess::registeRunnerMap(runnerPtr runnerObj) {
	wLock lck(mtx_);
	auto iter = runnerMap_.find(runnerObj->getRunnerName());
	if (iter == runnerMap_.end()) {
		runnerMap_.emplace(runnerObj->getRunnerName(), runnerObj);
	}
}

void BaseProcess::unRegisteRunnerMap(const std::string& runnerName) {
	wLock lck(mtx_);
	auto iter = runnerMap_.find(runnerName);
	if (iter!=runnerMap_.end()) {
		runnerMap_.erase(runnerName);
	}
}

BaseProcess& BaseProcess::instance() {
	static BaseProcess process;
	return process;
}

void BaseProcess::start() {
	{
		wLock lck(mtx_);
		for (auto& item : runnerMap_) {
			item.second->start();
		}
	}
	waitAll();
}

void BaseProcess::stop() {
	signalRunning_ = false;
	{
		wLock lck(mtx_);
		for (auto& item : runnerMap_) {
			item.second->stop();
			std::cout << item.first << " exit" << std::endl;
		}
	}
}

BaseProcess::BaseProcess():signalRunning_(true) {
	installSignal();
}

void BaseProcess::signalHandle() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	siginfo_t sigInfo;
	int err = 0;
	while (signalRunning_) {
		struct timespec timeout = { 1, 0 };
		err = sigtimedwait(&set, &sigInfo, &timeout);
		if (-1 == err) {
			continue;
		}
		std::cout << "catch signal : " << sigInfo.si_signo << std::endl;
		if (SIGINT == sigInfo.si_signo || SIGQUIT == sigInfo.si_signo) {
			stop();
		}
	}
}

rtsp::common::BaseProcess::~BaseProcess() {
	if (signalHandleThread_.joinable()) {
		signalHandleThread_.join();
	}
}

void rtsp::common::BaseProcess::installSignal() {
	/* 主线程中屏蔽部分信号子线程会继承信号屏蔽掩码 */
	sigset_t newMask, oldMask;
	sigemptyset(&newMask);
	sigaddset(&newMask, SIGINT);
	sigaddset(&newMask, SIGQUIT);
	pthread_sigmask(SIG_BLOCK, &newMask, &oldMask);
	signalHandleThread_ = std::thread([this]() {
		std::cout << "signal handle thread start" << std::endl;
		this->signalHandle();
		std::cout << "signal handle thread end" << std::endl;
	});
}

void rtsp::common::BaseProcess::waitAll() {
	while (true) {
		bool isAllStoped = true;
		{
			rLock lck(mtx_);
			for (auto& item : runnerMap_) {
				if (item.second->isRunning()) {
					isAllStoped = false;
					break;
				}
			}
		}
		if (isAllStoped) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	wLock lck(mtx_);
	for (auto& item : runnerMap_) {
		item.second->wait();
	}
}

