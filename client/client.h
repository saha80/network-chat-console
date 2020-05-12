#pragma once
#include <iostream>
#include <string>
#include <strsafe.h>
#include <tchar.h>
#include <utility>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

constexpr auto BUFSIZE = 4096;

void GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes)
{
	_tprintf(TEXT("Client Request String:\"%s\"\n"), pchRequest);
	if (FAILED(StringCchCopy(pchReply, BUFSIZE, TEXT(pchRequest))))
	{
		*pchBytes = 0;
		pchReply[0] = 0;
		printf("StringCchCopy failed, no outgoing message.\n");
		return;
	}
	*pchBytes = lstrlen(pchReply) + 1;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
{
	auto hHeap = GetProcessHeap();
	auto pchRequest = (char*)HeapAlloc(hHeap, 0, BUFSIZE);
	auto pchReply = (char*)HeapAlloc(hHeap, 0, BUFSIZE);
	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = nullptr;
	if (lpvParam == nullptr) {
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL value in lpvParam.\n");
		printf("   InstanceThread exiting.\n");
		if (pchReply != nullptr) HeapFree(hHeap, 0, pchReply);
		if (pchRequest != nullptr) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}
	if (pchRequest == nullptr) {
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		printf("   InstanceThread exitting.\n");
		if (pchReply != nullptr) HeapFree(hHeap, 0, pchReply);
		return (DWORD)-1;
	}
	if (pchReply == nullptr)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		printf("   InstanceThread exitting.\n");
		if (pchRequest != nullptr) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}
	printf("InstanceThread created, receiving and processing messages.\n");
	hPipe = (HANDLE)lpvParam;
	while (true)
	{
		fSuccess = WriteFile(
			hPipe,        // handle to pipe
			pchReply,     // buffer to write from
			cbReplyBytes, // number of bytes to write
			&cbWritten,   // number of bytes written
			nullptr);     // not overlapped I/O
		if (!fSuccess || cbReplyBytes != cbWritten)
		{
			_tprintf(TEXT("InstanceThread WriteFile failed, GLE=%lu.\n"), GetLastError());
			break;
		}
	}
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);
	printf("InstanceThread exiting.\n");
	return 1;
}

