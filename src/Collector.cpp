#include "Collector.h"

static const std::string APIKEY = "D6E5D888-7251-11E5-9D70-FEFF819CDC9F";
static std::string cpuHistoryFileLoc = "/tmp/cpuhistory";

void Collector::SetHostID(JSON & json)
{
	const char *cmd;

	if (m_OS_VERSION >= 7)
		cmd = "ifconfig | grep ether | awk 'NR==1 {print $2}'";
	else
		cmd = "ifconfig | grep HWaddr | awk '{print $5}'";

	std::string cpuid = Collector::exec(cmd, m_debug);
	json["HOSTID"] = cpuid;
}

void Collector::SetHostname(JSON& json)
{
	
	json["Hostname"] = GetHostname();
}

std::string Collector::GetHostname()
{
	const char *cmd = "hostname";
	std::string s = Collector::exec(cmd,m_debug);
	return s;
}

void Collector::SetLocalIp(JSON& json)
{
	const char *cmd = "ifconfig | grep -Eo 'inet (addr:)?([0-9]*\\.){3}[0-9]*' | grep -Eo '([0-9]*\\.){3}[0-9]*' | grep '10.'";

	std::string s = Collector::exec(cmd, m_debug);
	std::stringstream ss(s);
	std::string line;

	json["LocalIP"] = {};

	while (ss.good())
	{
		getline(ss, line, '\n');
		if (!trim(line).empty())
		{
			json["localIps"].push_back(trim(line));
		}
	}
}

void Collector::SetDiskUsage(JSON & json)
{
	const char *cmd = "df / | awk '/[0-9]%/{print $(NF-1)}'";
	std::string result = "";
	result = Collector::exec(cmd, m_debug);
	json["DiskUsage"] = { {"/", trim(result)} };
}

void Collector::SetAPIKey(JSON & json)
{
	json["APIKEY"] = APIKEY;
}

void Collector::SetDateTime(JSON& json)
{
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

	json["DateTime"] = buf;
}

void Collector::SetCPUInfo(JSON & json)
{

	CPUInfo cpuInfo;
	AvgLoad avLoad;
	CPUHistory cpuHistory;
	ReadFile("/tmp/cpuhistory", cpuHistory);
	ReadFile("/proc/cpuinfo", cpuInfo);
	ReadFile("/proc/loadavg", avLoad);

	json["Processor"] = {
		{"Architecture", cpuInfo.Architecture},
		{"Level", cpuInfo.Level},
		{"Cores", {{"0", "0"}} },
		{"Identifier", cpuInfo.Identifier},
		{"Revision", cpuInfo.Revision},
		{"AvgLoad", avLoad.Load},
		{"History", cpuHistory.History}
	};
}

void Collector::SetMemInfo(JSON& json)
{
	MemInfo memInfo;
	ReadFile("/proc/meminfo", memInfo);

	json["Memory"] = {
		{"FreePhysical", memInfo.FreePhysical},
		{"FreeVirtual", memInfo.FreeVirtual},
		{"PCTPhysicalAvailable", memInfo.PCTPhysicalAvailable},
		{"TotalPhysical", memInfo.TotalPhysical},
		{"TotalVirtual", memInfo.TotalVirtual}
	};
}

void Collector::SetNullValues(JSON & json)
{
	json["CPUTime"] = "";
	json["Domain"] = "";
	json["EnvironmentVar"] = "";
	json["JSONTime"] = "";
	json["MemTime"] = "";
	json["TCP"] = JSON::object();;
}

void Collector::SetTCPPorts(JSON &json)
{
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		if (m_debug)
			std::cout << "Error opening socket!" << std::endl;
		return;
	}

	try
	{
		const char* host = GetHostname().c_str();
		server = gethostbyname(host);
		if (server == nullptr)
			throw("Unable to get");

		bzero((char *)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	}
	catch (...)
	{
		close(sockfd);
		if (m_debug)
			std::cout << "Uable to obtain hardware address" << std::endl;
		return;
	}
	
	//iterate over all ports available in the map
	for (auto it = m_ports.begin(); it != m_ports.end(); it++)
	{

		serv_addr.sin_port = htons((uint16_t)it->first);
		if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
			if (m_debug)
				std::cout << "Port " << it->first << " is closed" << std::endl;

			m_ports[it->first] = "F";
			json["TCP"].emplace(std::to_string(it->first), "F");
		}
		else {
			if (m_debug)
				std::cout << "Port " << it->first << " is active" << std::endl;

			json["TCP"].emplace(std::to_string(it->first), "T");
		}

	}

	close(sockfd);
}

const void Collector::WriteCPUHistory()
{

	const char *cmd = "ps aux|awk 'NR > 0 { s +=$3 }; END {print s}'";

	std::string r = Collector::exec(cmd, false);

	if (trim(r) != "")
	{

		std::string cpuUsage = r;
		std::vector<std::string> perc;

		int numberOfLines = 0;
		int maxNumberOfLines = 300;

		//first read the file
		std::ifstream file(cpuHistoryFileLoc);
		//count by splitting
		if (file.is_open())
		{
			std::string segment;
			while (std::getline(file, segment, ','))
			{
				//each segment is a cpu percentage
				++numberOfLines; //add one
				perc.push_back(segment); //add the percentage to the vector
				if (numberOfLines >= maxNumberOfLines)
				{
					//the current nummer exceeds the max number so delete the first
					perc.erase(perc.begin());
				}
			}
		}

		//add the latest cpu info
		perc.push_back(cpuUsage);

		std::ofstream cpuHistoryFile;
		cpuHistoryFile.open(cpuHistoryFileLoc, std::ios::trunc);

		if (cpuHistoryFile.is_open())
		{
			int counter = 0;
			for (std::string value : perc) {
				if (counter == 0)
				{
					cpuHistoryFile << value;
				}
				else
				{
					cpuHistoryFile << "," + value;
				}
				counter++;

			}
			cpuHistoryFile.close();
		}
	}
}

