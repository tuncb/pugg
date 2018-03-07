Pugg Programming Documentation 1.0.0
===========
This documentation is intended for developers of Pugg. Explains rational for some of the design choices made during development of the code.

Code Style
----
~~~~cpp
namespace pugg {

class ExampleClass
{
public:
    void public_function();
    int public_variable;
private:
    void _private_function();
    int _private_variable;    
};
}
~~~~

Rationale
--------
**Copying of strings in Kernel class:** Copying of Driver and Server names obviously decrease performance, however I don't think this will be a major hit for most projects.

**Omitting C++11 features:** I didn't want to exclude any compiler version.

**Error Handling and function returns:** Functions that could not add or get Drivers can fail due to many reasons. I didn't want to handle them with exceptions because these failures are commonly ignored. Exceptions for some conditions that should not occur and some information about failures for ignored cases should be added later.

**std::wstring feature and unicode filename support** Unicode is problematic in C++ and old version of handling strings with templates complicated to code. Could be added later.

Notes
-----
***register_pugg_plugin* function:** Function name was changed in order to prevent any name collusions with other exported functions from dlls.
