#ifndef __COMMON_H__
#define __COMMON_H__
#include <string>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include "message.h"
typedef unsigned short WORD;
typedef unsigned long DWORD;
#ifndef __MAIN__
extern log4cxx::LoggerPtr firebird_log;
extern std::string KDS_CODE_INFO;
#endif
#endif

