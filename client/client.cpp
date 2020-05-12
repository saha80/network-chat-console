#include "client.h"

#include <thread>

using namespace std;

struct handle_client
{
	HANDLE h;
	client c;
};

void my_thread_func(client &c)
{
	char write_buf[BUFSIZE];
	while (true)
	{
		WaitForSingleObject(c.get_server_can_enter(), INFINITE);
		const auto s = c.receive();
		if (s.empty()) {
			ReleaseSemaphore(c.get_server_can_enter(), 1, nullptr);
			continue;
		}
		strcpy_s(write_buf, BUFSIZE, s.c_str());
		DWORD cbWritten;
		if (!WriteFile(c.get_pipe(), write_buf, s.size(), &cbWritten, nullptr)) {
			cerr << GetLastError() << endl;
		}
		ReleaseSemaphore(c.get_client_can_print(), 1, nullptr);
	}
	FlushFileBuffers(c.get_pipe());
	DisconnectNamedPipe(c.get_pipe());
	CloseHandle(c.get_pipe());
}

int main(int argc, char** argv)
{
	try
	{
		if (argc < 2) {
			cout << "no ip, no port" << endl;
			return 0;
		}
		const auto port = stoi(argv[2]);
		client client(argv[1], port);
		client.send("nickname");
		std::thread thread(my_thread_func, std::ref(client));
		//handle_client h_c{ client.get_pipe(), client };
		//auto f = [](LPVOID lpvParam) -> DWORD WINAPI
		//{
		//	auto hHeap = GetProcessHeap();
		//	auto pchRequest = (char*)HeapAlloc(hHeap, 0, BUFSIZE);
		//	auto write_buf = (char*)HeapAlloc(hHeap, 0, BUFSIZE);
		//	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
		//	BOOL fSuccess = FALSE;
		//	HANDLE hPipe = nullptr;
		//	if (lpvParam == nullptr) {
		//		printf("\nERROR - Pipe Server Failure:\n");
		//		printf("   InstanceThread got an unexpected NULL value in lpvParam.\n");
		//		printf("   InstanceThread exiting.\n");
		//		if (write_buf != nullptr) HeapFree(hHeap, 0, write_buf);
		//		if (pchRequest != nullptr) HeapFree(hHeap, 0, pchRequest);
		//		return (DWORD)-1;
		//	}
		//	if (pchRequest == nullptr) {
		//		printf("\nERROR - Pipe Server Failure:\n");
		//		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		//		printf("   InstanceThread exitting.\n");
		//		if (write_buf != nullptr) HeapFree(hHeap, 0, write_buf);
		//		return (DWORD)-1;
		//	}
		//	if (write_buf == nullptr)
		//	{
		//		printf("\nERROR - Pipe Server Failure:\n");
		//		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		//		printf("   InstanceThread exitting.\n");
		//		if (pchRequest != nullptr) HeapFree(hHeap, 0, pchRequest);
		//		return (DWORD)-1;
		//	}
		//	printf("InstanceThread created, receiving and processing messages.\n");
		//	auto h_c = (handle_client*)lpvParam;
		//	hPipe = h_c->h;
		//	auto clnt = h_c->c;
		//	while (true)
		//	{
		//		const auto s = clnt.receive();
		//		if (s.empty()) {
		//			continue;
		//		}
		//		strcpy_s(write_buf, BUFSIZE, s.c_str());
		//		fSuccess = WriteFile(
		//			hPipe,        // handle to pipe
		//			write_buf,     // buffer to write from
		//			cbReplyBytes, // number of bytes to write
		//			&cbWritten,   // number of bytes written
		//			nullptr);     // not overlapped I/O
		//		if (!fSuccess && GetLastError() != ERROR_NO_DATA)
		//		{
		//			_tprintf(TEXT("InstanceThread WriteFile failed, GLE=%lu.\n"), GetLastError());
		//			break;
		//		}
		//	}
		//	FlushFileBuffers(hPipe);
		//	DisconnectNamedPipe(hPipe);
		//	CloseHandle(hPipe);
		//	HeapFree(hHeap, 0, pchRequest);
		//	HeapFree(hHeap, 0, write_buf);
		//	printf("InstanceThread exiting.\n");
		//	return 1;
		//};
		//DWORD  dwThreadId = 0;
		//printf("Client connected, creating a processing thread.\n");
		//auto hThread = CreateThread(nullptr, 0, f, (LPVOID)&h_c, 0, &dwThreadId);
		//if (hThread == nullptr) {
		//	const auto msg = "CreateThread failed, E" + std::to_string(GetLastError());
		//	throw std::exception(msg.c_str());
		//}
		//CloseHandle(hThread);
		client.user_loop();
		thread.join();
		//auto running = true;
		//std::string user_input;
		//while (running)
		//{
		//	std::cout << ">";
		//	std::getline(std::cin, user_input);
		//	if (user_input == "\\disconnect/") {
		//		running = false;
		//		break;
		//	}
		//	if (!user_input.empty())
		//	{
		//		client.send(user_input);
		//		const auto rcv = client.receive();
		//		cout << rcv << endl;
		//		//if (send(sock_, user_input.c_str(), (int)user_input.size() + 1, 0) != SOCKET_ERROR) {
		//		//	ZeroMemory(buf, sizeof(buf));
		//		//	const auto bytesReceived = recv(sock_, buf, 4096, 0);
		//		//	if (bytesReceived == SOCKET_ERROR) {
		//		//		std::cerr << "Error in recv(). Quitting" << std::endl;
		//		//		break;
		//		//	}
		//		//	if (bytesReceived > 0) {
		//		//		std::cout << "SERVER> " << std::string(buf, 0, bytesReceived) << std::endl;
		//		//	}
		//		//}
		//		//else {
		//		//	std::cout << "Error in send()" << std::endl;
		//		//	break;
		//		//}
		//	}
		//}
	}
	catch (std::exception &e)
	{
		cerr << e.what() << endl;
	}
	catch (...)
	{
		cerr << bad_exception().what() << endl;
	}
}