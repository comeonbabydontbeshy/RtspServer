#include "rtsp/RtspMain.h"

#include <iostream>
#include "boost/bind.hpp"

using rtsp::common::RunningStatus;

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

rtsp::server::RtspHandle::RtspHandle(boost::asio::io_service& ioService) :rtspSock_(ioService) {
	memset(readBuf_, '\0', RTSP_READ_MSG_BUF_LEN);
	memset(sendBuf_, '\0', RTSP_SEND_MSG_BUF_LEN);
}

rtsp::server::RtspHandle::~RtspHandle() {
	std::cout << "remote socket close " << remoteEndPoint_.address() << ":" <<
		remoteEndPoint_.port() << std::endl;
	rtspSock_.close();
}

void rtsp::server::RtspHandle::start() {
	// TODO ÉèÖÃtcp»º³åÇø
	remoteEndPoint_ = rtspSock_.remote_endpoint();
	std::cout << "new connect " << remoteEndPoint_.address() << ":" <<
		remoteEndPoint_.port() << std::endl;

	rtspSock_.async_receive(boost::asio::buffer(readBuf_, RTSP_READ_MSG_BUF_LEN),
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

	rtspSock_.async_receive(boost::asio::buffer(readBuf_, RTSP_READ_MSG_BUF_LEN),
		boost::bind(&RtspHandle::readHandle, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}
