#include "server.h"

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "Usage:" << std::endl;
		std::cout << '\t' << argv[0] << " [port]" << std::endl;
		return 0;
	}
	try {
		const auto port = std::stoi(argv[1]);
		if (port < 0 || port > UINT16_MAX) {
			std::cout << "Invalid port" << std::endl;
			return 0;
		}
		server server(port);
		server.wait_for_connections();
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
}