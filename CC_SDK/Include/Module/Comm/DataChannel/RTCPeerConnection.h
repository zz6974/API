#ifndef CC_RTCPeerConnection_H_
#define CC_RTCPeerConnection_H_

#include "RTCPeerConnection.h"
#include "Socket/CCSocket.h"
#include "PeerConnectionConfig.h"

#ifdef LIBJUICE_INC
#include "juice/juice.h"
typedef juice_agent_t Juice;
#else
typedef int Juice;
#endif

namespace CTL {
    enum PeerIceState {
        DISCONNECTED = 0,
        GATHERING,
        CONNECTING,
        CONNECTED,
        COMPLETED,
        FAILED
    };
    class PeerConnection{
        Juice* m_iceAgent = nullptr;
        std::function<void(const std::string&)> m_iceCandidateCallback;
        std::function<void()> m_gatheringDoneCallback;
        std::function<void(const PeerIceState&)> m_onIceStateCallback;
        std::vector<std::string> m_remoteCandidates;
    public:
    private:
        static void onICEStateChange(juice_agent_t *agent, juice_state_t state, void *user_ptr);
        static void onICECandidateCallback(juice_agent_t *agent, const char *sdp, void *user_pt);
        static void onGatheringDoneCallback(juice_agent_t *agent, void *user_ptr);
    public:
        PeerConnection();
        ~PeerConnection();
        // SDP处理方法
        bool setRemoteDescription(const std::string& sdp) const;
        [[nodiscard]] std::string createOffer() const;
        [[nodiscard]] std::string createAnswer() const;

        // ICE候选处理
        void addIceCandidate(const std::string& candidate) const;
        void onIceCandidate(const std::function<void(const std::string&)> &callback);
        void onGatheringDone(const std::function<void()> &callback);
    };
}


#endif