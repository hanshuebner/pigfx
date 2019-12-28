// -*- C++ -*-

#pragma once

#include <cstdarg>

#include <string>

#include <circle/logger.h>

using namespace std;

class Logging
{
protected:
  Logging(const char* name) : _name(name) {}

  void log(TLogSeverity severity, const char* fmt, ...);

private:
  string _name;
};
