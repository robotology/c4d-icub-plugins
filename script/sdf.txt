import c4d
from c4d import *
from c4d.utils import *
import copy
import xml.etree.ElementTree as ET
rootp = "C:/software/icub-gazebo/"
root = ET.parse(rootp+'icub_with_hands/icub_with_hands.sdf').getroot()[0]

def getwcoord(o):
    return o.GetMg().off

def setwcoord(o, v):
    o.SetRelPos(v * ~o.GetUpMg())
    return getwcoord(o)

def GetTag(taglist, root):
    if len(taglist) == 0:
        return root
    else:
        r = [i for i in root if i.tag == taglist[0]]
        taglist.pop(0)

    if len(r):
        return GetTag(taglist, r[0])
    else:
        return None

def get_all_objects(op, output):
    while op:
        output.append(op)
        get_all_objects(op.GetDown(), output)
        op = op.GetNext()
    return output

def resetpivot(op):
    if not op.CheckType(c4d.Opolygon):
        return
    v = op.GetRelPos()
    op.SetRelPos(Vector(0,0,0))
    for p in xrange(op.GetPointCount()):
        op.SetPoint(p, op.GetPoint(p) + v)
    op.Message (c4d.MSG_UPDATE)

def axisrot(v, op):
    op.SetMl(MatrixRotX(v.x)*MatrixRotX(v.y)*MatrixRotX(v.z))

def switchZY(o):
    #v = o.GetRelPos()
    #c = -v.z
    #v.z = v.y
    #v.y = c
    #o.SetRelPos(v)
    #v = o.GetRelRot()
    #v.z = v.z + 3.141/2
    #o.SetRelRot(v)
    #for ob in o.GetChildren():
    #    switchZY(ob)
    if o.GetType() == c4d.Opolygon:
        for p in xrange(o.GetPointCount()):
            v   = o.GetPoint(p)
            v.z = -v.z
            o.SetPoint(p, v)
        o.Message (c4d.MSG_UPDATE)

def applyrot(o, p):
    o.SetMg(MatrixRotX(p.x))
    o.SetMg(MatrixRotY(p.y) * o.GetMg())
    o.SetMg(MatrixRotZ(p.z) * o.GetMg())

def applyrotl(o, p):
    o.SetMl(MatrixRotX(p.x))
    o.SetMl(MatrixRotY(p.y) * o.GetMl())
    o.SetMl(MatrixRotZ(p.z) * o.GetMl())

def openlink(i):
    global rootp
    global root
    posevectorP = c4d.Vector()
    posevectorR = c4d.Vector()
    visualRvector = c4d.Vector()
    visualPvector = c4d.Vector()
    uri = GetTag(["visual","geometry","mesh","uri"], i)
    pose = GetTag(["pose",], i)
    visualpose = GetTag(["visual", "pose",], i)
    if pose != None:
        ps = pose.text.split()
        posevectorP = c4d.Vector(float(ps[0])*100, float(ps[1])*100, -float(ps[2])*100)
        posevectorR = c4d.Vector(float(ps[3]),     float(ps[4]),     -float(ps[5]))
    if visualpose != None:
        ps2 = visualpose.text.split()
        visualPvector = c4d.Vector(float(ps2[0])*100, float(ps2[1])*100, -float(ps2[2])*100)
        visualRvector = c4d.Vector(float(ps2[3]),     float(ps2[4]),     -float(ps2[5]))
    nulobj = c4d.BaseObject(Onull)
    jobj = c4d.BaseObject(Ojoint)
    nulobj.SetName(i.attrib["name"])
    v = posevectorR
    applyrot(nulobj, posevectorR)
    nulobj.SetAbsPos(posevectorP)
    jobj.InsertUnder(nulobj)

    if uri != None:
        dae =  rootp+uri.text[7:]
        objs0 = doc.GetObjects()
        c4d.documents.MergeDocument(doc, dae, c4d.SCENEFILTER_OBJECTS)
        objs = [i for i in doc.GetObjects() if i not in objs0]
        doc.InsertObject(nulobj)
        for o in objs:
            o.InsertUnder(jobj)
            applyrotl(o, visualRvector)
            o.SetRelPos(visualPvector)

        for o in get_all_objects(jobj, []):

            if o.GetName() == "Lamp" or o.GetName() == "Camera":
                o.Remove()


    else:
        doc.InsertObject(nulobj)

    return nulobj



