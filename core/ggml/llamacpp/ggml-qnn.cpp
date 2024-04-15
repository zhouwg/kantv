/*
 * MIT license
 * Copyright (C) 2024 KanTV Authors
 * SPDX-License-Identifier: MIT
 *
 * this is implementation of ggml QNN(Qualcomm Nerual Network, aka AI Engine Direct) backend
 *
 * status: core implementation(data path) has been completed on 04/13/2024
 *         major GGML OP(mulmat using QNN CPU backend) has been completed
 *         lack of implementation of QNN GPU backend
 *         lack of implementation of QNN DSP backend
 *         lack of implementation of other GGML-OPs using QNN API
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <tuple>
#include <queue>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <memory>
#include <regex>
#include <random>
#include <functional>
#include <unordered_map>
#include <condition_variable>
#include <cassert>
#include <unordered_set>
#include <utility>

#include "QnnTypes.h"
#include "QnnCommon.h"
#include "QnnContext.h"
#include "QnnBackend.h"
#include "QnnGraph.h"
#include "QnnProperty.h"
#include "QnnTensor.h"
#include "QnnInterface.h"
#include "Saver/QnnSaver.h"
#include "System/QnnSystemInterface.h"
#include "HTP/QnnHtpDevice.h"

#include "ggml-qnn.h"
#include "ggml-jni.h" //for validation purpose during development stage, should be removed before PR to upstream GGML/whisper.cpp

#include "ggml-backend-impl.h"


#define GGML_QNN_LOGBUF_LEN                             4096
#define RPCMEM_DEFAULT_FLAGS                            1
#define RPCMEM_HEAP_ID_SYSTEM                           25
#define QNN_VER_PTR(x)                                  (&((x).v1))
#define TENSOR_DUMP(tensor)                             tensor_dump(tensor, #tensor)
#define QNN_OP_CFG_VALID(opConfig)                      ((opConfig).version == QNN_OPCONFIG_VERSION_1)


#define GGML_QNN_MAX_BUFFERS                            128
#define MATRIX_ROW_PADDING                              512

#define BUF_MAJOR_MASK                                  0xFF000000
#define BUF_CONTROL_BASE                                0xEE000000


using pfn_rpc_mem_alloc = void *(*)(int, uint32_t, int);
using pfn_rpc_mem_free = void (*)(void *);
using pfn_rpc_mem_to_fd = int (*)(void *);

using _pfn_QnnSaver_initialize = decltype(QnnSaver_initialize);
using _pfn_QnnInterface_getProviders = decltype(QnnInterface_getProviders);
using _pfn_QnnSystemInterface_getProviders = decltype(QnnSystemInterface_getProviders);

template<typename Fn>
Fn load_qnn_functionpointers(void * handle, const char * function_name) {
    return reinterpret_cast<Fn>(dlsym(handle, function_name));
}


class qnn_instance;
typedef struct qnn_buf_s qnn_buf_t;
typedef struct qnn_buf_s qnn_buf_buffer_t;
typedef struct buf_element_s buf_element_t;
typedef void (*ggml_qnn_func_t)(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst);

enum class ggml_qnn_profile_level {
    profile_off = 0,
    profile_basic = 1,
    profile_detail = 2
};


struct buf_element_s
{
    buf_element_t        * next;

    unsigned char        * mem;
    unsigned char        * content;   /* start of raw content in mem  */

    uint32_t              size ;      /* size of content  */
    int32_t               max_size;   /* size of pre-allocated memory pointed to by mem   */
    uint32_t              type;
    void (*free_buffer) (buf_element_t * buf);
    void                 * source;   /* CPU, GPU, DSP, ... */
    int                   id;
} ;


struct qnn_buf_s
{
    buf_element_t  * first, * last;

    size_t          qnn_buf_size;
    uint32_t        qnn_buf_data_size;
    void            * qnn_buf_empty_cb_data;
    const           char * name;

    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;

    void (*put) (qnn_buf_t * fifo, buf_element_t * buf);

    buf_element_t *(*get) (qnn_buf_t * fifo);

    void (*clear) (qnn_buf_t * fifo) ;

    int (*size) (qnn_buf_t * fifo);

    int (*num_free) (qnn_buf_t * fifo);

    uint32_t (*data_size) (qnn_buf_t * fifo);

    void (*destroy) (qnn_buf_t * fifo);

    buf_element_t * (*buffer_alloc) (qnn_buf_t * self);

    buf_element_t * (*buffer_try_alloc) (qnn_buf_t * self);

    buf_element_t   * buffer_pool_top;
    pthread_mutex_t  buffer_pool_mutex;
    pthread_cond_t   buffer_pool_cond_not_empty;
    int              buffer_pool_num_free;
    int              buffer_pool_capacity;
    int              buffer_pool_buf_size;
    void            * buffer_pool_base; /* used to free mem pool */
} ;


struct ggml_backend_qnn_context {
    int device;
    char name[GGML_MAX_NAME];
    char lib[GGML_MAX_NAME];
    qnn_instance * instance;
    qnn_buf_t * buffer_pool;
    QNN_INTERFACE_VER_TYPE raw_interface;
    QNN_SYSTEM_INTERFACE_VER_TYPE raw_system_interface;
} ;


//TODO: not real backend buffer(aka not real QNN buffer),
//      there are various memory / various memory type in an embedded system
//      (the Android phone is also an embedded system):
//      system memory, device memory(GPU memory, DSP memory...), TEE memory...,
//      on-screen memory, off-screen memory,
//      ...
//      here I use system memory to simulate backend buffer(GPU buffer, DSP buffer...)
//      and study the internal mechanism of ggml
//      in the HLD stage(PoC-S41: HLD of ggml-qnn backend,
//      https://github.com/zhouwg/kantv/issues/121)
struct ggml_backend_qnn_buffer_context {
    ~ggml_backend_qnn_buffer_context() {
        if (buffer) {
            free(buffer);
        }
        for (auto * sub_buffer : sub_buffers) {
            free(sub_buffer);
        }
    }
    void * buffer       = nullptr;
    size_t buffer_size  = 0;
    std::vector<void *> sub_buffers;
};

//TODO: should be removed in the future
static bool g_qnn_loaded = false;

//TODO: this global static var is used to adapt to whisper.cpp because whisper.cpp will
// calling whisper_backend_init many times
//
// I have no idea how to handle it properly at the moment
static ggml_backend_t g_qnn_backend = nullptr;

//TODO: should be removed in the future, just used to tracking ggml internal
static int i_alloc_buffer_counts    = 0;
static long i_alloc_buffer_sizes    = 0;
static int i_get_alloc_counts       = 0;
static long i_get_alloc_sizes       = 0;

//use a prebuild static memory layout to avoid complex resource management, this method also used
//in GGML internal or FFmpeg
static struct ggml_backend_qnn_context g_qnn_mgr[GGML_QNN_MAX_DEVICES] = {
        [QNN_CPU]   = {.device = 0, .name =   "qnn-cpu", .lib = "libQnnCpu.so", .instance = nullptr, .buffer_pool = nullptr, .raw_interface = nullptr, .raw_system_interface = nullptr},
        [QNN_GPU]   = {.device = 1, .name =   "qnn-gpu", .lib = "libQnnGpu.so", .instance = nullptr, .buffer_pool = nullptr, .raw_interface = nullptr, .raw_system_interface = nullptr},
        [QNN_HTP]   = {.device = 2, .name =   "qnn-htp", .lib = "libQnnHtp.so", .instance = nullptr, .buffer_pool = nullptr, .raw_interface = nullptr, .raw_system_interface = nullptr},
};


static float tensor_sum_elements(const ggml_tensor * tensor) {
    double sum = 0;
    float  value = 0;
    std::ostringstream tmposs;
    if (tensor->type == GGML_TYPE_F32) {
        for (int h = 0; h < tensor->ne[3]; h++) {
            for (int i = 0; i < tensor->ne[2]; i++) {
                for (int j = 0; j < tensor->ne[1]; j++) {
                    for (int k = 0; k < tensor->ne[0]; k++) {
                        value = ((float *) tensor->data)[h * tensor->ne[2] + i * tensor->ne[1] + j * tensor->ne[0] + k];
                        sum += value;
                        //LOGGD("[%d][%d][%d][%d]%.2f \t", h, i, j, k, value);
                        tmposs << std::setw(8) << std::fixed << std::setprecision(2) << value << "\t";
                    }
                    if (strlen(tmposs.str().c_str()) > 4000) {

                    } else {
                        LOGGD("%s", tmposs.str().c_str());
                    }
                    tmposs.clear();
                    tmposs.str("");
                    LOGGD("\n");
                }
            }
        }
    }
    LOGGD("\n");
    return sum;
}


static void tensor_dump(const ggml_tensor * tensor, const char * name) {
    LOGGD("dump ggml tensor %s\n", name);
    LOGGD("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", name,
          tensor->type, ggml_type_name(tensor->type),
          tensor->ne[0], tensor->ne[1], tensor->ne[2], tensor->nb[0], tensor->nb[1], tensor->nb[2]);
    float sum = tensor_sum_elements(tensor);

    //LOGGD("\n");
    //LOGGD("Sum of tensor %s is %6.2f\n", name, sum);
}


static uint32_t get_tensor_rank(const ggml_tensor * tensor) {
    uint32_t rank = 0;
    for (int i = 0; i < GGML_MAX_DIMS; i++) {
        if (0 != tensor->ne[0]) {
            rank++;
        }
    }

    return rank;
}

static uint32_t get_tensor_data_size(const ggml_tensor * tensor) {
    /*
    size_t data_size = ggml_row_size(tensor->type, tensor->ne[0]);
    size_t n_dims = get_tensor_rank(tensor);
    for (int i = 1; i < n_dims; i++) {
        data_size *= tensor->ne[i];
    }

    return data_size;
     */
    return ggml_nbytes(tensor);
}


static void qnn_xfree(void * ptr) {
    if (nullptr != ptr) {
        free(ptr);
        ptr = nullptr;
    }
}


static void * qnn_xmalloc(size_t size) {
    void * ptr;

    if (!size)
        size++;

    if ((ptr = calloc(1, size)) == nullptr) {
        LOGGW("malloc(%d) failed: %s\n",size, strerror(errno));
        return nullptr;
    }

    return ptr;
}


static void * qnn_xmalloc_aligned(size_t alignment, size_t size, void ** base) {
    char * ptr;

    *base = ptr = static_cast<char *>(qnn_xmalloc(size + alignment));

    while ((size_t) ptr % alignment)
        ptr++;

    return ptr;
}


// put a previously allocated buffer element back into the buffer pool
static void buffer_pool_free (buf_element_t * element) {
    qnn_buf_t * self = (qnn_buf_t *) element->source;

    pthread_mutex_lock(&self->buffer_pool_mutex);

    element->next = self->buffer_pool_top;
    self->buffer_pool_top = element;

    self->buffer_pool_num_free++;
    if (self->buffer_pool_num_free > self->buffer_pool_capacity) {
        LOGGD("TOO MANY FREE\n");
    }

    pthread_cond_signal (&self->buffer_pool_cond_not_empty);

    pthread_mutex_unlock (&self->buffer_pool_mutex);
}


static buf_element_t * buffer_pool_alloc (qnn_buf_t * self) {
    buf_element_t * buf = nullptr;
    int i;

    pthread_mutex_lock (&self->buffer_pool_mutex);

    while (self->buffer_pool_num_free < 2) {
        pthread_cond_wait (&self->buffer_pool_cond_not_empty, &self->buffer_pool_mutex);
    }

    buf = self->buffer_pool_top;
    self->buffer_pool_top = self->buffer_pool_top->next;
    self->buffer_pool_num_free--;

    buf->content = buf->mem;
    buf->size = 0;
    buf->type = 0;

    pthread_mutex_unlock (&self->buffer_pool_mutex);

    return buf;
}


static buf_element_t * buffer_pool_try_alloc (qnn_buf_t * self) {
    buf_element_t * buf = nullptr;

    pthread_mutex_lock (&self->buffer_pool_mutex);

    if (self->buffer_pool_top) {
        buf = self->buffer_pool_top;
        self->buffer_pool_top = self->buffer_pool_top->next;
        self->buffer_pool_num_free--;
    } else {
        buf = nullptr;
    }

    pthread_mutex_unlock (&self->buffer_pool_mutex);

    if (buf) {
        buf->content = buf->mem;
        buf->size = 0;
    }

    return buf;
}


