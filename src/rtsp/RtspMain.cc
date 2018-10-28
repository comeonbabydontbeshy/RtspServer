#include "rtsp/RtspMain.h"

#include <iostream>
#include "boost/bind.hpp"
#include "boost/random.hpp"

using rtsp::common::RunningStatus;

static ResponseData responseDataInfos[RTSP_RESPONSE_MSG_NUM] = {
	{ 100, "100  Continue" },
	{ 110, "110 Connect Timeout" },
	{ 200, "200 OK" },
	{ 201, "201 Created" },
	{ 250, "250 Low on Storage Space" },
	{ 300, "300 Multiple Choices" },
	{ 301, "301 Moved Permanently" },
	{ 302, "302 Moved Temporarily" },
	{ 303, "303 See Other" },
	{ 304, "304 Not Modified" },
	{ 305, "305 Use Proxy" },
	{ 350, "350 Going Away" },
	{ 351, "351 Load Balancing" },
	{ 400, "400 Bad Request" },
	{ 401, "401 Unauthorized" },
	{ 402, "402 Payment Required" },
	{ 403, "403 Forbidden" },
	{ 404, "404 Not Found" },
	{ 405, "405 Method Not Allowed" },
	{ 406, "406 Not Acceptable" },
	{ 407, "407 Proxy Authentication Required" },
	{ 408, "408 Request Time-out" },
	{ 410, "410 Gone" },
	{ 411, "411 Length Required" },
	{ 412, "412 Precondition Failed" },
	{ 413, "413 Request Entity Too Large" },
	{ 414, "414 Request-URI Too Large" },
	{ 415, "415 Unsupported Media Type" },
	{ 451, "451 Parameter Not Understood" },
	{ 452, "452 reserved" },
	{ 453, "453 Not Enough Bandwidth" },
	{ 454, "454 Session Not Found" },
	{ 455, "455 Method Not Valid in This State" },
	{ 456, "456 Header Field Not Valid for Resource" },
	{ 457, "457 Invalid Range" },
	{ 458, "458 Parameter Is Read-Only" },
	{ 459, "459 Aggregate operation not allowed" },
	{ 460, "460 Only aggregate operation allowed" },
	{ 461, "461 Unsupported transport" },
	{ 462, "462 Destination unreachable" },
	{ 500, "500 Internal Server Error" },
	{ 501, "501 Not Implemented" },
	{ 502, "502 Bad Gateway" },
	{ 503, "503 Service Unavailable" },
	{ 504, "504 Gateway Time-out" },
	{ 505, "505 RTSP Version not supported" },
	{ 551, "551 Option not supported" }
};

rtsp::server::RtspSrv::RtspSrv(const std::string& runnerName) :Runner(runnerName),
rtspSrvEndpoint_(boost::asio::ip::address_v4::any(), RTSP_SERVER_PORT),
acc_(ioService_) {

}

rtsp::server::RtspSrv::~RtspSrv() {

}

void rtsp::server::RtspSrv::run() {
	openEndpoint();

	beginRegistAcceptHandle();

	ioService_.run();
}

void rtsp::server::RtspSrv::stop() {
	runStatus_ = RunningStatus::stop;
	ioService_.stop();
}

void rtsp::server::RtspSrv::openEndpoint() {
	acc_.open(rtspSrvEndpoint_.protocol());
	boost::asio::ip::tcp::acceptor::reuse_address option(true);
	acc_.set_option(option);
	boost::system::error_code ec;
	acc_.bind(rtspSrvEndpoint_, ec);
	if (ec) {
		std::cout << "bind " << rtspSrvEndpoint_.address().to_string() << ":"
			<< rtspSrvEndpoint_.port() << " failed ." << std::endl;
		throw std::runtime_error(ec.message());
	}
	acc_.listen();
}


void rtsp::server::RtspSrv::beginRegistAcceptHandle() {
	std::shared_ptr<RtspHandle> rtspHandle = std::make_shared<RtspHandle>(ioService_);
	acc_.async_accept(rtspHandle->getSocket(),
		boost::bind(&RtspSrv::acceptHandle, this, boost::asio::placeholders::error, rtspHandle));
}

void rtsp::server::RtspSrv::acceptHandle(
	const boost::system::error_code& error, 
	std::shared_ptr<RtspHandle> rtspHandle) {

	if (error) {
		std::cout << "accept handle occur error : " << error.message() << std::endl;
		return;
	}

	rtspHandle->start();

	std::shared_ptr<RtspHandle> rtspSession = std::make_shared<RtspHandle>(ioService_);
	acc_.async_accept(rtspSession->getSocket(),
		boost::bind(&RtspSrv::acceptHandle, this, boost::asio::placeholders::error, rtspSession));
}

