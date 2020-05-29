#pragma once
#include <algorithm>
#include <iostream>
#include <regex>
#include <string>
#include <WS2tcpip.h>

#include "output_console.h"
#pragma comment(lib, "ws2_32.lib")

class client
{
public:
	client(const std::string &path, std::string ip, const u_short port, std::string nick)
		: ip_address_(std::move(ip)), port_(port), console_(path)
	{
		nick.erase(std::remove_if(nick.begin(), nick.end(), [](const char &c) -> bool {
			return !(c > 0x001F && c < 0x007F);
		}), nick.end());
		const auto WSA_res = WSAStartup(MAKEWORD(2, 2), &wsa_data_);
		if (WSA_res != 0) {
			throw std::logic_error("Can't start WinSock, error code: " + std::to_string(WSA_res));
		}
		sock_ = socket(AF_INET, SOCK_STREAM, 0);
		if (sock_ == INVALID_SOCKET) {
			const auto msg = "Can't create socket, WSAData error code: " + std::to_string(WSAGetLastError());
			WSACleanup();
			throw std::exception(msg.c_str());
		}
		hint_.sin_family = AF_INET;
		hint_.sin_port = htons(port_);
		const auto inet_pton_res = inet_pton(AF_INET, ip_address_.c_str(), &hint_.sin_addr);
		if (inet_pton_res == 0) {
			WSACleanup();
			throw std::exception("Invalid IPv4");
		}
		if (inet_pton_res == -1) {
			const auto msg = "WSAGetLastError = " + std::to_string(WSAGetLastError());
			WSACleanup();
			throw std::exception(msg.c_str());
		}
		if (connect(sock_, reinterpret_cast<sockaddr*>(&hint_), sizeof(hint_)) == SOCKET_ERROR) {
			const auto msg = "Can't connect to server, WSAGetLastError = " + std::to_string(WSAGetLastError());
			closesocket(sock_);
			WSACleanup();
			throw std::exception(msg.c_str());
		}
		send(nick);
	}
	void connect_to_server()
	{
		DWORD id;
		auto output_to_console_thread = CreateThread(nullptr, 0, output_to_another_console, (void*)this, 0, &id);
		std::string user_input;
		while (true)
		{
			std::cout << ">";
			std::getline(std::cin, user_input);
			format_user_input(user_input);
			if (user_input == "\\disconnect/") {
				send(user_input);
				TerminateThread(output_to_console_thread, EXIT_SUCCESS);
				CloseHandle(output_to_console_thread);
				break;
			}
			if (!user_input.empty()) {
				send(user_input);
			}
			system("cls");
		}
	}
	~client()
	{
		closesocket(sock_);
		WSACleanup();
	}
private:
	void send(const std::string& msg) const
	{
		if (::send(sock_, msg.c_str(), int(msg.size()) + 1, 0) == SOCKET_ERROR) {
			throw std::exception("Server has been disconnected");
		}
	}
	std::string receive() const
	{
		char read_buf[BUFSIZ]{};
		const auto received_bytes = recv(sock_, read_buf, sizeof(read_buf), 0);
		if (received_bytes == SOCKET_ERROR) {
			throw std::exception("Server has been disconnected");
		}
		return std::string(read_buf, 0, received_bytes);
	}
	static void format_user_input(std::string &s)
	{
		std::replace(s.begin(), s.end(), '\t', ' ');
		s = std::regex_replace(s, std::regex("\\s{2,}"), " ");
		if (!s.empty() && s.front() == ' ') {
			s.erase(s.begin());
		}
		if (!s.empty() && s.back() == ' ') {
			s.pop_back();
		}
	}
	static DWORD WINAPI output_to_another_console(void* p)
	{
		auto c = (client*)p;
		std::string received;
		while (true)
		{
			c->console_.wait_output_received_msg();
			try {
				received = c->receive();
			}
			catch (std::exception &e) {
				c->console_.write_to_console(e.what());
				c->console_.release_wait_for_output();
				return 1;
			}
			if (received.empty()) {
				c->console_.release_output_received_msg();
				continue;
			}
			c->console_.write_to_console(received);
			c->console_.release_wait_for_output();
		}
	}
private:
	WSADATA wsa_data_;
	SOCKET sock_;
	sockaddr_in hint_;
	std::string ip_address_;
	u_short port_;

	output_console console_;
};