static void qnn_buf_buffer_put(qnn_buf_t * fifo, buf_element_t * element) {
    int i;

    pthread_mutex_lock (&fifo->mutex);

    if (fifo->last)
        fifo->last->next = element;
    else
        fifo->first = element;

    fifo->last = element;
    element->next = nullptr;
    fifo->qnn_buf_size++;
    fifo->qnn_buf_data_size += element->size;

    LOGJ("put:index %d, fifo->size is %d, self->buffer_pool_num_free %d\n", element->id, fifo->qnn_buf_size, fifo->buffer_pool_num_free);
    pthread_cond_signal (&fifo->not_empty);

    pthread_mutex_unlock (&fifo->mutex);
}


static buf_element_t * qnn_buf_buffer_get (qnn_buf_t * fifo)
{
    int i;
    buf_element_t *buf;

    pthread_mutex_lock (&fifo->mutex);

#if 0
    while (fifo->first == nullptr) {
        pthread_cond_wait (&fifo->not_empty, &fifo->mutex);
    }
#else
    if (fifo->first == nullptr) {
        pthread_mutex_unlock (&fifo->mutex);
        return nullptr;
    }
#endif

    buf = fifo->first;

    fifo->first = fifo->first->next;
    if (fifo->first==nullptr)
        fifo->last = nullptr;

    fifo->qnn_buf_size--;
    fifo->qnn_buf_data_size -= buf->size;

    pthread_mutex_unlock (&fifo->mutex);

    return buf;
}


static void qnn_buf_buffer_clear (qnn_buf_t * fifo) {
    buf_element_t * buf, * next, * prev;

    pthread_mutex_lock (&fifo->mutex);

    buf = fifo->first;
    prev = nullptr;

    while (buf != nullptr) {
        next = buf->next;
        if ((buf->type & BUF_MAJOR_MASK) !=  BUF_CONTROL_BASE) {
            if (prev)
                prev->next = next;
            else
                fifo->first = next;

            if (!next)
                fifo->last = prev;

            fifo->qnn_buf_size--;
            fifo->qnn_buf_data_size -= buf->size;

            buf->free_buffer(buf);
        } else {
            prev = buf;
        }

        buf = next;
    }

    LOGGD("free buffers after clear: %d\n", fifo->buffer_pool_num_free);
    pthread_mutex_unlock (&fifo->mutex);
}


static int qnn_buf_buffer_size (qnn_buf_t * self) {
    int size = 0;

    pthread_mutex_lock(&self->mutex);
    size = self->qnn_buf_size;
    pthread_mutex_unlock(&self->mutex);

    return size;
}


static uint32_t qnn_buf_buffer_data_size (qnn_buf_t * self) {
    uint32_t data_size;

    pthread_mutex_lock(&self->mutex);
    data_size = self->qnn_buf_data_size;
    pthread_mutex_unlock(&self->mutex);

    return data_size;
}


static int qnn_buf_buffer_num_free (qnn_buf_t * self) {
    int buffer_pool_num_free = 0;

    pthread_mutex_lock(&self->mutex);
    buffer_pool_num_free = self->buffer_pool_num_free;
    pthread_mutex_unlock(&self->mutex);

    return buffer_pool_num_free;
}


static void qnn_buf_buffer_dispose (qnn_buf_t * self) {
    ENTER_FUNC();
    buf_element_t * buf, * next;
    int received = 0;

    self->clear( self );
    buf = self->buffer_pool_top;

    while (buf != nullptr) {
        next = buf->next;
        qnn_xfree(buf);
        received++;

        buf = next;
    }

    while (received < self->buffer_pool_capacity) {
        buf = self->get(self);
        qnn_xfree(buf);
        received++;
    }

    qnn_xfree(self->buffer_pool_base);
    pthread_mutex_destroy(&self->mutex);
    pthread_cond_destroy(&self->not_empty);
    pthread_mutex_destroy(&self->buffer_pool_mutex);
    pthread_cond_destroy(&self->buffer_pool_cond_not_empty);
    qnn_xfree((void *)self->name);
    qnn_xfree (self);

    LEAVE_FUNC();
}


qnn_buf_t * qnn_buf_new (const char * name, int num_buffers, uint32_t buf_size) {
    int    i                = 0;
    int    alignment        = 4;
    qnn_buf_t * self        = nullptr;
    uint8_t  * multi_buffer = nullptr;

    self = (qnn_buf_t*)qnn_xmalloc(sizeof(qnn_buf_t));
    if (nullptr == self) {
        LOGGW("malloc memory failed\n");
        return nullptr;
    }

    self->name                = strdup(name);
    self->first               = nullptr;
    self->last                = nullptr;
    self->qnn_buf_size        = 0;
    self->put                 = qnn_buf_buffer_put;
    self->get                 = qnn_buf_buffer_get;
    self->clear               = qnn_buf_buffer_clear;
    self->size                = qnn_buf_buffer_size;
    self->num_free            = qnn_buf_buffer_num_free;
    self->data_size           = qnn_buf_buffer_data_size;
    self->destroy             = qnn_buf_buffer_dispose;
    pthread_mutex_init (&self->mutex, nullptr);
    pthread_cond_init (&self->not_empty, nullptr);


    if (buf_size % alignment != 0)
        buf_size += alignment - (buf_size % alignment);

    LOGGI("[%s]allocating %d Mbytes memory(alignment = %d)\n", name, (num_buffers * buf_size) / (1 << 20), alignment);

    multi_buffer = (uint8_t *)qnn_xmalloc_aligned (alignment, num_buffers * buf_size, &self->buffer_pool_base);
    if (nullptr == multi_buffer) {
        LOGGW("malloc memory failed\n");
        free(self);
        return nullptr;
    }

    self->buffer_pool_top       = nullptr;

    pthread_mutex_init (&self->buffer_pool_mutex, nullptr);
    pthread_cond_init (&self->buffer_pool_cond_not_empty, nullptr);

    self->buffer_pool_num_free  = 0;
    self->buffer_pool_capacity  = num_buffers;
    self->buffer_pool_buf_size  = buf_size;
    self->buffer_alloc          = buffer_pool_alloc;
    self->buffer_try_alloc      = buffer_pool_try_alloc;

    for (i = 0; i < num_buffers; i++) {
        buf_element_t * buf = nullptr;

        buf = (buf_element_t *)qnn_xmalloc(sizeof (buf_element_t));
        if (nullptr == buf) {
            LOGGW("malloc memory failed");
            free(multi_buffer);
            free(self);
            return nullptr;
        }

        buf->id          = i;
        buf->mem         = multi_buffer;
        multi_buffer     += buf_size;

        buf->max_size    = buf_size;
        buf->free_buffer = buffer_pool_free;
        buf->source      = self;

        buffer_pool_free(buf);
    }

    return self;
}


static const char * get_qnn_backend_name(int n_backend_type) {
    switch (n_backend_type) {
        case 0:
            return "QNN-CPU";
        case 1:
            return "QNN-GPU";
        case 2:
            return "QNN-HTP";
        case 3:
            return "QNN-cDSP";
        case 4:
            return "QNN-HTA";

        default:
            return "unknown";
    }
}


static intptr_t align_to(size_t alignment, intptr_t offset) {
    return offset % alignment == 0 ? offset
                                   : offset +
                                     (static_cast<intptr_t>(alignment) -
                                      offset % static_cast<intptr_t>(alignment));
}


// =================================================================================================
//
//  wrapper class of Qualcomm QNN(Qualcomm Neural Network, aka Qualcomm AI Engine Direct) SDK
//
// =================================================================================================
class qnn_interface {

#define DEFINE_SHIM_FUNCTION_INTERFACE(F, pointer_name)           \
  template <typename... Args>                                     \
  inline auto qnn_##F(Args... args) const {                       \
    return (_qnn_interface->QNN_INTERFACE_VER_NAME.pointer_name)( \
        std::forward<Args>(args)...);                             \
  }


#define DEFINE_SHIM_FUNCTION_SYS_INTERFACE(F, pointer_name)                  \
  template <typename... Args>                                                \
  inline auto qnn_##F(Args... args) const {                                  \
    return (_qnn_sys_interface->QNN_SYSTEM_INTERFACE_VER_NAME.pointer_name)( \
        std::forward<Args>(args)...);                                        \
  }

    friend class qnn_instance;

public:
    qnn_interface() = default;

    // QnnBackend
    DEFINE_SHIM_FUNCTION_INTERFACE(backend_create, backendCreate);

    DEFINE_SHIM_FUNCTION_INTERFACE(backend_free, backendFree);

    DEFINE_SHIM_FUNCTION_INTERFACE(backend_register_op_package, backendRegisterOpPackage);

    DEFINE_SHIM_FUNCTION_INTERFACE(backend_validate_op_config, backendValidateOpConfig);

    DEFINE_SHIM_FUNCTION_INTERFACE(backend_get_api_version, backendGetApiVersion);

    // QnnDevice
    DEFINE_SHIM_FUNCTION_INTERFACE(device_create, deviceCreate);

    DEFINE_SHIM_FUNCTION_INTERFACE(device_free, deviceFree);

    DEFINE_SHIM_FUNCTION_INTERFACE(device_get_infrastructure, deviceGetInfrastructure);

    DEFINE_SHIM_FUNCTION_INTERFACE(device_get_platform_info, deviceGetPlatformInfo);

    DEFINE_SHIM_FUNCTION_INTERFACE(device_get_info, deviceGetInfo);

    // QnnContext
    DEFINE_SHIM_FUNCTION_INTERFACE(context_create, contextCreate);

    DEFINE_SHIM_FUNCTION_INTERFACE(context_get_binary_size, contextGetBinarySize);

    DEFINE_SHIM_FUNCTION_INTERFACE(context_get_binary, contextGetBinary);

    DEFINE_SHIM_FUNCTION_INTERFACE(context_create_from_binary, contextCreateFromBinary);

    DEFINE_SHIM_FUNCTION_INTERFACE(context_free, contextFree);

    // QnnGraph
    DEFINE_SHIM_FUNCTION_INTERFACE(graph_create, graphCreate);

    DEFINE_SHIM_FUNCTION_INTERFACE(graph_add_node, graphAddNode);

    DEFINE_SHIM_FUNCTION_INTERFACE(graph_finalize, graphFinalize);

    DEFINE_SHIM_FUNCTION_INTERFACE(graph_execute, graphExecute);

    DEFINE_SHIM_FUNCTION_INTERFACE(graph_retrieve, graphRetrieve);

    // QnnLog
    DEFINE_SHIM_FUNCTION_INTERFACE(log_create, logCreate);

    DEFINE_SHIM_FUNCTION_INTERFACE(log_free, logFree);

    DEFINE_SHIM_FUNCTION_INTERFACE(log_set_log_level, logSetLogLevel);

    // QnnProfile
    DEFINE_SHIM_FUNCTION_INTERFACE(profile_create, profileCreate);

    DEFINE_SHIM_FUNCTION_INTERFACE(profile_get_events, profileGetEvents);

    DEFINE_SHIM_FUNCTION_INTERFACE(profile_get_sub_events, profileGetSubEvents);

    DEFINE_SHIM_FUNCTION_INTERFACE(profile_get_event_data, profileGetEventData);

    DEFINE_SHIM_FUNCTION_INTERFACE(profile_free, profileFree);

    // QnnMem
    DEFINE_SHIM_FUNCTION_INTERFACE(mem_register, memRegister);

    DEFINE_SHIM_FUNCTION_INTERFACE(mem_de_register, memDeRegister);

    // QnnProperty
    DEFINE_SHIM_FUNCTION_INTERFACE(property_has_capability, propertyHasCapability);

    // QnnTensor
    DEFINE_SHIM_FUNCTION_INTERFACE(tensor_create_context_tensor, tensorCreateContextTensor);

    DEFINE_SHIM_FUNCTION_INTERFACE(tensor_create_graph_tensor, tensorCreateGraphTensor);

    // QnnSystem
    DEFINE_SHIM_FUNCTION_SYS_INTERFACE(system_context_create, systemContextCreate);

    DEFINE_SHIM_FUNCTION_SYS_INTERFACE(system_context_get_binary_info, systemContextGetBinaryInfo);

    DEFINE_SHIM_FUNCTION_SYS_INTERFACE(system_context_free, systemContextFree);

    void set_qnn_interface(const QnnInterface_t * qnn_interface) {
        _qnn_interface = qnn_interface;
    }

    void set_qnn_system_interface(const QnnSystemInterface_t * qnn_sys_interface) {
        _qnn_sys_interface = qnn_sys_interface;
    }

    uint32_t get_backend_id() const {
        return _qnn_interface->backendId;
    }

    bool is_loaded() const {
        return ((_qnn_sys_interface != nullptr) && (_qnn_interface != nullptr));
    }

