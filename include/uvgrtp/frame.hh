#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2def.h>
#else
#include <netinet/in.h>
#endif

#include <string>

#include "util.hh"

#define INVALID_FRAME_TYPE(ft) (ft < RTP_FT_GENERIC|| ft > RTP_FT_HEVC_FU)

namespace uvg_rtp {
    namespace frame {
        enum HEADER_SIZES {
            HEADER_SIZE_RTP      = 12,
            HEADER_SIZE_OPUS     =  1,
            HEADER_SIZE_HEVC_NAL =  2,
            HEADER_SIZE_HEVC_FU  =  1,
        };

        enum RTP_FRAME_TYPE {
            RTP_FT_GENERIC = 0, /* payload length + RTP Header size (N + 12) */
            RTP_FT_OPUS    = 1, /* payload length + RTP Header size + Opus header (N + 12 + 0 [for now]) */
            RTP_FT_HEVC_FU = 2, /* payload length + RTP Header size + HEVC NAL Header + FU Header (N + 12 + 2 + 1) */
        };

        enum RTCP_FRAME_TYPE {
            RTCP_FT_SR   = 200, /* Sender report */
            RTCP_FT_RR   = 201, /* Receiver report */
            RTCP_FT_SDES = 202, /* Source description */
            RTCP_FT_BYE  = 203, /* Goodbye */
            RTCP_FT_APP  = 204  /* Application-specific message */
        };

        PACKED_STRUCT(rtp_header) {
            uint8_t version:2;
            uint8_t padding:1;
            uint8_t ext:1;
            uint8_t cc:4;
            uint8_t marker:1;
            uint8_t payload:7;
            uint16_t seq;
            uint32_t timestamp;
            uint32_t ssrc;
        };

        PACKED_STRUCT(ext_header) {
            uint16_t type;
            uint16_t len;
            uint8_t *data;
        };

        struct rtp_frame {
            struct rtp_header header;
            uint32_t *csrc;
            struct ext_header *ext;

            size_t padding_len; /* non-zero if frame is padded */
            size_t payload_len; /* payload_len: total_len - header_len - padding length (if padded) */

            /* Probation zone is a small area of free-to-use memory for the frame receiver
             * when handling fragments. For example HEVC fragments that belong to future frames
             * but cannot be relocated there (start sequence missing) are copied to probation
             * zone and when the frame becomes active, all fragments in the probation are relocated
             *
             * NOTE 1: Probation zone will increase the memory usage and will increase
             * the internal fragmentation as this memory is not usable for anything else
             *
             * NOTE 2: This is a Linux-only optimization */
            size_t probation_len;
            size_t probation_off;
            uint8_t *probation;
            uint8_t *payload;

            rtp_format_t format;
            int  type;
            sockaddr_in src_addr;
        };

        PACKED_STRUCT(rtcp_header) {
            uint8_t version:2;
            uint8_t padding:1;
            uint8_t count:5;
            uint8_t pkt_type;
            uint16_t length;
        };

        PACKED_STRUCT(rtcp_sender_info) {
            uint32_t ntp_msw; /* NTP timestamp, most significant word */
            uint32_t ntp_lsw; /* NTP timestamp, least significant word */
            uint32_t rtp_ts;  /* RTP timestamp corresponding to same time as NTP */
            uint32_t pkt_cnt;
            uint32_t byte_cnt;
        };

        PACKED_STRUCT(rtcp_report_block) {
            uint32_t ssrc;
            uint8_t  fraction;
            int32_t  lost:24;
            uint32_t last_seq;
            uint32_t jitter;
            uint32_t lsr;  /* last Sender Report */
            uint32_t dlsr; /* delay since last Sender Report */
        };

        PACKED_STRUCT(rtcp_sender_frame) {
            struct rtcp_header header;
            uint32_t sender_ssrc;
            struct rtcp_sender_info s_info;
            struct rtcp_report_block blocks[1];
        };

        PACKED_STRUCT(rtcp_receiver_frame) {
            struct rtcp_header header;
            uint32_t sender_ssrc;
            struct rtcp_report_block blocks[1];
        };

