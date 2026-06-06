// CP Language — vm_opt stub for MSVC (computed-goto not supported)
// vm_opt.cpp uses GNU computed-goto extension, incompatible with MSVC.
// This file provides the symbols needed for MSVC builds.
// The actual vm_opt functionality is not available on MSVC;
// the main VM loop in vm_exec.cpp uses switch-case dispatch instead.

#include "vm/vm.hpp"

namespace cplang {
// All opt symbols are either inline (in vm_opt.hpp) or not needed for MSVC builds.
// This file exists as a compilation unit placeholder for the build system.
}