private:
    const QnnInterface_t *_qnn_interface = nullptr;

    const QnnSystemInterface_t *_qnn_sys_interface = nullptr;
};


class qnn_instance {
public:
    using BackendIdType = decltype(QnnInterface_t{}.backendId);

    explicit qnn_instance(const std::string & lib_path, const std::string & backend_name,
                                const std::string & model_name) :
            _lib_path(std::move(lib_path)),
            _backend_name(std::move(backend_name)),
            _model_name(std::move(model_name)) {};

    ~qnn_instance() {

    }

    int qnn_init(const QnnSaver_Config_t ** saver_config);

    int qnn_finalize();

    const qnn_interface &get_qnn_interface() {
        if (!_qnn_interface.is_loaded()) {
            LOGGW("pls check why _qnn_interface is not loaded\n");
        }
        return _qnn_interface;
    }


    const QNN_INTERFACE_VER_TYPE &get_qnn_raw_interface() {
        if (!_qnn_interface.is_loaded()) {
            LOGGW("pls check why _qnn_interface is not loaded\n");
        }
        return _qnn_raw_interface;
    }

    const QNN_SYSTEM_INTERFACE_VER_TYPE &get_qnn_raw_system_interface() {
        if (!_qnn_interface.is_loaded()) {
            LOGGW("pls check why _qnn_interface is not loaded\n");
        }
        return _qnn_raw_system_interface;
    }

    const Qnn_LogHandle_t get_qnn_log_handle() { return _qnn_log_handle; }

    const Qnn_ProfileHandle_t get_qnn_profile_handle() { return _qnn_profile_handle; }

    const Qnn_DeviceHandle_t get_qnn_device_handle() { return _qnn_device_handle; }

    const Qnn_BackendHandle_t get_qnn_backend_handle() { return _qnn_backend_handle; }

    const Qnn_ContextHandle_t get_qnn_context_handle() { return _qnn_context_handle; }

    const QnnSystemContext_Handle_t get_qnn_system_handle() { return _qnn_system_handle; }

    const Qnn_GraphHandle_t get_qnn_graph_handle() { return _qnn_graph_handle; }


    int init_qnn_model(const char * graph_name,
                       bool debug,
                       uint8_t do_node_validation = 1,
                       const QnnGraph_Config_t ** graph_configs = nullptr
    );

    int finalize_qnn_model();

    int init_htp_perfinfra() {
        QnnDevice_Infrastructure_t device_infra = nullptr;
        int error = _qnn_raw_interface.deviceGetInfrastructure(&device_infra);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to get qnn device infra\n");
            return 1;
        }

        QnnHtpDevice_Infrastructure_t *htp_infra = static_cast<QnnHtpDevice_Infrastructure_t *>(device_infra);
        QnnHtpDevice_PerfInfrastructure_t *htp_perfinfra = &htp_infra->perfInfra;
        uint32_t power_configid = 1;
        uint32_t device_id = 0;
        uint32_t core_id = 0;
        htp_perfinfra->createPowerConfigId(device_id, core_id, &power_configid);
        _qnn_htp_perfinfra = htp_perfinfra;
        _qnn_power_configid = power_configid;

        return 0;
    }


    int set_rpc_polling() {
        if (_qnn_rpc_pollingtime > 0) {
            QnnHtpPerfInfrastructure_PowerConfig_t rpc_pollingTime;
            memset(&rpc_pollingTime, 0, sizeof(rpc_pollingTime));
            rpc_pollingTime.option =
                    QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_RPC_POLLING_TIME;
            rpc_pollingTime.rpcPollingTimeConfig = _qnn_rpc_pollingtime;
            const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = {&rpc_pollingTime, nullptr};
            if (_qnn_htp_perfinfra) {
                _qnn_htp_perfinfra->setPowerConfig(_qnn_power_configid, powerConfigs);
            }
        }
        return 0;
    }


    int set_high_performance_mode() {
        if (nullptr == _qnn_htp_perfinfra) {
            LOGGD("perf intra is null\n");
            return 1;
        }

        QnnHtpPerfInfrastructure_PowerConfig_t powerConfig;
        memset(&powerConfig, 0, sizeof(powerConfig));
        powerConfig.option = QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_DCVS_V3;
        powerConfig.dcvsV3Config.dcvsEnable = 0;
        powerConfig.dcvsV3Config.setDcvsEnable = 1;
        powerConfig.dcvsV3Config.contextId = _qnn_power_configid;
        powerConfig.dcvsV3Config.powerMode = QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_PERFORMANCE_MODE;
        powerConfig.dcvsV3Config.setSleepLatency = 1; // True to consider Latency parameter otherwise False
        powerConfig.dcvsV3Config.setBusParams = 1; // True to consider Bus parameter otherwise False
        powerConfig.dcvsV3Config.setCoreParams = 1; // True to consider Core parameter otherwise False
        powerConfig.dcvsV3Config.sleepDisable = 0; // True to consider sleep/LPM modes, False to enable
        powerConfig.dcvsV3Config.setSleepDisable = 0; // True to consider sleep disable/enable parameter otherwise False
        // Set Sleep latency parameter
        uint32_t latencyValue = 40;
        powerConfig.dcvsV3Config.sleepLatency = latencyValue; // range 40-2000 micro sec
        // set Bus Clock Parameters (refer QnnHtpPerfInfrastructure_VoltageCorner_t enum)
        powerConfig.dcvsV3Config.busVoltageCornerMin = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.busVoltageCornerMax = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        // set Core Clock Parameters (refer QnnHtpPerfInfrastructure_VoltageCorner_t enum)
        powerConfig.dcvsV3Config.coreVoltageCornerMin = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.coreVoltageCornerMax = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        // Set power config with different performance parameters
        const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = {&powerConfig, nullptr};

        _qnn_htp_perfinfra->setPowerConfig(_qnn_power_configid, powerConfigs);

        return 0;
    }

    std::string &get_qnn_graph_name() { return _graph_name; }

    bool is_rpcmem_initialized() {
        return _rpcmem_initialized;
    }

    void set_rpcmem_initialized(bool initialized) {
        _rpcmem_initialized = initialized;
    }

    int32_t rpcmem_to_fd(void * buf);

    int register_rpcmem(void * p_data, Qnn_Tensor_t * p_tensor);

    void unregister_rpcmem();

    void *alloc_rpcmem(size_t bytes, size_t alignment);

    void free_rpcmem(void * buf);

    bool is_rpcmem_allocated(void * buf);

    bool is_rpcmem_registered(Qnn_MemHandle_t handle) {
        return _qnn_mem_set.count(handle) != 0U;
    }

private:
    int load_system();

    int unload_system();

    int load_backend(std::string &lib_path, const QnnSaver_Config_t ** saver_config);

    int unload_backend();

    void set_qnn_raw_interface(QNN_INTERFACE_VER_TYPE & raw_interface) {
        _qnn_raw_interface = raw_interface;
    }

    void set_qnn_raw_system_interface(QNN_SYSTEM_INTERFACE_VER_TYPE &raw_interface) {
        _qnn_raw_system_interface = raw_interface;
    }

private:
    static constexpr const int _required_num_providers = 1;

private:
    std::string _lib_path;
    std::string _backend_name;
    std::string _model_name;                         // prebuilt QNN model name
    BackendIdType _backend_id;

    bool _debug_tensor                      = false; // flag to indicate if requested graph is to be run in debug mode
    bool _do_node_validations               = true;  // flag to indicate whether all add_node calls need to be validated
    QnnLog_Level_t _qnn_log_level           = QNN_LOG_LEVEL_DEBUG;

    ggml_qnn_profile_level _profile_level   = ggml_qnn_profile_level::profile_detail;

    qnn_interface _qnn_interface;

    void *_system_lib_handle = nullptr;
    void *_model_lib_handle = nullptr;

    Qnn_GraphHandle_t _qnn_graph_handle = nullptr;

    Qnn_LogHandle_t _qnn_log_handle = nullptr;

    Qnn_ProfileHandle_t _qnn_profile_handle = nullptr;

    Qnn_DeviceHandle_t _qnn_device_handle = nullptr;

    Qnn_BackendHandle_t _qnn_backend_handle = nullptr;

    Qnn_ContextHandle_t _qnn_context_handle = nullptr;

    QnnSystemContext_Handle_t _qnn_system_handle = nullptr;

    QnnHtpDevice_PerfInfrastructure_t *_qnn_htp_perfinfra = nullptr;
    uint32_t _qnn_power_configid = 1;
    uint32_t _qnn_rpc_pollingtime = 9999; // 0-10000 us for high performing

    QNN_INTERFACE_VER_TYPE _qnn_raw_interface;
    QNN_SYSTEM_INTERFACE_VER_TYPE _qnn_raw_system_interface;

    std::unordered_set<Qnn_MemHandle_t> _qnn_mem_set;

    static std::mutex _init_mutex;
    static std::unordered_map<BackendIdType, void *> _loaded_lib_handle;
    static std::unordered_map<std::string, BackendIdType> _lib_path_to_backend_id;
    static std::unordered_map<BackendIdType, const QnnInterface_t *> _loaded_backend;

    void *_rpc_lib_handle = nullptr;
    std::atomic_bool _rpcmem_initialized{false};
    pfn_rpc_mem_alloc _pfn_rpc_mem_alloc;
    pfn_rpc_mem_free _pfn_rpc_mem_free;
    pfn_rpc_mem_to_fd _pfn_rpc_mem_to_fd;
    std::unordered_map<void *, void *> _rpcmem_store_map;


    std::string _graph_name;
};


std::mutex qnn_instance::_init_mutex;

std::unordered_map<qnn_instance::BackendIdType, void *> qnn_instance::_loaded_lib_handle;

std::unordered_map<std::string, qnn_instance::BackendIdType> qnn_instance::_lib_path_to_backend_id;

std::unordered_map<qnn_instance::BackendIdType, const QnnInterface_t *> qnn_instance::_loaded_backend;


void *qnn_instance::alloc_rpcmem(size_t bytes, size_t alignment) {
    if (!_rpcmem_initialized) {
        LOGGW("rpc memory not initialized\n");
        return nullptr;
    }

    auto allocate_bytes = static_cast<int32_t>(bytes + alignment);
    void *buf = _pfn_rpc_mem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, allocate_bytes);
    if (buf == nullptr) {
        LOGGW("failed to allocate rpc memory\n");
        return nullptr;
    }

    auto aligned_buf = reinterpret_cast<void *>(align_to(alignment,
                                                         reinterpret_cast<intptr_t>(buf)));
    bool status = _rpcmem_store_map.insert(std::pair<void *, void *>(aligned_buf, buf)).second;
    if (!status) {
        LOGGW("failed to allocate rpc memory\n");
        _pfn_rpc_mem_free(buf);
    }

    return aligned_buf;
}


void qnn_instance::free_rpcmem(void * buf) {
    if (!_rpcmem_initialized) {
        LOGGW("rpc memory not initialized\n");
    } else if (0 == _rpcmem_store_map.count(buf)) {
        LOGGW("no allocated tensor\n");
    } else {
        _pfn_rpc_mem_free(_rpcmem_store_map[buf]);
        _rpcmem_store_map.erase(buf);
    }
}


int32_t qnn_instance::rpcmem_to_fd(void *buf) {
    int32_t mem_fd = -1;
    if (!is_rpcmem_initialized()) {
        LOGGW("rpc memory not initialized\n");
    } else {
        mem_fd = _pfn_rpc_mem_to_fd(buf);
    }

    return mem_fd;
}


