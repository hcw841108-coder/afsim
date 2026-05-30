#ifndef CLIENT_SESSION_HPP
#define CLIENT_SESSION_HPP

#include "wsf_onboard_computer_export.h"
#include "boost/asio.hpp"

using boost::asio::ip::tcp;

namespace XDU
{
    class ClientSession : public std::enable_shared_from_this<ClientSession>
    {
    public:
        ClientSession(tcp::socket aSocket);

        void Start()
        {
            do_read();
        }

    private:
        void do_read()
        {
            auto self(shared_from_this());
            mSocket.async_read_some(boost::asio::buffer(data_, max_length),
                [this, self](boost::system::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    do_write(length);
                }
            });
        }

        void do_write(std::size_t length)
        {
            auto self(shared_from_this());
            boost::asio::async_write(mSocket, boost::asio::buffer(data_, length),
                [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    do_read();
                }
            });
        }
        enum { max_length = 1024 };
        char data_[max_length];

        tcp::socket mSocket;
    };
}
#endif // CLIENT_SESSION_HPP
