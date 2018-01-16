//
// Created by Mike Mitterer on 10.03.17.
//
// cmake-Template für config.h
//

#ifndef XCTEST_CONFIG_H_IN_H
#define XCTEST_CONFIG_H_IN_H

#include <string>

// the configured options and settings for HelloWorld
const std::string WebSocket_VERSION_MAJOR = "0";
const std::string WebSocket_VERSION_MINOR = "1";

// Password compiled into source
// Set in .config/passwords.cmake (or via ENV-VAR)
const std::string WebSocket_PASSWORD_LS = "q9uPadfh#udshf";
const std::string WebSocket_PASSWORD_TPL = "8+uCXTsouf#auh";

#endif //XCTEST_CONFIG_H_IN_H