int qnn_instance::register_rpcmem(void * p_data, Qnn_Tensor_t * p_tensor) {
    if (nullptr == p_data || (nullptr == p_tensor)) {
        LOGGW("invalid param\n");
        return 1;
    }

    if (!is_rpcmem_initialized()) {
        LOGGW("rpc memory not initialized\n");
        return 2;
    }

    if (is_rpcmem_allocated(p_data)) {
        LOGGW("rpc memory already allocated\n");
        //return 3;
    }
    if (is_rpcmem_registered((QNN_VER_PTR(*p_tensor)->memHandle))) {
        LOGGW("tensor %s has been registered shared memory\n", (QNN_VER_PTR(*p_tensor)->name));
        return 4;
    }

    int32_t mem_fd = rpcmem_to_fd(p_data);
    if (-1 == mem_fd) {
        LOGGW("failed to get file descriptor\n");
        return 5;
    }
    LOGGD("mem_fd %d\n", mem_fd);
    Qnn_MemDescriptor_t descriptor = {
            {QNN_VER_PTR(*p_tensor)->rank, QNN_VER_PTR(*p_tensor)->dimensions, nullptr},
            QNN_VER_PTR(*p_tensor)->dataType,
            QNN_MEM_TYPE_ION,
            {{mem_fd}}};
    Qnn_MemHandle_t handle = nullptr;
    int error = QNN_SUCCESS;
    error = _qnn_interface.qnn_mem_register(
            _qnn_context_handle,
            &descriptor,
            /*numDescriptors=*/1,
            &handle);
    if (error != QNN_SUCCESS) {
        LOGGW("failed to register shared memory, error %d, %s\n", QNN_GET_ERROR_CODE(error),
              strerror(error));
        return 6;
    } else {
        LOGGI("tensor %s successfully register shared memory\n", (QNN_VER_PTR(*p_tensor)->name));
    }
    QNN_VER_PTR(*p_tensor)->memHandle = handle;
    _qnn_mem_set.insert(handle);

    return 0;
}


void qnn_instance::unregister_rpcmem() {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;

    if (_qnn_mem_set.empty()) {
        LOGGW("no rpcmem registered\n");
    }

    for (auto &mem_handle : _qnn_mem_set) {
        error = _qnn_interface.qnn_mem_de_register(&mem_handle, 1);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to unregister shared memory, error %d\n", QNN_GET_ERROR_CODE(error));
        }
    }
    _qnn_mem_set.clear();
}


bool qnn_instance::is_rpcmem_allocated(void * buf) {
    return _rpcmem_store_map.count(buf) != 0U;
}


int qnn_instance::load_backend(std::string & lib_path, const QnnSaver_Config_t ** saver_config) {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    LOGGD("lib_path:%s\n", lib_path.c_str());

    void *lib_handle = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (nullptr == lib_handle) {
        LOGGW("can not open QNN library %s, with error: %s", lib_path.c_str(), dlerror());
        return 1;
    }

    // load get_provider function
    auto get_providers = load_qnn_functionpointers<_pfn_QnnInterface_getProviders *>(lib_handle,
                                                                                     "QnnInterface_getProviders");
    if (nullptr == get_providers) {
        LOGGW("can not load symbol QnnInterface_getProviders : %s", dlerror());
        return 2;
    }

    // get QnnInterface Providers
    std::uint32_t num_providers = 0;
    const QnnInterface_t **provider_list = nullptr;
    error = get_providers(&provider_list, &num_providers);
    if (error != QNN_SUCCESS) {
        LOGGW("failed to get providers, error %d", QNN_GET_ERROR_CODE(error));
        return 3;
    }
    LOGGD("num_providers=%d\n", num_providers);
    if (num_providers != _required_num_providers) {
        LOGGW("providers is %d instead of required %d", num_providers, _required_num_providers);
        return 4;
    }

    if (nullptr == provider_list) {
        LOGGW("failed to get qnn interface providers\n");
        return 5;
    }
    bool found_valid_interface = false;
    QNN_INTERFACE_VER_TYPE qnn_interface;
    for (size_t idx = 0; idx < num_providers; idx++) {
        if (QNN_API_VERSION_MAJOR == provider_list[idx]->apiVersion.coreApiVersion.major &&
            QNN_API_VERSION_MINOR <= provider_list[idx]->apiVersion.coreApiVersion.minor) {
            found_valid_interface = true;
            qnn_interface = provider_list[idx]->QNN_INTERFACE_VER_NAME;
            break;
        }
    }

    if (!found_valid_interface) {
        LOGGW("unable to find a valid qnn interface\n");
        return 6;
    } else {
        LOGGI("find a valid qnn interface\n");
    }
    set_qnn_raw_interface(qnn_interface);

    BackendIdType backend_id = provider_list[0]->backendId;
    _lib_path_to_backend_id[lib_path] = backend_id;
    if (_loaded_backend.count(backend_id) > 0) {
        LOGGW("lib_path %s is loaded, but backend %d already exists\n",
              lib_path.c_str(), backend_id);
    }
    _loaded_backend[backend_id] = provider_list[0];
    if (_loaded_lib_handle.count(backend_id) > 0) {
        LOGGW("closing %p\n", _loaded_lib_handle[backend_id]);
        int dlclose_error = dlclose(_loaded_lib_handle[backend_id]);
        if (dlclose_error != 0) {
            LOGGW("fail to close %p with error %s\n", _loaded_lib_handle[backend_id], dlerror());
        }
    }
    _loaded_lib_handle[backend_id] = lib_handle;
    _backend_id = backend_id;

    QnnSaver_Config_t outputdir_cfg;
    outputdir_cfg.option = QNN_SAVER_CONFIG_OPTION_OUTPUT_DIRECTORY;
    outputdir_cfg.outputDirectory = "/data/data/com.cdeos.kantv/qnn/";

    QnnSaver_Config_t backendid_cfg;
    backendid_cfg.option = QNN_SAVER_CONFIG_OPTION_BACKEND_ID;
    backendid_cfg.backendId = _backend_id;
    const QnnSaver_Config_t *saverCfg[] = {&outputdir_cfg, &backendid_cfg, nullptr};
    if (0 == QnnSaver_initialize(saverCfg)) {
        LOGGI("QnnSaver_initialize successfully");
    } else {
        LOGGI("QnnSaver_initialize failure");
    }

    auto saver_initialize = load_qnn_functionpointers<_pfn_QnnSaver_initialize *>(
            _loaded_lib_handle[backend_id], "QnnSaver_initialize");
    if (nullptr != saver_initialize) {
        error = saver_initialize(saver_config);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to saver_initialize，error %d", QNN_GET_ERROR_CODE(error));
            return 7;
        }
    } else {
        LOGGW("saver_initialize is null\n");
    }

    return 0;
}


int qnn_instance::unload_backend() {
    ENTER_FUNC();
    int dlclose_error = 0;
    for (auto &it : _loaded_lib_handle) {
        dlclose_error = dlclose(it.second);
        if (dlclose_error != 0) {
            LOGGW("failed to close QNN backend %d, error %s\n", it.first, dlerror());
        }
    }

    _loaded_lib_handle.clear();
    _lib_path_to_backend_id.clear();
    _loaded_backend.clear();

    LEAVE_FUNC();

    return 0;
}


