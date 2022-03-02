#include "cwrap.h"

static ServerUncoreMemoryMetrics metrics;
static ServerUncoreCounterState * BeforeState;
static ServerUncoreCounterState * AfterState;
static uint64 BeforeTime, AfterTime;
static memdata_t md;

static bool skipInactiveChannels = true;

static inline bool anyPmem(const ServerUncoreMemoryMetrics & metrics)
{
    return (metrics == Pmem) || (metrics == PmemMixedMode) || (metrics == PmemMemoryMode);
}

static void calculate_bandwidth(const uint64 elapsedTime, const ServerUncoreMemoryMetrics & metrics) {
    //const uint32 num_imc_channels = m->getMCChannelsPerSocket();
    //const uint32 num_edc_channels = m->getEDCChannelsPerSocket();
    PCM * m = PCM::getInstance();
    md.metrics = metrics;
    md.M2M_NM_read_hit_rate_supported = (m->getCPUModel() == PCM::SKX);
    md.iMC_NM_hit_rate_supported = (m->getCPUModel() == PCM::ICX);
    static bool mm_once = true;
    if (metrics == Pmem && md.M2M_NM_read_hit_rate_supported == false && md.iMC_NM_hit_rate_supported == true && mm_once)
    {
        cerr << "INFO: Use -mm option to monitor NM Memory Mode metrics\n";
        mm_once = false;
    }
    static bool mm_once1 = true;
    if (metrics == PmemMemoryMode && md.M2M_NM_read_hit_rate_supported == true && md.iMC_NM_hit_rate_supported == false && mm_once1)
    {
        cerr << "INFO: Use -pmem option to monitor NM Memory Mode metrics\n";
        mm_once1 = false;
    }

    for(uint32 skt = 0; skt < max_sockets; ++skt)
    {
        md.iMC_Rd_socket[skt] = 0.0;
        md.iMC_Wr_socket[skt] = 0.0;
        md.iMC_PMM_Rd_socket[skt] = 0.0;
        md.iMC_PMM_Wr_socket[skt] = 0.0;
        md.iMC_PMM_MemoryMode_Miss_socket[skt] = 0.0;
        md.iMC_PMM_MemoryMode_Hit_socket[skt] = 0.0;
        md.iMC_NM_hit_rate[skt] = 0.0;
        md.EDC_Rd_socket[skt] = 0.0;
        md.EDC_Wr_socket[skt] = 0.0;
        md.partial_write[skt] = 0;
		for (uint32 i = 0; i < max_imc_controllers; ++i)
		{
			md.M2M_NM_read_hit_rate[skt][i] = 0.;
		}
    }

    for(uint32 skt = 0; skt < m->getNumSockets(); ++skt)
    {
		const uint32 numChannels1 = (uint32)m->getMCChannels(skt, 0); // number of channels in the first controller

		auto toBW = [&elapsedTime](const uint64 nEvents)
		{
			return (float)(nEvents * 64 / 1000000.0 / (elapsedTime / 1000.0));
		};

        if (m->MCDRAMmemoryTrafficMetricsAvailable())
        {
            for (uint32 channel = 0; channel < max_edc_channels; ++channel)
            {
                if (skipInactiveChannels && getEDCCounter(channel, ServerPCICFGUncore::EventPosition::READ, BeforeState[skt], AfterState[skt]) == 0.0 && getEDCCounter(channel, ServerPCICFGUncore::EventPosition::WRITE, BeforeState[skt], AfterState[skt]) == 0.0)
                {
                    md.EDC_Rd_socket_chan[skt][channel] = -1.0;
                    md.EDC_Wr_socket_chan[skt][channel] = -1.0;
                    continue;
                }

                md.EDC_Rd_socket_chan[skt][channel] = toBW(getEDCCounter(channel, ServerPCICFGUncore::EventPosition::READ, BeforeState[skt], AfterState[skt]));
                md.EDC_Wr_socket_chan[skt][channel] = toBW(getEDCCounter(channel, ServerPCICFGUncore::EventPosition::WRITE, BeforeState[skt], AfterState[skt]));

                md.EDC_Rd_socket[skt] += md.EDC_Rd_socket_chan[skt][channel];
                md.EDC_Wr_socket[skt] += md.EDC_Wr_socket_chan[skt][channel];
            }
        }

        {
            for (uint32 channel = 0; channel < max_imc_channels; ++channel)
            {
                uint64 reads = 0, writes = 0, pmmReads = 0, pmmWrites = 0, pmmMemoryModeCleanMisses = 0, pmmMemoryModeDirtyMisses = 0;
                uint64 pmmMemoryModeHits = 0;
                reads = getMCCounter(channel, ServerPCICFGUncore::EventPosition::READ, BeforeState[skt], AfterState[skt]);
                writes = getMCCounter(channel, ServerPCICFGUncore::EventPosition::WRITE, BeforeState[skt], AfterState[skt]);
                if (metrics == Pmem)
                {
                    pmmReads = getMCCounter(channel, ServerPCICFGUncore::EventPosition::PMM_READ, BeforeState[skt], AfterState[skt]);
                    pmmWrites = getMCCounter(channel, ServerPCICFGUncore::EventPosition::PMM_WRITE, BeforeState[skt], AfterState[skt]);
                }
                else if (metrics == PmemMixedMode || metrics == PmemMemoryMode)
                {
                    pmmMemoryModeCleanMisses = getMCCounter(channel, ServerPCICFGUncore::EventPosition::PMM_MM_MISS_CLEAN, BeforeState[skt], AfterState[skt]);
                    pmmMemoryModeDirtyMisses = getMCCounter(channel, ServerPCICFGUncore::EventPosition::PMM_MM_MISS_DIRTY, BeforeState[skt], AfterState[skt]);
                }
                if (metrics == PmemMemoryMode)
                {
                    pmmMemoryModeHits = getMCCounter(channel, ServerPCICFGUncore::EventPosition::NM_HIT, BeforeState[skt], AfterState[skt]);
                }
                if (skipInactiveChannels && (reads + writes == 0))
                {
                    if ((metrics != Pmem) || (pmmReads + pmmWrites == 0))
                    {
                        if ((metrics != PmemMixedMode) || (pmmMemoryModeCleanMisses + pmmMemoryModeDirtyMisses == 0))
                        {

                            md.iMC_Rd_socket_chan[skt][channel] = -1.0;
                            md.iMC_Wr_socket_chan[skt][channel] = -1.0;
                            continue;
                        }
                    }
                }

                if (metrics != PmemMemoryMode)
                {
                    md.iMC_Rd_socket_chan[skt][channel] = toBW(reads);
                    md.iMC_Wr_socket_chan[skt][channel] = toBW(writes);

                    md.iMC_Rd_socket[skt] += md.iMC_Rd_socket_chan[skt][channel];
                    md.iMC_Wr_socket[skt] += md.iMC_Wr_socket_chan[skt][channel];
                }

                if (metrics == Pmem)
                {
                    md.iMC_PMM_Rd_socket_chan[skt][channel] = toBW(pmmReads);
                    md.iMC_PMM_Wr_socket_chan[skt][channel] = toBW(pmmWrites);

                    md.iMC_PMM_Rd_socket[skt] += md.iMC_PMM_Rd_socket_chan[skt][channel];
                    md.iMC_PMM_Wr_socket[skt] += md.iMC_PMM_Wr_socket_chan[skt][channel];

                    md.M2M_NM_read_hit_rate[skt][(channel < numChannels1) ? 0 : 1] += (float)reads;
                }
                else if (metrics == PmemMixedMode)
                {
                    md.iMC_PMM_MemoryMode_Miss_socket_chan[skt][channel] = toBW(pmmMemoryModeCleanMisses + 2 * pmmMemoryModeDirtyMisses);
                    md.iMC_PMM_MemoryMode_Miss_socket[skt] += md.iMC_PMM_MemoryMode_Miss_socket_chan[skt][channel];
                }
                else if (metrics == PmemMemoryMode)
                {
                    md.iMC_PMM_MemoryMode_Miss_socket[skt] += (float)((pmmMemoryModeCleanMisses + pmmMemoryModeDirtyMisses) / (elapsedTime / 1000.0));
                    md.iMC_PMM_MemoryMode_Hit_socket[skt] += (float)((pmmMemoryModeHits) / (elapsedTime / 1000.0));
                }
                else
                {
                    md.partial_write[skt] += (uint64)(getMCCounter(channel, ServerPCICFGUncore::EventPosition::PARTIAL, BeforeState[skt], AfterState[skt]) / (elapsedTime / 1000.0));
                }
            }
        }
        if (metrics == PmemMemoryMode)
        {
            md.iMC_Rd_socket[skt] += toBW(getFreeRunningCounter(ServerUncoreCounterState::ImcReads, BeforeState[skt], AfterState[skt]));
            md.iMC_Wr_socket[skt] += toBW(getFreeRunningCounter(ServerUncoreCounterState::ImcWrites, BeforeState[skt], AfterState[skt]));
        }
        if (metrics == PmemMixedMode || metrics == PmemMemoryMode)
        {
            const int64 pmmReads = getFreeRunningCounter(ServerUncoreCounterState::PMMReads, BeforeState[skt], AfterState[skt]);
            if (pmmReads >= 0)
            {
                md.iMC_PMM_Rd_socket[skt] += toBW(pmmReads);
            }
            else for(uint32 c = 0; c < max_imc_controllers; ++c)
            {
                md.iMC_PMM_Rd_socket[skt] += toBW(getM2MCounter(c, ServerPCICFGUncore::EventPosition::PMM_READ, BeforeState[skt],AfterState[skt]));
            }

            const int64 pmmWrites = getFreeRunningCounter(ServerUncoreCounterState::PMMWrites, BeforeState[skt], AfterState[skt]);
            if (pmmWrites >= 0)
            {
                md.iMC_PMM_Wr_socket[skt] += toBW(pmmWrites);
            }
            else for(uint32 c = 0; c < max_imc_controllers; ++c)
            {
                md.iMC_PMM_Wr_socket[skt] += toBW(getM2MCounter(c, ServerPCICFGUncore::EventPosition::PMM_WRITE, BeforeState[skt],AfterState[skt]));;
            }
        }
        if (metrics == Pmem)
        {
            for(uint32 c = 0; c < max_imc_controllers; ++c)
            {
                if(md.M2M_NM_read_hit_rate[skt][c] != 0.0)
                {
                    md.M2M_NM_read_hit_rate[skt][c] = ((float)getM2MCounter(c, ServerPCICFGUncore::EventPosition::NM_HIT, BeforeState[skt],AfterState[skt]))/ md.M2M_NM_read_hit_rate[skt][c];
                }
            }
        }
        const auto all = md.iMC_PMM_MemoryMode_Miss_socket[skt] + md.iMC_PMM_MemoryMode_Hit_socket[skt];
        if (metrics == PmemMemoryMode && all != 0.0)
        {
            md.iMC_NM_hit_rate[skt] = md.iMC_PMM_MemoryMode_Hit_socket[skt] / all;
        }
    }
}

