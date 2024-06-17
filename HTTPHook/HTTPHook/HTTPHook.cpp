#include <MinHook.h>
#include <Windows.h>
#include <winhttp.h>
#include <iostream>
#include "Debug.h"



typedef BOOL(WINAPI* WinHttpSendRequest_t)(
    HINTERNET hRequest,
    LPCWSTR pwszHeaders,
    DWORD dwHeadersLength,
    LPVOID lpOptional,
    DWORD dwOptionalLength,
    DWORD dwTotalLength,
    DWORD_PTR dwContext
    );

typedef BOOL(WINAPI* WinHttpReadData_t)(
    HINTERNET hRequest,
    LPVOID lpBuffer,
    DWORD dwNumberOfBytesToRead,
    LPDWORD lpdwNumberOfBytesRead
    );


WinHttpSendRequest_t ogSendRequest = NULL;
WinHttpReadData_t ogReadData = NULL;

BOOL WINAPI dtHttpSendRequest(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext) {
    std::cout << "outgoing request captured, Size: " << dwHeadersLength << std::endl;
    out(lpszHeaders);
    return ogSendRequest(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);
}


BOOL WINAPI dtHttpReadData(HINTERNET hRequest, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead) {
    BOOL result = ogReadData(hRequest, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
    std::cout << "incoming information." << std::endl;
    in(lpBuffer);
    return result;
}

int main() {
    if (MH_Initialize() != MH_OK) {
        std::cerr << "MinHook init failure" << std::endl;
        return 1;
    }

    
    HMODULE hModule = GetModuleHandle(L"winhttp.dll");
    if (!hModule) {
        std::cerr << "couldnt get handle for winhttp" << std::endl;
        return 1;
    }

    LPVOID pTargetSendRequest = (LPVOID)GetProcAddress(hModule, "WinHttpSendRequest");
    LPVOID pTargetReadData = (LPVOID)GetProcAddress(hModule, "WinHttpReadData");

    if (!pTargetSendRequest || !pTargetReadData) {
        std::cerr << "Failed to get address of target functions" << std::endl;
        return 1;
    }

    
    if (MH_CreateHook(pTargetSendRequest, &dtHttpSendRequest, reinterpret_cast<LPVOID*>(&ogSendRequest)) != MH_OK) {
        std::cerr << "Failed to create hook for WinHttpSendRequest" << std::endl;
        return 1;
    }

    if (MH_CreateHook(pTargetReadData, &dtHttpReadData, reinterpret_cast<LPVOID*>(&ogReadData)) != MH_OK) {
        std::cerr << "Failed to create hook for WinHttpReadData" << std::endl;
        return 1;
    }

    
    if (MH_EnableHook(pTargetSendRequest) != MH_OK) {
        std::cerr << "Failed to enable hook for WinHttpSendRequest" << std::endl;
        return 1;
    }

    if (MH_EnableHook(pTargetReadData) != MH_OK) {
        std::cerr << "Failed to enable hook for WinHttpReadData" << std::endl;
        return 1;
    }

    std::cout << "Hooks enabled. Press any key to remove hooks and exit." << std::endl;
    getchar();

    MH_DisableHook(pTargetSendRequest);
    MH_DisableHook(pTargetReadData);

    MH_Uninitialize();

    return 0;
}