rtsp::server::RtspHandle::RtspHandle(boost::asio::io_service& ioService) :rtspSock_(ioService),
																		  rtpSock_(ioService), 
																		  rtcpSock_(ioService),
																		  ioService_(ioService) {
	memset(readBuf_, '\0', RTSP_READ_MSG_BUF_LEN);
	memset(sendBuf_, '\0', RTSP_SEND_MSG_BUF_LEN);
	memset(rtcpBuf_, '\0', RTSP_READ_MSG_BUF_LEN);
}

rtsp::server::RtspHandle::~RtspHandle() {
	std::cout << "remote socket close " << remoteEndPoint_.address() << ":" <<
		remoteEndPoint_.port() << std::endl;
	rtspSock_.close();
}

void rtsp::server::RtspHandle::start() {
	try {
		remoteEndPoint_ = rtspSock_.remote_endpoint();
		std::cout << "new connect " << remoteEndPoint_.address() << ":" <<
			remoteEndPoint_.port() << std::endl;
	} catch (boost::system::system_error& ec) {
		std::cout << "disconnect : " << ec.what() << std::endl;
		return;
	}

	boost::asio::socket_base::send_buffer_size sizeOption(RTSP_SEND_MSG_BUF_LEN);
	rtspSock_.set_option(sizeOption);

	rtspSock_.async_read_some(boost::asio::buffer(readBuf_, RTSP_READ_MSG_BUF_LEN),
		boost::bind(&RtspHandle::readHandle, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

boost::asio::ip::tcp::socket& rtsp::server::RtspHandle::getSocket() {
	return rtspSock_;
}

void rtsp::server::RtspHandle::readHandle(const boost::system::error_code& error,
	std::size_t readLen) {
	if (error) {
		std::cout << "read handle occur error : " << error.message() << std::endl;
		return;
	}

	RtspRequestFunc func = parseRtspRequestFunc(readBuf_);

	switch (func) {
	case RtspRequestFunc::kOPTION:
		responseOptionMsg(readBuf_);
		break;
	case RtspRequestFunc::kDESCRIBE:
		responseDescribeMsg(readBuf_);
		break;
	case RtspRequestFunc::kSETUP:
		responseSetupMsg(readBuf_);
		break;
	default:
		break;
	}

	rtspSock_.async_read_some(boost::asio::buffer(readBuf_, RTSP_READ_MSG_BUF_LEN),
		boost::bind(&RtspHandle::readHandle, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

RtspRequestFunc rtsp::server::RtspHandle::parseRtspRequestFunc(const char* receiveData) {
	char funcBuf[20] = { 0 };
	char urlBuf[255] = { 0 };
	char rtspVersionBuf[10] = { 0 };
	sscanf(receiveData, "%19s %255s %10s\r\n", funcBuf, urlBuf, rtspVersionBuf);
	std::cout << "func : " << funcBuf << std::endl;
	std::cout << "url : " << urlBuf << std::endl;
	std::cout << "rtsp version : " << rtspVersionBuf << std::endl;

	if (NULL == strstr(rtspVersionBuf, "RTSP/")) {
		return RtspRequestFunc::kINVALID;
	}
	if (0 == strcmp(funcBuf, "OPTIONS")) {
		return RtspRequestFunc::kOPTION;
	}
	if (0 == strcmp(funcBuf, "DESCRIBE")) {
		return RtspRequestFunc::kDESCRIBE;
	}
	if (0==strcmp(funcBuf,"SETUP")) {
		return RtspRequestFunc::kSETUP;
	}
	if (0==strcmp(funcBuf,"PLAY")) {
		return RtspRequestFunc::kPLAY;
	}
	if (0 == strcmp(funcBuf, "TEARDOWN")) {
		return RtspRequestFunc::kTEARDOWN;
	}
	if (0==strcmp(funcBuf,"PAUSE")) {
		return RtspRequestFunc::kPAUSE;
	}
	if (0 == strcmp(funcBuf, "GET_PARAMETER")) {
		return RtspRequestFunc::kGET_PARAMETER;
	}
	if (0 == strcmp(funcBuf, "SET_PARAMETER")) {
		return RtspRequestFunc::kSET_PARAMETER;
	}
	return RtspRequestFunc::kINVALID;
}

void rtsp::server::RtspHandle::responseOptionMsg(const char* receiveData) {
	ResponseCode responseCode = ResponseCode::kOk;

	uint32_t cseq;
	parseRtspRequestCseq(receiveData, cseq);

	std::string responseCodeMsg;
	getRtspResponseCodeMsg(static_cast<uint32_t>(responseCode), responseCodeMsg);

	std::string responseMsg = "RTSP/1.0 " + responseCodeMsg + "\r\n";
	responseMsg += "CSeq: " + std::to_string(cseq) + "\r\n";
	responseMsg += "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER\r\n\r\n";
	sendMsg(responseMsg.c_str(), responseMsg.size());
}

void rtsp::server::RtspHandle::sendMsg(const char* buf, size_t bufLen) {
	try {
		rtspSock_.write_some(boost::asio::buffer(buf, bufLen));
	} catch (const boost::system::system_error& ec) {
		std::cout << "write msg error : " << ec.what() << std::endl;
	}
}

bool rtsp::server::RtspHandle::parseRtspRequestCseq(const char* receiveData, uint32_t& cseq) {
	char* pos = strstr(const_cast<char*>(receiveData), "CSeq");
	if (NULL == pos) {
		return false;
	}
	sscanf(pos, "%*s %ud", &cseq);
	return true;
}

void rtsp::server::RtspHandle::getRtspResponseCodeMsg(uint32_t responseCode,
	std::string& responseCodeMsg) {
	for (int32_t i = 0; i < RTSP_RESPONSE_MSG_NUM; ++i) {
		if (responseDataInfos[i].responseCode == responseCode) {
			responseCodeMsg.assign(responseDataInfos[i].responseMsg);
		}
	}
}

void rtsp::server::RtspHandle::responseDescribeMsg(const char* receiveData) {
	ResponseCode responseCode = ResponseCode::kOk;

	uint32_t cseq;
	parseRtspRequestCseq(receiveData, cseq);

	std::string sdpMsg;
	getRtspResponseSdpMsg(StreamType::kH264, sdpMsg);

	std::string responseCodeMsg;
	getRtspResponseCodeMsg(static_cast<uint32_t>(responseCode), responseCodeMsg);

	std::string responseMsg = "RTSP/1.0 " + responseCodeMsg + "\r\n";
	responseMsg += "CSeq: " + std::to_string(cseq) + "\r\n";
	responseMsg += "Content-Type: application/sdp\r\n";
	responseMsg += "Content-Length: " + std::to_string(sdpMsg.size()) + "\r\n\r\n";
	responseMsg += sdpMsg;
	sendMsg(responseMsg.c_str(), responseMsg.size());
}

void rtsp::server::RtspHandle::parseRtspRequestUrl(const char* receiveData, std::string& url) {
	char urlBuf[255] = { 0 };
	sscanf(receiveData, "%*s %s", urlBuf);
	url.assign(urlBuf);
}

void rtsp::server::RtspHandle::getRtspResponseSdpMsg(
	const StreamType& streamType, 
	std::string& sdpMsg) {
	// v£¬o£¬s£¬t£¬mÎª±ØÐëµÄ
	if (StreamType::kH264 == streamType) {
		sdpMsg += "v=0\r\n";
		sdpMsg += "o=- 1 1 IN IP4 " + rtspSock_.local_endpoint().address().to_string() + "\r\n";
		sdpMsg += "s=H.264 Video\r\n";
		sdpMsg += "t=0 0\r\n";
		sdpMsg += "a=control:*\r\n";
		sdpMsg += "m=video 0 RTP/AVP 96\r\n";
		sdpMsg += "a=rtpmap:96 H264/90000\r\n\r\n";
	}
	if (StreamType::KH265==streamType) {
		sdpMsg += "v=0\r\n";
		sdpMsg += "o=- 1 1 IN IP4 " + rtspSock_.local_endpoint().address().to_string() + "\r\n";
		sdpMsg += "s=H.264 Video\r\n";
		sdpMsg += "t=0 0\r\n";
		sdpMsg += "a=control:*\r\n";
		sdpMsg += "m=video 0 RTP/AVP 96\r\n";
		sdpMsg += "a=rtpmap:96 H264/90000\r\n\r\n";
	}
}

void rtsp::server::RtspHandle::responseSetupMsg(const char* receiveData) {
	ResponseCode responseCode = ResponseCode::kOk;

	parseRtspRequestCseq(receiveData, setUpInfo_.cseq);

	if (false == getRtspTransportInfo(receiveData, setUpInfo_.transport)) {
		responseCode = ResponseCode::kParameterNotUnderstood;
	}

	std::string responseCodeMsg;
	getRtspResponseCodeMsg(static_cast<uint32_t>(responseCode), responseCodeMsg);
	std::string responseMsg; 
	if (responseCode != ResponseCode::kOk) {
		responseMsg += "RTSP/1.0 " + responseCodeMsg + "\r\n";
		responseMsg += "CSeq: " + std::to_string(setUpInfo_.cseq) + "\r\n\r\n";
		sendMsg(responseMsg.c_str(), responseMsg.size());
		return;
	}
	
	if (TransportProtocol::kUDP == setUpInfo_.transport.protocol) {
		if (!getUdpPort()) {
			responseCode = ResponseCode::kInternalServerError;
		}
	}
	
	getRtspResponseCodeMsg(static_cast<uint32_t>(responseCode), responseCodeMsg);
	responseMsg += "RTSP/1.0 " + responseCodeMsg + "\r\n";
	responseMsg += "CSeq: " + std::to_string(setUpInfo_.cseq) + "\r\n\r\n";
	if (responseCode != ResponseCode::kOk) {
		sendMsg(responseMsg.c_str(), responseMsg.size());
		return;
	}
	if (TransportProtocol::kUDP == setUpInfo_.transport.protocol) {
		responseMsg += "Transport: RTP/AVP/UDP;unicast;destination=" + remoteEndPoint_.address().to_string() + ";";
		responseMsg += "source=" + rtspSock_.local_endpoint().address().to_string() + ";";
		responseMsg += "client-port=" + std::to_string(setUpInfo_.transport.remoteClientPort[0])
			+ "-" + std::to_string(setUpInfo_.transport.remoteClientPort[1]) + ";";
		responseMsg += "server_port=" + rtpEndPoint_.address().to_string() + ":"
			+ rtcpEndPorint_.address().to_string() + "\r\n";
		// todo session id 
		boost::random::mt19937 gen;
		boost::uniform_int<> real(100000000, 99999999);
		snprintf(setUpInfo_.session, 20, "%d", real(gen));
		responseMsg += "Session: " + std::string(setUpInfo_.session) + "\r\n\r\n";
	}
	if (TransportProtocol::kTCP == setUpInfo_.transport.protocol) {
		// todo 
	}
	sendMsg(responseMsg.c_str(), responseMsg.size());
}

bool rtsp::server::RtspHandle::getRtspTransportInfo(
	const char* receiveData, TransportInfo& transportInfo) {
	std::string remoteClientAddr = remoteEndPoint_.address().to_string();
	snprintf(transportInfo.remoteClientAddr, 255, "%s", const_cast<char*>(remoteClientAddr.c_str()));
	char* pos = strstr(const_cast<char*>(receiveData), "Transport");
	if (NULL == pos) {
		return false;
	}
	if (NULL == strstr(pos, "RTP/AVP")) {
		return false;
	}
	transportInfo.protocol = TransportProtocol::kUDP;
	if (NULL != strstr(pos, "RTP/AVP/TCP")) {
		transportInfo.protocol = TransportProtocol::kTCP;
	}
	if (TransportProtocol::kUDP == transportInfo.protocol) {
		pos = strstr(pos, "client_port=");
		if (NULL == pos) {
			return false;
		}
		int ret = sscanf(pos + strlen("client_port="), "%ud-%ud", &transportInfo.remoteClientPort[0],
			&transportInfo.remoteClientPort[1]);
		if (2 != ret) {
			return false;
		}
	}
	return true;
}

bool rtsp::server::RtspHandle::getUdpPort() {
	uint32_t port = RTSP_UDP_BEGIN_PORT;
	for (; port < RTSP_UDP_BEGIN_PORT + 100; ++port) {
		boost::system::error_code err;
		rtpEndPoint_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), port);
		rtpSock_.bind(rtpEndPoint_, err);
		if (err) {
			continue;
		}
		err.clear();
		rtpSock_.open(boost::asio::ip::udp::v4(), err);
		if (err) {
			continue;
		}
		err.clear();
		rtcpEndPorint_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), port + 1);
		rtcpSock_.bind(rtcpEndPorint_, err);
		if (err) {
			continue;
		}
		err.clear();
		rtcpSock_.open(boost::asio::ip::udp::v4(), err);
		if (err) {
			continue;
		}
	}
	if ((RTSP_UDP_BEGIN_PORT + 100) == port) {
		return false;
	}
	rtcpSock_.async_receive(boost::asio::buffer(rtcpBuf_, RTSP_READ_MSG_BUF_LEN),
		boost::bind(&RtspHandle::rtcpHandle, shared_from_this(), boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
	return true;
}

void rtsp::server::RtspHandle::rtcpHandle(
	const boost::system::error_code& error, std::size_t bytes_transferred) {
	if (error) {
		std::cout << "occur error : " << error.message() << std::endl;
		return;
	}
	rtcpSock_.async_receive(boost::asio::buffer(rtcpBuf_, RTSP_READ_MSG_BUF_LEN),
		boost::bind(&RtspHandle::rtcpHandle, shared_from_this(), boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}
