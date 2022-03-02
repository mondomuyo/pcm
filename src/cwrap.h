#ifndef CWRAP_H
#define CWRAP_H

#include<iostream>
#include "cpucounters.h"
#include "utils.h"

#ifdef __cplusplus
using namespace std;
using namespace pcm;
extern "C" {
#endif

    const uint32 max_sockets = 256;
    uint32 max_imc_channels = ServerUncoreCounterState::maxChannels;
    const uint32 max_edc_channels = ServerUncoreCounterState::maxChannels;
    const uint32 max_imc_controllers = ServerUncoreCounterState::maxControllers;

    typedef struct memdata {
        float iMC_Rd_socket_chan[max_sockets][ServerUncoreCounterState::maxChannels]{};
        float iMC_Wr_socket_chan[max_sockets][ServerUncoreCounterState::maxChannels]{};
        float iMC_PMM_Rd_socket_chan[max_sockets][ServerUncoreCounterState::maxChannels]{};
        float iMC_PMM_Wr_socket_chan[max_sockets][ServerUncoreCounterState::maxChannels]{};
        float iMC_PMM_MemoryMode_Miss_socket_chan[max_sockets][ServerUncoreCounterState::maxChannels]{};
        float iMC_Rd_socket[max_sockets]{};
        float iMC_Wr_socket[max_sockets]{};
        float iMC_PMM_Rd_socket[max_sockets]{};
        float iMC_PMM_Wr_socket[max_sockets]{};
        float iMC_PMM_MemoryMode_Miss_socket[max_sockets]{};
        bool iMC_NM_hit_rate_supported{};
        float iMC_PMM_MemoryMode_Hit_socket[max_sockets]{};
        bool M2M_NM_read_hit_rate_supported{};
        float iMC_NM_hit_rate[max_sockets]{};
        float M2M_NM_read_hit_rate[max_sockets][max_imc_controllers]{};
        float EDC_Rd_socket_chan[max_sockets][max_edc_channels]{};
        float EDC_Wr_socket_chan[max_sockets][max_edc_channels]{};
        float EDC_Rd_socket[max_sockets]{};
        float EDC_Wr_socket[max_sockets]{};
        uint64 partial_write[max_sockets]{};
        ServerUncoreMemoryMetrics metrics{};
    } memdata_t;

    void PCMMemoryInit();
    void updateMemoryStats();
    void PCMMemoryFree();
    float getChannelReadThroughput(int socket, int channel);
    float getChannelWriteThroughput(int socket, int channel);
    float getChannelPMMReadThroughput(int socket, int channel);
    float getChannelPMMWriteThroughput(int socket, int channel);
    int getNumSockets();
    int getMaxChannel();
    bool socketHasChannel(int socket, int channel);

#ifdef __cplusplus
}
#endif

#endif