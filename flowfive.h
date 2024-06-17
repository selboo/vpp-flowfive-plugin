#ifndef __included_flowfive_h__
#define __included_flowfive_h__

#include <vnet/vnet.h>
#include <vnet/ip/ip.h>
#include <vppinfra/hash.h>
#include <vppinfra/error.h>
#include <vppinfra/elog.h>
#include <vppinfra/vector.h>
#include <string.h>
#include <vlibmemory/api.h>
#include <vppinfra/ring.h>


#define CACHE_SIZE_LIMIT 8192
#define MAX_LOG_ENTRY_SIZE 4
#define CACHE_BATCH_SIZE 32

// 记录开启状态
typedef struct {
    u32 sw_if_index;
    int enabled;
} interface_t;

// rsyslog 相关参数
typedef struct {
    char *host;
    i32 port;
    i32 facility;
    i32 severity;
    i32 id;
    i32 sockfd;
} rsyslog_t;

typedef struct {

    vnet_main_t *vnet_main;
    rsyslog_t rsyslog;

    struct sockaddr_in servaddr;
    struct sockaddr *servaddr_ptr;
    u64 servaddr_size;

    u32 cache_size;
    u32 log_type;   // 1 文本,  其他 json

    svm_queue_t *queue;

    interface_t *interfaces;
} flowfive_main_t;

extern flowfive_main_t flowfive_main;
extern vlib_node_registration_t flowfive_node;

void flush_cache(flowfive_main_t *sm);
void send_log(flowfive_main_t *sm);

#define FLOWFIVE_PLUGIN_BUILD_VER "1.9"
#define FLOWFIVE_PLUGIN_DESCRIPTION "Mirror network to rsyslog"

#define LOG_FORMAT_TXT_STRING "<%d> %U %U %d %d %d %d %d %d\n"
#define LOG_FORMAT_JSON_STRING "<%d> {\"src_ip\": \"%U\", \"dst_ip\": \"%U\", \"src_port\": %d, \"dst_port\": %d, \"proto\": %d, \"len\": %d, \"timestamp\": %d, \"timefive\": %d}\n"

#endif /* __included_flowfive_h__ */
