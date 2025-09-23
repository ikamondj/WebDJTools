#ifndef MYPLUGIN8_H
#define MYPLUGIN8_H

#include <stdio.h>
#include "vdjDsp8.h"
#include <string>
#include <iostream>
#include <thread>
#include <ws2tcpip.h>
#include <chrono>
#include <vector>
#include <array>
#include <winsock2.h>
#include <atomic>

class UDPTrackInfoSender : public IVdjPlugin8
{
private:
    std::atomic<bool> running{ false };
    int frequencyMs = 1000;
    std::thread senderThread;
    SOCKET udpSocket = INVALID_SOCKET;
    sockaddr_in serverAddr{};

    std::string GetInfoText(const std::string& command);
    double GetInfoDouble(const std::string& command);
    void SendTrackInfo();

public:
    HRESULT VDJ_API OnLoad();
    HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8* infos);
    ULONG VDJ_API Release();
    HRESULT VDJ_API OnGetUserInterface(TVdjPluginInterface8* pluginInterface) override;
    HRESULT VDJ_API OnParameter(int id) override;
    HRESULT VDJ_API OnGetParameterString(int id, char* outParam, int outParamSize) override;

private:
    int m_Reset = 0;
    float m_Dry = 0.0f;
    float m_Wet = 0.0f;

    bool isMasterFX(); // an example of additional function for the use of GetInfo()

protected:
    enum ID_Interface
    {
        ID_BUTTON_1,
        ID_SLIDER_1
    };
};

#endif
