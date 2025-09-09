#ifndef __OPENAMP_VIRTIO_DEVICE_H__
#define __OPENAMP_VIRTIO_DEVICE_H__

#include "metal/device.h"
#include "metal/sys.h"
#include "openamp/open_amp.h"
#include "openamp/virtio.h"
#include "platform_port.h"
#include <metal/utilities.h>
#include <stdio.h>

#define OP_ALIGN(size, align) ((uintptr_t)((size) + (align) - 1) & (~((uintptr_t)(align) - 1)))

#ifndef OPENAMP_APP_NODE_NAME_MAXLEN
#define OPENAMP_APP_NODE_NAME_MAXLEN 20
#endif

struct openamp_app_node {
    struct rpmsg_endpoint ept;
    char                  name[OPENAMP_APP_NODE_NAME_MAXLEN];
    unsigned int          src;
    unsigned int          dest;
    rpmsg_ept_cb          cb;
    rpmsg_ns_unbind_cb    unbind_cb;
    struct metal_list     node_list;
};
typedef struct openamp_app_node *openamp_app_node_t;

typedef struct secondary_config {
    unsigned int cpu_id;
    long long    entry;
} secondary_config;
typedef secondary_config *secondary_config_t;

struct amp_sigiface_ops {
    int (*sigiface_init)(void *dev);
    int (*sigiface_send)(int sigiface_id, rt_uint8_t remote_cpu);
};
typedef struct amp_sigiface_ops *amp_sigiface_ops_t;

struct openamp_virtio_device {
    struct rpmsg_virtio_device   rvdev;
    struct virtio_device         vdev;
    struct rpmsg_endpoint       *ept;
    rpmsg_ns_bind_cb             ns_bind_cb;
    struct rpmsg_virtio_shm_pool shpool;
    struct virtio_vring_info     vrings[2];
    void                        *shm_physaddr;
    void                        *shm_vaddr;
    unsigned int                 shm_size;
    secondary_config             secondary_config;
    unsigned long long           vring_tx;
    unsigned int                 tx_num;
    unsigned long long           vring_rx;
    unsigned int                 rx_num;
    unsigned long long           vring_buf;
    unsigned int                 vring_size;
    unsigned long long           align_size;
    unsigned int                 vdev_status_size;
    unsigned int                 local_sgi_id;
    unsigned int                 remote_sgi_id;
    amp_sigiface_ops_t           sigiface_ops;
    int (*auto_node_register)(const char *name);
    void *user_data;
};
typedef struct openamp_virtio_device *openamp_virtio_device_t;

int                     openamp_dev_create(openamp_virtio_device_t dev);
int                     openamp_dev_delete(openamp_virtio_device_t dev);
openamp_virtio_device_t openamp_dev_get(void);
struct virtqueue       *openamp_get_vq(openamp_virtio_device_t dev);
int                     openamp_virqueue_notification(struct virtqueue *vq);
openamp_app_node_t      openamp_app_node_register(const char *name, rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb, openamp_virtio_device_t dev);
int                     openamp_app_node_unregister(const char *name);
openamp_app_node_t      openamp_find_app_node(const char *name);
void                    openamp_dump_app_node(void);
int                     openamp_app_send(struct rpmsg_endpoint *ept, const void *data, size_t len);
struct metal_io_region *openamp_shmem_init(unsigned long long vaaddr, unsigned long long physaddr, unsigned int shm_size);

#endif /* __OPENAMP_VIRTIO_DEVICE_H__ */