#ifndef RTSP_MAIN_H_
#define RTSP_MAIN_H_

#include "boost/asio.hpp"
#include "common/runner.h"
#include "rtsp/RtspPub.h"

using rtsp::common::Runner;

namespace rtsp {
namespace server {

class RtspHandle:public std::enable_shared_from_this<RtspHandle> {
 public:
	 RtspHandle(boost::asio::io_service& ioService);

	 ~RtspHandle();

	 void start();

	 boost::asio::ip::tcp::socket& getSocket();
 private:
	 void readHandle(const boost::system::error_code& error,
		 std::size_t readLen);

	 boost::asio::ip::tcp::socket rtspSock_;

	 boost::asio::ip::tcp::endpoint remoteEndPoint_;

	 char readBuf_[RTSP_READ_MSG_BUF_LEN];

	 char sendBuf_[RTSP_SEND_MSG_BUF_LEN];
};

class RtspSrv :public Runner {
 public:
	 RtspSrv(const std::string& runnerName);

	 ~RtspSrv();

	 virtual void run() override;

	 virtual void stop() override;
 private:
	 void openEndpoint();

	 void beginRegistAcceptHandle();

	 void acceptHandle(const boost::system::error_code& error, std::shared_ptr<RtspHandle> rtspHandle);

	 boost::asio::io_service ioService_;

	 boost::asio::ip::tcp::endpoint rtspSrvEndpoint_;

	 boost::asio::ip::tcp::acceptor acc_;

};

}
}

#endif // !RTSP_MAIN_H_
