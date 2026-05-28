#include "core/verbose.hpp"

namespace cplang {

static bool g_verbose = false;

bool verboseEnabled() {
    return g_verbose;
}

void setVerbose(bool v) {
    g_verbose = v;
}

} // namespace cplang
