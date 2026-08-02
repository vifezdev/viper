#include <viper/minecraft/module_system.h>

namespace viper {
namespace minecraft {

void Module::setEnabled(bool enabled) {
    if (enabled_ != enabled) {
        enabled_ = enabled;
        if (enabled_) {
            onEnable();
        } else {
            onDisable();
        }
    }
}

void ModuleManager::registerModule(std::unique_ptr<Module> mod) {
    module_map_[mod->name()] = mod.get();
    modules_.push_back(std::move(mod));
}

Module* ModuleManager::getModule(const std::string& name) {
    auto it = module_map_.find(name);
    if (it != module_map_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<Module*> ModuleManager::getModules() {
    std::vector<Module*> result;
    result.reserve(modules_.size());
    for (const auto& mod : modules_) {
        result.push_back(mod.get());
    }
    return result;
}

std::vector<Module*> ModuleManager::getModulesByCategory(const std::string& category) {
    std::vector<Module*> result;
    for (const auto& mod : modules_) {
        if (mod->category() == category) {
            result.push_back(mod.get());
        }
    }
    return result;
}

void ModuleManager::tickAll() {
    for (const auto& mod : modules_) {
        if (mod->isEnabled()) {
            mod->onTick();
        }
    }
}

std::string ModuleManager::serializeState() {
    std::string state = "{";
    bool first = true;
    for (const auto& mod : modules_) {
        if (!first) state += ",";
        state += "\"" + mod->name() + "\":" + (mod->isEnabled() ? "true" : "false");
        first = false;
    }
    state += "}";
    return state;
}

void ModuleManager::deserializeState(const std::string& state) {
    
    
    for (const auto& mod : modules_) {
        std::string search = "\"" + mod->name() + "\":true";
        if (state.find(search) != std::string::npos) {
            mod->setEnabled(true);
        } else {
            mod->setEnabled(false);
        }
    }
}

} 
} 
