#include "common/BaseService.h"

using rtsp::common::BaseProcess;

int main() {
	BaseProcess& process = BaseProcess::instance();
	process.start();
}