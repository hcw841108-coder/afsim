#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "wsf_onboard_computer_export.h"
#include "boost/asio.hpp"
#include "ClientSession.hpp"

using boost::asio::ip::tcp;

namespace XDU
{
    class TcpServer
    {
    public:
        TcpServer(boost::asio::io_context& aIOContext, short aPort);

    private:
        void Accept();
        void OnAccept(boost::system::error_code ec);

        tcp::acceptor mAcceptor;
        tcp::socket mSocket;
    };
}
#endif // TCP_SERVER_HPP
