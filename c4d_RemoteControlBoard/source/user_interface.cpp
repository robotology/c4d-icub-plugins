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
#include <algorithm>

#include <yarp/os/Network.h>
#include <yarp/os/LogStream.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Vocab.h>
#include <yarp/dev/IPositionDirect.h>
#include <yarp/dev/IControlMode.h>
#include <yarp/dev/IAxisInfo.h>

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

    //yarp objects
    yarp::os::Network           yarpnet;
    yarp::dev::PolyDriver       pdr;

    //interfaces
    yarp::dev::IPositionDirect* pdir;
    yarp::dev::IControlMode*    cm;
    yarp::dev::IAxisInfo*       ai;
    
    //cinema objects
    BaseDocument*               doc;

    //others
    int                         axisCount{0};
    std::vector<std::string>    axisNames;
    std::vector<BaseObject*>    jObjects;
    std::vector<double>         jointData;

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
        doc                 = GetActiveDocument();
        if (!data)
            return false;

        return true;
    }

    virtual BaseObject* GetVirtualObjects(BaseObject* op, HierarchyHelp* hh) override
    {
        static BaseObject* ret = BaseObject::Alloc(Onull);

        if (axisCount)
        {
            
            jointData.resize(axisCount);
            for (int i = 0; i < axisCount; i++)
            {
                if (jObjects[i])
                {
                    jointData[i] = maxon::RadToDeg(jObjects[i]->GetRelRot().x);
                }
            }
            pdir->setPositions(jointData.data());
        }

        return ret;
    }

    virtual Bool GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC &flags) override
    {
        if (!description->LoadDescription(node->GetType())) 
            return false;

        GeData data;
        this->Get()->GetParameter(DescID(JOINT_COUNT), data, DESCFLAGS_GET::NONE);
        axisCount = data.GetInt32();
        auto currentcount = axisNames.size();
        if (currentcount < axisCount)
        {
            std::generate_n(std::back_inserter(axisNames), axisCount - axisNames.size(), [this]
            { 
                auto ret = std::string("joint") + std::to_string(axisNames.size());
                return ret;
            });
        }
        const DescID* singleid = description->GetSingleDescID();
        for (int i = 0; i < axisCount; i++)
        {
            DescID cid = DescLevel(JOINTS+i, DTYPE_BASELISTLINK, 0);

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

        jObjects.resize(axisCount, nullptr);

        for (int i = 0; i < axisCount; i++)
        {
            this->Get()->GetParameter(DescID(JOINTS + i), data, DESCFLAGS_GET::NONE);
            jObjects[i] = (BaseObject*)data.GetLink(doc);
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
            case CONFIGURE_BUTTON:
            {
                BaseContainer* data = ((BaseObject*)node)->GetDataInstance();
                auto bp = data->GetString(BODY_PART);
                return autoConfigure(bp.GetCStringCopy());
                break;
            }
            }
            break;
        }
        }

        return SUPER::Message(node, type, data);
    }

    bool autoConfigure(const std::string part)
    {
        if (openPolydrv(part))
            return false;

        pdir->getAxes(&axisCount);
        GeData data;
        data.SetInt32(axisCount);
        this->Get()->SetParameter(DescID(JOINT_COUNT), data, DESCFLAGS_SET::NONE);

        if(!setControlMode(VOCAB_CM_POSITION_DIRECT) || !setAxisNames()) return closeDevice();
        return true;
    }

    static NodeData* Alloc() { return NewObjClear(C4DRemoteControlBoard); }

    inline bool openPolydrv(const std::string part)
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

        return true;
    }

    inline bool setAxisNames()
    {
        if (!ai)
            return false;
        axisNames.resize(axisCount);
        int aid = 0;
        for (auto& i : axisNames)
        {
            if(!ai->getAxisName(aid, i))
                return false;
            aid++;
        }

        updateGui();
        return true;
    }

    inline bool setControlMode(int cmode)
    {
        if (!cm || !axisCount)
            return false;
        std::vector<int> cmvec(axisCount);
        std::fill(cmvec.begin(), cmvec.end(), cmode);
        if(!cm->setControlModes(cmvec.data()))
            return false;
        
        jointData.resize(axisCount);
        if (!pdir->getRefPositions(jointData.data()))
            return false;
        
        return true;
    }

    Bool openDevice(const std::string part)
    {
        if (openPolydrv(part))
            return false;

        int actualaxiscount;
        pdir->getAxes(&actualaxiscount);
        
        if (actualaxiscount != axisCount)
        {
            pdr.close();
            return false;
        }

        if(!setControlMode(VOCAB_CM_POSITION_DIRECT) || !setAxisNames())
            return closeDevice();
        
        return true;
    }

    void resetInterfaces()
    {
        cm   = nullptr;
        ai   = nullptr;
        pdir = nullptr;
    }
    Bool closeDevice()
    {
        resetInterfaces();
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