int qnn_instance::load_system() {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;

    std::string system_lib_path = _lib_path + "libQnnSystem.so";
    LOGGD("system_lib_path:%s\n", system_lib_path.c_str());

    _system_lib_handle = dlopen(system_lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (nullptr == _system_lib_handle) {
        LOGGW("can not pen QNN library %s, error: %s\n", system_lib_path.c_str(), dlerror());
        return 1;
    }

    auto *get_providers = reinterpret_cast<_pfn_QnnSystemInterface_getProviders *>(dlsym(
            _system_lib_handle, "QnnSystemInterface_getProviders"));
    if (nullptr == get_providers) {
        LOGGW("can not load QNN symbol QnnSystemInterface_getProviders: %s\n", dlerror());
        return 2;
    }

    uint32_t num_providers = 0;
    const QnnSystemInterface_t ** provider_list = nullptr;
    error = get_providers(&provider_list, &num_providers);
    if (error != QNN_SUCCESS) {
        LOGGW("failed to get providers, error %d\n", QNN_GET_ERROR_CODE(error));
        return 3;
    }

    if (num_providers != _required_num_providers) {
        LOGGW("providers is %d instead of required %d\n", num_providers, _required_num_providers);
        return 4;
    }

    if (nullptr == provider_list) {
        LOGGW("can not get providers\n");
        return 5;
    }

    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_system_interface;
    bool found_valid_system_interface = false;
    for (size_t idx = 0; idx < num_providers; idx++) {
        if (QNN_SYSTEM_API_VERSION_MAJOR ==
            provider_list[idx]->systemApiVersion.major &&
            QNN_SYSTEM_API_VERSION_MINOR <=
            provider_list[idx]->systemApiVersion.minor) {
            found_valid_system_interface = true;
            qnn_system_interface = provider_list[idx]->QNN_SYSTEM_INTERFACE_VER_NAME;
            break;
        }
    }
    if (!found_valid_system_interface) {
        LOGGW("unable to find a valid qnn system interface\n");
        return 6;
    } else {
        LOGGI("find a valid qnn system interface\n");
    }
    set_qnn_raw_system_interface(qnn_system_interface);

    _qnn_interface.set_qnn_system_interface(provider_list[0]);

    _qnn_interface.qnn_system_context_create(&_qnn_system_handle);
    if (nullptr == _qnn_system_handle) {
        LOGW("can not create QNN system contenxt\n");
    } else {
        LOGGD("initialize qnn system successfully\n");
    }

    return 0;
}


int qnn_instance::unload_system() {
    ENTER_FUNC();

    int result = 0;

    if (nullptr == _system_lib_handle) {
        LOGGD("system lib handle is null\n");
        return 1;
    }

    if (nullptr != _qnn_system_handle) {
        result = _qnn_interface.qnn_system_context_free(_qnn_system_handle);
        if (result != QNN_SUCCESS) {
            LOGGW("failed to free QNN system context\n");
        }
        _qnn_system_handle = nullptr;
    }

    int dlclose_error = dlclose(_system_lib_handle);
    if (dlclose_error != 0) {
        LOGGW("failed to close QnnSystem library, error %s\n", dlerror());
        return 2;
    }

    _system_lib_handle = nullptr;
    LEAVE_FUNC();

    return 0;
}


static void ggml_qnn_logcallback(const char * fmt,
                                 QnnLog_Level_t level,
                                 uint64_t timestamp,
                                 va_list argp) {

    static std::mutex _log_mutex;
    static unsigned char s_ggml_qnn_logbuf[GGML_QNN_LOGBUF_LEN];

    const char *levelStr = "";
    switch (level) {
        case QNN_LOG_LEVEL_ERROR:
            levelStr = " ERROR ";
            break;
        case QNN_LOG_LEVEL_WARN:
            levelStr = "WARNING";
            break;
        case QNN_LOG_LEVEL_INFO:
            levelStr = "  INFO ";
            break;
        case QNN_LOG_LEVEL_DEBUG:
            levelStr = " DEBUG ";
            break;
        case QNN_LOG_LEVEL_VERBOSE:
            levelStr = "VERBOSE";
            break;
        case QNN_LOG_LEVEL_MAX:
            levelStr = "UNKNOWN";
            break;
    }

    double ms = (double) timestamp / 1000000.0;

    {
        std::lock_guard<std::mutex> lock(_log_mutex);

        int len_content = 0;
        memset(s_ggml_qnn_logbuf, 0, GGML_QNN_LOGBUF_LEN);
        len_content = vsnprintf(reinterpret_cast<char *const>(s_ggml_qnn_logbuf), GGML_QNN_LOGBUF_LEN, fmt, argp);
        LOGGD("%8.1fms [%-7s] %s ", ms, levelStr, s_ggml_qnn_logbuf);
        //for validation purpose during development stage, should be removed before PR to upstream GGML/whisper.cpp
        GGML_JNI_NOTIFY("%8.1fms [%-7s] %s ", ms, levelStr, s_ggml_qnn_logbuf);
    }
}


int qnn_instance::qnn_init(const QnnSaver_Config_t ** saver_config) {
    BackendIdType backend_id = QNN_BACKEND_ID_NULL;
    LOGGD("enter qni_init\n");

    const std::lock_guard<std::mutex> lock(_init_mutex);

    if (0 != load_system()) {
        LOGGW("can not load QNN system lib, pls check why?\n");
        return 1;
    } else {
        LOGGD("load QNN system lib successfully\n");
    }

    std::string bakend_lib_path = _lib_path + _backend_name;
    if (0 == _lib_path_to_backend_id.count(bakend_lib_path)) {
        int is_load_ok = load_backend(bakend_lib_path, saver_config);
        if (0 != is_load_ok) {
            LOGGW("failed to load QNN backend\n");
            return 2;
        }
    }

    backend_id = _lib_path_to_backend_id[bakend_lib_path];
    if (0 == _loaded_backend.count(backend_id) ||
        0 == _loaded_lib_handle.count(backend_id)) {
        LOGGW("library %s is loaded but loaded backend count=%zu, loaded lib_handle count=%zu\n",
              bakend_lib_path.c_str(),
              _loaded_backend.count(backend_id),
              _loaded_lib_handle.count(backend_id));
        return 3;
    }

    _qnn_interface.set_qnn_interface(_loaded_backend[backend_id]);

    _qnn_interface.qnn_log_create(ggml_qnn_logcallback, _qnn_log_level, &_qnn_log_handle);
    if (nullptr == _qnn_log_handle) {
        LOGGW("why failed to initialize qnn log\n");
        return 4;
    } else {
        LOGGD("initialize qnn log successfully\n");
    }

    std::vector<const QnnBackend_Config_t *> temp_backend_config; //TODO:now is empty because I don't know how to use QnnBackend_Config_t currently
    _qnn_interface.qnn_backend_create(_qnn_log_handle, temp_backend_config.empty() ? nullptr
                                                                                   : temp_backend_config.data(),
                                      &_qnn_backend_handle);
    if (nullptr == _qnn_backend_handle) {
        LOGGW("why failed to initialize qnn backend\n");
        return 5;
    } else {
        LOGGD("initialize qnn backend successfully\n");
    }

    LOGGD("here");

    if (nullptr != _qnn_raw_interface.propertyHasCapability) {
        auto qnnStatus = _qnn_raw_interface.propertyHasCapability(QNN_PROPERTY_GROUP_DEVICE);
        if (QNN_PROPERTY_NOT_SUPPORTED == qnnStatus) {
            LOGGW("device property is not supported\n");
        }
        if (QNN_PROPERTY_ERROR_UNKNOWN_KEY == qnnStatus) {
            LOGGW("device property is not known to backend\n");
        }
    }

    auto qnnStatus = _qnn_raw_interface.deviceCreate(_qnn_log_handle, nullptr, &_qnn_device_handle);
    if (QNN_SUCCESS != qnnStatus && QNN_DEVICE_ERROR_UNSUPPORTED_FEATURE != qnnStatus) {
        LOGGW("failed to create QNN device\n");
    } else {
        LOGGI("create device successfully\n");
    }

    /*
    std::vector<const QnnDevice_Config_t*> temp_device_config;
    _qnn_interface.qnn_device_create(_qnn_log_handle, temp_device_config.empty() ? nullptr : temp_device_config.data(), &_qnn_device_handle);
    if (nullptr == _qnn_device_handle) {
        LOGGW("why failed to initialize qnn device\n");
        //return 6;
    }
    */

    if (ggml_qnn_profile_level::profile_off != _profile_level) {
        LOGGI("profiling turned on; level = %d", _profile_level);
        if (ggml_qnn_profile_level::profile_basic == _profile_level) {
            LOGGI("basic profiling requested. creating Qnn Profile object\n");
            if (QNN_PROFILE_NO_ERROR != _qnn_raw_interface.profileCreate(
                    _qnn_backend_handle, QNN_PROFILE_LEVEL_BASIC, &_qnn_profile_handle)) {
                LOGGW("unable to create profile handle in the backend\n");
                return 7;
            } else {
                LOGGD("initialize qnn profile successfully\n");
            }
        } else if (ggml_qnn_profile_level::profile_detail == _profile_level) {
            LOGGI("detailed profiling requested. Creating Qnn Profile object\n");
            if (QNN_PROFILE_NO_ERROR != _qnn_raw_interface.profileCreate(
                    _qnn_backend_handle, QNN_PROFILE_LEVEL_DETAILED, &_qnn_profile_handle)) {
                LOGGW("unable to create profile handle in the backend\n");
                return 7;
            } else {
                LOGGD("initialize qnn profile successfully\n");
            }
        }
    }

    /* not remove, for future use
    _rpc_lib_handle = dlopen("libcdsprpc.so", RTLD_NOW | RTLD_LOCAL);
    if (nullptr == _rpc_lib_handle) {
        LOGGW("failed to load qualcomm's rpc lib, error:%s\n", dlerror());
        return 9;
    } else {
        LOGGD("load rpcmem lib successfully\n");
        set_rpcmem_initialized(true);
    }
    _pfn_rpc_mem_alloc = reinterpret_cast<pfn_rpc_mem_alloc>(dlsym(_rpc_lib_handle,
                                                                   "rpcmem_alloc"));
    _pfn_rpc_mem_free = reinterpret_cast<pfn_rpc_mem_free>(dlsym(_rpc_lib_handle, "rpcmem_free"));
    _pfn_rpc_mem_to_fd = reinterpret_cast<pfn_rpc_mem_to_fd>(dlsym(_rpc_lib_handle,
                                                                   "rpcmem_to_fd"));
    if (nullptr == _pfn_rpc_mem_alloc || nullptr == _pfn_rpc_mem_free ||
        nullptr == _pfn_rpc_mem_to_fd) {
        LOGGW("unable to access symbols in shared buffer. dlerror(): %s", dlerror());
        dlclose(_rpc_lib_handle);
        return 10;
    }
    */

    std::vector<const QnnContext_Config_t *> temp_context_config;
    _qnn_interface.qnn_context_create(_qnn_backend_handle, _qnn_device_handle,
                                      temp_context_config.empty() ? nullptr
                                                                  : temp_context_config.data(),
                                      &_qnn_context_handle);
    if (nullptr == _qnn_context_handle) {
        LOGGW("why failed to initialize qnn context\n");
        return 8;
    } else {
        LOGGD("initialize qnn context successfully\n");
    }

    LOGGD("leave qni_init\n");

    return 0;
}


int qnn_instance::qnn_finalize() {
    int ret_status = 0;
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    ENTER_FUNC();

    /* not remove, for future use
    if (dlclose(_rpc_lib_handle) != 0) {
        LOGGW("failed to unload qualcomm's rpc lib, error:%s\n", dlerror());
    } else {
        LOGGD("succeed to close rpcmem lib\n");
    }
    */

    if (nullptr != _qnn_context_handle) {
        error = _qnn_interface.qnn_context_free(_qnn_context_handle, _qnn_profile_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN context_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));

        }
        _qnn_context_handle = nullptr;
    }

    if (nullptr != _qnn_profile_handle) {
        error = _qnn_interface.qnn_profile_free(_qnn_profile_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN profile_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));

        }
        _qnn_profile_handle = nullptr;
    }

    if (nullptr != _qnn_device_handle) {
        error = _qnn_interface.qnn_device_free(_qnn_device_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN device_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));

        }
        _qnn_device_handle = nullptr;
    }

    if (nullptr != _qnn_backend_handle) {
        error = _qnn_interface.qnn_backend_free(_qnn_backend_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN backend_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));
        }
        _qnn_backend_handle = nullptr;

    }

    if (nullptr != _qnn_log_handle) {
        error = _qnn_interface.qnn_log_free(_qnn_log_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN log_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));
        }
        _qnn_log_handle = nullptr;
    }

    unload_backend();

    unload_system();

    LEAVE_FUNC();

    return ret_status;
}


int qnn_instance::init_qnn_model(const char * graph_name, bool debug, uint8_t do_node_validation,
                                   const QnnGraph_Config_t ** graph_configs) {
    int result = 0;

    ENTER_FUNC();

    if (nullptr == graph_name) {
        LOGGW("graph name is null\n");
        return 1;
    }

    if (!_graph_name.empty()) {
        LOGGW("qnn model for graph %s already initialized\n", graph_name);
        return 2;
    }

    if (!do_node_validation) {
        LOGGW("node validation disabled, backend will not perform op validation prior to adding node\n");
    }

    _graph_name = graph_name;
    _debug_tensor = debug;
    _do_node_validations = do_node_validation;

    result = _qnn_raw_interface.graphCreate(_qnn_context_handle, graph_name, graph_configs,
                                            &_qnn_graph_handle);
    if (result != QNN_GRAPH_NO_ERROR || nullptr == _qnn_graph_handle) {
        LOGGW("failed to create graph in qnn context\n");
        return 3;
    } else {
        LOGGI("succeed to create graph %s, %p\n", graph_name, _qnn_graph_handle);
    }

    LEAVE_FUNC();
    return 0;
}


int qnn_instance::finalize_qnn_model() {
    ENTER_FUNC();
    if (nullptr != _qnn_graph_handle) {
        if (_qnn_raw_interface.graphFinalize(_qnn_graph_handle, _qnn_profile_handle, nullptr) !=
            QNN_GRAPH_NO_ERROR) {
            LOGGW("finalizing graph failure\n");
            //return 1;
        }
        _qnn_graph_handle = nullptr;
    } else {
        LOGGD("qnn graph handle is null\n");
    }

    LEAVE_FUNC();

    return 0;
}


bool ggml_qnn_can_mul_mat(const struct ggml_tensor * src0, const struct ggml_tensor * src1,
                                 struct ggml_tensor * dst) {
    if (!g_qnn_loaded) {
        LOGGW("qnn backend not initialized");
        return false;
    }

    const int64_t ne10 = src1->ne[0];

    const int64_t ne0 = dst->ne[0];
    const int64_t ne1 = dst->ne[1];

    /*
    return (src0->type == GGML_TYPE_F32 || src0->type == GGML_TYPE_F16 || ggml_is_quantized(src0->type)) &&
           (src1->type == GGML_TYPE_F32 || src1->type == GGML_TYPE_F16 || ggml_is_quantized(src1->type)) &&
           dst->type == GGML_TYPE_F32 &&
           ((ne0 >= 32 && ne1 >= 32 && ne10 >= 32) || src0->backend == GGML_BACKEND_TYPE_GPU);*/
    return (src0->type == GGML_TYPE_F32 || src0->type == GGML_TYPE_F16) &&
           (src1->type == GGML_TYPE_F32 || src1->type == GGML_TYPE_F16) &&
           dst->type == GGML_TYPE_F32 &&
           ((ne0 >= 32 && ne1 >= 32 && ne10 >= 32) || src0->backend == GGML_BACKEND_TYPE_GPU);

}


