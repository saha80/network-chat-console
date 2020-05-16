#pragma once
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class client
{
public:
	client(const std::string &path, std::string ip, const u_short port)
		: ip_address_(std::move(ip)), port_(port)
	{
		setup_client();
		setup_output(path);
	}
	void send(const std::string& msg) const
	{
		if (::send(sock_, msg.c_str(), int(msg.size()) + 1, 0) == SOCKET_ERROR) {
			throw std::exception("SOCKET_ERROR");
		}
	}
	std::string receive()
	{
		ZeroMemory(read_buf_, sizeof(read_buf_));
		const auto received_bytes = recv(sock_, read_buf_, sizeof(read_buf_), 0);
		if (received_bytes == SOCKET_ERROR) {
			throw std::exception("SOCKET_ERROR");
		}
		return std::string(read_buf_, 0, received_bytes);
	}
	HANDLE get_pipe() const
	{
		return pipe_;
	}
	HANDLE get_client_output_received_msg() const
	{
		return client_output_received_msg;
	}
	HANDLE get_wait_for_output() const
	{
		return wait_for_output;
	}
	~client()
	{
		closesocket(sock_);
		WSACleanup();
	}
private:
	void setup_client()
	{
		const auto result = WSAStartup(MAKEWORD(2, 2), &wsa_data_);
		if (result != 0) {
			throw std::logic_error("Can't start WinSock, error code: " + std::to_string(result));
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
	}
	void setup_output(std::string path)
	{
		STARTUPINFO si{ sizeof(si) };
		PROCESS_INFORMATION pi{};
		while (!path.empty() && path.back() != '\\') {
			path.pop_back();
		}
		const auto output_console = path + "output_console.exe";
		if (!CreateProcessA(output_console.c_str(), nullptr, nullptr, nullptr, true,
			CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi))
		{
			const auto msg = "CreateProcess wailed, E" + std::to_string(GetLastError());
			closesocket(sock_);
			WSACleanup();
			throw std::exception(msg.c_str());
		}
		const auto pipe_name = R"(\\.\pipe\output_console)";
		pipe_ = CreateNamedPipeA(pipe_name, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE, 2, BUFSIZ, BUFSIZ, 0, nullptr);
		client_output_received_msg = CreateSemaphoreA(nullptr, 1, 1, "client_output_received_msg");
		wait_for_output = CreateSemaphoreA(nullptr, 0, 1, "wait_for_output");
		if (pipe_ == INVALID_HANDLE_VALUE) {
			closesocket(sock_);
			WSACleanup();
			const auto msg = "CreateNamedPipe failed, E" + std::to_string(GetLastError());
			throw std::exception(msg.c_str());
		}
		fConnected = ConnectNamedPipe(pipe_, nullptr) ? TRUE : GetLastError() == ERROR_PIPE_CONNECTED;
	}
private:
	WSADATA wsa_data_;
	SOCKET sock_;
	sockaddr_in hint_;
	std::string ip_address_;
	int port_;
	char read_buf_[4096];
	HANDLE client_output_received_msg;
	HANDLE wait_for_output;
	BOOL   fConnected = FALSE;
	HANDLE pipe_ = INVALID_HANDLE_VALUE;
};
