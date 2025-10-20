#if defined(_WIN_PLATFORM_)
#include "run_as_node.h"

#include <map>
#include <vector>
#include <codecvt>
#include <algorithm>

#if defined(_X64_ARCH_) // {call winmain, check run as node function}
std::map<std::string, std::tuple<uint64_t, uint64_t, uint64_t>> mainAddrMap = {
    {"9.9.12-25234", {0x457A76D, 0x3A5D70, 0x1FFF710}}};
#endif

int(__stdcall *oriWinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);

void(__fastcall *checkRunAsNode)(void *a1);

std::shared_ptr<void> (*nodeInitializeOncePerProcess)(
    const std::vector<std::string> &args,
    uint32_t flags);

int __stdcall fakeWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    checkRunAsNode(nullptr);
    return oriWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}

bool RunAsNode::RunNode()
{
    struct Arguments
    {
        int argc = 0;
        wchar_t **argv =
            ::CommandLineToArgvW(::GetCommandLineW(), &argc);

        ~Arguments() { LocalFree(argv); }
    } arguments;

    std::vector<std::string> argv(arguments.argc);

    std::transform(arguments.argv, arguments.argv + arguments.argc, argv.begin(),
                   [](auto &a)
                   { return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(a); });

    nodeInitializeOncePerProcess(argv, (1 << 6) | (1 << 7));
    return true;
}

bool RunAsNode::Init(const std::string &version)
{
    uint64_t baseAddr = 0;
#if defined(_WIN_PLATFORM_)
    HMODULE wrapperModule = GetModuleHandleW(NULL);
    if (wrapperModule == NULL)
        throw std::runtime_error("Can't GetModuleHandle");
    baseAddr = reinterpret_cast<uint64_t>(wrapperModule);
    printf("baseAddr: %llx\n", baseAddr);
#elif defined(_MAC_PLATFORM_)
    auto pmap = hak::get_maps();
    do
    {
        if (pmap->module_name.find("QQ") != std::string::npos && pmap->offset == 0)
        {
            baseAddr = pmap->start();
            printf("baseAddr: %llx\n", baseAddr);
            break;
        }
    } while ((pmap = pmap->next()) != nullptr);
#elif defined(_LINUX_PLATFORM_)
    auto pmap = hak::get_maps();
    do
    {
        if (pmap->module_name.find("QQ") != std::string::npos && pmap->offset == 0)
        {
            baseAddr = pmap->start();
            printf("baseAddr: %lx\n", baseAddr);
            break;
        }
    } while ((pmap = pmap->next()) != nullptr);
#endif
    if (baseAddr == 0)
        throw std::runtime_error("Can't find hook address");

    auto [callptr, func1ptr, func2ptr] = mainAddrMap[version];

    uint8_t *abscallptr = reinterpret_cast<uint8_t *>(baseAddr + callptr);
    oriWinMain = reinterpret_cast<int(__stdcall *)(HINSTANCE, HINSTANCE, LPSTR, int)>(moehoo::get_call_address(abscallptr));
    checkRunAsNode = reinterpret_cast<void(__fastcall *)(void *)>(baseAddr + func1ptr);
    nodeInitializeOncePerProcess = reinterpret_cast<std::shared_ptr<void> (*)(const std::vector<std::string> &, uint32_t)>(baseAddr + func2ptr);
    return moehoo::hook(abscallptr, &fakeWinMain);
}

#endif