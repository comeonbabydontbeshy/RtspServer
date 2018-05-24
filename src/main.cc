#include "common/BaseService.h"
#include "rtsp/RtspMain.h"

using rtsp::common::BaseProcess;
using rtsp::server::RtspSrv;

int main() {
	BaseProcess& process = BaseProcess::instance();

	std::shared_ptr<Runner> rtspRunner = std::make_shared<RtspSrv>("rtsp server");
	process.registeRunnerMap(rtspRunner);

	process.start();
}