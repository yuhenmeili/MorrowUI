//
// Created by 0060328 on 25-9-22.
//
#include "Log.h"
namespace morrow {
void Log::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentLevel_ = level;
}

bool Log::setFileOutput(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.open(filename, std::ios::app);
    return file_.is_open();
}

Log::~Log() {
    if (file_.is_open()) {
        file_.close();
    }
}
}
