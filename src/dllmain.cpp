#include "stdafx.h"
#include "helper.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <inipp/inipp.h>
#include <safetyhook.hpp>

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule;

// Version
std::string sFixName = "SotDFix";
std::string sFixVer = "0.0.3";
std::string sLogFile = sFixName + ".log";
std::filesystem::path sThisModulePath;

// Ini
inipp::Ini<char> ini;
std::string sConfigFile = sFixName + ".ini";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::filesystem::path sExePath;
std::string sExeName;

// Aspect ratio / FOV related
std::pair DesktopDimensions = { 0,0 };
const float fPi = 3.1415926535f;
const float fNativeAspect = 1.777777791f;
float fAspectRatio;
float fAspectMultiplier;
float fHUDWidth;
float fHUDWidthOffset;
float fHUDHeight;
float fHUDHeightOffset;

// Ini variables
bool bUncapFPS;
bool bFixResolution;
bool bFixAspect;
bool bFixFOV;
float fAdditionalFOV;
bool bFixHUD;
bool bLODDistance;

// Variables
int iCurrentResX;
int iCurrentResY;
int iOldResX;
int iOldResY;
bool bIsMoviePlaying = false;

void Logging()
{
    // Get this module path
    WCHAR thisModulePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(thisModule, thisModulePath, MAX_PATH);
    sThisModulePath = thisModulePath;
    sThisModulePath = sThisModulePath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // spdlog initialisation
    {
        try {
            logger = spdlog::basic_logger_st(sFixName.c_str(), sExePath.string() + sLogFile, true);
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::info("----------");
            spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
            spdlog::info("----------");
            spdlog::info("Log file: {}", sThisModulePath.string() + sLogFile);
            spdlog::info("----------");
            spdlog::info("Module Name: {0:s}", sExeName.c_str());
            spdlog::info("Module Path: {0:s}", sExePath.string());
            spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
            spdlog::info("Module Timestamp: {0:d}", Memory::ModuleTimestamp(baseModule));
            spdlog::info("----------");
        }
        catch (const spdlog::spdlog_ex& ex) {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
            FreeLibraryAndExitThread(baseModule, 1);
            return;
        }
    }
}

void Configuration()
{
    // inipp initialisation
    std::ifstream iniFile(sThisModulePath.string() + sConfigFile);
    if (!iniFile) {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVer.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sThisModulePath.string().c_str() << std::endl;
        FreeLibraryAndExitThread(baseModule, 1);
        return;
    }
    else {
        spdlog::info("Config file: {}", sThisModulePath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Parse config
    ini.strip_trailing_comments();
    spdlog::info("----------");

    inipp::get_value(ini.sections["Uncap Framerate"], "Enabled", bUncapFPS);
    spdlog::info("Config Parse: bUncapFPS: {}", bUncapFPS);

    inipp::get_value(ini.sections["Gameplay FOV"], "AdditionalFOV", fAdditionalFOV);
    if (fAdditionalFOV < -80.00f || fAdditionalFOV > 80.00f) {
        fAdditionalFOV = std::clamp(fAdditionalFOV, -80.00f, 80.00f);
        spdlog::warn("Config Parse: fAdditionalFOV value invalid, clamped to {}", fAdditionalFOV);
    }
    spdlog::info("Config Parse: fAdditionalFOV: {}", fAdditionalFOV);

    inipp::get_value(ini.sections["Fix Resolution"], "Enabled", bFixResolution);
    spdlog::info("Config Parse: bFixResolution: {}", bFixResolution);

    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bFixAspect);
    spdlog::info("Config Parse: bFixAspect: {}", bFixAspect);

    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bFixHUD);
    spdlog::info("Config Parse: bFixHUD: {}", bFixHUD);

    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFixFOV);
    spdlog::info("Config Parse: bFixFOV: {}", bFixFOV);

    inipp::get_value(ini.sections["LOD Distance"], "Enabled", bLODDistance);
    spdlog::info("Config Parse: bLODDistance: {}", bLODDistance);

    spdlog::info("----------");
}

