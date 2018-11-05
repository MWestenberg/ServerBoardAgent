#pragma once
#include <nlohmann/json.h>
#include <curl/curl.h>
#include <iostream>
#include <sstream>


using JSON = nlohmann::json;

class Connect
{
private:
	int m_interval;
	bool m_debug;
	std::map<int, std::string> m_tcpports;
public:
	explicit Connect(JSON& json, bool debug = false) : m_interval(2000), m_debug(debug)
	{
		PostJson(json);
	};

	std::map<int, std::string>& GetTCPPorts()
	{
		return m_tcpports;
	}

	int GetInterval() const
	{
		return m_interval;
	}

	~Connect()
	{
		if (m_debug)
			std::cout << "Connector was destroyed" << std::endl;
	};

private:
	bool PostJson(JSON& json);
	void SetInterval(JSON& response);
	void SetTCPPorts(JSON& response);
};