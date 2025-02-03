#include "pch.h"
#include "UdpSender.h"
#include "vdjPlugin8.h"

#include "json.hpp"

#pragma comment(lib, "ws2_32.lib")
using json = nlohmann::json;
std::string UDPTrackInfoSender::GetInfoText(const std::basic_string<char>& command) {
	char output[1024];
	GetInfo(command.c_str(), (double*)output);
	return std::string(&output[0]);
}

double UDPTrackInfoSender::GetInfoDouble(const std::basic_string<char>& command) {
	double d;
	GetInfo(command.c_str(), &d);
	return d;
}

void UDPTrackInfoSender::SendTrackInfo()
{
	while (running)
	{
		json payload;
		for (int deck = 1; deck <= 4; ++deck)
		{
			std::string title, artist;
			double playPos, audible;

			title = GetInfoText("deck " + std::to_string(deck) + " get_loaded_song");
			artist = GetInfoText("deck " + std::to_string(deck) + " get_song_artist");
			playPos = GetInfoDouble("deck " + std::to_string(deck) + " get_position");
			audible = GetInfoDouble("deck " + std::to_string(deck) + " is_audible");

			if (audible > 0.0f && playPos > 0.0f && playPos < 1.0f)
			{
				payload["deck" + std::to_string(deck)] = {
					{"title", title},
					{"artist", artist}
				};
			}
			else
			{
				payload["deck" + std::to_string(deck)] = {
					{"title", ""},
					{"artist", ""}
				};
			}
		}

		std::string jsonStr = payload.dump();
		sendto(udpSocket, jsonStr.c_str(), jsonStr.size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

		std::this_thread::sleep_for(std::chrono::milliseconds(frequencyMs));
	}
}

//-----------------------------------------------------------------------------
HRESULT VDJ_API UDPTrackInfoSender::OnLoad()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(5000); // Set target UDP port
	serverAddr.sin_addr.s_addr = InetPton(AF_INET, L"127.0.0.1", &serverAddr.sin_addr);

	running = true;
	senderThread = std::thread(&UDPTrackInfoSender::SendTrackInfo, this);
	auto loadMsg = std::string("UDP Plugin Loaded!");
	auto dbg = std::string("Debug");
	MessageBoxW(NULL, std::wstring(loadMsg.begin(), loadMsg.end()).c_str(), std::wstring(dbg.begin(), dbg.end()).c_str(), MB_OK);
	return S_OK;
}
//-----------------------------------------------------------------------------
HRESULT VDJ_API UDPTrackInfoSender::OnGetPluginInfo(TVdjPluginInfo8* infos)
{
	infos->PluginName = "UDPSender";
	infos->Author = "Ikamon";
	infos->Description = "Sends currently playing decks via UDP";
	infos->Version = "1.0";
	infos->Flags = 0x00;
	infos->Bitmap = NULL;

	return S_OK;
}
//---------------------------------------------------------------------------
ULONG VDJ_API UDPTrackInfoSender::Release()
{
	running = false;
	if (senderThread.joinable())
	{
		senderThread.join();
	}
	closesocket(udpSocket);
	WSACleanup();
	return S_OK;

	delete this;
	return 0;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API UDPTrackInfoSender::OnGetUserInterface(TVdjPluginInterface8* pluginInterface)
{
	pluginInterface->Type = VDJINTERFACE_DEFAULT;

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API UDPTrackInfoSender::OnParameter(int id)
{
	switch (id)
	{
	case ID_BUTTON_1:
		if (m_Reset == 1)
		{
			m_Wet = 0.5f;
			//HRESULT hr;
			//hr = SendCommand("effect_slider 1 50%");
		}
		break;

	case ID_SLIDER_1:
		m_Dry = 1 - m_Wet;
		break;
	}

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API UDPTrackInfoSender::OnGetParameterString(int id, char* outParam, int outParamSize)
{
	switch (id)
	{
	case ID_SLIDER_1:
		sprintf(outParam, "+%.0f%%", m_Wet * 100);
		break;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------------------------------------------
// BELOW, ADDITIONAL FUNCTIONS ONLY TO EXPLAIN SOME FEATURES (CAN BE REMOVED)
//-------------------------------------------------------------------------------------------------------------------------------------
bool UDPTrackInfoSender::isMasterFX()
{
	double qRes;
	HRESULT hr = S_FALSE;

	hr = GetInfo("get_deck 'master' ? true : false", &qRes);

	if (qRes == 1.0f) return true;
	else return false;
}