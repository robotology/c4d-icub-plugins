# C4DPlugins
plugins library for integrating icub platform and the cg animation software Cinema4D. useful for scripting complex animation,
prototyping application and dataflow programming.
plugin developed:
- remotecontrolboard: connect with a part of the robot and exposes inside cinema4d some objects input (as many as the part joint count)
to be filled with the object whom rotation will be used to control the joint.
- sdf importer: very rough python script to import the kinematic and the visual model of a sdf inside cinema4d.

the following is an example of a realtime application, working with gazebo, that makes the eyes and 
the neck of icub following a virtual redball. only the redball is being animated/moved. all the neck and eyes movement are
implemented via the tools avaiable in cinema4d and sended to the robot via the remotecontrolboard plugin
![xpresso schema](icub_cinema.gif)

this is the xpresso schema that do the magic.
![xpresso schema](icub_cinema.png)