void PCMMemoryInit() {
    PCM * m = PCM::getInstance();
    
    metrics = m->PMMTrafficMetricsAvailable() ? Pmem : PartialWrites;

    m->disableJKTWorkaround();
    //print_cpu_details();

    if (!m->hasPCICFGUncore()) {
        cerr << "Unsupported processor model (" << m->getCPUModel() << ").\n";
        if (m->memoryTrafficMetricsAvailable())
            cerr << "For processor-level memory bandwidth statistics please use 'pcm' utility\n";
        exit(EXIT_FAILURE);
    }
    if (anyPmem(metrics) && (m->PMMTrafficMetricsAvailable() == false)) {
        cerr << "PMM/Pmem traffic metrics are not available on your processor.\n";
        exit(EXIT_FAILURE);
    }
    PCM::ErrorCode status = m->programServerUncoreMemoryMetrics(metrics);
    m->checkError(status);
    if(m->getNumSockets() > max_sockets) {
        cerr << "Only systems with up to " << max_sockets << " sockets are supported! Program aborted\n";
        exit(EXIT_FAILURE);
    }

    max_imc_channels = (pcm::uint32)m->getMCChannelsPerSocket();
    BeforeState = new ServerUncoreCounterState[m->getNumSockets()];
    AfterState = new ServerUncoreCounterState[m->getNumSockets()];
    BeforeTime = 0;
    AfterTime = 0;

    m->setBlocked(false);
    for(uint32 i=0; i<m->getNumSockets(); ++i)
        BeforeState[i] = m->getServerUncoreCounterState(i);
    BeforeTime = m->getTickCount();
}

