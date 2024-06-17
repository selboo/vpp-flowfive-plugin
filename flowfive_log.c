#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <vppinfra/vec.h>
#include <flowfive/flowfive.h>


void *flowfive_log_thread(void *arg) {
    flowfive_main_t *ffm = &flowfive_main;
    u8 *res;
    u8 *batch[CACHE_BATCH_SIZE];
    i32 batch_count;

    while (!ffm->queue) {
        usleep(1000000); // 1秒
    }

    while (1) {
        batch_count = 0;

        while (batch_count < CACHE_BATCH_SIZE && svm_queue_sub(ffm->queue, (u8 *)&res, SVM_Q_WAIT, ~0) == 0) {
            batch[batch_count++] = res;
        }

        // 发送批量日志条目
        for (int i = 0; i < batch_count; ++i) {
            sendto(
                ffm->rsyslog.sockfd, batch[i], strlen((char *)batch[i]), 0,
                ffm->servaddr_ptr, ffm->servaddr_size
            );
            vec_free(batch[i]);
        }

    }
    return NULL;
}

static clib_error_t *
flowfive_log_init(vlib_main_t *vm) {
    pthread_t my_thread;

    int rc = pthread_create(&my_thread, NULL, flowfive_log_thread, NULL);
    if (rc) {
        return clib_error_return(0, "Failed to create thread");
    }
    pthread_detach(my_thread);

    return 0;
}

VLIB_INIT_FUNCTION(flowfive_log_init);