class client
{
public:
	client() = default;
	client(std::string ip, const u_short port) : ip_address_(std::move(ip)), port_(port)
	{
		setup_client();
		setup_output();
	}
	void user_loop()
	{
		//auto running = true;
		//std::string user_input;
		//while (running)
		//{
		//	std::cout << ">";
		//	std::getline(std::cin, user_input);
		//	if (user_input == "\\disconnect/") {
		//		break;
		//	}
		//	if (!user_input.empty())
		//	{
		//		if (::send(sock_, user_input.c_str(), (int)user_input.size() + 1, 0) != SOCKET_ERROR) {
		//			ZeroMemory(buf_, 4096);
		//			const auto bytesReceived = recv(sock_, buf_, 4096, 0);
		//			if (bytesReceived == SOCKET_ERROR) {
		//				std::cerr << "Error in recv(). Quitting" << std::endl;
		//				break;
		//			}
		//			if (bytesReceived > 0) {
		//				std::cout << "SERVER> " << std::string(buf_, 0, bytesReceived) << std::endl;
		//			}
		//		}
		//		else {
		//			std::cout << "Error in recv()" << std::endl;
		//			break;
		//		}
		//	}
		//}
		auto running = true;
		std::string user_input;
		while (running)
		{
			std::cout << ">";
			std::getline(std::cin, user_input);
			if (user_input == "\\disconnect/") {
				running = false;
				break;
			}
			if (!user_input.empty())
			{
				send(user_input);
				//const auto rcv = receive();
				//std::cout << rcv << std::endl;

				//if (send(sock_, user_input.c_str(), (int)user_input.size() + 1, 0) != SOCKET_ERROR) {
				//	ZeroMemory(buf, sizeof(buf));
				//	const auto bytesReceived = recv(sock_, buf, 4096, 0);
				//	if (bytesReceived == SOCKET_ERROR) {
				//		std::cerr << "Error in recv(). Quitting" << std::endl;
				//		break;
				//	}
				//	if (bytesReceived > 0) {
				//		std::cout << "SERVER> " << std::string(buf, 0, bytesReceived) << std::endl;
				//	}
				//}
				//else {
				//	std::cout << "Error in send()" << std::endl;
				//	break;
				//}
			}
		}
	}
	void send(const std::string& msg) const
	{
		if (::send(sock_, msg.c_str(), int(msg.size()) + 1, 0) == SOCKET_ERROR) {
			throw std::exception("SOCKET_ERROR");
		}
	}
	std::string receive()
	{
		ZeroMemory(buf_, sizeof(buf_));
		const auto received_bytes = recv(sock_, buf_, sizeof(buf_), 0);
		if (received_bytes == SOCKET_ERROR) {
			throw std::exception("SOCKET_ERROR");
		}
		return std::string(buf_, 0, received_bytes);
	}
	HANDLE get_pipe() const
	{
		return pipe_;
	}
	HANDLE get_server_can_enter() const
	{
		return server_can_enter;
	}
	HANDLE get_client_can_print() const
	{
		return client_can_print;
	}
	~client()
	{
		closesocket(sock_);
		WSACleanup();
	}
private:
	void setup_client()
	{
		// setup win sock
		const auto result = WSAStartup(MAKEWORD(2, 2), &wsa_data_);
		if (result != 0) {
			throw std::logic_error("Can't start WinSock, error code: " + std::to_string(result));
		}
		// setup socket
		sock_ = socket(AF_INET, SOCK_STREAM, 0);
		if (sock_ == INVALID_SOCKET) {
			const auto msg = "Can't create socket, WSAData error code: " + std::to_string(WSAGetLastError());
			WSACleanup();
			throw std::exception(msg.c_str());
		}
		// fill hint
		hint_.sin_family = AF_INET;
		hint_.sin_port = htons(port_);
		inet_pton(AF_INET, ip_address_.c_str(), &hint_.sin_addr);
		// connect to server
		if (connect(sock_, (sockaddr*)&hint_, sizeof(hint_)) == SOCKET_ERROR) {
			const auto msg = "Can't connect to server, WSAData error code: " + std::to_string(WSAGetLastError());
			closesocket(sock_);
			WSACleanup();
			throw std::exception(msg.c_str());
		}
	}
	void setup_output()
	{
		STARTUPINFO si{ sizeof(si) };
		PROCESS_INFORMATION pi{};
		if (!CreateProcessA((char*)R"(C:\Users\Alex\source\repos\network chat console\Debug\output_console.exe)",
			0, 0, 0, true, CREATE_NEW_CONSOLE, 0, 0, &si, &pi))
		{
			const auto msg = "CreateProcess wailed, E" + std::to_string(GetLastError());
			throw std::exception(msg.c_str());
		}
		const auto pipe_name = R"(\\.\pipe\my_pipe)";
		_tprintf(TEXT("\nPipe Server: Main thread awaiting client connection on %s\n"), pipe_name);
		pipe_ = CreateNamedPipe(pipe_name, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE, 2, BUFSIZE, BUFSIZE, 20000000, nullptr);
		server_can_enter = CreateSemaphoreA(nullptr, 1, 1, "_pipe_server_can_enter_event_");
		client_can_print = CreateSemaphoreA(nullptr, 0, 1, "_pipe_client_can_print_event_");
		if (pipe_ == INVALID_HANDLE_VALUE)
		{
			const auto msg = "CreateNamedPipe failed, E" + std::to_string(GetLastError());
			throw std::exception(msg.c_str());
		}
		fConnected = ConnectNamedPipe(pipe_, nullptr) ? TRUE : GetLastError() == ERROR_PIPE_CONNECTED;

		//DWORD  dwThreadId = 0;
		//if (fConnected) {
		//	printf("Client connected, creating a processing thread.\n");
		//	hThread = CreateThread(nullptr, 0, InstanceThread, (LPVOID)pipe_, 0, &dwThreadId);
		//	if (hThread == nullptr) {
		//		const auto msg = "CreateThread failed, E" + std::to_string(GetLastError());
		//		throw std::exception(msg.c_str());
		//	}
		//	CloseHandle(hThread);
		//}
		//else
		//	CloseHandle(pipe_);
	}
private:
	WSAData wsa_data_;
	SOCKET sock_;
	sockaddr_in hint_;
	std::string ip_address_;
	int port_;
	char buf_[4096];

	///
	HANDLE server_can_enter;
	HANDLE client_can_print;
	BOOL   fConnected = FALSE;
	HANDLE pipe_ = INVALID_HANDLE_VALUE, hThread = nullptr;
	LPCTSTR pipe_name_ = TEXT("\\\\.\\pipe\\pipe_name");
};
