#include <nlohmann/json.h>
#include "Collector.h"
#include <iostream>
#include "Connect.h"
#include <thread>


using JSON = nlohmann::json;


static int taskDelay = 2000;
static std::map<int, std::string> tcpPorts;


void backgroundTask(bool debug = false) 
{
	do {
		{
			Collector::WriteCPUHistory();
		}

		std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds(1));
	} while (!debug);
}


void MainTask(bool debug = false)
{
	JSON j;
	Collector c(j, tcpPorts, debug);
	Connect conn(j, debug);
	taskDelay = conn.GetInterval();
	tcpPorts = conn.GetTCPPorts();
}

int main(int argc, const char * argv[]) 
{
	
	bool debug = false;

	if (argc > 1) {
		std::string arg1 = argv[1];
		if (arg1 == "--debug") {
			debug = true;
		}
	}

	debug = false; //overrride

	std::thread t(backgroundTask, debug);
	t.detach();

	do
	{
		MainTask();
		std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::seconds((taskDelay / 1000)));
		
	} while (!debug);
	
	
    return 0;
}