#ifndef RTSP_PUB_H_
#define RTSP_PUB_H_

#define RTSP_SERVER_PORT (8001)

#define RTSP_READ_MSG_BUF_LEN (1024)

#define RTSP_SEND_MSG_BUF_LEN (1024)

#define RTSP_RESPONSE_MSG_NUM (47)

#define RTSP_SEND_BUFFER_SIZE (8*1024*1024)

#define RTSP_UDP_BEGIN_PORT (6970)

enum class StreamType {
	kH264,
	KH265
};

enum class RtspRequestFunc {
	kOPTION,
	kDESCRIBE,
	kANNOUNCE,
	kPAUSE,
	kPLAY,
	kRECORD,
	kSETUP,
	kTEARDOWN,
	kSET_PARAMETER,
	kGET_PARAMETER,
	kREDIRECT,
	kINVALID
};

enum class TransportProtocol {
	kUDP,
	kTCP
};

using TransportInfo = struct TransportInfo_ {
	TransportProtocol protocol;
	char remoteClientAddr[255];
	uint32_t remoteClientPort[2];
	bool bMulticast;
};

using SetUpInfo = struct SetUpInfo_ {
	TransportInfo transport;
	uint32_t cseq;
	char session[20];
	char url[255];
};

using ResponseData = struct ResponseData_ {
	uint32_t responseCode;
	char responseMsg[48];
};

enum class ResponseCode {
	kContinue = 100,
	kTimeOut = 110,
	kOk = 200,
	kCreated = 201,
	kLowStroageSpace = 250,
	kMultiChoices = 300,
	kMovePermanently = 301,
	kMoveTemporarily = 302,
	kSeeOther = 303,
	kNotModified = 304,
	kUseProxy = 305,
	kGoingAway = 350,
	kLoadBalancing = 351,
	kBadRequest = 400,
	kUnauthorized = 401,
	kPaymentRequired = 402,
	kForbidden = 403,
	kNotFound = 404,
	kMethodNotAllowed = 405,
	kNotAcceptable = 406,
	kProxyAuthenticationRequired = 407,
	kRequestTimeout = 408,
	kGone = 410,
	kLengthRequired = 411,
	kPreconditionFailed = 412,
	kRequestEntityTooLarge = 413,
	kRequestURITooLarge = 414,
	kUnsupportedMediaType = 415,
	kParameterNotUnderstood = 451,
	kreserved = 452,
	kNotEnoughBandwidth = 453,
	kSessionNotFound = 454,
	kMethodNotValidInThisState = 455,
	kHeaderFieldNotValidForResource = 456,
	kInvalidRange = 457,
	kParameterIsReadOnly = 458,
	kAggregateOperationNotAllowed = 459,
	kOnlyAggregateOperationAllowed = 460,
	kInternalServerError = 500,
	kNotImplemented = 501,
	kBadGateway = 502,
	kServiceUnavailable = 503,
	kGatewayTimeOut = 504,
	kRTSPVersionNotSupported = 505,
	kOptionNotSupported = 551
};



#endif // !RTSP_PUB_H_
