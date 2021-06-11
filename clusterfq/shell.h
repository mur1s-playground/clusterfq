#pragma once

#ifdef _WIN32
#include "windows.h"
#endif

#include <vector>
#include <string>

#include "mutex.h"

void shell_loop(void *param);

extern struct mutex	shell_cmd_lock;
extern std::vector<std::vector<std::string>>	shell_cmd_queue;
