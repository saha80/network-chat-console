#pragma once
#include <iostream>
#include <sstream>
#include <conio.h>
#include <map>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

class server
{
public:
	server(const u_short port) : port_(port)
	{
		initialize_win_sock();
		setup_listening_socket();
	}
	void server_loop()
	{
		char buf[4096];
		const std::string disconnect_cmd("\\disconnect/");
		std::cout << "server has started" << std::endl;
		while (true)
		{
			auto master_copy = master_; // select is destructive
			const auto socket_count = select(0, &master_copy, nullptr, nullptr, nullptr);
			for (auto i = 0; i < socket_count; ++i) {
				const auto sock = master_copy.fd_array[i];
				if (sock == listening_) {
					const auto new_client = accept(listening_, nullptr, nullptr);
					FD_SET(new_client, &master_); // add to set
					ZeroMemory(buf, sizeof(buf));
					const auto rcv = recv(new_client, buf, sizeof(buf), 0);
					auto nick_name = std::string(buf, 0, rcv);
					if (nick_name.empty()) {
						nick_name = "CS" + std::to_string(new_client);
					}
					std::cout << nick_name << std::endl;
					clients_.insert({ new_client, nick_name });
					send_to_all(nick_name + " has joined!\n");
				}
				else {
					ZeroMemory(buf, sizeof(buf));
					const auto bytes_received = recv(sock, buf, sizeof(buf), 0);
					if (bytes_received == SOCKET_ERROR) {
						closesocket(sock);
						FD_CLR(sock, &master_);
						std::cerr << "Client sock #" << sock
							<< " has been dropped, E" << WSAGetLastError() << std::endl;
					}
					if (bytes_received == 0) {
						disconnect_client(sock);
					}
					else {
						if (buf[0] == '\\') {
							if (std::string(buf) == disconnect_cmd) {
								disconnect_client(sock);
								continue;
							}
						}
						std::ostringstream oss;
						oss << "<" << clients_.find(sock)->second << ">: " << buf << "\n";
						send_to_all(oss.str());
					}
				}
			}
		}
	}
	~server()
	{
		FD_CLR(listening_, &master_);
		closesocket(listening_);
		const auto msg = "Server is shutting down. Goodbye\r\n";
		while (master_.fd_count > 0)
		{
			send(master_.fd_array[0], msg, (int)strlen(msg) + 1, 0);
			FD_CLR(master_.fd_array[0], &master_);
			closesocket(master_.fd_array[0]);
		}
		WSACleanup();
	}
private:
	void initialize_win_sock()
	{
		const auto result = WSAStartup(MAKEWORD(2, 2), &wsa_data_);
		if (result != 0) {
			throw std::logic_error("Can't start WinSock, Err #" + std::to_string(result));
		}
	}
	void setup_listening_socket()
	{
		listening_ = socket(AF_INET, SOCK_STREAM, 0);
		if (listening_ == INVALID_SOCKET) {
			const auto msg = "Can't create socket, WSAData error code: " + std::to_string(WSAGetLastError());
			WSACleanup();
			throw std::logic_error(msg);
		}
		hint_.sin_family = AF_INET;
		hint_.sin_port = htons(port_);
		hint_.sin_addr.S_un.S_addr = INADDR_ANY;
		bind(listening_, reinterpret_cast<sockaddr*>(&hint_), sizeof(hint_));
		listen(listening_, SOMAXCONN);
		FD_ZERO(&master_);
		FD_SET(listening_, &master_);
	}
	void send_to_all(const std::string &msg)
	{
		for (u_int i = 0; i < master_.fd_count; ++i) {
			if (master_.fd_array[i] != listening_) {
				send(master_.fd_array[i], msg.c_str(), int(msg.size()) + 1, 0);
			}
		}
	}
	void disconnect_client(const SOCKET sock)
	{
		const auto client = clients_.find(sock);
		const auto client_nick = client->second;
		send_to_all(client_nick + " has left\n");
		closesocket(sock);
		FD_CLR(sock, &master_);
		clients_.erase(client);
	}
	//void send_to_all(const std::string &msg, const SOCKET except)
	//{
	//	for (u_int i = 0; i < master_.fd_count; ++i) {
	//		if (master_.fd_array[i] != listening_ && master_.fd_array[i] != except) {
	//			send(master_.fd_array[i], msg.c_str(), (int)msg.size() + 1, 0);
	//		}
	//	}
	//}
private:
	WSADATA wsa_data_;
	SOCKET listening_;
	sockaddr_in hint_;
	u_short port_;
	fd_set master_;
	std::map<SOCKET, std::string> clients_;
};
