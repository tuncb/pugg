#pragma once
#include <string>

namespace pugg
{

class Driver
{
public:
  Driver(std::string server_name, std::string name, int version)
      : _server_name(server_name), _name(name), _version(version)
  {
  }
  virtual ~Driver()
  {
  }

  std::string server_name() const
  {
    return _server_name;
  }
  std::string name() const
  {
    return _name;
  }
  int version() const
  {
    return _version;
  }

private:
  std::string _name;
  std::string _server_name;
  int _version;
};

} // namespace pugg
