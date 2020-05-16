#include "client.h"
#include <thread>

void output_to_another_console(client&);

int main(int argc, char** argv)
{
	system("cls");
	if (argc != 4) {
		std::cout << "Usage:" << std::endl;
		std::cout << '\t' << argv[0] << " [ip] [port] [nickname]" << std::endl;
		std::cout << R"(When you want to disconnected from server type "\disconnect/")" << std::endl;
		system("pause");
		return 0;
	}
	try
	{
		const auto port = std::stoi(argv[2]);
		if (port < 0) {
			std::cout << "Invalid port" << std::endl;
			return 0;
		}
		client client(argv[0], argv[1], std::stoi(argv[2]), argv[3]);
		client.send(argv[3]);
		std::thread thread(output_to_another_console, std::ref(client));
		std::string user_input;
		while (true)
		{
			std::cout << ">";
			std::getline(std::cin, user_input);
			if (user_input == "\\disconnect/") {
				client.send(user_input);
				break;
			}
			if (!user_input.empty()) {
				client.send(user_input);
			}
			system("cls");
		}
		thread.join();
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	system("pause");
	return 0;
}

void output_to_another_console(client &c)
{
	char write_buf[BUFSIZ];
	while (true)
	{
		WaitForSingleObject(c.get_server_can_enter(), INFINITE);
		const auto received = c.receive();
		if (received.empty()) {
			ReleaseSemaphore(c.get_server_can_enter(), 1, nullptr);
			continue;
		}
		strcpy_s(write_buf, BUFSIZ, received.c_str());
		DWORD written;
		if (!WriteFile(c.get_pipe(), write_buf, received.size(), &written, nullptr)) {
			std::cerr << GetLastError() << std::endl;
			break;
		}
		ReleaseSemaphore(c.get_client_can_print(), 1, nullptr);
	}
	FlushFileBuffers(c.get_pipe());
	DisconnectNamedPipe(c.get_pipe());
	CloseHandle(c.get_pipe());
}