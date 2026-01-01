//
// Created by JasonGu on 26-1-2.
//

#ifndef ANDROIDLOGGER_H
#define ANDROIDLOGGER_H
#include "IPlatformLogger.h"
namespace PrismaEngine {
class AndroidLogger: public PrismaEngine::IPlatformLogger {
public:
    ~AndroidLogger() override;
    void LogToConsole(LogLevel level, const char* tag, const char* message) override;
    const char* GetLogDirectoryPath() const override;
};
}

#endif //ANDROIDLOGGER_H
