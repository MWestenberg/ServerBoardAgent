#include "Connect.h"

static const char *POSTURL = "https://api.serverboard.nl/monitor_update/";

//call back function for writing
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}



void Connect::SetInterval(JSON& response)
{
	if (response["config"]["updateinterval"] == nullptr || response["config"]["updateinterval"].empty())
		return;

	//set interval time
	std::string interval = response["config"]["updateinterval"];
	m_interval = (std::stoi(interval) > 0) ? std::stoi(interval) : 2000;
}

void Connect::SetTCPPorts(JSON& response) 
{

	if (response["config"]["tcpports"] == nullptr || response["config"]["tcpports"].empty())
		return;


	std::string jPorts = response["config"]["tcpports"];
	std::stringstream tcpPorts;
	tcpPorts << jPorts;
	std::string port;
	std::map<int, std::string> portMap;
	while (getline(tcpPorts, port, ','))
	{

		portMap[stoi(port)] = "";
	}

	m_tcpports = portMap;
}

bool Connect::PostJson(JSON & json)
{
	std::string jsonString = json.dump();
	std::string readBuffer;
	JSON Response;
	CURLcode res;

	CURL *curl = curl_easy_init();
	if (!curl)
		return false;

	curl_easy_setopt(curl, CURLOPT_URL, POSTURL);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (m_debug)
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	
	try {
		
		if (res != CURLE_OK) {
			throw "Unable to initiate curl";
		}
		
		Response = JSON::parse(readBuffer.c_str());     //parse process
		if (Response == nullptr)
		{
			throw "Failed to parse json";
		}

		SetInterval(Response);
		SetTCPPorts(Response);

	}
	catch (const char *p) {
		std::cout << p << std::endl;

		return false;
	}
	catch (...)
	{
		return false;
	}

	if (m_debug)
		std::cout << "Response: " << std::endl << Response.dump(4) << std::endl;

	return true;
}


