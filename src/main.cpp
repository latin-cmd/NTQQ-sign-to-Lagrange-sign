#include "server.h"

#include "../include/rapidjson/document.h"

#include <filesystem>
#include <fstream>
#include <iostream>

void init()
{
#if defined(_WIN_PLATFORM_)
    std::string version = "9.9.20-37051";
    try
    {
        TCHAR pathm[MAX_PATH];
        GetModuleFileName(NULL, pathm, MAX_PATH);
        std::filesystem::path path = pathm;
        path = path.parent_path().append("resources\\app\\versions\\config.json");
        std::ifstream versionConfig(path.wstring());
        if (versionConfig.is_open())
        {
            std::stringstream versionConfigStream;
            versionConfigStream << versionConfig.rdbuf();
            versionConfig.close();
            rapidjson::Document doc;
            doc.Parse(versionConfigStream.str().c_str(), versionConfigStream.str().size());
            if (doc.HasMember("curVersion") && doc["curVersion"].IsString())
                version = doc["curVersion"].GetString();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
#elif defined(_MAC_PLATFORM_)
    std::string version = "6.9.19-16183";
#elif defined(_LINUX_PLATFORM_)
    std::string version = "3.2.9-24815";
    try
    {
        std::ifstream versionConfig("/opt/QQ/resources/app/package.json");
        if (versionConfig.is_open())
        {
            std::stringstream versionConfigStrBuf;
            versionConfigStrBuf << versionConfig.rdbuf();
            versionConfig.close();
            rapidjson::Document doc;
            doc.Parse(versionConfigStrBuf.str().c_str(), versionConfigStrBuf.str().size());
            if (doc.HasMember("version") && doc["version"].IsString())
                version = doc["version"].GetString();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
#endif
    std::string ip = "0.0.0.0";
    int port = 8080;

    std::string default_config = R"({"ip":"0.0.0.0","port":8080})";

    rapidjson::Document doc;

    std::ifstream configFile("sign.json");
    if (!configFile.is_open())
    {
        printf("sign.json not found, use default\n");
        std::ofstream("sign.json") << default_config;
        doc.Parse(default_config.c_str(), default_config.size());
    }
    else
    {
        std::stringstream configStream;
        configStream << configFile.rdbuf();
        configFile.close();
        try
        {
            doc.Parse(configStream.str().c_str(), configStream.str().size());
        }
        catch (const std::exception &e)
        {
            printf("Parse config failed, use default: %s\n", e.what());
            doc.Parse(default_config.c_str(), default_config.size());
        }
    }

    if (doc.HasMember("ip") && doc["ip"].IsString())
        ip = doc["ip"].GetString();
    if (doc.HasMember("port") && doc["port"].IsInt())
        port = doc["port"].GetInt();
    if (doc.HasMember("version") && doc["version"].IsString())
        version = doc["version"].GetString();

    std::thread sign_init([version, ip, port]
                          {
        printf("Start Init sign\n");
        printf("Using version: %s\n", version.c_str());
        printf("Server will listen on %s:%d\n", ip.c_str(), port);
        for (int i = 0; i < 10; i++)
        {
            try
            {
                printf("Attempt %d/10 to initialize...\n", i + 1);
                if (Sign::Init(version))
                {
                    printf("Sign initialized successfully!\n");
                    printf("Start Init server\n");
                    Server server;
                    server.Init();
                    printf("Server initialized, starting to listen...\n");
                    if (!server.Run(ip, port))
                        printf("Server run failed\n");
                    else
                        printf("Server running on %s:%d\n", ip.c_str(), port);
                    return;
                }
                else
                {
                    printf("Sign::Init returned false\n");
                }
            }
            catch (const std::exception &e)
            {
                printf("Init failed: %s\n", e.what());
            }
            catch (...)
            {
                printf("Init failed: Unknown error\n");
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        printf("Failed to initialize after 10 attempts\n"); });
    sign_init.detach();
}

void uninit()
{
}

#if defined(_WIN_PLATFORM_)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        init();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        uninit();
        break;
    }
    return TRUE;
}
#elif defined(_LINUX_PLATFORM_) || defined(_MAC_PLATFORM_)
void __attribute__((constructor)) my_init(void)
{
    init();
}

void __attribute__((destructor)) my_fini(void)
{
    uninit();
}
#endif