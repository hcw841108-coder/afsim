#include "ClientSession.hpp"

XDU::ClientSession::ClientSession(tcp::socket aSocket)
	: mSocket(std::move(aSocket))
{
}
