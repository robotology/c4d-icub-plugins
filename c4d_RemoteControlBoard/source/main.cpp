// classic API header files
#include<_include_dll.hxx>
#include "c4d_plugin.h"
#include "c4d_resource.h"
#include "c4d_file.h"
#include <vector>
// project header files
#include "user_interface.h"
#include <cstdlib>
#include <vector>
using namespace std;
// If the classic API is used PluginStart(), PluginMessage() and PluginEnd() must be implemented.

const vector<string> explode(const string& s, const char& c)
{
    string buff{ "" };
    vector<string> v;

    for (auto n : s)
    {
        if (n != c) buff += n; else
            if (n == c && buff != "") { v.push_back(buff); buff = ""; }
    }
    if (buff != "") v.push_back(buff);

    return v;
}

::Bool PluginStart()
{
    std::string libprefix = std::getenv("YARP_DIR");
    libprefix += "/bin/Debug/";
    std::string path = std::getenv("Path");
    for (const auto& s : explode(path, ';'))
    {
        auto ret = maxon::DllInterface::LoadDll(maxon::Url(maxon::String((s+"/ACEd.dll").c_str())), true, false);
        if (ret == maxon::OK)
        {
            DiagnosticOutput("ACE library loaded!");
        }
    }
    

    std::vector<std::string> dlls = { "YARP_OSd.dll", "YARP_initd.dll", "YARP_sigd.dll", "YARP_mathd.dll", "YARP_devd.dll"};
    for (auto d : dlls)
    {
        std::string s = libprefix + d;
        auto url = maxon::Url(maxon::String(s.data()));
        auto ret = maxon::DllInterface::LoadDll(url, true, false);
        if (ret == maxon::FAILED)
        {
            auto error = ret.GetError();
            error.DiagOutput();
        }
    }
    

    // register classic API plugins
    yarpC4D::RegisterRemoteControlBoard();

    return true;
}

void PluginEnd()
{
	// free resources
}

::Bool PluginMessage(::Int32 id, void* data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
		{
			// load resources defined in the the optional "res" folder
            auto a = Filename(GeGetPluginPath());
			if (!g_resource.Init())
				return false;

			return true;
		}
		case C4DPL_PROGRAM_STARTED:
		{
			// perform some action after the program has started
			break;
		}
	}

	return true;
}
