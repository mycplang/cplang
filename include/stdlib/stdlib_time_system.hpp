#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

namespace time {
    Value now(std::vector<Value>& args);
    Value date(std::vector<Value>& args);
    Value format(std::vector<Value>& args);
    Value parse(std::vector<Value>& args);
    Value sleep(std::vector<Value>& args);
    Value tick(std::vector<Value>& args);
}

namespace time_more {
    Value timeFormat(std::vector<Value>& args);
    Value timeParse(std::vector<Value>& args);
    Value timeSleep(std::vector<Value>& args);
    Value timeDiff(std::vector<Value>& args);
    Value timeAdd(std::vector<Value>& args);
    Value timeNowMs(std::vector<Value>& args);
    Value timeTimerStart(std::vector<Value>& args);
    Value timeTimerElapsed(std::vector<Value>& args);
}

namespace system {
    Value exit(std::vector<Value>& args);
    Value getEnv(std::vector<Value>& args);
    Value setEnv(std::vector<Value>& args);
    Value exec(std::vector<Value>& args);
    Value spawn(std::vector<Value>& args);
    Value platform(std::vector<Value>& args);
    Value arch(std::vector<Value>& args);
    Value pid(std::vector<Value>& args);
    Value cwd(std::vector<Value>& args);
    Value chdir(std::vector<Value>& args);
}

namespace sys_more {
    Value sysGetEnv(std::vector<Value>& args);
    Value sysSetEnv(std::vector<Value>& args);
    Value sysExec(std::vector<Value>& args);
    Value sysShell(std::vector<Value>& args);
    Value cpuCount(std::vector<Value>& args);
    Value osVersion(std::vector<Value>& args);
}

namespace proc {
    Value procId(std::vector<Value>& args);
    Value procParentId(std::vector<Value>& args);
    Value procName(std::vector<Value>& args);
    Value procArgs(std::vector<Value>& args);
}

} // namespace cplang
