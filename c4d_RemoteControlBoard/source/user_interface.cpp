// project header files
#include "user_interface.h"
#include "customgui_linkbox.h"
// classic API header files
#include "c4d_objectplugin.h"
#include "c4d_gui.h"
#include "c4d_general.h"
#include "c4d_basedocument.h"
#include "c4d_resource.h"
#include "lib_description.h"
#include "obase.h"
#include "c4d_customdatatypeplugin.h"
#include "../res/description/oremotecontrolboard.h"

#include <yarp/os/Network.h>
#include <yarp/os/LogStream.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Vocab.h>
#include <yarp/dev/IPositionDirect.h>
#include <yarp/dev/IControlMode.h>
#include <yarp/dev/IAxisInfo.h>

using namespace maxon;
namespace yarpC4D
{
//-------------------------------------------------------------------------------------------
/// A simple command that will be displayed in the GUI.
/// Used to call MakeCube() and insert the created object into the active document.
//-------------------------------------------------------------------------------------------
class C4DRemoteControlBoard : public ObjectData
{
	INSTANCEOF(C4DRemoteControlBoard, ObjectData)

public:

    yarp::os::Network yarpnet;

    yarp::dev::PolyDriver       pdr;
    yarp::dev::IPositionDirect* pdir;
    yarp::dev::IControlMode*    cm;
    yarp::dev::IAxisInfo*       ai;

    int axisCount{0};
    std::vector<std::string> axisNames;

    virtual Bool GetDEnabling(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) override
    {
        if(id[0].id == CONNECT_BUTTON)
        {
            if (pdr.isValid())
            {
                return false;
            }
        }

        if (id[0].id == DISCONNECT_BUTTON)
        {
            if (!pdr.isValid())
            {
                return false;
            }
        }
        return true;
    }

    virtual Bool Init(GeListNode* node) override
    {
        BaseObject*    op   = (BaseObject*)node;
        BaseContainer* data = op->GetDataInstance();
        if (!data)
            return false;

        return true;
    }

    virtual BaseObject* GetVirtualObjects(BaseObject* op, HierarchyHelp* hh) override
    {
        static BaseObject* ret = BaseObject::Alloc(Onull);

        if (axisCount)
        {
            BaseDocument* doc = GetActiveDocument();
            static std::vector<double> jointdata;
            jointdata.resize(axisCount);
            for (int i = 0; i < axisCount; i++)
            {
                GeData data;
                const Bool success = this->Get()->GetParameter(DescID(1003+i), data, DESCFLAGS_GET::NONE);

                if (success && data.GetType() == DA_ALIASLINK)
                {
                    BaseObject* jnt = (BaseObject*)data.GetLink(doc);

                    if (jnt)
                    {
                        jointdata[i] = maxon::RadToDeg(jnt->GetRelRot().x);
                    }
                }
                
            }
            pdir->setPositions(jointdata.data());
        }

        return ret;
    }

    virtual Bool GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC &flags) override
    {
        if (!description->LoadDescription(node->GetType())) 
            return false;

        GeData data;
        this->Get()->GetParameter(DescID(BODY_PART), data, DESCFLAGS_GET::NONE);

        const DescID* singleid = description->GetSingleDescID();
        for (int i = 0; i < axisCount; i++)
        {
            DescID cid = DescLevel(1003+i, DTYPE_BASELISTLINK, 0);

            if (!singleid || cid.IsPartOf(*singleid, nullptr))
            {
                BaseContainer settings = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);
                settings.SetString(DESC_NAME, maxon::String(axisNames[i].c_str()));
                settings.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_LINKBOX);

                BaseContainer ac;
                ac.SetInt32(Obase, 1);
                settings.SetContainer(DESC_ACCEPT, ac);

                description->SetParameter(cid, settings, ID_OBJECTPROPERTIES);


            }
        }

        flags |= DESCFLAGS_DESC::LOADED;
        return SUPER::GetDDescription(node, description, flags);

    }

    virtual Bool Message(GeListNode *node, Int32 type, void *data) override
    {
        switch (type)
        {
        case MSG_DESCRIPTION_COMMAND:
        {
            DescriptionCommand* dc = (DescriptionCommand*)data;

            const Int32 id = dc->_descId[0].id;

            switch (id)
            {
            case CONNECT_BUTTON:
            {
                BaseContainer* data = ((BaseObject*)node)->GetDataInstance();
                auto bp = data->GetString(BODY_PART);
                return openDevice(bp.GetCStringCopy());
                break;
            }
            case DISCONNECT_BUTTON:
            {
                return closeDevice();
                break;
            }
            }
            break;
        }
        }

        return SUPER::Message(node, type, data);
    }

    static NodeData* Alloc() { return NewObjClear(C4DRemoteControlBoard); }

    Bool openDevice(const std::string part)
    {
        yarp::os::Property conf;
        conf.put("device", "remote_controlboard");
        conf.put("remote", part);
        conf.put("local", "/c4dremotecb");
        conf.put("carrier", "tcp");
        if (!pdr.open(conf))
        {
            DiagnosticOutput(("unable to connect to " + part).c_str());
            return false;
        }
            
        if (!pdr.view(cm))
        {
            DiagnosticOutput("ControlMode view failed");
            return false;
        }

        if (!pdr.view(pdir))
        {
            DiagnosticOutput("PositionDirect view failed");
            return false;
        }

        if (!pdr.view(ai))
        {
            DiagnosticOutput("AxisInfo view failed");
            return false;
        }
        pdir->getAxes(&axisCount);
        axisNames.resize(axisCount);
        int aid = 0;
        std::vector<int> cmvec(axisCount);
        std::fill(cmvec.begin(), cmvec.end(), VOCAB_CM_POSITION_DIRECT);
        cm->setControlModes(cmvec.data());
        for (auto& i : axisNames)
        {
            ai->getAxisName(aid, i);
            aid++;
        }
        
        updateGui();
        return true;
    }

    Bool closeDevice()
    {
        axisCount = 0;
        pdr.close();
        updateGui();
        return true;
    }

    void updateGui()
    {
        SendCoreMessage(COREMSG_CINEMA, BaseContainer(COREMSG_CINEMA_FORCE_AM_UPDATE), 0);
    }
};

void RegisterRemoteControlBoard()
{
	// plugin IDs must be obtained from plugincafe.com to avoid collisions
	const Int32 pluginID = 1041028;
	const Bool	success	 = RegisterObjectPlugin(pluginID, "RemoteControlBoard"_s, OBJECT_GENERATOR | OBJECT_INPUT, C4DRemoteControlBoard::Alloc, "Oremotecontrolboard"_s, nullptr, 0);

	if (!success)
	{
		DiagnosticOutput("Could not register plugin.");
	}
}
}