        PACKED_STRUCT(rtcp_sdes_item) {
            uint8_t type;
            uint8_t length;
            uint8_t data[1];
        };

        PACKED_STRUCT(rtcp_sdes_frame) {
            struct rtcp_header header;
            uint32_t sender_ssrc;
            struct rtcp_sdes_item items[1];
        };

        PACKED_STRUCT(rtcp_bye_frame) {
            struct rtcp_header header;
            uint32_t ssrc[1];
        };

        PACKED_STRUCT(rtcp_app_frame) {
            uint8_t version:2;
            uint8_t padding:1;
            uint8_t pkt_subtype:5;
            uint8_t pkt_type;
            uint16_t length;

            uint32_t ssrc;
            uint8_t name[4];
            uint8_t payload[1];
        };

        PACKED_STRUCT(zrtp_frame) {
            uint8_t version:4;
            uint16_t unused:12;
            uint16_t seq;
            uint32_t magic;
            uint32_t ssrc;
            uint8_t payload[1];
        };

        /* Allocate an RTP frame
         *
         * First function allocates an empty RTP frame (no payload)
         *
         * Second allocates an RTP frame with payload of size "payload_len",
         *
         * Third allocate an RTP frame with payload of size "payload_len"
         * + probation zone of size "pz_size" * MAX_PAYLOAD
         *
         * Return pointer to frame on success
         * Return nullptr on error and set rtp_errno to:
         *    RTP_MEMORY_ERROR if allocation of memory failed */
        rtp_frame *alloc_rtp_frame();
        rtp_frame *alloc_rtp_frame(size_t payload_len);
        rtp_frame *alloc_rtp_frame(size_t payload_len, size_t pz_size);

        /* Allocate ZRTP frame
         * Parameter "payload_size" defines the length of the frame 
         *
         * Return pointer to frame on success
         * Return nullptr on error and set rtp_errno to:
         *    RTP_MEMORY_ERROR if allocation of memory failed
         *    RTP_INVALID_VALUE if "payload_size" is 0 */
        zrtp_frame *alloc_zrtp_frame(size_t payload_size);

        /* Allocate various types of RTCP frames, see src/rtcp.cc for more details
         *
         * Return pointer to frame on success
         * Return nullptr on error and set rtp_errno to:
         *    RTP_MEMORY_ERROR if allocation of memory failed
         *    RTP_INVALID_VALUE if one of the parameters was invalid */
        rtcp_app_frame      *alloc_rtcp_app_frame(std::string name, uint8_t subtype, size_t payload_len);
        rtcp_sdes_frame     *alloc_rtcp_sdes_frame(size_t ssrc_count, size_t total_len);
        rtcp_receiver_frame *alloc_rtcp_receiver_frame(size_t nblocks);
        rtcp_sender_frame   *alloc_rtcp_sender_frame(size_t nblocks);
        rtcp_bye_frame      *alloc_rtcp_bye_frame(size_t ssrc_count);

        /* Deallocate RTP frame
         *
         * Return RTP_OK on successs
         * Return RTP_INVALID_VALUE if "frame" is nullptr */
        rtp_error_t dealloc_frame(uvg_rtp::frame::rtp_frame *frame);

        /* Deallocate ZRTP frame
         *
         * Return RTP_OK on successs
         * Return RTP_INVALID_VALUE if "frame" is nullptr */
        rtp_error_t dealloc_frame(uvg_rtp::frame::zrtp_frame *frame);

        /* Deallocate various types of RTCP frames
         *
         * Return RTP_OK on successs
         * Return RTP_INVALID_VALUE if "frame" is nullptr */
        rtp_error_t dealloc_frame(rtcp_sender_frame *frame);
        rtp_error_t dealloc_frame(rtcp_receiver_frame *frame);
        rtp_error_t dealloc_frame(rtcp_sdes_frame *frame);
        rtp_error_t dealloc_frame(rtcp_bye_frame *frame);
        rtp_error_t dealloc_frame(rtcp_app_frame *frame);
    };
};
