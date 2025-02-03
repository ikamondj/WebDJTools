#include "pch.h"
#include <windows.h>
#include <string>
#include "vdjOnlineSource.h"
#include "vdjDsp8.h"
#include "WebRTCConnection.hpp"

class WebDJPlugin : public IVdjPluginOnlineSource {
public:
    char commandBuffer[4096];
    HRESULT VDJ_API OnLoad() override {
        OutputDebugStringA("WebDJ Plugin Loaded\n");

        // Declare a custom command for WebDJ
        DeclareParameterCommand(commandBuffer, 1, "WebDJ Command", "WebDJ", sizeof(commandBuffer));

        return S_OK;
    }

    HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8* infos) override {
        infos->PluginName = "WebDJ Provider";
        infos->Author = "Ikamon";
        infos->Description = "Allows streaming and remote DJ collaboration via WebDJ.";
        infos->Version = "0.1";
        return S_OK;
    }

    float GetNextWebDJStreamSample() {
        return 0;
    }

    void UpdateWaveform() {
        SendCommand("set_waveform \"deck 1\"");
    }

    HRESULT VDJ_API OnUnload() {
        OutputDebugStringA("WebDJ Plugin Unloaded\n");
        return S_OK;
    }

    // Inherited via IVdjPluginOnlineSource
    HRESULT VDJ_API OnSearch(const char* search, IVdjTracksList* tracksList) override
    {
        tracksList->add("0abingustest1", "test-track", "ikamon", "remix", "house", "label", "comment", "coverURL", "http://127.0.0.1:5959", 3600.f, 130, 5, 2025, false, false);
        SendCommand("browser_refresh");
        return S_OK;
    }

    HRESULT VDJ_API OnSearchCancel() override
    {
        return S_OK;
    }

    HRESULT VDJ_API GetStreamUrl(const char* uniqueId, IVdjString& url, IVdjString& errorMessage) override
    {
        url = "http://127.0.0.1:5959";
        return S_OK;
    }
    HRESULT VDJ_API GetContextMenu(const char* uniqueId, IVdjContextMenu* contextMenu) override
    {
        contextMenu->add("Open in TestProvider");
        return S_OK;
    }


    HRESULT VDJ_API OnContextMenu(const char* uniqueId, size_t menuIndex) override
    {
        return S_OK;
    }
};

// Create an instance of the plugin
extern "C" __declspec(dllexport) IVdjPlugin8* VDJ_API CreateVDJPlugin() {
    return new WebDJPlugin();
}