template <typename T>
void Collector::ReadFile(const std::string& filePath, T& info) const
{	
	if (!CanReadFile(filePath))
		return;

	std::ifstream infile;
	infile.open(filePath, std::ios_base::in);
	if (infile.is_open())
		SetInfo(infile, info);
	infile.close();
}

void Collector::SetInfo(std::istream& f, CPUInfo& cpuinfo) const
{
	enum class InfoType
	{
		NONE, model, level, identifier, COUNT
	};
	std::string line;
	std::string delimiter = ";";
	InfoType type = InfoType::NONE;
	std::stringstream ss[(int)InfoType::COUNT];

	while (getline(f, line, '\n'))
	{
		if (line.find("model name") != std::string::npos)
			type = InfoType::model;
		else if (line.find("cpuid level") != std::string::npos)
			type = InfoType::level;
		else
			continue;

		ss[(int)type] << line;
	}

	std::vector<std::string> modelVec = Split(ss[(int)InfoType::model].str(), ':');
	std::vector<std::string> levelVec = Split(ss[(int)InfoType::level].str(), ':');

	cpuinfo.Architecture = modelVec[(modelVec.size() - 1)];
	cpuinfo.Level = levelVec[(levelVec.size() - 1)];
}

void Collector::SetInfo(std::istream& f, AvgLoad& avgl) const
{
	std::string line;
	std::string delimiter = ";";

	getline(f, line, '\n');

	if (line.empty())
		return;

	std::vector<std::string> load = Split(line, ' ');

	if (load.size() > 0)
		avgl.Load = load[0];

}


void Collector::SetInfo(std::istream& f, MemInfo& meminfo) const
{
	enum class InfoType
	{
		NONE, MemTotal, MemFree, SwapFree, SwapTotal, COUNT  
	};
	
	std::string line;
	std::string delimiter = ";";
	InfoType type = InfoType::NONE;
	std::stringstream ss[(int)InfoType::COUNT];

	while (getline(f, line, '\n'))
	{
		if (line.find("MemTotal") != std::string::npos)
			type = InfoType::MemTotal;
		else if (line.find("MemFree") != std::string::npos)
			type = InfoType::MemFree;
		else if (line.find("SwapTotal") != std::string::npos)
			type = InfoType::SwapTotal;
		else if (line.find("SwapFree") != std::string::npos)
			type = InfoType::SwapFree;
		else
			continue;

		ss[(int)type] << line;
	}

	std::vector<std::string> memFreeVec = Split(ss[(int)InfoType::MemFree].str(), ':');
	std::vector<std::string> memTotalVec = Split(ss[(int)InfoType::MemTotal].str(), ':');
	std::vector<std::string> swapFreeVec = Split(ss[(int)InfoType::SwapFree].str(), ':');
	std::vector<std::string> swapTotalVec = Split(ss[(int)InfoType::SwapTotal].str(), ':');
	
	int freePhysical = stoi(memFreeVec[(memFreeVec.size() - 1)].substr(0, memFreeVec[(memFreeVec.size() - 1)].find(" "))) / 1024;
	meminfo.FreePhysical = std::to_string(freePhysical);
	int totalPhysical = stoi(memTotalVec[(memTotalVec.size() - 1)].substr(0, memTotalVec[(memTotalVec.size() - 1)].find(" "))) / 1024;
	meminfo.TotalPhysical = std::to_string(totalPhysical);
	int greeVirtual = stoi(swapFreeVec[(swapFreeVec.size() - 1)].substr(0, swapFreeVec[(swapFreeVec.size() - 1)].find(" "))) / 1024;
	meminfo.FreeVirtual = std::to_string(greeVirtual);
	int totalVirtual = stoi(swapTotalVec[(swapTotalVec.size() - 1)].substr(0, swapTotalVec[(swapTotalVec.size() - 1)].find(" "))) / 1024;
	meminfo.TotalVirtual = std::to_string(totalVirtual);

	int percentageFree = 0;
	if (totalPhysical > 0)
		percentageFree = round((double(freePhysical) / double(totalPhysical)) * 100);
	
	meminfo.PCTPhysicalAvailable = std::to_string(percentageFree);

}

void Collector::SetInfo(std::istream& f, CPUHistory& cpuHistory) const
{
	std::string line;
	std::string history;
	while (getline(f, line))
	{
		history += line;
	}

	cpuHistory.History = history;
}


bool Collector::CanReadFile(const std::string& filePath) const
{
	std::ifstream f(filePath);
	bool result = f.good();
	if (!result)
	{
		if (m_debug)
			std::cout << "ERROR: " << "unknown file path: " << filePath << std::endl;
	}

	return result;
}

