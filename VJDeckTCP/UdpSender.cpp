#include "pch.h"
#include "UdpSender.h"
#include "vdjPlugin8.h"
#include "json.hpp"
#include <queue>
#include <thread>
#include <functional>
#include <tuple>
#include <set>
#include <map>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

std::map<int, std::string> deckSongTitle;
std::map<int, std::queue<std::pair<int, double>>> deckCueQueue;
extern std::set<std::pair<std::string, std::string>> alreadySent;

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

void UDPTrackInfoSender::SendJsonMessage(const std::string& jsonStr) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5028);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, jsonStr.c_str(), (int)jsonStr.length(), 0);
    }
    closesocket(sock);
}

void UDPTrackInfoSender::PollStateChanges() {
    std::string lastMasterTitle;

    while (running.load()) {
        std::string masterDeckStr = GetInfoText("get_activedeck");
        int masterDeck = std::stoi(masterDeckStr.empty() ? "0" : masterDeckStr);
        if (masterDeck >= 1 && masterDeck <= 4) {
            std::string title = GetInfoText("deck " + std::to_string(masterDeck) + " get_title");
            std::string cleanTitle = title.empty() ? "" : title;

            if (cleanTitle != lastMasterTitle) {
                lastMasterTitle = cleanTitle;
                json payload = { {"title", cleanTitle}, {"cue", "master"} };
                SendJsonMessage(payload.dump());
            }
        }

        for (int deck = 1; deck <= 4; ++deck) {
            std::string audible = GetInfoText("deck " + std::to_string(deck) + " is_audible");
            if (audible != "on" && audible != "yes" && audible != "true") continue;
            std::string title = GetInfoText("deck " + std::to_string(deck) + " get_title");
            if (title.empty()) continue;

            double cursorPercent = GetInfoDouble("deck " + std::to_string(deck) + " get_position");

            for (int cue = 1; cue <= 8; ++cue) {
                std::string hasCue = GetInfoText("deck " + std::to_string(deck) + " has_cue " + std::to_string(cue));
                std::transform(hasCue.begin(), hasCue.end(), hasCue.begin(), ::tolower);
                if (hasCue != "on") continue;

                double cuePercent = GetInfoDouble("deck " + std::to_string(deck) + " cue_pos " + std::to_string(cue));
                if (cuePercent < 0 || cursorPercent < cuePercent) continue;

                auto key = std::make_pair(title, std::to_string(cue));
                if (alreadySent.find(key) == alreadySent.end()) {
                    json payload = { {"title", title}, {"cue", std::to_string(cue)} };
                    SendJsonMessage(payload.dump());
                    alreadySent.insert(key);
                }
            }
        }

        const int chunkMs = 10;
        int remaining = frequencyMs;
        while (running.load() && remaining > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(min(chunkMs, remaining)));
            remaining -= chunkMs;
        }
    }
}

void UDPTrackInfoSender::StartResetListener() {
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) return;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5029);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(listenSock);
        return;
    }

    listen(listenSock, 1);

    while (running.load()) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenSock, &readfds);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout

        int activity = select(0, &readfds, NULL, NULL, &tv);
        if (!running.load()) break;

        if (activity > 0 && FD_ISSET(listenSock, &readfds)) {
            SOCKET clientSock = accept(listenSock, NULL, NULL);
            if (clientSock != INVALID_SOCKET) {
                char buffer[256] = {};
                int received = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
                if (received > 0) {
                    std::string msg(buffer, received);
                    if (msg.find("reset") != std::string::npos) {
                        alreadySent.clear();
                    }
                }
                closesocket(clientSock);
            }
        }
    }

    closesocket(listenSock);
}

HRESULT VDJ_API UDPTrackInfoSender::OnLoad() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return E_FAIL;
    }

    running.store(true);
    senderThread = std::thread(&UDPTrackInfoSender::PollStateChanges, this);
    resetListenerThread = std::thread(&UDPTrackInfoSender::StartResetListener, this);

    return S_OK;
}

HRESULT VDJ_API UDPTrackInfoSender::OnGetPluginInfo(TVdjPluginInfo8* infos) {
    infos->PluginName = "TCP Event Sender";
    infos->Author = "Ikamon";
    infos->Description = "Sends VDJ cue/master events as JSON over TCP";
    infos->Version = "1.0";
    infos->Flags = 0x00;
    infos->Bitmap = NULL;
    return S_OK;
}

ULONG VDJ_API UDPTrackInfoSender::Release() {
    running.store(false);

    if (senderThread.joinable()) senderThread.join();
    if (resetListenerThread.joinable()) resetListenerThread.join();

    WSACleanup();
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
