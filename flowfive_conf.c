#include <vnet/plugin/plugin.h>
#include <flowfive/flowfive.h>
#include <arpa/inet.h>


// 解析配置
static clib_error_t *
flowfive_config_fn (vlib_main_t *vm, unformat_input_t *input) {
    flowfive_main_t *ffm = &flowfive_main;

    // rsyslog
    ffm->rsyslog.host = "127.0.0.1";
    ffm->rsyslog.port = 514;
    ffm->rsyslog.facility = 16;
    ffm->rsyslog.severity = 6;
    ffm->rsyslog.id = 0;

    // cache size
    ffm->cache_size = CACHE_SIZE_LIMIT;
    ffm->log_type = 1;

    while (unformat_check_input(input) != UNFORMAT_END_OF_INPUT) {
        if (     unformat (input, "rsyslog-host %s",     &ffm->rsyslog.host));
        else if (unformat (input, "rsyslog-port %d",     &ffm->rsyslog.port));
        else if (unformat (input, "rsyslog-facility %d", &ffm->rsyslog.facility));
        else if (unformat (input, "rsyslog-severity %d", &ffm->rsyslog.severity));
        else if (unformat (input, "cache-size %d",       &ffm->cache_size));
        else if (unformat (input, "log-type %d",         &ffm->log_type));
        else
        break;
    }

    ffm->rsyslog.id = (ffm->rsyslog.facility << 3) + ffm->rsyslog.severity;

    if (!ffm->rsyslog.host) {
        return clib_error_return (0, "rsyslog-host must be specified");
    }

    // memset TODO DPDK
    clib_memset(&ffm->servaddr, 0, sizeof(ffm->servaddr));
    ffm->servaddr.sin_family = AF_INET;
    ffm->servaddr.sin_port = htons(ffm->rsyslog.port);
    ffm->servaddr.sin_addr.s_addr = inet_addr(ffm->rsyslog.host);

    ffm->servaddr_ptr = (struct sockaddr *)&ffm->servaddr;
    ffm->servaddr_size = sizeof(ffm->servaddr);

    if ((ffm->rsyslog.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        clib_error("FlowFive Rsyslog Socket creation failed");
        return 0;
    }
    // memset TODO DPDK

    // queue
    svm_queue_t *q = svm_queue_alloc_and_init(ffm->cache_size, sizeof(vlib_buffer_t *), 0);
    ffm->queue = q;
    // queue

    return 0;
}

// 注册->解析配置
VLIB_CONFIG_FUNCTION (flowfive_config_fn, "flowfive");

