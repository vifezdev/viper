#pragma once









#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace viper {
namespace minecraft {

class Module {
public:
    Module(std::string name, std::string desc, std::string category)
        : name_(std::move(name)), desc_(std::move(desc)), category_(std::move(category)) {}
    virtual ~Module() = default;

    const std::string& name() const { return name_; }
    const std::string& description() const { return desc_; }
    const std::string& category() const { return category_; }

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled);
    void toggle() { setEnabled(!enabled_); }

    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onTick() {}

private:
    std::string name_;
    std::string desc_;
    std::string category_;
    bool enabled_ = false;
};

class ModuleManager {
public:
    void registerModule(std::unique_ptr<Module> mod);
    Module* getModule(const std::string& name);
    std::vector<Module*> getModules();
    std::vector<Module*> getModulesByCategory(const std::string& category);
    
    void tickAll();
    
    std::string serializeState();
    void deserializeState(const std::string& state);

private:
    std::vector<std::unique_ptr<Module>> modules_;
    std::unordered_map<std::string, Module*> module_map_;
};

} 
} 
