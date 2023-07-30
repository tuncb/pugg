Pugg Programming Documentation 2.0.0
===========
This documentation is intended for developers of Pugg. Explains rational for some of the design choices made during development of the code.

# Design

We call a function called register_pugg_pluggin from a DLL. => This registers all drivers.
A plugin is a DLL file. 
Do we need to register plugins? I guess to unload the DLL. 

A driver is a class that acts as a factory for another. 
It has a string name and an integer version. 


