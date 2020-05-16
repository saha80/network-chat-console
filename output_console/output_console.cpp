#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <iostream>
#include <string>

int main()
{
	char buf[BUFSIZ];
	const auto pipe_name = R"(\\.\pipe\output_console)";
	auto pipe = CreateFileA(pipe_name, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (pipe == INVALID_HANDLE_VALUE) {
		std::cerr << "CreateFileA failed, GetLastError = " << GetLastError() << std::endl;
		system("pause");
		return EXIT_FAILURE;
	}
	auto server_can_enter = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, false, "_pipe_server_can_enter_event_");
	auto client_can_print = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, false, "_pipe_client_can_print_event_");
	if (!server_can_enter || !client_can_print) {
		std::cerr << "OpenSemaphoreA failed, GetLastError = " << GetLastError() << std::endl;
		system("pause");
		return EXIT_FAILURE;
	}
	while (true) {
		WaitForSingleObject(client_can_print, INFINITE);
		ZeroMemory(buf, BUFSIZ);
		DWORD  read;
		if (!ReadFile(pipe, buf, BUFSIZ, &read, nullptr)) {
			std::cerr << "ReadFile failed, GetLastError = " << GetLastError() << std::endl;
			break;
		}
		std::cout << buf;
		ReleaseSemaphore(server_can_enter, 1, nullptr);
	}
	CloseHandle(pipe);
	CloseHandle(client_can_print);
	CloseHandle(server_can_enter);
	return 0;
}
#endif