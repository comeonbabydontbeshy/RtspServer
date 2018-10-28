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

	 void rtcpHandle(const boost::system::error_code& error, std::size_t bytes_transferred);

	 void sendMsg(const char* buf, size_t bufLen);

	 RtspRequestFunc parseRtspRequestFunc(const char* receiveData);  // get request func from request

	 void parseRtspRequestUrl(const char* receiveData, std::string& url);  // get request url from request

	 void getRtspResponseCodeMsg(uint32_t responseCode, std::string& responseCodeMsg);  // get response code and msg

	 void getRtspResponseSdpMsg(const StreamType& streamType, std::string& sdpMsg);  // pack sdp msg by stream type

	 bool getRtspTransportInfo(const char* receiveData, TransportInfo& transportInfo);  // get transport info from request

	 bool getUdpPort();

	 bool parseRtspRequestCseq(const char* receiveData, uint32_t& cseq);   // get cseq from request

	 void responseOptionMsg(const char* receiveData);   // response msg for option msg

	 void responseDescribeMsg(const char* receiveData);  // response msg for describe msg

	 void responseSetupMsg(const char* receiveData);  // response msg for setup mgs

	 boost::asio::ip::tcp::socket rtspSock_; 

	 boost::asio::ip::udp::endpoint rtpEndPoint_;
	 boost::asio::ip::udp::socket rtpSock_;

	 boost::asio::ip::udp::endpoint rtcpEndPorint_;
	 boost::asio::ip::udp::socket rtcpSock_;

	 boost::asio::ip::tcp::endpoint remoteEndPoint_;

	 boost::asio::io_service& ioService_;

	 SetUpInfo setUpInfo_;

	 bool isAlive_;

	 char readBuf_[RTSP_READ_MSG_BUF_LEN];

	 char sendBuf_[RTSP_SEND_MSG_BUF_LEN];

	 char rtcpBuf_[RTSP_READ_MSG_BUF_LEN];
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
