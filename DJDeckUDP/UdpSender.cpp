#include "pch.h"
#include "UdpSender.h"
#include "vdjPlugin8.h"
#include "json.hpp"

#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

std::string UDPTrackInfoSender::GetInfoText(const std::string& command) {
    char output[1024] = { 0 };
    HRESULT result = GetStringInfo(command.c_str(), output, sizeof(output));
    return (result == S_OK) ? std::string(output) : "";
}

double UDPTrackInfoSender::GetInfoDouble(const std::string& command) {
    double d;
    GetInfo(command.c_str(), &d);
    return d;
}

void UDPTrackInfoSender::SendTrackInfo() {
    json payload;
    while (running.load()) {
        bool anyDeckPlaying = false;
        payload.clear();

        for (int deck = 1; deck <= 4; ++deck) {
            std::string title = GetInfoText("deck " + std::to_string(deck) + " get_title");
            std::string artist = GetInfoText("deck " + std::to_string(deck) + " get_artist");
            double playPos = GetInfoDouble("deck " + std::to_string(deck) + " get_position");
            double audible = GetInfoDouble("deck " + std::to_string(deck) + " is_audible");

            if (audible > 0.1f && playPos >= -0.01 && playPos <= 1.01) {
                anyDeckPlaying = true;
                payload["deck" + std::to_string(deck)] = { {"title", title}, {"artist", artist} };
            }
        }

        if (anyDeckPlaying) {
            std::string jsonStr = payload.dump();
            sendto(udpSocket, jsonStr.c_str(), (int)jsonStr.size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        }

        // shorter sleep chunks to allow fast shutdown
        const int chunkMs = 10;
        int remaining = frequencyMs;
        while (running.load() && remaining > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(min(chunkMs, remaining)));
            remaining -= chunkMs;
        }
    }
}

//-----------------------------------------------------------------------------
// VDJ Plugin Lifecycle Methods
//-----------------------------------------------------------------------------
HRESULT VDJ_API UDPTrackInfoSender::OnLoad() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return E_FAIL;
    }

    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        WSACleanup();
        return E_FAIL;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);
    InetPton(AF_INET, L"127.0.0.1", &serverAddr.sin_addr);

    running.store(true);
    senderThread = std::thread(&UDPTrackInfoSender::SendTrackInfo, this);

    return S_OK;
}

HRESULT VDJ_API UDPTrackInfoSender::OnGetPluginInfo(TVdjPluginInfo8* infos) {
    infos->PluginName = "UDPSender";
    infos->Author = "Ikamon";
    infos->Description = "Sends track metadata and PCM audio over UDP";
    infos->Version = "2.0";
    infos->Flags = 0x00;
    infos->Bitmap = NULL;
    return S_OK;
}

ULONG VDJ_API UDPTrackInfoSender::Release() {
    running.store(false);

    if (udpSocket != INVALID_SOCKET) {
        shutdown(udpSocket, SD_BOTH);
    }

    if (senderThread.joinable()) {
        senderThread.join();
    }

    if (udpSocket != INVALID_SOCKET) {
        closesocket(udpSocket);
        udpSocket = INVALID_SOCKET;
    }
    WSACleanup();

    // safer: let VDJ handle deletion
    return 0;
}

HRESULT VDJ_API UDPTrackInfoSender::OnGetUserInterface(TVdjPluginInterface8* pluginInterface) {
    return S_OK;
}
HRESULT VDJ_API UDPTrackInfoSender::OnParameter(int id) {
    return S_OK;
}
HRESULT VDJ_API UDPTrackInfoSender::OnGetParameterString(int id, char* outParam, int outParamSize) {
    return S_OK;
}