static void ggml_qnn_repeat(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_get_rows(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


//ref: PoC-S26: offload simple f32 2x2 matrix addition operation to QNN CPU
// https://github.com/zhouwg/kantv/blob/kantv-poc-with-qnn/core/ggml/jni/ggml-jni-impl-external.cpp#L6736
static void ggml_qnn_add(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_acc(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


//ref: PoC-S29:mapping ggml_tensor to QNN_tensor
// https://github.com/zhouwg/kantv/blob/kantv-poc-with-qnn/core/ggml/jni/ggml-jni-impl-external.cpp#L7060
static void ggml_qnn_mul(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_div(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_gelu(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_silu(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_gelu_quick(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_tanh(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_relu(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_hardsigmoid(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_hardswish(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_leaky_relu(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_sqr(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_norm(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_group_norm(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_concat(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_upscale(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_pad(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_rms_norm(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_cpy(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_dup(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    ggml_qnn_cpy(src0, dst, nullptr);
    (void) src1;
}


//ref: PoC-S29:mapping ggml_tensor to QNN_tensor
// https://github.com/zhouwg/kantv/blob/kantv-poc-with-qnn/core/ggml/jni/ggml-jni-impl-external.cpp#L7060
void ggml_qnn_mul_mat(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);
    LOGGI("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", src0->name,
          src0->type, ggml_type_name(src0->type), src0->ne[0], src0->ne[1], src0->ne[2], src0->nb[0], src0->nb[1], src0->nb[2]);
    LOGGI("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", src1->name,
          src1->type, ggml_type_name(src1->type), src1->ne[0], src1->ne[1], src1->ne[2], src1->nb[0], src1->nb[1], src1->nb[2]);
    LOGGI("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", dst->name,
          dst->type, ggml_type_name(dst->type), dst->ne[0], dst->ne[1], dst->ne[2], dst->nb[0], dst->nb[1], dst->nb[2]);
    //TENSOR_DUMP(src0);
     {
        uint32_t i                                  = 0;
        uint32_t j                                  = 0;
        int error                                   = 0;
        int result                                  = 0;


        int64_t  n_begin_time                       = 0LL;
        int64_t  n_end_time                         = 0LL;
        int64_t  n_durtion                          = 0LL;

        qnn_instance * instance                     = nullptr;
        Qnn_GraphHandle_t graph_handle              = nullptr;
        Qnn_Tensor_t tensor_0                       = QNN_TENSOR_INIT;
        Qnn_Tensor_t tensor_1                       = QNN_TENSOR_INIT;
        Qnn_Tensor_t tensor_2                       = QNN_TENSOR_INIT;
        Qnn_QuantizeParams_t quantize_param         = QNN_QUANTIZE_PARAMS_INIT;
        Qnn_OpConfig_t qnn_opconfig                 = QNN_OPCONFIG_INIT;
        Qnn_Param_t qnn_params[]                    = {};
        const char * qnn_op_typename                = QNN_OP_MAT_MUL;

        enum ggml_op     ggmlop                     = GGML_OP_MUL_MAT;
        Qnn_DataType_t   src0_qnn_type              = QNN_DATATYPE_FLOAT_32;
        Qnn_DataType_t  src1_qnn_type               = QNN_DATATYPE_FLOAT_32;

        ggml_time_init();
        n_begin_time                                = ggml_time_us();

        struct ggml_backend_qnn_context * ctx       = (struct ggml_backend_qnn_context*)g_qnn_backend->context;
        instance                                    = ctx->instance;

        if (src0->type == GGML_TYPE_F16)
            src0_qnn_type = QNN_DATATYPE_FLOAT_16;
         if (src1->type == GGML_TYPE_F16)
             src1_qnn_type = QNN_DATATYPE_FLOAT_16;

        QNN_INTERFACE_VER_TYPE qnn_raw_interface                = ctx->raw_interface;
        QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface  = ctx->raw_system_interface;
        error = qnn_raw_interface.graphCreate(instance->get_qnn_context_handle(), "mul_mat", nullptr, &graph_handle);
        LOGGI("error = %d\n", error);

        uint32_t dimensions_input_0[] = {(uint32_t)src0->ne[0], (uint32_t)src0->ne[1], (uint32_t)src0->ne[2], (uint32_t)src0->ne[3]};
        uint32_t dimensions_input_1[] = {(uint32_t)src1->ne[0], (uint32_t)src1->ne[1], (uint32_t)src1->ne[2], (uint32_t)src1->ne[3]};
        uint32_t dimensions_output[]  = {(uint32_t)dst->ne[0], (uint32_t)dst->ne[1], (uint32_t)dst->ne[2], (uint32_t)dst->ne[3]};

        tensor_0 = {
                .version= QNN_TENSOR_VERSION_1,
                {.v1= {
                        .id=0,
                        .name= "tensor_0",
                        .type= QNN_TENSOR_TYPE_APP_WRITE,
                        .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                        .dataType= src0_qnn_type,
                        .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                          QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                          {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= get_tensor_rank(src0),
                        .dimensions=dimensions_input_0,
                        .memType= QNN_TENSORMEMTYPE_RAW,
                        {.clientBuf= {.data=nullptr,
                                .dataSize=0}}}}
        };

        tensor_1 = {
                .version= QNN_TENSOR_VERSION_1,
                {.v1= {
                        .id=0,
                        .name= "tensor_1",
                        .type= QNN_TENSOR_TYPE_APP_WRITE,
                        .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                        .dataType= src1_qnn_type,
                        .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                          QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                          {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= get_tensor_rank(src1),
                        .dimensions=dimensions_input_1,
                        .memType= QNN_TENSORMEMTYPE_RAW,
                        {.clientBuf= {.data=nullptr,
                                .dataSize=0}}}}
        };

        tensor_2 = (Qnn_Tensor_t) {
                .version= QNN_TENSOR_VERSION_1,
                {.v1= {
                        .id=0,
                        .name= "tensor_2",
                        .type= QNN_TENSOR_TYPE_APP_READ,
                        .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                        .dataType= QNN_DATATYPE_FLOAT_32,
                        .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                          QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                          {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= get_tensor_rank(dst),
                        .dimensions= dimensions_output,
                        .memType= QNN_TENSORMEMTYPE_RAW,
                        {.clientBuf= {.data=nullptr,
                                .dataSize=0}}}}};


        error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_0);
        LOGGI("error = %d\n", error);
        error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_1);
        LOGGI("error = %d\n", error);
        error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_2);
        LOGGI("error = %d\n", error);

        // mapping GGML tensor to QNN tensor
        QNN_VER_PTR(tensor_0)->clientBuf = {src0->data, get_tensor_data_size(src0)};
        QNN_VER_PTR(tensor_1)->clientBuf = {src1->data, get_tensor_data_size(src1)};
        QNN_VER_PTR(tensor_2)->clientBuf = {dst->data, get_tensor_data_size(dst)};

        Qnn_Tensor_t tensor_inputs[] = {
                tensor_0,
                tensor_1
        };

        Qnn_Tensor_t tensor_outputs[] = {
                tensor_2
        };

        Qnn_OpConfig_t opconfig = {
                (Qnn_OpConfigVersion_t) 1, .v1 = {
                        "qnn_mul_mat",
                        QNN_OP_PACKAGE_NAME_QTI_AISW,
                        QNN_OP_MAT_MUL,
                        0,
                        qnn_params,
                        2,
                        tensor_inputs,
                        1,
                        tensor_outputs
                }
        };
        error = qnn_raw_interface.graphAddNode(graph_handle, opconfig);
        LOGGI("error = %d\n", error);
        error = qnn_raw_interface.graphFinalize(graph_handle, nullptr, nullptr);
        LOGGI("error = %d\n", error);
        error = qnn_raw_interface.graphExecute(graph_handle, tensor_inputs, 2, tensor_outputs, 1, nullptr, nullptr);
        LOGGI("error = %d\n", error);
falure:
        n_end_time  = ggml_time_us();
        n_durtion   = (n_end_time - n_begin_time) / 1000;
        LOGGD("duration of qnn mul_mat : %lld milliseconds\n", n_durtion);
    }
    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_mul_mat_id(const ggml_tensor * src0,
                                const ggml_tensor * src1,
                                ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_scale(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_clamp(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_diag_mask_inf(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_soft_max(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_rope(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    GGML_ASSERT(ggml_is_contiguous(src0));
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);

}


static void ggml_qnn_alibi(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_pool2d(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_im2col(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_sum_rows(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    GGML_ASSERT(ggml_is_contiguous(src0));
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_argsort(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    GGML_ASSERT(ggml_is_contiguous(src0));
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static void ggml_qnn_nop(const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst) {
    (void) src0;
    (void) src1;
    (void) dst;
    LOGGD("call %s\n", __func__);

    LOGGD("call %s done\n", __func__);
}


static bool ggml_qnn_compute_forward(struct ggml_compute_params * params, struct ggml_tensor * tensor) {
    //ENTER_FUNC();
    if (!g_qnn_loaded) return false;

    ggml_qnn_func_t func;

    const bool any_on_device = tensor->backend == GGML_BACKEND_TYPE_GPU
                               || (tensor->src[0] != nullptr &&
                                   (tensor->src[0]->backend == GGML_BACKEND_TYPE_GPU ||
                                    tensor->src[0]->backend == GGML_BACKEND_TYPE_GPU_SPLIT))
                               || (tensor->src[1] != nullptr &&
                                   tensor->src[1]->backend == GGML_BACKEND_TYPE_GPU);

    if (!any_on_device && tensor->op != GGML_OP_MUL_MAT && tensor->op != GGML_OP_MUL_MAT_ID) {
        return false;
    }

    if (tensor->op == GGML_OP_MUL_MAT) {
        if (tensor->src[0]->ne[3] != tensor->src[1]->ne[3]) {
            LOGGI("%s: cannot compute %s: src0->ne[3] = %" PRId64 ", src1->ne[3] = %" PRId64 " - fallback to CPU\n",
                  __func__, tensor->name, tensor->src[0]->ne[3], tensor->src[1]->ne[3]);
            return false;
        }
    }

    switch (tensor->op) {
        case GGML_OP_REPEAT:
            func = ggml_qnn_repeat;
            break;
        case GGML_OP_GET_ROWS:
            func = ggml_qnn_get_rows;
            break;
        case GGML_OP_DUP:
            func = ggml_qnn_dup;
            break;
        case GGML_OP_ADD:
            func = ggml_qnn_add;
            break;
        case GGML_OP_ACC:
            func = ggml_qnn_acc;
            break;
        case GGML_OP_MUL:
            func = ggml_qnn_mul;
            break;
        case GGML_OP_DIV:
            func = ggml_qnn_div;
            break;
        case GGML_OP_UNARY:
            switch (ggml_get_unary_op(tensor)) {
                case GGML_UNARY_OP_GELU:
                    func = ggml_qnn_gelu;
                    break;
                case GGML_UNARY_OP_SILU:
                    func = ggml_qnn_silu;
                    break;
                case GGML_UNARY_OP_GELU_QUICK:
                    func = ggml_qnn_gelu_quick;
                    break;
                case GGML_UNARY_OP_TANH:
                    func = ggml_qnn_tanh;
                    break;
                case GGML_UNARY_OP_RELU:
                    func = ggml_qnn_relu;
                    break;
                case GGML_UNARY_OP_HARDSIGMOID:
                    func = ggml_qnn_hardsigmoid;
                    break;
                case GGML_UNARY_OP_HARDSWISH:
                    func = ggml_qnn_hardswish;
                    break;
                default:
                    return false;
            }
            break;
        case GGML_OP_NORM:
            func = ggml_qnn_norm;
            break;
        case GGML_OP_GROUP_NORM:
            func = ggml_qnn_group_norm;
            break;
        case GGML_OP_CONCAT:
            func = ggml_qnn_concat;
            break;
        case GGML_OP_UPSCALE:
            func = ggml_qnn_upscale;
            break;
        case GGML_OP_PAD:
            func = ggml_qnn_pad;
            break;
        case GGML_OP_LEAKY_RELU:
            func = ggml_qnn_leaky_relu;
            break;
        case GGML_OP_RMS_NORM:
            func = ggml_qnn_rms_norm;
            break;
        case GGML_OP_MUL_MAT:
            if (!any_on_device && !ggml_qnn_can_mul_mat(tensor->src[0], tensor->src[1], tensor)) {
                return false;
            }
            func = ggml_qnn_mul_mat;
            break;
        case GGML_OP_MUL_MAT_ID:
            if (!any_on_device && !ggml_qnn_can_mul_mat(tensor->src[2], tensor->src[1], tensor)) {
                return false;
            }
            func = ggml_qnn_mul_mat_id;
            break;
        case GGML_OP_SCALE:
            func = ggml_qnn_scale;
            break;
        case GGML_OP_SQR:
            func = ggml_qnn_sqr;
            break;
        case GGML_OP_CLAMP:
            func = ggml_qnn_clamp;
            break;
        case GGML_OP_CPY:
            func = ggml_qnn_cpy;
            break;
        case GGML_OP_CONT:
            func = ggml_qnn_dup;
            break;
        case GGML_OP_NONE:
        case GGML_OP_RESHAPE:
        case GGML_OP_VIEW:
        case GGML_OP_PERMUTE:
        case GGML_OP_TRANSPOSE:
            func = ggml_qnn_nop;
            break;
        case GGML_OP_DIAG_MASK_INF:
            func = ggml_qnn_diag_mask_inf;
            break;
        case GGML_OP_SOFT_MAX:
            func = ggml_qnn_soft_max;
            break;
        case GGML_OP_ROPE:
            func = ggml_qnn_rope;
            break;
        case GGML_OP_ALIBI:
            func = ggml_qnn_alibi;
            break;
        case GGML_OP_IM2COL:
            func = ggml_qnn_im2col;
            break;
        case GGML_OP_POOL_2D:
            func = ggml_qnn_pool2d;
            break;
        case GGML_OP_SUM_ROWS:
            func = ggml_qnn_sum_rows;
            break;
        case GGML_OP_ARGSORT:
            func = ggml_qnn_argsort;
            break;
        default:
            return false;
    }

    if (params->ith != 0) {
        return true;
    }

    if (params->type == GGML_TASK_TYPE_INIT || params->type == GGML_TASK_TYPE_FINALIZE) {
        return true;
    }

    func(tensor->src[0], tensor->src[1], tensor);

    LEAVE_FUNC();

    return true;
}


static const char * ggml_backend_qnn_buffer_get_name(ggml_backend_buffer_t buffer) {
    GGML_UNUSED(buffer);
    return "QNN";
}


GGML_CALL static bool ggml_backend_buffer_is_qnn(ggml_backend_buffer_t buffer) {
    ENTER_FUNC();
    LEAVE_FUNC();
    return buffer->iface.get_name == ggml_backend_qnn_buffer_get_name;
}


static void ggml_backend_qnn_buffer_free_buffer(ggml_backend_buffer_t buffer) {
    ENTER_FUNC();
    ggml_backend_qnn_buffer_context * ctx = (ggml_backend_qnn_buffer_context *) buffer->context;
    delete ctx;
    LEAVE_FUNC();
}


//TODO
static void * ggml_backend_qnn_buffer_get_base(ggml_backend_buffer_t buffer) {
    ggml_backend_qnn_buffer_context * ctx = (ggml_backend_qnn_buffer_context *) buffer->context;

    return ctx->buffer;
}


static void ggml_backend_qnn_buffer_set_tensor(ggml_backend_buffer_t buffer, ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    ENTER_FUNC();
    GGML_UNUSED(buffer);

    LOGGD("tensor name: %s, size %d", tensor->name, size);
    memcpy((char *)tensor->data + offset, data, size);

    LEAVE_FUNC();
}


static void ggml_backend_qnn_buffer_get_tensor(ggml_backend_buffer_t buffer, const ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    ENTER_FUNC();
    GGML_UNUSED(buffer);
    LOGGD("tensor name: %s, size %d", tensor->name, size);
    memcpy(data, (const char *)tensor->data + offset, size);

    LEAVE_FUNC();
}


static bool ggml_backend_qnn_buffer_cpy_tensor(ggml_backend_buffer_t buffer, const struct ggml_tensor * src, struct ggml_tensor * dst) {
    GGML_UNUSED(buffer);
    if (ggml_backend_buffer_is_host(src->buffer)) {
        memcpy(dst->data, src->data, ggml_nbytes(src));
        return true;
    }
    return false;
}


static void ggml_backend_qnn_buffer_clear(ggml_backend_buffer_t buffer, uint8_t value) {
    ENTER_FUNC();
    ggml_backend_qnn_buffer_context * ctx = (ggml_backend_qnn_buffer_context *) buffer->context;

    memset(ctx->buffer, value, ctx->buffer_size);
    LEAVE_FUNC();
}



static void ggml_backend_qnn_buffer_reset(ggml_backend_buffer_t buffer) {
    ENTER_FUNC();
    ggml_backend_qnn_buffer_context * ctx = (ggml_backend_qnn_buffer_context *) buffer->context;
    for (auto * sub_buffer : ctx->sub_buffers) {
        free(sub_buffer);
    }
    ctx->sub_buffers.clear();
    LEAVE_FUNC();
}


static ggml_backend_buffer_i ggml_backend_qnn_buffer_interface = {
        /* .get_name        = */ ggml_backend_qnn_buffer_get_name,
        /* .free_buffer     = */ ggml_backend_qnn_buffer_free_buffer,
        /* .get_base        = */ ggml_backend_qnn_buffer_get_base,
        /* .init_tensor     = */ nullptr,
        /* .set_tensor      = */ ggml_backend_qnn_buffer_set_tensor,
        /* .get_tensor      = */ ggml_backend_qnn_buffer_get_tensor,
        /* .cpy_tensor      = */ ggml_backend_qnn_buffer_cpy_tensor,
        /* .clear           = */ ggml_backend_qnn_buffer_clear,
        /* .reset           = */ nullptr,
};


static const char * ggml_backend_qnn_buffer_type_name(ggml_backend_buffer_type_t buft) {
    ENTER_FUNC();
    LEAVE_FUNC();
    return "QNN";
}


static void * ggml_qnn_host_malloc(size_t n) {
    void * data = nullptr;
    const int result = posix_memalign((void **) &data, sysconf(_SC_PAGESIZE), n);
    if (result != 0) {
        LOGGW("%s: error: posix_memalign failed\n", __func__);
        return nullptr;
    }

    return data;
}


//TODO
static ggml_backend_buffer_t ggml_backend_qnn_buffer_type_alloc_buffer(ggml_backend_buffer_type_t buft, size_t size) {
    ENTER_FUNC();

    ggml_backend_qnn_buffer_context * ctx = new ggml_backend_qnn_buffer_context;

    const size_t size_page = sysconf(_SC_PAGESIZE);

    size_t size_aligned = size;
    if ((size_aligned % size_page) != 0) {
        size_aligned += (size_page - (size_aligned % size_page));
    }

    LOGGD("size %d, %d MB", size_aligned, size_aligned / (1 << 20));

    //TODO:use pre-mallocated buffer in internal memory pool
    ctx->buffer = ggml_qnn_host_malloc(size_aligned);
    ctx->buffer_size = size_aligned;

    if (nullptr == ctx->buffer) {
        LOGGW("%s: failed to allocate %.2f MiB\n", __func__, size / (1 << 20));
        LEAVE_FUNC();
        return nullptr;
    }
    i_alloc_buffer_counts++;
    i_alloc_buffer_sizes += size;
    LOGGI("total buffer counts %d, total buffer size %d(%d MB)", i_alloc_buffer_counts, i_alloc_buffer_sizes, i_alloc_buffer_sizes / (1 << 20));

    LEAVE_FUNC();

    return ggml_backend_buffer_init(buft, ggml_backend_qnn_buffer_interface, ctx, size);
}


static size_t ggml_backend_qnn_buffer_type_get_alignment(ggml_backend_buffer_type_t buft) {
    GGML_UNUSED(buft);
    return 32;
}


//TODO: this value is an experimental value
static size_t ggml_backend_qnn_buffer_type_get_max_size(ggml_backend_buffer_type_t buft) {
    GGML_UNUSED(buft);
    //works fine with ggml-tiny.en-q8_0.bin for whisper.cpp
    return (38 * 1024 * 1024);
}


static bool ggml_backend_qnn_buffer_type_supports_backend(ggml_backend_buffer_type_t buft,
                                                          ggml_backend_t backend) {
    ENTER_FUNC();
    GGML_UNUSED(buft);
    LEAVE_FUNC();

    return ggml_backend_is_qnn(backend) || ggml_backend_is_cpu(backend);
}


// attention here because Qualcomm's QNN SDK is a highly well-designed SDK
//
// refer to https://developer.qualcomm.com/sites/default/files/attachments/qnn_software_stack.png
//          https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-50/overview.html
static bool ggml_backend_qnn_buffer_is_host(ggml_backend_buffer_type_t buft) {
    GGML_UNUSED(buft);
    return true;
}

static ggml_backend_buffer_type_i ggml_backend_qnn_buffer_type_interface = {
        /* .get_name         = */ ggml_backend_qnn_buffer_type_name,
        /* .alloc_buffer     = */ ggml_backend_qnn_buffer_type_alloc_buffer,
        /* .get_alignment    = */ ggml_backend_qnn_buffer_type_get_alignment,
        /* .get_max_size     = */ ggml_backend_qnn_buffer_type_get_max_size,
        /* .get_alloc_size   = */ nullptr,
        /* .supports_backend = */ ggml_backend_qnn_buffer_type_supports_backend,
        /* .is_host          = */ ggml_backend_qnn_buffer_is_host
};


static const char * ggml_backend_qnn_name(ggml_backend_t backend) {
    return "QNN";
}


static void ggml_backend_qnn_free(ggml_backend_t backend) {
    ENTER_FUNC();
    ggml_backend_qnn_context * ctx = (ggml_backend_qnn_context *) backend->context;
    LOGGD("idx %d, name:%s", ctx->device, g_qnn_mgr[ctx->device].name);

    qnn_instance * instance = (qnn_instance*)g_qnn_mgr[ctx->device].instance;
    if (instance != nullptr) {
        instance->qnn_finalize();
        delete instance;
        g_qnn_mgr[ctx->device].instance = nullptr;
    }

    qnn_buf_t * buffer_pool = (qnn_buf_t*)g_qnn_mgr[ctx->device].buffer_pool;
    if (buffer_pool != nullptr) {
        buffer_pool->destroy(buffer_pool);
        g_qnn_mgr[ctx->device].buffer_pool = nullptr;
    }

    delete backend;

    LEAVE_FUNC();
}


static ggml_backend_buffer_type_t ggml_backend_qnn_get_default_buffer_type(ggml_backend_t backend) {
    ENTER_FUNC();
    ggml_backend_qnn_context * ctx = (ggml_backend_qnn_context *) backend->context;

    LOGGD("device %d,%s", ctx->device, ctx->name);
    LEAVE_FUNC();

    return ggml_backend_qnn_buffer_type(ctx->device);
}


//TODO: implement all supported GGML OP using QNN API
static ggml_status ggml_backend_qnn_graph_compute(ggml_backend_t backend, ggml_cgraph * cgraph) {
    ENTER_FUNC();
    ggml_backend_qnn_context * ctx = (ggml_backend_qnn_context *) backend->context;

    ggml_compute_params params = {};
    params.type = GGML_TASK_TYPE_COMPUTE;
    params.ith = 0;
    for (int i = 0; i < cgraph->n_nodes; i++) {
        ggml_tensor * node = cgraph->nodes[i];

        if (node->op == GGML_OP_RESHAPE || node->op == GGML_OP_TRANSPOSE ||
            node->op == GGML_OP_VIEW || node->op == GGML_OP_PERMUTE || node->op == GGML_OP_NONE) {
            continue;
        }

        assert(node->backend == GGML_BACKEND_TYPE_GPU ||
               node->backend == GGML_BACKEND_TYPE_GPU_SPLIT);
        assert(node->buffer->buft == ggml_backend_sycl_buffer_type(sycl_ctx->device));
        assert(node->extra != nullptr);

        for (int j = 0; j < GGML_MAX_SRC; j++) {
            if (node->src[j] != nullptr) {
                assert(node->src[j]->backend == GGML_BACKEND_TYPE_GPU ||
                       node->src[j]->backend == GGML_BACKEND_TYPE_GPU_SPLIT);
                assert(node->src[j]->buffer->buft ==
                       ggml_backend_sycl_buffer_type(sycl_ctx->device));
                assert(node->src[j]->extra != nullptr);
            }
        }

        bool ok = ggml_qnn_compute_forward(&params, node);
        if (!ok) {
            LOGGD("%s: error: op not supported %s (%s)\n", __func__, node->name, ggml_op_name(node->op));
        }
        //GGML_ASSERT(ok);
    }

    LEAVE_FUNC();

    return GGML_STATUS_SUCCESS;
}


#if 1
static bool ggml_backend_qnn_supports_op(ggml_backend_t backend, const ggml_tensor * op) {
    ENTER_FUNC();

    GGML_UNUSED(backend);

    switch (op->op) {
        case GGML_OP_UNARY:
            switch (ggml_get_unary_op(op)) {
                case GGML_UNARY_OP_GELU:
                case GGML_UNARY_OP_SILU:
                case GGML_UNARY_OP_RELU:
                case GGML_UNARY_OP_HARDSIGMOID:
                case GGML_UNARY_OP_HARDSWISH:
                case GGML_UNARY_OP_GELU_QUICK:
                case GGML_UNARY_OP_TANH:
                    return true;
                default:
                    return false;
            }
            break;
        case GGML_OP_MUL_MAT:
        case GGML_OP_MUL_MAT_ID: {
            struct ggml_tensor *a;
            struct ggml_tensor *b;
            if (op->op == GGML_OP_MUL_MAT) {
                a = op->src[0];
                b = op->src[1];
            } else {
                a = op->src[2];
                b = op->src[1];
            }
            if (a->ne[3] != b->ne[3]) {
                return false;
            }
            ggml_type a_type = a->type;
            if (a_type == GGML_TYPE_IQ4_NL || a_type == GGML_TYPE_IQ2_S ||
                a_type == GGML_TYPE_IQ4_XS) {
                return false;
            }
            return true;
        }
            break;
        case GGML_OP_GET_ROWS: {
            switch (op->src[0]->type) {
                case GGML_TYPE_F16:
                case GGML_TYPE_F32:
                case GGML_TYPE_Q4_0:
                case GGML_TYPE_Q4_1:
                case GGML_TYPE_Q5_0:
                case GGML_TYPE_Q5_1:
                case GGML_TYPE_Q8_0:
                    return true;
                default:
                    return false;
            }
        }
            break;
        case GGML_OP_CPY: {
            ggml_type src0_type = op->src[0]->type;
            ggml_type src1_type = op->src[1]->type;
            if (src0_type == GGML_TYPE_F32 && src1_type == GGML_TYPE_F32) {
                return true;
            }
            if (src0_type == GGML_TYPE_F32 && src1_type == GGML_TYPE_F16) {
                return true;
            }
            if (src0_type == GGML_TYPE_F32 && src1_type == GGML_TYPE_Q8_0) {
                return true;
            }
            if (src0_type == GGML_TYPE_F32 && src1_type == GGML_TYPE_Q4_0) {
                return true;
            }
            if (src0_type == GGML_TYPE_F32 && src1_type == GGML_TYPE_Q4_1) {
                return true;
            }
            if (src0_type == GGML_TYPE_F16 && src1_type == GGML_TYPE_F16) {
                return true;
            }
            if (src0_type == GGML_TYPE_F16 && src1_type == GGML_TYPE_F32) {
                return true;
            }
            return false;
        }
            break;
        case GGML_OP_CONCAT: {
            ggml_type src0_type = op->src[0]->type;
            return src0_type != GGML_TYPE_I32 && src0_type != GGML_TYPE_I16;
        }
            break;
        case GGML_OP_DUP:
        case GGML_OP_NONE:
        case GGML_OP_RESHAPE:
        case GGML_OP_REPEAT:
        case GGML_OP_VIEW:
        case GGML_OP_PERMUTE:
        case GGML_OP_TRANSPOSE:
        case GGML_OP_NORM:
        case GGML_OP_ADD:
        case GGML_OP_MUL:
        case GGML_OP_DIV:
        case GGML_OP_RMS_NORM:
        case GGML_OP_SCALE:
        case GGML_OP_SQR:
        case GGML_OP_CLAMP:
        case GGML_OP_CONT:
        case GGML_OP_DIAG_MASK_INF:
        case GGML_OP_SOFT_MAX:
        case GGML_OP_ROPE:
        case GGML_OP_ALIBI:
        case GGML_OP_IM2COL:
        case GGML_OP_POOL_2D:
        case GGML_OP_SUM_ROWS:
        case GGML_OP_ARGSORT:
        case GGML_OP_ACC:
        case GGML_OP_GROUP_NORM:
        case GGML_OP_UPSCALE:
        case GGML_OP_PAD:
        case GGML_OP_LEAKY_RELU:
            return true;
        default:
            return false;
    }

    LEAVE_FUNC();
}
# else
static bool ggml_backend_qnn_supports_op(ggml_backend_t backend, const ggml_tensor * op) {
    GGML_UNUSED(backend);

    switch (op->op) {
        case GGML_OP_MUL_MAT:
            return true;
        default:
            return false;
    }
}
#endif


static bool ggml_backend_qnn_offload_op(ggml_backend_t backend, const ggml_tensor * op) {
    ENTER_FUNC();
    GGML_UNUSED(backend);

    const int min_batch_size = 32;

    LEAVE_FUNC();

    return op->ne[1] >= min_batch_size && op->op != GGML_OP_GET_ROWS;

}


static ggml_backend_i ggml_backend_qnn_interface = {
        /* .get_name                = */ ggml_backend_qnn_name,
        /* .free                    = */ ggml_backend_qnn_free,
        /* .get_default_buffer_type = */ ggml_backend_qnn_get_default_buffer_type,
        /* .set_tensor_async        = */ nullptr,
        /* .get_tensor_async        = */ nullptr,
        /* .cpy_tensor_async        = */ nullptr,
        /* .synchronize             = */ nullptr,
        /* .graph_plan_create       = */ nullptr,
        /* .graph_plan_free         = */ nullptr,
        /* .graph_plan_compute      = */ nullptr,
        /* .graph_compute           = */ ggml_backend_qnn_graph_compute,
        /* .supports_op             = */ ggml_backend_qnn_supports_op,
        /* .offload_op              = */ nullptr,
        /* .event_new               = */ nullptr,
        /* .event_free              = */ nullptr,
        /* .event_record            = */ nullptr,
        /* .event_wait              = */ nullptr,
        /* .event_synchronize       = */ nullptr,
};


static ggml_guid_t ggml_backend_qnn_guid() {
    ENTER_FUNC();
    static ggml_guid guid = {0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70, 0x81, 0x92, 0xa3, 0xb4, 0xc5,
                             0xd6, 0xe7, 0xf8, 0x09};
    LEAVE_FUNC();

    return &guid;
}


static ggml_backend_t ggml_backend_qnn_reg_init(const char * params, void * user_data) {
    ENTER_FUNC();
    GGML_UNUSED(params);
    ggml_backend_t qnn_backend = ggml_backend_qnn_init((int) (intptr_t) user_data);
    LEAVE_FUNC();

    return qnn_backend;
}


bool ggml_backend_is_qnn(ggml_backend_t backend) {
    ENTER_FUNC();
    LEAVE_FUNC();

    return backend != nullptr && ggml_guid_matches(backend->guid, ggml_backend_qnn_guid());
}


int ggml_backend_qnn_get_device_count() {
    ENTER_FUNC();
    LEAVE_FUNC();

    return GGML_QNN_MAX_DEVICES;
}


void ggml_backend_qnn_get_device_description(int device, char * description, size_t description_size) {
    ENTER_FUNC();
    if (nullptr == description || 0 == description_size)
        return;

    if (device >= GGML_QNN_MAX_DEVICES)
        return;

    snprintf(description, description_size, "%s", g_qnn_mgr[device].name);
    LOGGD("description:%s", description);

    LEAVE_FUNC();
}


void ggml_backend_qnn_get_device_memory(int device, size_t * free, size_t * total) {
    ENTER_FUNC();
    LEAVE_FUNC();
}


ggml_backend_buffer_type_t ggml_backend_qnn_buffer_type(size_t device_index) {
    ENTER_FUNC();

    if (device_index >= GGML_QNN_MAX_DEVICES) {
        LOGGD("ggml_backend_qnn_buffer_type error: device_index:%d is out of range [0, %d]\n",
               device_index, GGML_QNN_MAX_DEVICES - 1);
        GGML_ASSERT(device_index < GGML_QNN_MAX_DEVICES);
    }

    static struct ggml_backend_buffer_type ggml_backend_buffer_type_qnn = {
            /* .iface   = */ {
                /* .get_name         = */ ggml_backend_qnn_buffer_type_name,
                /* .alloc_buffer     = */ ggml_backend_qnn_buffer_type_alloc_buffer,
                /* .get_alignment    = */ ggml_backend_qnn_buffer_type_get_alignment,
                /* .get_max_size     = */ ggml_backend_qnn_buffer_type_get_max_size,
                /* .get_alloc_size   = */ nullptr,// defaults to ggml_nbytes
                /* .supports_backend = */ ggml_backend_qnn_buffer_type_supports_backend,
                /* .is_host          = */ ggml_backend_qnn_buffer_is_host
            },
            /* .context = */ nullptr,
    };
    LEAVE_FUNC();

    return &ggml_backend_buffer_type_qnn;
}


ggml_backend_t ggml_backend_qnn_init(size_t device) {
    ENTER_FUNC();
    int result = 0;

    //TODO: better method to handle internal state between whisper.cpp/llama.cpp and qnn backend
    if (g_qnn_loaded) {
        LOGGE("qnn backend already loaded, it should not happened, pls check why?");
        if (nullptr != g_qnn_backend) {
            ggml_backend_qnn_free(g_qnn_backend);
            i_alloc_buffer_counts  = 0;
            i_alloc_buffer_sizes  = 0;
            i_get_alloc_counts   = 0;
            i_get_alloc_sizes  = 0;
        }
        //TODO: better method to handle internal state between whisper.cpp/llama.cpp and qnn backend
        //return nullptr;
    }

    LOGGD("device %d", device);
    if (device >= GGML_QNN_MAX_DEVICES) {
        LOGGW("invalid device index %d", device);
        return nullptr;
    }

    LOGGI("enter %s, device %d\n", __func__, device);

    if (QNN_HTP == device) {
        //TODO: hardcode path
        std::string path = "/data/data/com.cdeos.kantv/";
        if (0 == setenv("LD_LIBRARY_PATH",
                        (path +
                         ":/vendor/dsp/cdsp:/vendor/lib64:/vendor/dsp/dsp:/vendor/dsp/images").c_str(),
                        1)) {
            LOGGI("QNN DSP backend setenv successfully");
        } else {
            LOGGE("QNN DSP backend setenv failure");
        }
        if (0 == setenv("ADSP_LIBRARY_PATH",
                        (path +
                         ";/vendor/dsp/cdsp;/vendor/lib/rfsa/adsp;/system/lib/rfsa/adsp;/vendor/dsp/dsp;/vendor/dsp/images;/dsp").c_str(),
                        1)) {
            LOGGI("QNN DSP backend setenv successfully");
        } else {
            LOGGE("QNN DSP backend setenv failure");
        }
    }

    qnn_instance * instance = nullptr;
    //TODO:hardcode QNN lib path
    instance = new qnn_instance("/data/data/com.cdeos.kantv/", g_qnn_mgr[device].lib, "");
    result = instance->qnn_init(nullptr);
    if (0 != result) {
        LOGGW("init qnn subsystem failed with qnn backend %s, pls check why\n", get_qnn_backend_name(device));
        delete instance;
        return nullptr;
    }
    qnn_interface qnn_interface                             = instance->get_qnn_interface();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        delete instance;
        return nullptr;
    }
    std::string device_name = GGML_QNN_NAME + std::string("_") + std::to_string(device) + std::string("_") + get_qnn_backend_name(device);
    LOGGI("qnn device name %s", device_name.c_str());
    g_qnn_mgr[device].instance                  = instance;
    g_qnn_mgr[device].raw_interface             = instance->get_qnn_raw_interface();
    g_qnn_mgr[device].raw_system_interface      = instance->get_qnn_raw_system_interface();
    //TODO:refine internal buffer management
    g_qnn_mgr[device].buffer_pool               = qnn_buf_new(get_qnn_backend_name(device), GGML_QNN_MAX_BUFFERS, (1 << 20));
    GGML_ASSERT(g_qnn_mgr[device].buffer_pool != nullptr);
    g_qnn_loaded                                = true; // TODO: better method to handle multiple QNN device

    ggml_backend_t qnn_backend = new ggml_backend {
            /* .guid      = */ ggml_backend_qnn_guid(),
            /* .interface = */ ggml_backend_qnn_interface,
            /* .context   = */ &g_qnn_mgr[device]
    };

    LEAVE_FUNC();

    //TODO: better method to handle multi backend device
    g_qnn_backend = qnn_backend;

    return qnn_backend;
}


extern "C" int ggml_backend_qnn_reg_devices();

//TODO: better method to handle multi backend device
int ggml_backend_qnn_reg_devices() {
    ENTER_FUNC();
    for (size_t idx = 0; idx < GGML_QNN_MAX_DEVICES; idx++) {
        int id = g_qnn_mgr[idx].device;
        char name[128];
        snprintf(name, sizeof(name), "%s%d", GGML_QNN_NAME, id);
        ggml_backend_register(name, ggml_backend_qnn_reg_init, ggml_backend_qnn_buffer_type(idx),
                              (void *) (intptr_t)idx);
    }
    LEAVE_FUNC();

    return GGML_QNN_MAX_DEVICES;
}
