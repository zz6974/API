#include "DataChannel/RTCPeerConnection.h"


void CTL::PeerConnection::onICEStateChange(juice_agent_t *agent, juice_state_t state, void *user_ptr) {
    if (user_ptr) {
        const auto pc = static_cast<PeerConnection*>(user_ptr);
        if (pc->m_onIceStateCallback) {
            const auto iceState = static_cast<PeerIceState>(state);
            pc->m_onIceStateCallback(iceState);

        }
    }
}

void CTL::PeerConnection::onICECandidateCallback(juice_agent_t *agent, const char *sdp, void *user_pt) {
    if (user_pt) {
        const auto pc = static_cast<PeerConnection*>(user_pt);
        const std::string sdp_t(sdp);
        if (pc->m_iceCandidateCallback) {
            pc->m_iceCandidateCallback(sdp_t);
        }
    }
}

void CTL::PeerConnection::onGatheringDoneCallback(juice_agent_t *agent, void *user_ptr) {
    if (user_ptr) {
        const auto pc = static_cast<PeerConnection*>(user_ptr);
        if (pc->m_gatheringDoneCallback) {
            pc->m_gatheringDoneCallback();
        }
    }
}

CTL::PeerConnection::PeerConnection() {
#ifdef LIBJUICE_INC
    juice_config_t config = {};
    config.cb_state_changed = onICEStateChange;
    config.cb_candidate = onICECandidateCallback;
    config.cb_gathering_done = onGatheringDoneCallback;
    config.user_ptr = this;
    m_iceAgent = juice_create(&config);
#endif
}

CTL::PeerConnection::~PeerConnection() {
    if (m_iceAgent) {
        free(m_iceAgent);
    }
}

bool CTL::PeerConnection::setRemoteDescription(const std::string &sdp) const {
#ifdef LIBJUICE_INC
    if (!m_iceAgent) {
        return false;
    }
    const int result = juice_set_remote_description(m_iceAgent, sdp.c_str());
    return result >= 0;
#else
    return false;
#endif
}

std::string CTL::PeerConnection::createOffer() const {
#ifdef LIBJUICE_INC
    if (!m_iceAgent) {
        return "";
    }
    char sdp[JUICE_MAX_SDP_STRING_LEN]{};
    const int result = juice_get_local_description(m_iceAgent, sdp, JUICE_MAX_SDP_STRING_LEN);
    if (result < 0) {
        return "";
    }
    // 启动候选收集
    juice_gather_candidates(m_iceAgent);
    return sdp;
#else
    return "";
#endif
}

std::string CTL::PeerConnection::createAnswer() const {
#ifdef LIBJUICE_INC
    if (!m_iceAgent) {
        return "";
    }
    char sdp[JUICE_MAX_SDP_STRING_LEN];
    const int result = juice_get_local_description(m_iceAgent, sdp, JUICE_MAX_SDP_STRING_LEN);
    if (result < 0) {
        return "";
    }
    // 启动候选收集
    juice_gather_candidates(m_iceAgent);
    return sdp;
#else
    return "";
#endif
}

void CTL::PeerConnection::addIceCandidate(const std::string &candidate) const {
#ifdef LIBJUICE_INC
    if (!m_iceAgent) {
        return;
    }

    // 添加远程ICE候选
    int ret = juice_add_remote_candidate(m_iceAgent, candidate.c_str());
    if (ret != JUICE_ERR_SUCCESS) {
        return;
    }
#endif
}

void CTL::PeerConnection::onIceCandidate(const std::function<void(const std::string &)> &callback) {
    m_iceCandidateCallback = callback;
}

void CTL::PeerConnection::onGatheringDone(const std::function<void()> &callback) {
    m_gatheringDoneCallback = callback;
}
