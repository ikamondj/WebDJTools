#ifndef MYPLUGIN8_H
#define MYPLUGIN8_H

#include <stdio.h>

#include "vdjPlugin8.h"
#include <string>
#include <iostream>
#include <string>
#include <thread>
#include <ws2tcpip.h>
#include <chrono>
#include <winsock2.h>

class UDPTrackInfoSender : public IVdjPlugin8
{
private:
	bool running;
	int frequencyMs;
	std::thread senderThread;
	SOCKET udpSocket;
	sockaddr_in serverAddr;
	std::string GetInfoText(const std::basic_string<char>& command);
	double GetInfoDouble(const std::basic_string<char>& command);
	void SendTrackInfo();
public:
	
	HRESULT VDJ_API OnLoad();
	HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8* infos);
	ULONG VDJ_API Release();
	HRESULT VDJ_API OnGetUserInterface(TVdjPluginInterface8* pluginInterface);
	HRESULT VDJ_API OnParameter(int id);
	HRESULT VDJ_API OnGetParameterString(int id, char* outParam, int outParamSize);

private:
	int m_Reset;
	float m_Dry;
	float m_Wet;

	bool isMasterFX(); // an example of additional function for the use of GetInfo()

protected:
	typedef enum _ID_Interface
	{
		ID_BUTTON_1,
		ID_SLIDER_1
	} ID_Interface;
};

#endif