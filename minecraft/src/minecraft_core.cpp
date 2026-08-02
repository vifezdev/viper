#include <viper/minecraft/minecraft_core.h>

namespace viper {
namespace minecraft {





jobject VanillaMinecraftCore::getMinecraftInstance(JNIEnv* env) {
    jclass mcClass = env->FindClass("ave");
    if (!mcClass) return nullptr;
    jmethodID getMcMethod = env->GetStaticMethodID(mcClass, "A", "()Lave;");
    if (!getMcMethod) return nullptr;
    return env->CallStaticObjectMethod(mcClass, getMcMethod);
}

jobject VanillaMinecraftCore::getPlayer(JNIEnv* env) {
    jobject mc = getMinecraftInstance(env);
    if (!mc) return nullptr;
    jclass mcClass = env->GetObjectClass(mc);
    jfieldID playerField = env->GetFieldID(mcClass, "h", "Lbew;");
    if (!playerField) return nullptr;
    return env->GetObjectField(mc, playerField);
}

jobject VanillaMinecraftCore::getWorld(JNIEnv* env) {
    jobject mc = getMinecraftInstance(env);
    if (!mc) return nullptr;
    jclass mcClass = env->GetObjectClass(mc);
    jfieldID worldField = env->GetFieldID(mcClass, "f", "Lbdb;");
    if (!worldField) return nullptr;
    return env->GetObjectField(mc, worldField);
}

int VanillaMinecraftCore::getDisplayWidth(JNIEnv* env) {
    jobject mc = getMinecraftInstance(env);
    if (!mc) return 0;
    jclass mcClass = env->GetObjectClass(mc);
    jfieldID widthField = env->GetFieldID(mcClass, "d", "I");
    if (!widthField) return 0;
    return env->GetIntField(mc, widthField);
}

int VanillaMinecraftCore::getDisplayHeight(JNIEnv* env) {
    jobject mc = getMinecraftInstance(env);
    if (!mc) return 0;
    jclass mcClass = env->GetObjectClass(mc);
    jfieldID heightField = env->GetFieldID(mcClass, "e", "I");
    if (!heightField) return 0;
    return env->GetIntField(mc, heightField);
}

bool VanillaMinecraftCore::isInGame(JNIEnv* env) {
    jobject player = getPlayer(env);
    jobject world = getWorld(env);
    return player != nullptr && world != nullptr;
}





jobject ForgeMinecraftCore::getMinecraftInstance(JNIEnv* env) {
    jclass mcClass = env->FindClass("net/minecraft/client/Minecraft");
    if (!mcClass) return nullptr;
    jmethodID getMcMethod = env->GetStaticMethodID(mcClass, "getMinecraft", "()Lnet/minecraft/client/Minecraft;");
    if (!getMcMethod) {
        env->ExceptionClear();
        getMcMethod = env->GetStaticMethodID(mcClass, "func_71410_x", "()Lnet/minecraft/client/Minecraft;");
    }
    if (!getMcMethod) return nullptr;
    return env->CallStaticObjectMethod(mcClass, getMcMethod);
}

jobject ForgeMinecraftCore::getPlayer(JNIEnv* env) {
    jobject mc = getMinecraftInstance(env);
    if (!mc) return nullptr;
    jclass mcClass = env->GetObjectClass(mc);
    jfieldID playerField = env->GetFieldID(mcClass, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
    if (!playerField) {
        env->ExceptionClear();
        playerField = env->GetFieldID(mcClass, "field_71439_g", "Lnet/minecraft/client/entity/EntityPlayerSP;");
    }
    if (!playerField) return nullptr;
    return env->GetObjectField(mc, playerField);
}

jobject ForgeMinecraftCore::getWorld(JNIEnv* env) {
    jobject mc = getMinecraftInstance(env);
    if (!mc) return nullptr;
    jclass mcClass = env->GetObjectClass(mc);
    jfieldID worldField = env->GetFieldID(mcClass, "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;");
    if (!worldField) {
        env->ExceptionClear();
        worldField = env->GetFieldID(mcClass, "field_71441_e", "Lnet/minecraft/client/multiplayer/WorldClient;");
    }
    if (!worldField) return nullptr;
    return env->GetObjectField(mc, worldField);
}

int ForgeMinecraftCore::getDisplayWidth(JNIEnv* env) {
    jobject mc = getMinecraftInstance(env);
    if (!mc) return 0;
    jclass mcClass = env->GetObjectClass(mc);
    jfieldID widthField = env->GetFieldID(mcClass, "displayWidth", "I");
    if (!widthField) {
        env->ExceptionClear();
        widthField = env->GetFieldID(mcClass, "field_71443_c", "I");
    }
    if (!widthField) return 0;
    return env->GetIntField(mc, widthField);
}

int ForgeMinecraftCore::getDisplayHeight(JNIEnv* env) {
    jobject mc = getMinecraftInstance(env);
    if (!mc) return 0;
    jclass mcClass = env->GetObjectClass(mc);
    jfieldID heightField = env->GetFieldID(mcClass, "displayHeight", "I");
    if (!heightField) {
        env->ExceptionClear();
        heightField = env->GetFieldID(mcClass, "field_71440_d", "I");
    }
    if (!heightField) return 0;
    return env->GetIntField(mc, heightField);
}

bool ForgeMinecraftCore::isInGame(JNIEnv* env) {
    jobject player = getPlayer(env);
    jobject world = getWorld(env);
    return player != nullptr && world != nullptr;
}





std::unique_ptr<MinecraftCore> createMinecraftCore(MinecraftLauncher launcher, MappingStyle style) {
    if (style == MappingStyle::MCP) {
        return std::make_unique<ForgeMinecraftCore>();
    } else {
        return std::make_unique<VanillaMinecraftCore>();
    }
}

} 
} 
