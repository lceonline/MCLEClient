#pragma once
#include <string>

class Windows64Minecraft {
public:
    static bool IsOfflineMode();
    static std::string GetAuthenticationTicket();
    static bool IsExternalLauncher();

    struct Win64LaunchOptions
    {
        int screenMode;
        bool fullscreen;
        bool username;
        bool password;
        bool token;
    };
};
