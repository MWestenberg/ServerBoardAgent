#pragma once
#include <nlohmann/json.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <array> // array
#include <cmath> // isfinite, labs, ldexp, signbit
#include <forward_list> // forward_list
#include <numeric> // accumulate
#include <algorithm>
#include <memory>
#include <sstream>
#include <time.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using JSON = nlohmann::json;

class Collector {

private:
	void SetHostID(JSON&);
	void SetHostname(JSON&);
	void SetLocalIp(JSON&);
	void SetDiskUsage(JSON&);
	void SetAPIKey(JSON&);
	void SetCPUInfo(JSON&);
	void SetMemInfo(JSON&);
	void SetDateTime(JSON&);
	void SetNullValues(JSON&);
	void SetTCPPorts(JSON&);

public:
	
	Collector(JSON& json, const std::map<int, std::string>&tcpPorts , bool debug = false) : m_ports(tcpPorts), m_debug(debug)
	{
		const char* cmd = "cat /etc/redhat-release";
		std::string result = Collector::exec(cmd, debug);
		m_OS_VERSION = result.find("release 7.") ? 7 : result.find("release 6.") ? 6 : 5;
		if (m_debug)
			std::cout << "VERSION: " << m_OS_VERSION << std::endl;
		
		if (m_debug)
			//m_ports.insert(std::make_pair<int, std::string>(22, "F"));

		
		SetNullValues(json);
		SetAPIKey(json);
		SetHostID(json);
		SetCPUInfo(json);
		SetHostname(json);
		SetLocalIp(json);
		SetDiskUsage(json);
		SetMemInfo(json);
		SetDateTime(json);
		SetTCPPorts(json);

		if (m_debug)
			std::cout << json.dump(4) << std::endl;
	
	}


	static const void WriteCPUHistory();

	~Collector() {
		if (m_debug)
			std::cout << "Collector was destroyed" << std::endl;
	};
	

private:

	bool m_debug = false;
	unsigned int m_OS_VERSION;
	std::map<int, std::string> m_ports;

	struct CPUInfo
	{
		std::string Architecture;
		std::string AvgLoad;
		std::string Identifier;
		std::string Level;
		std::string Revision;
	};

	struct AvgLoad 
	{
		std::string Load;
	};

	struct CPUHistory
	{
		std::string History;
	};

	struct MemInfo
	{
		std::string FreePhysical;
		std::string FreeVirtual;
		std::string PCTPhysicalAvailable;
		std::string TotalPhysical;
		std::string TotalVirtual;
	};


	// trim from start
	static inline std::string& ltrim(std::string &s) 
{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
		return s;
	}

	// trim from end
	static inline std::string & rtrim(std::string &s) 
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
		return s;
	}


	// trim from both ends
	static inline std::string& trim(std::string &s) 
	{
		return ltrim(rtrim(s));
	}

	static inline std::vector<std::string> Split(const std::string& s, char delimiter)
	{
		std::vector<std::string> tokens;
		std::string token;
		std::istringstream tokenStream(s);
		while (std::getline(tokenStream, token, delimiter))
		{
			tokens.push_back(trim(token));
		}
		return tokens;
	}

	//method to execute commands
	static std::string exec(const char * cmd, bool debug)
	{
		std::array<char, 128> buffer;
		std::string result;

		if (debug)
			std::cout << cmd << std::endl << std::endl;

		std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
		if (!pipe) throw std::runtime_error("popen() failed!");
		while (!feof(pipe.get())) {
			if (fgets(buffer.data(), 128, pipe.get()) != NULL)
				result += buffer.data();
		}
		
		trim(result);
		if (debug && !result.empty())
			std::cout << result << std::endl << std::endl;

		return result;
	}


	template <typename T>
	void ReadFile(const std::string& filePath, T& info) const;
	bool CanReadFile(const std::string& filePath) const;
	void SetInfo(std::istream&, CPUInfo&) const;
	void SetInfo(std::istream&, AvgLoad&) const;
	void SetInfo(std::istream&, MemInfo&) const;
	void SetInfo(std::istream&, CPUHistory&) const;
	std::string GetHostname();

};
