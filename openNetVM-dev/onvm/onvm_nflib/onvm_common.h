/*********************************************************************
 *                     openNetVM
 *              https://sdnfv.github.io
 *
 *   BSD LICENSE
 *
 *   Copyright(c)
 *            2015-2016 George Washington University
 *            2015-2016 University of California Riverside
 *            2010-2014 Intel Corporation
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * The name of the author may not be used to endorse or promote
 *       products derived from this software without specific prior
 *       written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * onvm_common.h - shared data between host and NFs
 ********************************************************************/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

#include <rte_mbuf.h>
#include <rte_ether.h>

#include "onvm_msg_common.h"

#define ONVM_MAX_CHAIN_LENGTH 4   // the maximum chain length
#define MAX_CLIENTS 16            // total number of NFs allowed
#define MAX_SERVICES 16           // total number of unique services allowed
#define MAX_CLIENTS_PER_SERVICE 8 // max number of NFs per service.

#define ONVM_NF_ACTION_DROP 0   // drop packet
#define ONVM_NF_ACTION_NEXT 1   // to whatever the next action is configured by the SDN controller in the flow table
#define ONVM_NF_ACTION_TONF 2   // send to the NF specified in the argument field (assume it is on the same host)
#define ONVM_NF_ACTION_OUT 3    // send the packet out the NIC port set in the argument field

//extern uint8_t rss_symmetric_key[40];

//flag operations that should be used on onvm_pkt_meta
#define ONVM_CHECK_BIT(flags, n) !!((flags) & (1 << (n)))
#define ONVM_SET_BIT(flags, n) ((flags) | (1 << (n)))
#define ONVM_CLEAR_BIT(flags, n) ((flags) & (0 << (n)))

struct onvm_pkt_meta {
        uint8_t action; /* Action to be performed */
        uint16_t destination; /* where to go next */
        uint16_t src; /* who processed the packet last */
        uint8_t chain_index; /*index of the current step in the service chain*/
        uint8_t flags; /* bits for custom NF data. Use with caution to prevent collisions from different NFs. */

};

static inline struct onvm_pkt_meta* onvm_get_pkt_meta(struct rte_mbuf* pkt) {
        return (struct onvm_pkt_meta*)&pkt->udata64;
}

static inline uint8_t onvm_get_pkt_chain_index(struct rte_mbuf* pkt) {
        return ((struct onvm_pkt_meta*)&pkt->udata64)->chain_index;
}

/*
 * Shared port info, including statistics information for display by server.
 * Structure will be put in a memzone.
 * - All port id values share one cache line as this data will be read-only
 * during operation.
 * - All rx statistic values share cache lines, as this data is written only
 * by the server process. (rare reads by stats display)
 * - The tx statistics have values for all ports per cache line, but the stats
 * themselves are written by the clients, so we have a distinct set, on different
 * cache lines for each client to use.
 */
struct rx_stats{
        uint64_t rx[RTE_MAX_ETHPORTS];
};

struct tx_stats{
        uint64_t tx[RTE_MAX_ETHPORTS];
        uint64_t tx_drop[RTE_MAX_ETHPORTS];
};

struct port_info {
        uint8_t num_ports;
        uint8_t id[RTE_MAX_ETHPORTS];
        struct ether_addr mac[RTE_MAX_ETHPORTS];
        volatile struct rx_stats rx_stats;
        volatile struct tx_stats tx_stats;
};

/*
 * Define a structure with stats from the clients.
 */
struct client_tx_stats {
        /* these stats hold how many packets the manager will actually receive,
         * and how many packets were dropped because the manager's queue was full.
         */
        uint64_t tx[MAX_CLIENTS];
        uint64_t tx_drop[MAX_CLIENTS];
        uint64_t tx_buffer[MAX_CLIENTS];
        uint64_t tx_returned[MAX_CLIENTS];
        /* FIXME: Why are these stats kept separately from the rest?
         * Would it be better to have an array of struct client_tx_stats instead
         * of putting the array inside the struct? How can we avoid cache
         * invalidations from different NFs updating these stats?
         */
};

extern struct client_tx_stats *clients_stats;

/*
 * Define a structure to describe one NF
 */
struct onvm_nf_info {
        uint16_t instance_id;
        uint16_t service_id;
        uint8_t status;
        const char *tag;
};

/*
 * Define a structure to describe a service chain entry
 */
struct onvm_service_chain_entry {
        uint16_t destination;
        uint8_t action;
};

struct onvm_service_chain {
        struct onvm_service_chain_entry sc[ONVM_MAX_CHAIN_LENGTH];
        uint8_t chain_length;
        int ref_cnt;
};

/* define common names for structures shared between server and client */
#define MP_CLIENT_RXQ_NAME "MProc_Client_%u_RX"
#define MP_CLIENT_TXQ_NAME "MProc_Client_%u_TX"
#define PKTMBUF_POOL_NAME "MProc_pktmbuf_pool"
#define MZ_PORT_INFO "MProc_port_info"
#define MZ_CLIENT_INFO "MProc_client_info"
#define MZ_SCP_INFO "MProc_scp_info"
#define MZ_FTP_INFO "MProc_ftp_info"

#define _MGR_MSG_QUEUE_NAME "MSG_MSG_QUEUE"
#define _NF_MSG_QUEUE_NAME "NF_%u_MSG_QUEUE"
#define _NF_MEMPOOL_NAME "NF_INFO_MEMPOOL"
#define _NF_MSG_POOL_NAME "NF_MSG_MEMPOOL"

/* common names for NF states */
#define NF_WAITING_FOR_ID 0     // First step in startup process, doesn't have ID confirmed by manager yet
#define NF_STARTING 1           // When a NF is in the startup process and already has an id
#define NF_RUNNING 2            // Running normally
#define NF_PAUSED  3            // NF is not receiving packets, but may in the future
#define NF_STOPPED 4            // NF has stopped and in the shutdown process
#define NF_ID_CONFLICT 5        // NF is trying to declare an ID already in use
#define NF_NO_IDS 6             // There are no available IDs for this NF

#define NF_NO_ID -1

/*
 * Given the rx queue name template above, get the queue name
 */
static inline const char *
get_rx_queue_name(unsigned id) {
        /* buffer for return value. Size calculated by %u being replaced
         * by maximum 3 digits (plus an extra byte for safety) */
        static char buffer[sizeof(MP_CLIENT_RXQ_NAME) + 2];

        snprintf(buffer, sizeof(buffer) - 1, MP_CLIENT_RXQ_NAME, id);
        return buffer;
}

/*
 * Given the tx queue name template above, get the queue name
 */
static inline const char *
get_tx_queue_name(unsigned id) {
        /* buffer for return value. Size calculated by %u being replaced
         * by maximum 3 digits (plus an extra byte for safety) */
        static char buffer[sizeof(MP_CLIENT_TXQ_NAME) + 2];

        snprintf(buffer, sizeof(buffer) - 1, MP_CLIENT_TXQ_NAME, id);
        return buffer;
}

/*
 * Given the name template above, get the mgr -> NF msg queue name
 */
static inline const char *
get_msg_queue_name(unsigned id) {
        /* buffer for return value. Size calculated by %u being replaced
         * by maximum 3 digits (plus an extra byte for safety) */
        static char buffer[sizeof(_NF_MSG_QUEUE_NAME) + 2];

        snprintf(buffer, sizeof(buffer) - 1, _NF_MSG_QUEUE_NAME, id);
        return buffer;

}

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1

#endif  // _COMMON_H_
