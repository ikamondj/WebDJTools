#ifndef WEBRTCCONNECTION_H
#define WEBRTCCONNECTION_H

#include <string>
#include <functional>
#include <memory>
#include "api/peer_connection_interface.h"
#include "api/media_stream_interface.h"
#include "rtc_base/thread.h"

#ifdef READY_WEBRTC_IMPLEMENTATION

class WebRTCConnection : public webrtc::PeerConnectionObserver {
public:
    enum class Role { PLAYER, LISTENER };

    WebRTCConnection(Role role);
    ~WebRTCConnection();

    // Starts WebRTC session
    bool Start(const std::string& sessionKey);

    // Stops WebRTC session
    void Stop();

    // Sends JSON-formatted data over the connection
    void SendData(const std::string& jsonData);

    // Sets callback for receiving JSON data
    void SetOnDataReceived(std::function<void(const std::string&)> callback);


    // WebRTC Observer Callbacks
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

private:
    bool InitializePeerConnection();

    Role role_;
    std::string sessionKey_;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection_;
    std::function<void(const std::string&)> dataReceivedCallback_;
};

#endif
#endif // WEBRTC_CONNECTION_H
