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
	auto client_output_received_msg = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, false, "client_output_received_msg");
	auto wait_for_output = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, false, "wait_for_output");
	if (!client_output_received_msg || !wait_for_output) {
		std::cerr << "OpenSemaphoreA failed, GetLastError = " << GetLastError() << std::endl;
		system("pause");
		return EXIT_FAILURE;
	}
	while (true) {
		WaitForSingleObject(wait_for_output, INFINITE);
		ZeroMemory(buf, BUFSIZ);
		DWORD  read;
		if (!ReadFile(pipe, buf, BUFSIZ, &read, nullptr)) {
			std::cerr << "ReadFile failed, GetLastError = " << GetLastError() << std::endl;
			break;
		}
		std::cout << buf;
		ReleaseSemaphore(client_output_received_msg, 1, nullptr);
	}
	CloseHandle(pipe);
	CloseHandle(wait_for_output);
	CloseHandle(client_output_received_msg);
	return 0;
}
#endif