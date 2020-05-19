#include "client.h"
#include <thread>

int main(int argc, char** argv)
{
	system("cls");
	if (argc != 4) {
		std::cout << "Usage:" << std::endl;
		std::cout << '\t' << argv[0] << " [ip] [port] [nickname]" << std::endl;
		std::cout << R"(When you want to disconnect from server type "\disconnect/")" << std::endl;
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
		client.connect_to_server();
	}
	catch (std::exception &e) {
		system("cls");
		std::cerr << e.what() << std::endl;
	}
	system("pause");
	return 0;
}
