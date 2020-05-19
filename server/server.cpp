#include "server.h"

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "Usage:" << std::endl;
		std::cout << '\t' << argv[0] << " [port]" << std::endl;
		return 0;
	}
	try {
		server server(std::stoi(argv[1]));
		server.wait_for_connections();
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
}