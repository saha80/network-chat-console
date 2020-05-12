#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <tchar.h>

using namespace std;

int main(int argc, char** argv)
{
	char buf[BUFSIZ];
	DWORD  cbRead;
	const auto pipe_name = R"(\\.\pipe\my_pipe)";
	auto pipe = CreateFileA(pipe_name, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (pipe == INVALID_HANDLE_VALUE) {
		cerr << "CreateFile failed, GetLastError = " << GetLastError() << endl;
		system("pause");
		return EXIT_FAILURE;
	}
	auto server_can_enter = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, false, "_pipe_server_can_enter_event_");
	auto client_can_print = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, false, "_pipe_client_can_print_event_");
	if (!server_can_enter || !client_can_print) {
		cerr << "OpenSemaphoreA failed, GetLastError = " << GetLastError() << endl;
		system("pause");
		return EXIT_FAILURE;
	}
	cout << "connected to pipe" << endl;
	while (true)
	{
		WaitForSingleObject(client_can_print, INFINITE);
		ZeroMemory(buf, BUFSIZ);
		if (!ReadFile(pipe, buf, BUFSIZ, &cbRead, nullptr)) {
			cerr << "ReadFile failed, GetLastError = " << GetLastError() << endl;
		}
		string s(buf);
		cout << buf << endl;
		ReleaseSemaphore(server_can_enter, 1, nullptr);
	}
	CloseHandle(pipe);
	CloseHandle(client_can_print);
	CloseHandle(server_can_enter);
	return 0;
}
#endif