void CalculateAspectRatio(bool bLog)
{
    // Calculate aspect ratio
    fAspectRatio = (float)iCurrentResX / (float)iCurrentResY;
    fAspectMultiplier = fAspectRatio / fNativeAspect;

    // HUD 
    fHUDWidth = (float)iCurrentResY * fNativeAspect;
    fHUDHeight = (float)iCurrentResY;
    fHUDWidthOffset = (float)(iCurrentResX - fHUDWidth) / 2.00f;
    fHUDHeightOffset = 0.00f;

    if (fAspectRatio < fNativeAspect) {
        fHUDWidth = (float)iCurrentResX;
        fHUDHeight = (float)iCurrentResX / fNativeAspect;
        fHUDWidthOffset = 0.00f;
        fHUDHeightOffset = (float)(iCurrentResY - fHUDHeight) / 2.00f;
    }

    if (bLog) {
        // Log details about current resolution
        spdlog::info("----------");
        spdlog::info("Current Resolution: Resolution: {}x{}", iCurrentResX, iCurrentResY);
        spdlog::info("Current Resolution: fAspectRatio: {}", fAspectRatio);
        spdlog::info("Current Resolution: fAspectMultiplier: {}", fAspectMultiplier);
        spdlog::info("Current Resolution: fHUDWidth: {}", fHUDWidth);
        spdlog::info("Current Resolution: fHUDHeight: {}", fHUDHeight);
        spdlog::info("Current Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
        spdlog::info("Current Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
        spdlog::info("----------");
    }
}

void Resolution()
{
    // Grab desktop resolution/aspect just in case
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();
    iCurrentResX = DesktopDimensions.first;
    iCurrentResY = DesktopDimensions.second;
    CalculateAspectRatio(false);

    if (bFixResolution) {
        // Stop resolution from being scaled to 16:9 and log current resolution
        std::uint8_t* ResolutionFixScanResult = Memory::PatternScan(baseModule, "76 ?? C5 ?? ?? ?? C4 ?? ?? ?? ?? 8B ?? 41 ?? ?? ?? ?? ?? ??");
        if (ResolutionFixScanResult) {
            spdlog::info("Resolution: Address is {:s}+{:x}", sExeName.c_str(), ResolutionFixScanResult - (std::uint8_t*)baseModule);
            // Jump past code that resizes the resolution 16:9
            Memory::PatchBytes(ResolutionFixScanResult, "\xEB\x09", 2);
            spdlog::info("Resolution: Patched instruction.");

            static SafetyHookMid CurrentResolutionMidHook{};
            CurrentResolutionMidHook = safetyhook::create_mid(ResolutionFixScanResult - 0xD,
                [](SafetyHookContext& ctx) {
                    // Log current resolution
                    int iResX = (int)ctx.rdx;
                    int iResY = (int)ctx.r8;

                    if (iResX != iCurrentResX || iResY != iCurrentResY) {
                        iCurrentResX = iResX;
                        iCurrentResY = iResY;
                        CalculateAspectRatio(true);
                    }
                });
        }
        else {
            spdlog::error("Resolution: Pattern scan failed.");
        }
    }
}

void AspectRatio()
{
    if (bFixAspect) {
        // Cutscene aspect ratio
        std::uint8_t* CutsceneAspectRatioScanResult = Memory::PatternScan(baseModule, "F6 ?? ?? ?? ?? ?? 02 0F 84 ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ??");
        if (CutsceneAspectRatioScanResult) {
            spdlog::info("Cutscene Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), CutsceneAspectRatioScanResult - (std::uint8_t*)baseModule);
            // Force the test instruction for "bConstrainAspectRatio" to always set ZF so it jumps as though it was disabled
            Memory::PatchBytes(CutsceneAspectRatioScanResult + 0x6, "\x00", 1);
            spdlog::info("Cutscene Aspect Ratio: Patched instruction.");
        }
        else {
            spdlog::error("Cutscene Aspect Ratio: Pattern scan failed.");
        }
    }
}

void FOV()
{
    if (bFixFOV || fAdditionalFOV != 0.00f) {
        // FOV
        std::uint8_t* FOVScanResult = Memory::PatternScan(baseModule, "41 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 48 ?? ?? ?? 49 ?? ?? E8 ?? ?? ?? ??");
        if (FOVScanResult) {
            spdlog::info("FOV: Address is {:s}+{:x}", sExeName.c_str(), FOVScanResult - (std::uint8_t*)baseModule);
            static SafetyHookMid FOVMidHook{};
            FOVMidHook = safetyhook::create_mid(FOVScanResult,
                [](SafetyHookContext& ctx) {
                    if (bFixFOV) {
                        // Fix vert- FOV at >16:9
                        if (fAspectRatio > fNativeAspect)
                            ctx.xmm9.f32[0] = atanf(tanf(ctx.xmm9.f32[0] * (fPi / 360)) / fNativeAspect * fAspectRatio) * (360 / fPi);
                    }

                    if (fAdditionalFOV != 0.00f && ctx.rcx) {
                        // Only apply additional FOV outside of cutscenes by checking for bConstrainAspectRatio
                        if (!(*(reinterpret_cast<uint8_t*>(ctx.rcx + 0x270)) & 0x02))
                            ctx.xmm9.f32[0] += fAdditionalFOV;
                    }
                });
        }
        else {
            spdlog::error("FOV: Pattern scan failed.");
        }
    }
}

void HUD()
{
    if (bFixHUD) {
        // HUD
        std::uint8_t* HUDScanResult = Memory::PatternScan(baseModule, "45 ?? ?? ?? ?? ?? ?? 45 ?? ?? ?? ?? ?? ?? 49 ?? ?? ?? ?? ?? ?? 49 ?? ?? E8 ?? ?? ?? ??");
        if (HUDScanResult) {
            spdlog::info("HUD: Address is {:s}+{:x}", sExeName.c_str(), HUDScanResult - (std::uint8_t*)baseModule);
            static SafetyHookMid HUDMidHook{};
            HUDMidHook = safetyhook::create_mid(HUDScanResult,
                [](SafetyHookContext& ctx) {
                    // Set canvas size and offset
                    if (fAspectRatio > fNativeAspect) {
                        ctx.rbx = (int)ceilf(fHUDWidthOffset);
                        ctx.r8 = (int)ceilf(fHUDWidth);
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.rdi = (int)ceilf(fHUDHeightOffset);
                        ctx.r9 = (int)ceilf(fHUDHeight);
                    }
                });
        }
        else {
            spdlog::error("HUD: Pattern scan failed.");
        }
    }
}

SafetyHookInline IsMoviePlaying_sh{};
bool IsMoviePlaying_hk()
{
    return (bIsMoviePlaying = IsMoviePlaying_sh.fastcall<bool>());
}

void Misc()
{
    if (bUncapFPS) {
        // WS_GameInfo::IsMoviePlaying()
        std::uint8_t* IsMoviePlayingScanResult = Memory::PatternScan(baseModule, "48 ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 48 ?? ?? FF ?? ?? 83 ?? ?? 01 7E ??");
        if (IsMoviePlayingScanResult) {
            spdlog::info("IsMoviePlaying: Address is {:s}+{:x}", sExeName.c_str(), IsMoviePlayingScanResult - (std::uint8_t*)baseModule);
            IsMoviePlaying_sh = safetyhook::create_inline(IsMoviePlayingScanResult, reinterpret_cast<void*>(IsMoviePlaying_hk));
            if (IsMoviePlaying_sh) 
                spdlog::info("IsMoviePlaying: Hooked function successfully.");
        }
        else {
            spdlog::error("IsMoviePlaying: Pattern scan failed.");
        }

        // Remove framerate cap
        std::uint8_t* FramerateCapScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? C5 ?? ?? ?? C5 ?? ?? ?? 76 ?? C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? C4 ?? ?? ?? ?? C5 ?? ?? ?? 48 8D ?? ?? ??");
        if (FramerateCapScanResult) {
            spdlog::info("Framerate Cap: Address is {:s}+{:x}", sExeName.c_str(), FramerateCapScanResult - (std::uint8_t*)baseModule);
            // This effectively sets "MaxSmoothedFrameRate" to 0
            static SafetyHookMid FramerateCapMidHook{};
            FramerateCapMidHook = safetyhook::create_mid(FramerateCapScanResult,
                [](SafetyHookContext& ctx) {
                    if (!bIsMoviePlaying) {
                        // Uncap framerate
                        ctx.xmm0.f32[0] = 0.00f;
                    }
                });
        }
        else {
            spdlog::error("Framerate Cap: Pattern scan failed.");
        }
    }

    if (bLODDistance) {
        // LOD distance
        std::uint8_t* LODDistanceFactorScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? 48 8D ?? ?? 49 ?? ?? E8 ?? ?? ?? ?? 48 ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ??");
        if (LODDistanceFactorScanResult) {
            spdlog::info("LOD Distance: Address is {:s}+{:x}", sExeName.c_str(), LODDistanceFactorScanResult - (std::uint8_t*)baseModule);
            static SafetyHookMid LODDistanceFactorMidHook{};
            LODDistanceFactorMidHook = safetyhook::create_mid(LODDistanceFactorScanResult,
                [](SafetyHookContext& ctx) {
                    // Set "LODDistanceFactor"
                    ctx.xmm1.f32[0] = 0.001f;
                });
        }
        else {
            spdlog::error("LOD Distance: Pattern scan failed.");
        }
    }
}

typedef void(__thiscall* EngineExecType)(size_t, const wchar_t*, size_t);
EngineExecType EngineExec_fn = nullptr;

std::uint8_t* GEngine = nullptr;
size_t FOutputDevice = 0;

SafetyHookInline EngineExec_sh{};
void EngineExec_hk(size_t gameEngine, const wchar_t* cmd, size_t outputDevice)
{
    // GEngine
    GEngine = (uint8_t*)gameEngine;
    //spdlog::info("UGameEngine::Exec(): GEngine address: {:x}", (uintptr_t)GEngine);

    // FOutputDevice
    if (FOutputDevice == 0)
        FOutputDevice = outputDevice;

    // Log command
    //spdlog::info("UGameEngine::Exec(): Executed command: {}", Util::wstring_to_string(cmd));

    // Call original function
    EngineExec_sh.thiscall<void>(gameEngine, cmd, outputDevice);
}

void UE()
{
    // UGameEngine::Exec()
    std::uint8_t* EngineExecScanResult = Memory::PatternScan(baseModule, "48 89 ?? ?? ?? 55 56 57 41 ?? 41 ?? 41 ?? 41 ?? 48 8D ?? ?? ?? ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 0F 29 ?? ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 33 ?? 48 89 ?? ?? ?? ?? ?? 4D ?? ?? 48 ?? ?? 4C ?? ??");
    if (EngineExecScanResult) {
        spdlog::info("UGameEngine::Exec(): Address is {:s}+{:x}", sExeName.c_str(), EngineExecScanResult - (std::uint8_t*)baseModule);

        EngineExec_fn = (EngineExecType)EngineExecScanResult;
        EngineExec_sh = safetyhook::create_inline(EngineExec_fn, reinterpret_cast<void*>(EngineExec_hk));
        if (EngineExec_sh) {
            spdlog::info("UGameEngine::Exec(): Hooked function successfully.");
        }
        else {
            spdlog::info("UGameEngine::Exec(): Failed to hook function.");
            return;
        }

        for (int attempts = 0; attempts < 30; ++attempts) {
            if (GEngine) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (GEngine && FOutputDevice) {
            EngineExec_hk((size_t)GEngine, L"stat fps", FOutputDevice);
            EngineExec_hk((size_t)GEngine, L"stat unit", FOutputDevice);
        }
        else {
            spdlog::error("UGameEngine::Exec(): Failed to find valid GEngine address or FOutputDevice.");
        }
    }
    else {
        spdlog::error("UGameEngine::Exec(): Pattern scan failed.");
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    Configuration();
    Resolution();
    AspectRatio();
    FOV();
    HUD();
    Misc();
    //UE();
    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        thisModule = hModule;
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, CREATE_SUSPENDED, 0);
        if (mainHandle) {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_TIME_CRITICAL);
            ResumeThread(mainHandle);
            CloseHandle(mainHandle);
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}