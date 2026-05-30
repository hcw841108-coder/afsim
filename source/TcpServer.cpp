#include "TcpServer.hpp"

XDU::TcpServer::TcpServer(boost::asio::io_context& aIOContext, short aPort)
	: mAcceptor(aIOContext, tcp::endpoint(tcp::v4(), aPort))
	, mSocket(aIOContext)
{
	Accept();
}

void XDU::TcpServer::Accept()
{	
	mAcceptor.async_accept(mSocket, std::bind(&TcpServer::OnAccept, this, std::placeholders::_1));
}

void XDU::TcpServer::OnAccept(boost::system::error_code ec)
{
	if (!ec)
	{
		std::make_shared<ClientSession>(std::move(mSocket))->Start();
	}
	Accept();
}
