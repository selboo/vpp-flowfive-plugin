#include <vnet/plugin/plugin.h>
#include <vppinfra/format.h>
#include <vppinfra/vec.h>
#include <vppinfra/time_range.h>
#include <flowfive/flowfive.h>


typedef enum
{
    FLOWFIVE_NEXT_IP4,
    FLOWFIVE_DROP,
    FLOWFIVE_NEXT_N,
} flowfive_next_t;

flowfive_main_t flowfive_main;

static uword
flowfive_node_fn (vlib_main_t *vm, vlib_node_runtime_t *node, vlib_frame_t *frame) {
    u32 n_left_from, *from, *to_next;
    u32 now, now_five;
    flowfive_next_t next_index;
    flowfive_main_t *ffm = &flowfive_main;

    u8 *log_entry;

    from = vlib_frame_vector_args (frame);
    n_left_from = frame->n_vectors;
    next_index = node->cached_next_index;

    clib_timebase_t _tb, *tb = &_tb;
    clib_timebase_init (tb, +0 /* EST */ , CLIB_TIMEBASE_DAYLIGHT_NONE, &vm->clib_time);

    while (n_left_from > 0)
    {
        u32 n_left_to_next;
        vlib_get_next_frame (vm, node, next_index, to_next, n_left_to_next);

        while (n_left_from > 0 && n_left_to_next > 0)
        {
            u32 bi0;
            vlib_buffer_t *b0;

            bi0 = from[0];
            from += 1;
            n_left_from -= 1;

            b0 = vlib_get_buffer (vm, bi0);

            to_next[0] = bi0;
            to_next += 1;
            n_left_to_next -= 1;

            u8 *data = vlib_buffer_get_current (b0);
            u32 data_len = b0->current_length;

            if (data_len < sizeof(ethernet_header_t)) {
                // fformat(stderr, "Packet too short for Ethernet header\n");
                continue;
            }

            // 跳过以太网头部
            data += vnet_buffer(b0)->l2_hdr_offset;

            // 解析IPv4头部
            ip4_header_t *ip = (ip4_header_t *)data;
            u8 ip_version = ip->ip_version_and_header_length >> 4;

            if (ip_version == 4) {

                ip4_address_t src_ip = ip->src_address;
                ip4_address_t dst_ip = ip->dst_address;

                // 转换 IP 地址字节序
                u32 src_ip_host = src_ip.as_u32;
                u32 dst_ip_host = dst_ip.as_u32;

                // 获取 IP 包的总长度
                u16 ip_length = clib_net_to_host_u16(ip->length);

                // 协议
                u8 protocol = ip->protocol;

                u16 src_port = 0;   // 默认端口
                u16 dst_port = 0;   // 默认端口

                if (protocol == IP_PROTOCOL_IP_IN_IP) {
                    // Skip logging for IP-in-IP traffic
                    continue;
                }

                switch (protocol) {
                    case IP_PROTOCOL_TCP: {
                        tcp_header_t *tcp = (tcp_header_t *)(data + sizeof(ip4_header_t));
                        src_port = tcp->src_port;
                        dst_port = tcp->dst_port;
                        break;
                    }
                    case IP_PROTOCOL_UDP: {
                        udp_header_t *udp = (udp_header_t *)(data + sizeof(ip4_header_t));
                        src_port = udp->src_port;
                        dst_port = udp->dst_port;
                        break;
                    }
                    case IP_PROTOCOL_ICMP: {
                        // icmp46_header_t *icmp = (icmp46_header_t *)(data + sizeof(ip4_header_t));
                        // u8 icmp_type = icmp->type;
                        // u8 icmp_code = icmp->code;
                        src_port = 0;
                        dst_port = 0;
                        break;
                    }
                    default: {
                        src_port = 0;
                        dst_port = 0;
                        break;
                    }
                }

                now = clib_timebase_now (tb);
                now_five = now - now % 300;

                log_entry = vec_new(u8, MAX_LOG_ENTRY_SIZE);

                switch (ffm->log_type) {
                    case 1: {
                        log_entry = format(0, LOG_FORMAT_TXT_STRING,
                            ffm->rsyslog.id,
                            format_ip4_address, &src_ip_host, format_ip4_address, &dst_ip_host,
                            clib_net_to_host_u16(src_port), clib_net_to_host_u16(dst_port),
                            protocol, ip_length,
                            now, now_five);
                        break;
                    }
                    default: {
                        log_entry = format(0, LOG_FORMAT_JSON_STRING,
                            ffm->rsyslog.id,
                            format_ip4_address, &src_ip_host, format_ip4_address, &dst_ip_host,
                            clib_net_to_host_u16(src_port), clib_net_to_host_u16(dst_port),
                            protocol, ip_length,
                            now, now_five);
                        break;
                    }

                }

                if (svm_queue_add(ffm->queue, (u8 *)&log_entry, SVM_Q_NOWAIT) != 0) {
                    vec_free(log_entry);
                }

            }

        }

        vlib_put_next_frame (vm, node, next_index, n_left_to_next);
    }

    return frame->n_vectors;
}

// 注册 node
VLIB_REGISTER_NODE(flowfive_node) = {
    .function = flowfive_node_fn,
    .name = "flowfive",            // 名字很重要
    .vector_size = sizeof(u32),       // 很重要
    .type = VLIB_NODE_TYPE_INTERNAL,
    .n_next_nodes = FLOWFIVE_NEXT_N,
    .next_nodes = {
        [FLOWFIVE_NEXT_IP4] = "ip4-lookup",
        [FLOWFIVE_DROP] = "error-drop",
    },
};