# Main function
def main():
    #switchZY(doc.GetObjects()[0])
    #return
    links = {}
    for i in root:
        if i.tag == "link":
            links[i.attrib["name"]] = i
    joints = [i for i in root if i.tag == "joint"]
    l = {}
    tagged = []
    for i in joints:
        axis       = GetTag(["axis", "xyz"], i).text.split()
        parentname = GetTag(["parent"], i).text
        if parentname in l:
            #l[parentname][1] = i
            parent = l[parentname][0]

        else:
            parent = [n for n in openlink(links[parentname]).GetChildren() if n.GetType() == c4d.Ojoint][0]
            l[parentname] = [parent, None, parentname]

        parent.SetName(parentname)

        child = openlink(links[GetTag(["child"], i).text])
        mg    = child.GetMg()

        child.InsertUnder(parent)
        child.SetMg(mg)
        child = [n for n in child.GetChildren() if n.GetType() == c4d.Ojoint][0]
        l[GetTag(["child"], i).text] = [child, i, i.attrib["name"]]

    for o in l:
        parent = l[o][0]
        i      = l[o][1]
        if i == None:
            continue

        lowlim = float(GetTag(["axis", "limit", "lower"], i).text)
        hilim  = float(GetTag(["axis", "limit", "upper"], i).text)
        rot    = parent.GetRelRot()
        t      = parent.MakeTag(c4d.Tprotection)
        t2     = parent.GetUp().MakeTag(c4d.Tprotection)


        t[c4d.PROTECTION_P] = c4d.PROTECTION_LOCK
        t2[c4d.PROTECTION_P] = c4d.PROTECTION_LOCK
        t2[c4d.PROTECTION_R] = c4d.PROTECTION_LOCK

        t.SetParameter(PROTECTION_R_MIN_Y, rot.y, 0)
        t.SetParameter(PROTECTION_R_MAX_Y, rot.y, 0)
        t.SetParameter(PROTECTION_R_MIN_Z, rot.z, 0)
        t.SetParameter(PROTECTION_R_MAX_Z, rot.z, 0)
        print rot.x, lowlim, hilim, l[o][2]
        t.SetParameter(PROTECTION_R_MIN_X, lowlim, 0)
        t.SetParameter(PROTECTION_R_MAX_X, hilim, 0)
        t[c4d.PROTECTION_R] = c4d.PROTECTION_LIMIT


    allob = get_all_objects(doc.GetObjects()[0], [])
    m = [[i, i.GetMg()] for i in allob if i.GetType() != c4d.Ojoint]
    n = [i for i in allob if i.GetType() == c4d.Ojoint]


    #for o in l:
        #l[o].append(getwcoord(l[o][0]))
        #l[o].append(l[o][0].GetUpMg())

    #for jnt in n:
    #    o = jnt.GetName()
    #    j = l[o][0]
    #    ax = l[o][1]
    #    if ax:
    #        rotvec = Vector(float(ax[0]),float(ax[1]),-float(ax[2]))* ~l[o][-1]
    #        if rotvec.GetLength() < 0.001:
    #            rotvec = Vector(0,0,1) * ~l[o][-1]
    #        rotvec = VectorToHPB(rotvec)
    #        j.SetMg(HPBToMatrix(rotvec))

    #lwp = None
    #for jnt in n:
        #coord = l[jnt.GetName()][-2]
        #setwcoord(jnt, coord)

    for o in l:
        l[o][0].SetName(l[o][2])
        l[o][0].GetUp().SetName(l[o][2]+"null")

    for i in m:
        i[0].SetMg(i[1])



    c4d.EventAdd(c4d.MSG_UPDATE)



# Execute main()
if __name__=='__main__':
    main()