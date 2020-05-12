#include "server.h"
using namespace std;

int main(int argc, char **argv)
{
	try
	{
		if (argc != 2)
		{
			cout << "invalid args" << endl;
			return 0;
		}
		server server(stoi(argv[1]));
		server.server_loop();
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}
}