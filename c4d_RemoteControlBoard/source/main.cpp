// classic API header files
#include<_include_dll.hxx>
#include "c4d_plugin.h"
#include "c4d_resource.h"
#include "c4d_file.h"
#include <vector>
// project header files
#include "user_interface.h"
#include <cstdlib>
// If the classic API is used PluginStart(), PluginMessage() and PluginEnd() must be implemented.

::Bool PluginStart()
{
    std::string libprefix = std::getenv("YARP_DIR");
    libprefix += "/bin/Debug/";
    /*auto ret = maxon::DllInterface::LoadDll(maxon::Url(maxon::String("file:///C:/Program Files/robotology/ACE-6.5.0_v14_x86_amd64/bin/ACEd.dll")), true , false);
    if (ret == maxon::FAILED)
    {
        auto error = ret.GetError();
        error.DiagOutput();
    }*/

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