void updateMemoryStats() {
    PCM * m = PCM::getInstance();
    AfterTime = m->getTickCount();
    for(uint32 i=0; i<m->getNumSockets(); ++i)
        AfterState[i] = m->getServerUncoreCounterState(i);
    calculate_bandwidth(AfterTime-BeforeTime, metrics);
    swap(BeforeTime, AfterTime);
    swap(BeforeState, AfterState);
}

void PCMMemoryFree() {
    delete[] BeforeState;
    delete[] AfterState;
}

float getChannelReadThroughput(int socket, int channel) {
    return md.iMC_Rd_socket_chan[socket][channel];
}

float getChannelWriteThroughput(int socket, int channel) {
    return md.iMC_Wr_socket_chan[socket][channel];
}

float getChannelPMMReadThroughput(int socket, int channel) {
    return md.iMC_PMM_Rd_socket_chan[socket][channel];
}

float getChannelPMMWriteThroughput(int socket, int channel) {
    return md.iMC_PMM_Wr_socket_chan[socket][channel];
}

int getNumSockets() {
    PCM * m = PCM::getInstance();
    return m->getNumSockets();
}

int getMaxChannel() {
    return max_imc_channels;
}

bool socketHasChannel(int socket, int channel) {
    return md.iMC_Rd_socket_chan[socket][channel] >= 0.0 && md.iMC_Wr_socket_chan[socket][channel] >= 0.0;
}