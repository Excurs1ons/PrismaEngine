//
// Created by JasonGu on 26-1-2.
//

#include "AndroidLogger.h"
#include "AndroidOut.h"

PrismaEngine::AndroidLogger::~AndroidLogger() {

}

void PrismaEngine::AndroidLogger::LogToConsole(LogLevel level, const char* tag, const char* message) {
    aout << message << std::endl;
}

const char* PrismaEngine::AndroidLogger::GetLogDirectoryPath() const {

    return "";
}