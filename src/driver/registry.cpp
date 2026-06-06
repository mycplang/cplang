// ═══════════════════════════════════════════════════════════════════
//  DriverRegistry 实现
// ═══════════════════════════════════════════════════════════════════
#include "driver/driver.hpp"

namespace cplang {

DriverRegistry& DriverRegistry::instance() {
    static DriverRegistry reg;
    return reg;
}

bool DriverRegistry::registerDriver(const char* name, IDatabaseDriver* driver) {
    if (!name || !driver) return false;
    std::string key(name);
    if (drivers_.count(key)) return false;  // 已存在
    drivers_[key] = {driver, false};
    return true;
}

IDatabaseDriver* DriverRegistry::getDriver(const char* name) {
    if (!name) return nullptr;
    auto it = drivers_.find(std::string(name));
    return it != drivers_.end() ? it->second.driver : nullptr;
}

std::vector<const char*> DriverRegistry::listDrivers() {
    std::vector<const char*> result;
    for (auto& [key, entry] : drivers_) {
        result.push_back(entry.driver->name());
    }
    return result;
}

void DriverRegistry::setOwnership(const char* name, bool owned) {
    if (!name) return;
    auto it = drivers_.find(std::string(name));
    if (it != drivers_.end()) it->second.owned = owned;
}

DriverRegistry::~DriverRegistry() {
    for (auto& [key, entry] : drivers_) {
        if (entry.owned && entry.driver) {
            delete entry.driver;
            entry.driver = nullptr;
        }
    }
}

} // namespace cplang
