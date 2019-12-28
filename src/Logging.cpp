
#include "Logging.h"

extern "C" void
LogWrite(const char* source, unsigned severity, const char* fmt, ...)
{
  CLogger* logger = CLogger::Get();
  if (logger) {
    va_list vl;
    va_start(vl, fmt);
    logger->WriteV(source, (TLogSeverity) severity, fmt, vl);
    va_end(vl);
  }
}

void
Logging::log(TLogSeverity severity, const char* fmt, ...)
{
  CLogger* logger = CLogger::Get();
  if (logger) {
    va_list vl;
    va_start(vl, fmt);
    logger->WriteV(_name.c_str(), (TLogSeverity) severity, fmt, vl);
    va_end(vl);
  }
}

