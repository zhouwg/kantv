/*
 * Copyright (c) 2024- KanTV Authors
 *
 * this is implementation of Android command line application, it's used for verify/develop GGML
 * QNN backend in Android command line mode. it's more convenient then Android APK mode.
 * only used in this project.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * all codes references from:
 * https://github.com/zhouwg/kantv/blob/master/core/ggml/jni/ggml-jni-impl.cpp
 * https://github.com/zhouwg/kantv/blob/master/core/ggml/jni/ggml-jni-impl-external.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>

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
#include <iomanip>
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
#include <HTP/QnnHtpGraph.h>

extern "C" {
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
#include "libavutil/log.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libavutil/myfifo.h"
#include "libavutil/cde_log.h"
#include "libavutil/cde_assert.h"

#if CONFIG_AVFILTER
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#endif
}

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
#include "HTP/QnnHtpCommon.h"
#include "HTP/QnnHtpDevice.h"

#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-qnn.h"
#include "whisper.h"

static void ggml_qnn_log_internal(ggml_log_level level, const char *file, const char *func, int line, const char *format, ...);


#define GGML_QNN_DEBUG              1
#define GGML_QNN_LOGBUF_LEN         4096
#define ENABLE_TEST_WHISPERCPP      1
#define ENABLE_TEST_LLM             0
#define ENABLE_QNN_LOG              0  //enable/disable QNN internal log
#define ENABLE_QNNSDK_LOG           0     // enable/disable QNN SDK's internal log

#define QNN_LOG_ERROR(...)  ggml_qnn_log_internal(GGML_LOG_LEVEL_DEBUG,  __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define QNN_LOG_WARN(...)   ggml_qnn_log_internal(GGML_LOG_LEVEL_DEBUG , __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define QNN_LOG_INFO(...)   ggml_qnn_log_internal(GGML_LOG_LEVEL_DEBUG , __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#if GGML_QNN_DEBUG
#define QNN_LOG_DEBUG(...)  ggml_qnn_log_internal(GGML_LOG_LEVEL_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define QNN_LOG_DEBUG(...)
#endif

#define ENTER_FUNC()

#define LEAVE_FUNC()

#define RPCMEM_DEFAULT_FLAGS        1
#define RPCMEM_HEAP_ID_SYSTEM       25
#define QNN_VER_PTR(x)              (&((x).v1))
#define JNI_BUF_LEN                 4096
#define JNI_TMP_LEN                 256

class qnn_perf {
public:
    qnn_perf(const std::string & perf_name) : _perf_name(std::move(perf_name)) {};
    qnn_perf() = delete;
    qnn_perf(const qnn_perf & ) = delete;
    qnn_perf & operator= (const qnn_perf & ) = delete;

    void start() {
        ggml_time_init();
        _begin_time = ggml_time_us();
    }

    void info() {
        _end_time = ggml_time_us();
        _duration = (_end_time - _begin_time);
        QNN_LOG_DEBUG("duration of %s : %lld microseconds\n", _perf_name.c_str(), _duration);
    }

private:
    int64_t _begin_time = 0LL;
    int64_t _end_time   = 0LL;
    int64_t _duration   = 0LL;
    std::string _perf_name;
};

enum class OutputDataType {
    FLOAT_ONLY, NATIVE_ONLY, FLOAT_AND_NATIVE, INVALID
};


enum class InputDataType {
    FLOAT, NATIVE, INVALID
};


typedef struct GraphInfo {
    Qnn_GraphHandle_t graph;
    char *graphName;
    Qnn_Tensor_t *inputTensors;
    uint32_t numInputTensors;
    Qnn_Tensor_t *outputTensors;
    uint32_t numOutputTensors;
} GraphInfo_t;

typedef GraphInfo_t *GraphInfoPtr_t;

enum class ggml_qnn_profile_level {
    profile_off = 0,
    profile_basic = 1,
    profile_detail = 2
};

typedef struct GraphConfigInfo {
    char *graphName;
    const QnnGraph_Config_t **graphConfigs;
} GraphConfigInfo_t;

typedef int ( *ComposeGraphsHandleType_t)(
        Qnn_BackendHandle_t,
        QNN_INTERFACE_VER_TYPE,
        Qnn_ContextHandle_t,
        const GraphConfigInfo_t **,
        const uint32_t,
        GraphInfo_t ***,
        uint32_t *,
        bool,
        QnnLog_Callback_t,
        QnnLog_Level_t);


typedef int ( *FreeGraphsHandleType_t)(GraphInfo_t ***, uint32_t);

using pfn_rpc_mem_init = void (*)(void);
using pfn_rpc_mem_deinit = void (*)(void);

using pfn_rpc_mem_alloc = void *(*)(int, uint32_t, int);
using pfn_rpc_mem_free = void (*)(void *);
using pfn_rpc_mem_to_fd = int (*)(void *);

using _pfn_QnnSaver_initialize = decltype(QnnSaver_initialize);
using _pfn_QnnInterface_getProviders = decltype(QnnInterface_getProviders);
using _pfn_QnnSystemInterface_getProviders = decltype(QnnSystemInterface_getProviders);

using PopulateInputTensorsRetType_t = std::tuple<int, size_t, size_t>;
using ReadBatchDataRetType_t = std::tuple<int, size_t, size_t>;
using ReadInputListRetType_t = std::tuple<std::vector<std::vector<std::string>>, std::unordered_map<std::string, uint32_t>, bool>;
using ReadInputListsRetType_t = std::tuple<std::vector<std::vector<std::vector<std::string>>>, std::vector<std::unordered_map<std::string, uint32_t>>, bool>;


static uint32_t get_tensor_rank(const ggml_tensor *tensor) {
    uint32_t rank = 0;
    for (int i = 0; i < GGML_MAX_DIMS; i++) {
        if ((0 != tensor->ne[i]) && (1 != tensor->ne[i])) {
            rank++;
        }
    }
    return rank;
}


static uint32_t get_tensor_data_size(const ggml_tensor *tensor) {
    size_t data_size = ggml_row_size(tensor->type, tensor->ne[0]);
    size_t n_dims = get_tensor_rank(tensor);
    for (int i = 1; i < n_dims; i++) {
        data_size *= tensor->ne[i];
    }

    //QNN_LOG_DEBUG("get_tensor_data_size %d\n", data_size);
    //QNN_LOG_DEBUG("ggml_nbytes(tensor) %d\n", ggml_nbytes(tensor));

    return ggml_nbytes(tensor);
}

enum qcom_htp_arch {
    NONE = 0,
    V68 = 68,
    V69 = 69,
    V73 = 73,
    V75 = 75,
};

enum qcom_chipset {
    UNKNOWN_SM = 0,
    SM8450 = 36,  // v69
    SM8475 = 42,  // v69
    SM8550 = 43,  // v73
    SM8650 = 57,  // v75
};

struct qcom_socinfo {
    uint32_t soc_model;
    size_t htp_arch;
    size_t vtcm_size_in_mb;
};

static const char * qnn_get_chipset_desc(uint32_t chipset_id) {
    switch (chipset_id) {
        case SM8450:
            return "SM8450";
        case SM8475:
            return "SM8475";
        case SM8550:
            return "SM8550";
        case SM8650:
            return "SM8650";
        default:
            return "unknown";
    }
}

static const char * qnn_get_htparch_desc(size_t htp_arch) {
    switch (htp_arch) {
        case V68:
            return "QCOM_HTP_V68";
        case V69:
            return "QCOM_HTP_V69";
        case V73:
            return "QCOM_HTP_V73";
        case V75:
            return "QCOM_HTP_V75";
        default:
            return "unknown";
    }
}

inline int validateTensorVersion(Qnn_Tensor_t tensor) {
    if (tensor.version != QNN_TENSOR_VERSION_1) {
        LOGGW("validateTensorVersion() tensor %s, got unsupported version %d\n",
              tensor.v1.name,
              tensor.version);
        return 1;
    }
    return 0;
}


inline int validateOpConfigVersion(Qnn_OpConfig_t opConfig) {
    if (opConfig.version != QNN_OPCONFIG_VERSION_1) {
        LOGGW("validateOpConfigVersion() op %s, got unsupported version %d\n",
              opConfig.v1.name,
              opConfig.version);
        return 1;
    }
    return 0;
}

#define VALIDATE_TENSOR_VERSION(tensor, err)            VALIDATE(validateTensorVersion(tensor), err)

#define VALIDATE_OP_CONFIG_VERSION(op, err)             VALIDATE(validateOpConfigVersion(op), err)

#define FREE_MEMORY(ptr1, ptr2, ptr3) \
  do {                                \
    free(ptr1);                       \
    free(ptr2);                       \
    free(ptr3);                       \
  } while (0)

inline const char *getQnnOpConfigName(const Qnn_OpConfig_t &opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.name;
    }
    return NULL;
}


inline const char *getQnnOpConfigName(const Qnn_OpConfig_t *opConfig) {
    return getQnnOpConfigName(*opConfig);
}


inline const char *getQnnOpConfigPackageName(const Qnn_OpConfig_t &opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.packageName;
    }
    return NULL;
}


inline const char *getQnnOpConfigPackageName(const Qnn_OpConfig_t *opConfig) {
    return getQnnOpConfigPackageName(*opConfig);
}

inline const char *getQnnOpConfigTypeName(const Qnn_OpConfig_t &opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.typeName;
    }
    return NULL;
}


inline const char *getQnnOpConfigTypeName(const Qnn_OpConfig_t *opConfig) {
    return getQnnOpConfigTypeName(*opConfig);
}

inline uint32_t getQnnOpConfigNumParams(const Qnn_OpConfig_t &opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.numOfParams;
    }
    return 0u;
}


inline uint32_t getQnnOpConfigNumParams(const Qnn_OpConfig_t *opConfig) {
    return getQnnOpConfigNumParams(*opConfig);
}


inline const Qnn_Param_t *getQnnOpConfigParams(const Qnn_OpConfig_t &opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.params;
    }
    return NULL;
}


inline const Qnn_Param_t *getQnnOpConfigParams(const Qnn_OpConfig_t *opConfig) {
    return getQnnOpConfigParams(*opConfig);
}


inline uint32_t getQnnOpConfigNumInputs(const Qnn_OpConfig_t &opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.numOfInputs;
    }
    return 0u;
}


inline uint32_t getQnnOpConfigNumInputs(const Qnn_OpConfig_t *opConfig) {
    return getQnnOpConfigNumInputs(*opConfig);
}


inline const Qnn_Tensor_t *getQnnOpConfigInputs(const Qnn_OpConfig_t &opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.inputTensors;
    }
    return NULL;
}

inline const Qnn_Tensor_t *getQnnOpConfigInputs(const Qnn_OpConfig_t *opConfig) {
    return getQnnOpConfigInputs(*opConfig);
}


inline uint32_t getQnnOpConfigNumOutputs(const Qnn_OpConfig_t &opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.numOfOutputs;
    }
    return 0u;
}


inline uint32_t getQnnOpConfigNumOutputs(const Qnn_OpConfig_t *opConfig) {
    return getQnnOpConfigNumOutputs(*opConfig);
}


inline const Qnn_Tensor_t *getQnnOpConfigOutputs(const Qnn_OpConfig_t &opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.outputTensors;
    }
    return NULL;
}


inline const Qnn_Tensor_t *getQnnOpConfigOutputs(const Qnn_OpConfig_t *opConfig) {
    return getQnnOpConfigOutputs(*opConfig);
}

inline void setQnnOpConfigName(Qnn_OpConfig_t &opConfig, const char *name) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.name = name;
    }
}


inline void setQnnOpConfigName(Qnn_OpConfig_t *opConfig, const char *name) {
    setQnnOpConfigName(*opConfig, name);
}

inline void setQnnOpConfigPackageName(Qnn_OpConfig_t &opConfig, const char *packageName) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.packageName = packageName;
    }
}


inline void setQnnOpConfigPackageName(Qnn_OpConfig_t *opConfig, const char *packageName) {
    setQnnOpConfigPackageName(*opConfig, packageName);
}


inline void setQnnOpConfigTypeName(Qnn_OpConfig_t &opConfig, const char *typeName) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.typeName = typeName;
    }
}


inline void setQnnOpConfigTypeName(Qnn_OpConfig_t *opConfig, const char *typeName) {
    setQnnOpConfigTypeName(*opConfig, typeName);
}


inline void setQnnOpConfigParams(Qnn_OpConfig_t &opConfig,
                                 uint32_t numOfParams,
                                 Qnn_Param_t *params) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.numOfParams = numOfParams;
        opConfig.v1.params = params;
    }
}


inline void setQnnOpConfigParams(Qnn_OpConfig_t *opConfig,
                                 uint32_t numOfParams,
                                 Qnn_Param_t *params) {
    setQnnOpConfigParams(*opConfig, numOfParams, params);
}


inline void setQnnOpConfigInputs(Qnn_OpConfig_t &opConfig,
                                 uint32_t numOfInputs,
                                 Qnn_Tensor_t *inputTensors) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.numOfInputs = numOfInputs;
        opConfig.v1.inputTensors = inputTensors;
    }
}


inline void setQnnOpConfigInputs(Qnn_OpConfig_t *opConfig,
                                 uint32_t numOfInputs,
                                 Qnn_Tensor_t *inputTensors) {
    setQnnOpConfigInputs(*opConfig, numOfInputs, inputTensors);
}


inline void setQnnOpConfigOutputs(Qnn_OpConfig_t &opConfig,
                                  uint32_t numOfOutputs,
                                  Qnn_Tensor_t *outputTensors) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.numOfOutputs = numOfOutputs;
        opConfig.v1.outputTensors = outputTensors;
    }
}


inline void setQnnOpConfigOutputs(Qnn_OpConfig_t *opConfig,
                                  uint32_t numOfOutputs,
                                  Qnn_Tensor_t *outputTensors) {
    setQnnOpConfigOutputs(*opConfig, numOfOutputs, outputTensors);
}

inline uint32_t getQnnTensorId(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.id;
    }
    return 0u;
}


inline uint32_t getQnnTensorId(const Qnn_Tensor_t *tensor) { return getQnnTensorId(*tensor); }

inline const char *getQnnTensorName(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.name;
    }
    return 0u;
}


inline const char *getQnnTensorName(const Qnn_Tensor_t *tensor) {
    return getQnnTensorName(*tensor);
}

inline Qnn_TensorType_t getQnnTensorType(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.type;
    }
    return QNN_TENSOR_TYPE_UNDEFINED;
}

inline Qnn_TensorType_t getQnnTensorType(const Qnn_Tensor_t *tensor) {
    return getQnnTensorType(*tensor);
}


inline Qnn_TensorDataFormat_t getQnnTensorDataFormat(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.dataFormat;
    }
    return QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER;
}


inline Qnn_TensorDataFormat_t getQnnTensorDataFormat(const Qnn_Tensor_t *tensor) {
    return getQnnTensorDataFormat(*tensor);
}

inline Qnn_DataType_t getQnnTensorDataType(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.dataType;
    }
    return QNN_DATATYPE_UNDEFINED;
}

inline Qnn_DataType_t getQnnTensorDataType(const Qnn_Tensor_t *tensor) {
    return getQnnTensorDataType(*tensor);
}

inline Qnn_QuantizeParams_t getQnnTensorQuantParams(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.quantizeParams;
    }
    return QNN_QUANTIZE_PARAMS_INIT;
}

inline Qnn_QuantizeParams_t getQnnTensorQuantParams(const Qnn_Tensor_t *tensor) {
    return getQnnTensorQuantParams(*tensor);
}

inline uint32_t getQnnTensorRank(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.rank;
    }
    return 0u;
}

inline uint32_t getQnnTensorRank(const Qnn_Tensor_t *tensor) { return getQnnTensorRank(*tensor); }


inline uint32_t *getQnnTensorDimensions(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.dimensions;
    }
    return NULL;
}


inline uint32_t *getQnnTensorDimensions(const Qnn_Tensor_t *tensor) {
    return getQnnTensorDimensions(*tensor);
}


inline Qnn_TensorMemType_t getQnnTensorMemType(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.memType;
    }
    return QNN_TENSORMEMTYPE_UNDEFINED;
}


inline Qnn_TensorMemType_t getQnnTensorMemType(const Qnn_Tensor_t *tensor) {
    return getQnnTensorMemType(*tensor);
}

inline Qnn_ClientBuffer_t getQnnTensorClientBuf(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.clientBuf;
    }
    return QNN_CLIENT_BUFFER_INIT;
}

inline Qnn_ClientBuffer_t getQnnTensorClientBuf(const Qnn_Tensor_t *tensor) {
    return getQnnTensorClientBuf(*tensor);
}


inline Qnn_MemHandle_t getQnnTensorMemHandle(const Qnn_Tensor_t &tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.memHandle;
    }
    return NULL;
}


inline Qnn_MemHandle_t getQnnTensorMemHandle(const Qnn_Tensor_t *tensor) {
    return getQnnTensorMemHandle(*tensor);
}


inline void setQnnTensorId(Qnn_Tensor_t &tensor, uint32_t id) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.id = id;
    }
}


inline void setQnnTensorId(Qnn_Tensor_t *tensor, uint32_t id) { setQnnTensorId(*tensor, id); }

inline void setQnnTensorName(Qnn_Tensor_t &tensor, const char *name) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.name = name;
    }
}


inline void setQnnTensorName(Qnn_Tensor_t *tensor, const char *name) {
    setQnnTensorName(*tensor, name);
}


inline void setQnnTensorType(Qnn_Tensor_t &tensor, Qnn_TensorType_t type) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.type = type;
    }
}


inline void setQnnTensorType(Qnn_Tensor_t *tensor, Qnn_TensorType_t type) {
    setQnnTensorType(*tensor, type);
}


inline void setQnnTensorDataFormat(Qnn_Tensor_t &tensor, Qnn_TensorDataFormat_t format) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.dataFormat = format;
    }
}


inline void setQnnTensorDataFormat(Qnn_Tensor_t *tensor, Qnn_TensorDataFormat_t format) {
    setQnnTensorDataFormat(*tensor, format);
}


inline void setQnnTensorDataType(Qnn_Tensor_t &tensor, Qnn_DataType_t dataType) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.dataType = dataType;
    }
}


inline void setQnnTensorDataType(Qnn_Tensor_t *tensor, Qnn_DataType_t dataType) {
    setQnnTensorDataType(*tensor, dataType);
}


inline void setQnnTensorQuantParams(Qnn_Tensor_t &tensor, Qnn_QuantizeParams_t params) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.quantizeParams = params;
    }
}


inline void setQnnTensorQuantParams(Qnn_Tensor_t *tensor, Qnn_QuantizeParams_t params) {
    setQnnTensorQuantParams(*tensor, params);
}


inline void setQnnTensorRank(Qnn_Tensor_t &tensor, uint32_t rank) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.rank = rank;
    }
}


inline void setQnnTensorRank(Qnn_Tensor_t *tensor, uint32_t rank) {
    setQnnTensorRank(*tensor, rank);
}


inline void setQnnTensorDimensions(Qnn_Tensor_t &tensor, uint32_t *dims) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.dimensions = dims;
    }
}


inline void setQnnTensorDimensions(Qnn_Tensor_t *tensor, uint32_t *dims) {
    setQnnTensorDimensions(*tensor, dims);
}


inline void setQnnTensorMemType(Qnn_Tensor_t &tensor, Qnn_TensorMemType_t memType) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.memType = memType;
    }
}


inline void setQnnTensorMemType(Qnn_Tensor_t *tensor, Qnn_TensorMemType_t memType) {
    setQnnTensorMemType(*tensor, memType);
}


inline void setQnnTensorClientBuf(Qnn_Tensor_t &tensor, Qnn_ClientBuffer_t clientBuf) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.clientBuf = clientBuf;
    }
}


inline void setQnnTensorClientBuf(Qnn_Tensor_t *tensor, Qnn_ClientBuffer_t clientBuf) {
    setQnnTensorClientBuf(*tensor, clientBuf);
}


inline void setQnnTensorMemHandle(Qnn_Tensor_t &tensor, Qnn_MemHandle_t handle) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.memHandle = handle;
    }
}


inline void setQnnTensorMemHandle(Qnn_Tensor_t *tensor, Qnn_MemHandle_t handle) {
    setQnnTensorMemHandle(*tensor, handle);
}

const std::map<Qnn_DataType_t, size_t> g_dataTypeToSize = {
        {QNN_DATATYPE_INT_8,           1},
        {QNN_DATATYPE_INT_16,          2},
        {QNN_DATATYPE_INT_32,          4},
        {QNN_DATATYPE_INT_64,          8},
        {QNN_DATATYPE_UINT_8,          1},
        {QNN_DATATYPE_UINT_16,         2},
        {QNN_DATATYPE_UINT_32,         4},
        {QNN_DATATYPE_UINT_64,         8},
        {QNN_DATATYPE_FLOAT_16,        2},
        {QNN_DATATYPE_FLOAT_32,        4},
        {QNN_DATATYPE_FLOAT_64,        8},
        {QNN_DATATYPE_SFIXED_POINT_8,  1},
        {QNN_DATATYPE_SFIXED_POINT_16, 2},
        {QNN_DATATYPE_SFIXED_POINT_32, 4},
        {QNN_DATATYPE_UFIXED_POINT_8,  1},
        {QNN_DATATYPE_UFIXED_POINT_16, 2},
        {QNN_DATATYPE_UFIXED_POINT_32, 4},
        {QNN_DATATYPE_BOOL_8,          1},
};

static std::tuple<int, size_t> getDataTypeSizeInBytes(Qnn_DataType_t dataType) {
    if (g_dataTypeToSize.find(dataType) == g_dataTypeToSize.end()) {
        LOGGW("Invalid qnn data type provided");
        return std::make_tuple(1, 0);
    }
    return std::make_tuple(0, g_dataTypeToSize.find(dataType)->second);
}


static size_t calculateElementCount(std::vector<size_t> dims) {
    if (dims.size() == 0) {
        return 0;
    }
    return std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<size_t>());
}


static std::tuple<int, size_t> calculateLength(std::vector<size_t> dims,
                                               Qnn_DataType_t dataType) {
    if (dims.size() == 0) {
        LOGGW("dims.size() is zero");
        return std::make_tuple(1, 0);
    }
    int returnStatus = 0;
    size_t length{0};
    std::tie(returnStatus, length) = getDataTypeSizeInBytes(dataType);
    if (0 != returnStatus) {
        return std::make_tuple(returnStatus, 0);
    }
    length *= calculateElementCount(dims);
    return std::make_tuple(0, length);
}

// Accessors for QNN Op Config
#define QNN_OP_CFG_GET_NAME(opConfig)         getQnnOpConfigName(opConfig)
#define QNN_OP_CFG_GET_PACKAGE_NAME(opConfig) getQnnOpConfigPackageName(opConfig)
#define QNN_OP_CFG_GET_TYPE_NAME(opConfig)    getQnnOpConfigTypeName(opConfig)
#define QNN_OP_CFG_GET_NUM_PARAMS(opConfig)   getQnnOpConfigNumParams(opConfig)
#define QNN_OP_CFG_GET_PARAMS(opConfig)       getQnnOpConfigParams(opConfig)
#define QNN_OP_CFG_GET_NUM_INPUTS(opConfig)   getQnnOpConfigNumInputs(opConfig)
#define QNN_OP_CFG_GET_INPUTS(opConfig)       getQnnOpConfigInputs(opConfig)
#define QNN_OP_CFG_GET_NUM_OUTPUTS(opConfig)  getQnnOpConfigNumOutputs(opConfig)
#define QNN_OP_CFG_GET_OUTPUTS(opConfig)      getQnnOpConfigOutputs(opConfig)

// Modifiers for QNN Op Config
#define QNN_OP_CFG_SET_NAME(opConfig, value)         setQnnOpConfigName(opConfig, value)
#define QNN_OP_CFG_SET_PACKAGE_NAME(opConfig, value) setQnnOpConfigPackageName(opConfig, value)
#define QNN_OP_CFG_SET_TYPE_NAME(opConfig, value)    setQnnOpConfigTypeName(opConfig, value)
#define QNN_OP_CFG_SET_PARAMS(opConfig, numOfParams, params) \
  setQnnOpConfigParams(opConfig, numOfParams, params)
#define QNN_OP_CFG_SET_INPUTS(opConfig, numOfInputs, inputTensors) \
  setQnnOpConfigInputs(opConfig, numOfInputs, inputTensors)
#define QNN_OP_CFG_SET_OUTPUTS(opConfig, numOfOutputs, outputTensors) \
  setQnnOpConfigOutputs(opConfig, numOfOutputs, outputTensors)


// Accessors for QNN Tensor
#define QNN_TENSOR_GET_ID(tensor)           getQnnTensorId(tensor)
#define QNN_TENSOR_GET_NAME(tensor)         getQnnTensorName(tensor)
#define QNN_TENSOR_GET_TYPE(tensor)         getQnnTensorType(tensor)
#define QNN_TENSOR_GET_DATA_FORMAT(tensor)  getQnnTensorDataFormat(tensor)
#define QNN_TENSOR_GET_DATA_TYPE(tensor)    getQnnTensorDataType(tensor)
#define QNN_TENSOR_GET_QUANT_PARAMS(tensor) getQnnTensorQuantParams(tensor)
#define QNN_TENSOR_GET_RANK(tensor)         getQnnTensorRank(tensor)
#define QNN_TENSOR_GET_DIMENSIONS(tensor)   getQnnTensorDimensions(tensor)
#define QNN_TENSOR_GET_MEM_TYPE(tensor)     getQnnTensorMemType(tensor)
#define QNN_TENSOR_GET_CLIENT_BUF(tensor)   getQnnTensorClientBuf(tensor)
#define QNN_TENSOR_GET_MEM_HANDLE(tensor)   getQnnTensorMemHandle(tensor)

// Modifiers for QNN Tensor
#define QNN_TENSOR_SET_ID(tensor, value)           setQnnTensorId(tensor, value)
#define QNN_TENSOR_SET_NAME(tensor, value)         setQnnTensorName(tensor, value)
#define QNN_TENSOR_SET_TYPE(tensor, value)         setQnnTensorType(tensor, value)
#define QNN_TENSOR_SET_DATA_FORMAT(tensor, value)  setQnnTensorDataFormat(tensor, value)
#define QNN_TENSOR_SET_DATA_TYPE(tensor, value)    setQnnTensorDataType(tensor, value)
#define QNN_TENSOR_SET_QUANT_PARAMS(tensor, value) setQnnTensorQuantParams(tensor, value)
#define QNN_TENSOR_SET_RANK(tensor, value)         setQnnTensorRank(tensor, value)
#define QNN_TENSOR_SET_DIMENSIONS(tensor, value)   setQnnTensorDimensions(tensor, value)
#define QNN_TENSOR_SET_MEM_TYPE(tensor, value)     setQnnTensorMemType(tensor, value)
#define QNN_TENSOR_SET_CLIENT_BUF(tensor, value)   setQnnTensorClientBuf(tensor, value)
#define QNN_TENSOR_SET_MEM_HANDLE(tensor, value)   setQnnTensorMemHandle(tensor, value)


#define VALIDATE(value, status)                         \
  do {                                                  \
    status = value;                                     \
    if (status != QNN_SUCCESS) {                        \
      LOGGW("%s expected QNN_SUCCESS\n", #value);       \
      return status;                                    \
    }                                                   \
  } while (0)

static size_t memscpy(void *dst, size_t dstSize, const void *src, size_t copySize) {
    if (!dst || !src || !dstSize || !copySize) return 0;

    size_t minSize = dstSize < copySize ? dstSize : copySize;

    memcpy(dst, src, minSize);

    return minSize;
}


static char *ggml_qnn_strndup(const char *source, size_t maxlen) {
    return ::strndup(source, maxlen);
}


static int deepCopyQnnTensors(Qnn_Tensor_t &src, Qnn_Tensor_t &dst) {
    int err = 0;
    VALIDATE_TENSOR_VERSION(src, err);

    dst.version = src.version;
    QNN_TENSOR_SET_NAME(
            dst, ggml_qnn_strndup(QNN_TENSOR_GET_NAME(src),
                                  std::string(QNN_TENSOR_GET_NAME(src)).size()));
    if (QNN_TENSOR_GET_NAME(dst) == nullptr) {
        return 1;
    }
    QNN_TENSOR_SET_ID(dst, QNN_TENSOR_GET_ID(src));
    QNN_TENSOR_SET_TYPE(dst, QNN_TENSOR_GET_TYPE(src));
    QNN_TENSOR_SET_DATA_FORMAT(dst, QNN_TENSOR_GET_DATA_FORMAT(src));
    QNN_TENSOR_SET_DATA_TYPE(dst, QNN_TENSOR_GET_DATA_TYPE(src));
    QNN_TENSOR_SET_MEM_TYPE(dst, QNN_TENSOR_GET_MEM_TYPE(src));

    // Only metadata (i.e. non-static data) is copied from source to destination. The union still
    // must be initialized so that the clientBuf/memHandle do not contain garbage data
    if (QNN_TENSOR_GET_MEM_TYPE(src) == QNN_TENSORMEMTYPE_RAW) {
        Qnn_ClientBuffer_t clientBuf = {nullptr, 0};
        QNN_TENSOR_SET_CLIENT_BUF(dst, clientBuf);
    } else if (QNN_TENSOR_GET_MEM_TYPE(src) == QNN_TENSORMEMTYPE_MEMHANDLE) {
        QNN_TENSOR_SET_MEM_HANDLE(dst, nullptr);
    } else {
        return 1;
    }

    Qnn_QuantizeParams_t srcQParam = QNN_TENSOR_GET_QUANT_PARAMS(src);
    Qnn_QuantizationEncoding_t encoding = srcQParam.quantizationEncoding;
    if (encoding == QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET) {
        // need to allocate and copy memory for scaleOffset as it is a pointer array
        Qnn_QuantizeParams_t srcQParamCpy = srcQParam;
        Qnn_AxisScaleOffset_t &axisScaleOffset = srcQParamCpy.axisScaleOffsetEncoding;
        Qnn_ScaleOffset_t **scaleOffset = &axisScaleOffset.scaleOffset;
        size_t scaleOffsetSize = axisScaleOffset.numScaleOffsets * sizeof(Qnn_ScaleOffset_t);
        *scaleOffset = (Qnn_ScaleOffset_t *) malloc(scaleOffsetSize);
        memscpy(*scaleOffset,
                scaleOffsetSize,
                srcQParam.axisScaleOffsetEncoding.scaleOffset,
                scaleOffsetSize);
        QNN_TENSOR_SET_QUANT_PARAMS(dst, srcQParamCpy);
    } else if (encoding == QNN_QUANTIZATION_ENCODING_BW_AXIS_SCALE_OFFSET) {
        // need to allocate and copy memory for scaleOffset as it is a pointer array
        Qnn_QuantizeParams_t srcQParamCpy = srcQParam;
        Qnn_BwAxisScaleOffset_t &bwAxisScaleOffset = srcQParamCpy.bwAxisScaleOffsetEncoding;
        size_t scaleSize = bwAxisScaleOffset.numElements * sizeof(float);
        float **scales = &bwAxisScaleOffset.scales;
        int32_t **offsets = &bwAxisScaleOffset.offsets;
        *scales = (float *) malloc(scaleSize);
        memscpy(*scales, scaleSize, srcQParam.bwAxisScaleOffsetEncoding.scales, scaleSize);

        // Only copy offsets if present, nullptr implies all offsets are 0
        if (bwAxisScaleOffset.offsets != nullptr) {
            size_t offsetSize = bwAxisScaleOffset.numElements * sizeof(int32_t);
            *offsets = (int32_t *) malloc(offsetSize);
            memscpy(*offsets, offsetSize, srcQParam.bwAxisScaleOffsetEncoding.offsets, offsetSize);
        }
        QNN_TENSOR_SET_QUANT_PARAMS(dst, srcQParamCpy);
    } else {
        QNN_TENSOR_SET_QUANT_PARAMS(dst, srcQParam);
    }

    // need to allocate and copy memory for all the pointer members
    uint32_t rank = QNN_TENSOR_GET_RANK(src);
    QNN_TENSOR_SET_RANK(dst, rank);
    size_t dimSize = rank * sizeof(uint32_t);
    uint32_t *dimensions = (uint32_t *) malloc(dimSize);
    if (dimensions == nullptr) {
        LOGGW("deepCopyQnnTensors() allocation error while copying tensor %s\n",
              QNN_TENSOR_GET_NAME(src));
        return 1;
    }
    memscpy(dimensions, dimSize, QNN_TENSOR_GET_DIMENSIONS(src), dimSize);
    QNN_TENSOR_SET_DIMENSIONS(dst, dimensions);

    return err;
}

static int writeBinaryToFile(std::string fileDir,
                             std::string fileName,
                             uint8_t *buffer,
                             size_t bufferSize) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return 1;
    }
    if (!mkdir(fileDir.c_str(), 0777)) {
        LOGGW("Failed to create output directory: %s", fileDir.c_str());
        return 1;
    }
    const std::string outputPath(fileDir + "/" + fileName);
    std::ofstream os(outputPath, std::ofstream::binary);
    if (!os) {
        LOGGW("Failed to open output file for writing: %s", outputPath.c_str());
        return 1;
    }
    os.write(reinterpret_cast<char *>(buffer), bufferSize);
    return 0;
}


template<typename T_QuantType>
int floatToTfN(
        T_QuantType *out, float *in, int32_t offset, float scale, size_t numElements) {
    static_assert(std::is_unsigned<T_QuantType>::value, "floatToTfN supports unsigned only!");

    if (nullptr == out || nullptr == in) {
        LOGGW("received a nullptr");
        return 1;
    }

    size_t dataTypeSizeInBytes = sizeof(T_QuantType);
    size_t bitWidth = dataTypeSizeInBytes * 8;;
    double trueBitWidthMax = pow(2, bitWidth) - 1;
    double encodingMin = offset * scale;
    double encodingMax = (trueBitWidthMax + offset) * scale;
    double encodingRange = encodingMax - encodingMin;

    for (size_t i = 0; i < numElements; ++i) {
        int quantizedValue = round(trueBitWidthMax * (in[i] - encodingMin) / encodingRange);
        if (quantizedValue < 0)
            quantizedValue = 0;
        else if (quantizedValue > (int) trueBitWidthMax)
            quantizedValue = (int) trueBitWidthMax;
        out[i] = static_cast<T_QuantType>(quantizedValue);
    }
    return 0;
}

template int floatToTfN<uint8_t>(
        uint8_t *out, float *in, int32_t offset, float scale, size_t numElements);

template int floatToTfN<uint16_t>(
        uint16_t *out, float *in, int32_t offset, float scale, size_t numElements);

template<typename T_QuantType>
int tfNToFloat(
        float *out, T_QuantType *in, int32_t offset, float scale, size_t numElements) {
    static_assert(std::is_unsigned<T_QuantType>::value, "tfNToFloat supports unsigned only!");

    if (nullptr == out || nullptr == in) {
        LOGGW("received a nullptr");
        return 1;
    }
    for (size_t i = 0; i < numElements; i++) {
        double quantizedValue = static_cast<double>(in[i]);
        double offsetDouble = static_cast<double>(offset);
        out[i] = static_cast<double>((quantizedValue + offsetDouble) * scale);
    }
    return 0;
}

template int tfNToFloat<uint8_t>(
        float *out, uint8_t *in, int32_t offset, float scale, size_t numElements);

template int tfNToFloat<uint16_t>(
        float *out, uint16_t *in, int32_t offset, float scale, size_t numElements);

template<typename T_QuantType>
int castToFloat(float *out, T_QuantType *in, size_t numElements) {
    if (nullptr == out || nullptr == in) {
        LOGGW("Received a nullptr");
        return 1;
    }
    for (size_t i = 0; i < numElements; i++) {
        out[i] = static_cast<float>(in[i]);
    }
    return 0;
}

template int castToFloat<uint8_t>(float *out,
                                  uint8_t *in,
                                  size_t numElements);

template int castToFloat<uint16_t>(float *out,
                                   uint16_t *in,
                                   size_t numElements);

template int castToFloat<uint32_t>(float *out,
                                   uint32_t *in,
                                   size_t numElements);

template int castToFloat<int8_t>(float *out,
                                 int8_t *in,
                                 size_t numElements);

template int castToFloat<int16_t>(float *out,
                                  int16_t *in,
                                  size_t numElements);

template int castToFloat<int32_t>(float *out,
                                  int32_t *in,
                                  size_t numElements);

template<typename T_QuantType>
int castFromFloat(T_QuantType *out, float *in, size_t numElements) {
    if (nullptr == out || nullptr == in) {
        LOGGW("received a nullptr");
        return 1;
    }
    for (size_t i = 0; i < numElements; i++) {
        out[i] = static_cast<T_QuantType>(in[i]);
    }
    return 0;
}

template int castFromFloat<uint8_t>(uint8_t *out,
                                    float *in,
                                    size_t numElements);

template int castFromFloat<uint16_t>(uint16_t *out,
                                     float *in,
                                     size_t numElements);

template int castFromFloat<uint32_t>(uint32_t *out,
                                     float *in,
                                     size_t numElements);

template int castFromFloat<int8_t>(int8_t *out,
                                   float *in,
                                   size_t numElements);

template int castFromFloat<int16_t>(int16_t *out,
                                    float *in,
                                    size_t numElements);

template int castFromFloat<int32_t>(int32_t *out,
                                    float *in,
                                    size_t numElements);


static intptr_t alignTo(size_t alignment, intptr_t offset) {
    return offset % alignment == 0 ? offset
                                   : offset +
                                     (static_cast<intptr_t>(alignment) -
                                      offset % static_cast<intptr_t>(alignment));
}

template<typename Fn>
Fn load_qnn_functionpointers(void *handle, const char *function_name) {
    return reinterpret_cast<Fn>(dlsym(handle, function_name));
}


static void ggml_qnn_create_directory(const std::string &path) {
    if (path.empty()) {
        LOGGD("invalid param\n");
        return;
    }
    std::size_t pos = path.find_last_of('/');
    std::string subdir = (std::string::npos == pos) ? "" : path.substr(0, pos);
    if (subdir.empty() || subdir == "." || subdir == "..") {
        return;
    }
    ggml_qnn_create_directory(subdir);
    int mkdir_err = mkdir(subdir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (mkdir_err != 0 && errno != EEXIST) {
        std::string err_msg = "failed to create " + subdir + " folder\n";
        LOGGW(err_msg.c_str());
    }
}


static ReadBatchDataRetType_t readBatchDataAndUpdateQueue(
        std::queue<std::string> &filePaths,
        std::vector<size_t> dims,
        Qnn_DataType_t dataType,
        uint8_t *buffer) {

    return std::make_tuple(1, 0, 0);

}

static bool deepCopyQnnTensorInfo(Qnn_Tensor_t *dst, const Qnn_Tensor_t *src) {
    if (nullptr == dst || nullptr == src) {
        LOGGW("Received nullptr");
        return false;
    }
    // set tensor.version before using QNN_TENSOR_SET macros, as they require the version to be set
    // to correctly assign values
    dst->version = src->version;
    const char *tensorName = QNN_TENSOR_GET_NAME(src);
    if (!tensorName) {
        QNN_TENSOR_SET_NAME(dst, nullptr);
    } else {
        QNN_TENSOR_SET_NAME(dst, strndup(tensorName, strlen(tensorName)));
    }
    QNN_TENSOR_SET_ID(dst, QNN_TENSOR_GET_ID(src));
    QNN_TENSOR_SET_TYPE(dst, QNN_TENSOR_GET_TYPE(src));
    QNN_TENSOR_SET_DATA_FORMAT(dst, QNN_TENSOR_GET_DATA_FORMAT(src));
    QNN_TENSOR_SET_DATA_TYPE(dst, QNN_TENSOR_GET_DATA_TYPE(src));
    Qnn_QuantizeParams_t qParams = QNN_QUANTIZE_PARAMS_INIT;
    qParams.encodingDefinition = QNN_TENSOR_GET_QUANT_PARAMS(src).encodingDefinition;
    qParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_UNDEFINED;
    if (QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding ==
        QNN_QUANTIZATION_ENCODING_SCALE_OFFSET) {
        qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding;
        qParams.scaleOffsetEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).scaleOffsetEncoding;
    } else if (QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding ==
               QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET) {
        qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding;
        qParams.axisScaleOffsetEncoding.axis =
                QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.axis;
        qParams.axisScaleOffsetEncoding.numScaleOffsets =
                QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets;
        if (QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets > 0) {
            qParams.axisScaleOffsetEncoding.scaleOffset = (Qnn_ScaleOffset_t *) malloc(
                    QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets *
                    sizeof(Qnn_ScaleOffset_t));
            if (qParams.axisScaleOffsetEncoding.scaleOffset) {
                for (size_t idx = 0;
                     idx < QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets;
                     idx++) {
                    qParams.axisScaleOffsetEncoding.scaleOffset[idx].scale =
                            QNN_TENSOR_GET_QUANT_PARAMS(
                                    src).axisScaleOffsetEncoding.scaleOffset[idx].scale;
                    qParams.axisScaleOffsetEncoding.scaleOffset[idx].offset =
                            QNN_TENSOR_GET_QUANT_PARAMS(
                                    src).axisScaleOffsetEncoding.scaleOffset[idx].offset;
                }
            }
        }
    }
    QNN_TENSOR_SET_QUANT_PARAMS(dst, qParams);
    QNN_TENSOR_SET_RANK(dst, QNN_TENSOR_GET_RANK(src));
    QNN_TENSOR_SET_DIMENSIONS(dst, nullptr);
    if (QNN_TENSOR_GET_RANK(src) > 0) {
        QNN_TENSOR_SET_DIMENSIONS(dst,
                                  (uint32_t *) malloc(QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t)));
        if (QNN_TENSOR_GET_DIMENSIONS(dst)) {
            memscpy(QNN_TENSOR_GET_DIMENSIONS(dst),
                    QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t),
                    QNN_TENSOR_GET_DIMENSIONS(src),
                    QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t));
        }
    }
    return true;
}


static int freeQnnTensor(Qnn_Tensor_t &tensor) {
    int err = 0;
    VALIDATE_TENSOR_VERSION(tensor, err);

    free((void *) QNN_TENSOR_GET_NAME(tensor));
    free(QNN_TENSOR_GET_DIMENSIONS(tensor));

    return err;
}

static int freeQnnTensors(Qnn_Tensor_t *&tensors, uint32_t numTensors) {
    int err = 0;

    // free all pointer allocations in struct
    for (size_t i = 0; i < numTensors; i++) {
        freeQnnTensor(tensors[i]);
    }
    free(tensors);

    return err;
}


enum ut_test_type {
    TEST_SIMPLE_UT = 0,
    TEST_AUTOMATION_UT,
    TEST_WHISPERCPP,
    TEST_QNN_FUZZ,
    TEST_QNN_RPC,
    TEST_LLM,
    UT_COUNTS
};

static void tensor_dump(const ggml_tensor *tensor, const char *name);

#define TENSOR_DUMP(tensor) tensor_dump(tensor, #tensor)

static void ggml_qnn_log_internal(ggml_log_level level, const char *file, const char *func, int line, const char *format, ...) {
    static std::mutex ggml_qnn_log_internal_mutex;
    static char s_ggml_qnn_log_internal_buf[GGML_QNN_LOGBUF_LEN];

    {
        std::lock_guard<std::mutex> lock(ggml_qnn_log_internal_mutex);
        va_list args;
        va_start(args, format);
        int len_prefix = snprintf(s_ggml_qnn_log_internal_buf, GGML_QNN_LOGBUF_LEN, "[%s, %d]: ",
                                  func, line);
        int len = vsnprintf(s_ggml_qnn_log_internal_buf + len_prefix,
                            GGML_QNN_LOGBUF_LEN - len_prefix, format, args);
        if (len < (GGML_QNN_LOGBUF_LEN - len_prefix)) {
            //for Android command line application or WoA
            printf("%s", s_ggml_qnn_log_internal_buf);
        }
        va_end(args);
    }
}


static const char *get_qnn_backend_name(int n_backend_type) {
    switch (n_backend_type) {
        case 0:
            return "QNN-CPU";
        case 1:
            return "QNN-GPU";
        case 2:
            return "QNN-NPU";
        case 3:
            return "ggml";
        default:
            return "unknown";
    }
}


static bool ggml_graph_compute_helper(
        struct ggml_backend *backend,
        struct ggml_cgraph *graph,
        std::vector<uint8_t> &buf,
        int n_threads,
        ggml_abort_callback abort_callback,
        void *abort_callback_data) {
    struct ggml_cplan plan = ggml_graph_plan(graph, n_threads);

    plan.abort_callback = abort_callback;
    plan.abort_callback_data = abort_callback_data;

    if (plan.work_size > 0) {
        buf.resize(plan.work_size);
        plan.work_data = buf.data();
    }

    if (ggml_backend_is_cpu(backend)) {
        ggml_backend_cpu_set_n_threads(backend, n_threads);
    }

#ifdef GGML_USE_QNN
    if (ggml_backend_is_qnn(backend)) {
        ggml_backend_qnn_set_n_threads(backend, n_threads);
    }
#endif

    //a new approch of mixed inference
    if (nullptr != backend)
        return ggml_backend_graph_compute(backend, graph) == GGML_STATUS_SUCCESS;
    else
        return ggml_graph_compute(graph, &plan);
}


#define QK8_0 32
typedef struct {
    uint16_t d;       // delta
    int8_t qs[QK8_0]; // quants
} block_q8_0;


static inline float ggml_compute_fp16_to_fp32(uint16_t h) {
#if defined(__ARM_NEON)
    __fp16 tmp;
#else
    #ifdef _MSC_VER
    uint16_t tmp;
    #endif
#endif
    memcpy(&tmp, &h, sizeof(uint16_t));
    return (float) tmp;
}

#define GGML_FP16_TO_FP32(x) ggml_compute_fp16_to_fp32(x)


static void tensor_dump(const ggml_tensor *tensor, const char *name) {
    QNN_LOG_DEBUG(
            "dump ggml tensor %s(%s): type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n",
            name, tensor->name,
            tensor->type, ggml_type_name(tensor->type),
            tensor->ne[0], tensor->ne[1], tensor->ne[2],
            tensor->nb[0], tensor->nb[1], tensor->nb[2]);
    float value = 0;
    std::ostringstream tmposs;
    if (nullptr == tensor) {
        QNN_LOG_WARN("tensor is null");
        return;
    }
    int sizex = tensor->ne[0];
    int sizey = tensor->ne[1];

    if (tensor->type == GGML_TYPE_I8) {
        for (int h = 0; h < tensor->ne[3]; h++) {
            for (int i = 0; i < tensor->ne[2]; i++) {
                for (int j = 0; j < tensor->ne[1]; j++) {
                    for (int k = 0; k < tensor->ne[0]; k++) {
                        value = ((int8_t *) tensor->data)[h * tensor->ne[2] + i * tensor->ne[1] +
                                                          j * tensor->ne[0] + k];
                        tmposs << std::setw(8) << std::fixed << std::setprecision(2) << value
                               << " ";
                    }
                    tmposs << "\n";
                }
            }
        }
        if (strlen(tmposs.str().c_str()) <= (GGML_QNN_LOGBUF_LEN - 96)) {
            QNN_LOG_DEBUG("\n%s\n", tmposs.str().c_str());
            tmposs.clear();
            tmposs.str("");
        }
    }

    if (tensor->type == GGML_TYPE_F32) {
        if (get_tensor_data_size(tensor) < (8 * 8)) {
            for (int h = 0; h < tensor->ne[3]; h++) {
                for (int i = 0; i < tensor->ne[2]; i++) {
                    for (int j = 0; j < tensor->ne[1]; j++) {
                        for (int k = 0; k < tensor->ne[0]; k++) {
                            value = ((float *) tensor->data)[h * tensor->ne[2] + i * tensor->ne[1] +
                                                             j * tensor->ne[0] + k];
                            tmposs << std::setw(8) << std::fixed << std::setprecision(2) << value
                                   << " ";
                        }
                        tmposs << "\n";
                    }
                }
            }
            if (strlen(tmposs.str().c_str()) <= (GGML_QNN_LOGBUF_LEN - 96)) {
                QNN_LOG_DEBUG("\n%s\n", tmposs.str().c_str());
                tmposs.clear();
                tmposs.str("");
            }
        } else {
            int rows = ((8 >= tensor->ne[1]) ? tensor->ne[1] : 8);
            int cols = ((8 >= tensor->ne[0]) ? tensor->ne[0] : 8);
            for (int j = 0; j < rows; j++) {
                for (int k = 0; k < cols; k++) {
                    value = ((float *) tensor->data)[j * tensor->ne[0] + k];
                    tmposs << std::setw(8) << std::fixed << std::setprecision(2) << value << " ";
                }
                tmposs << "\n";
            }
            if (strlen(tmposs.str().c_str()) <= (GGML_QNN_LOGBUF_LEN - 96)) {
                QNN_LOG_DEBUG("(%dx%d in %dx%d)\n%s\n", cols, rows, tensor->ne[0], tensor->ne[1], tmposs.str().c_str());
                tmposs.clear();
                tmposs.str("");
            }
        }
    }

    if (tensor->type == GGML_TYPE_F16) {
        for (int h = 0; h < tensor->ne[3]; h++) {
            for (int i = 0; i < tensor->ne[2]; i++) {
                for (int j = 0; j < tensor->ne[1]; j++) {
                    for (int k = 0; k < tensor->ne[0]; k++) {
                        unsigned short tmpvalue = ((unsigned short *) tensor->data)[
                                h * tensor->ne[2] + i * tensor->ne[1] +
                                j * tensor->ne[0] + k];
                        value = GGML_FP16_TO_FP32(tmpvalue);
                        tmposs << std::setw(8) << std::fixed << std::setprecision(2) << value
                               << " ";
                    }
                    tmposs << "\n";
                }
            }
        }
        if (strlen(tmposs.str().c_str()) <= (GGML_QNN_LOGBUF_LEN - 96)) {
            QNN_LOG_DEBUG("\n%s\n", tmposs.str().c_str());
            tmposs.clear();
            tmposs.str("");
        }
    }

    if (tensor->type == GGML_TYPE_Q8_0) {
        if (get_tensor_data_size(tensor) < (8 * 8)) {
            block_q8_0 *tmp = ((block_q8_0 *) tensor->data);
            for (int j = 0; j < tensor->ne[1]; j++) {
                int n = tensor->ne[0] / QK8_0; //blocks per row
                for (int z = 0; z < n; z++) {
                    const float d = GGML_FP16_TO_FP32(tmp[j * n + z].d);
                    for (int k = 0; k < QK8_0; k++) {
                        value = tmp[j * n + z].qs[k] * d;
                        tmposs << std::setw(8) << std::fixed << std::setprecision(2) << value
                               << " ";
                    }
                }
                tmposs << "\n";
            }
            if (strlen(tmposs.str().c_str()) <= (GGML_QNN_LOGBUF_LEN - 96)) {
                QNN_LOG_DEBUG("\n%s\n", tmposs.str().c_str());
                tmposs.clear();
                tmposs.str("");
            }
        } else {
            int rows = ((8 >= tensor->ne[1]) ? tensor->ne[1] : 8);
            int cols = ((8 >= tensor->ne[0]) ? tensor->ne[0] : 8);
            block_q8_0 *tmp = ((block_q8_0 *) tensor->data);
            for (int j = 0; j < rows; j++) {
                int n = tensor->ne[0] / QK8_0; //blocks per row
                for (int z = 0; z < n; z++) {
                    int block_index = j * n + z;
                    const float d = GGML_FP16_TO_FP32(tmp[block_index].d);
                    if (0 == z) {
                        for (int k = 0; k < 8; k++) {
                            value = tmp[block_index].qs[k] * d;
                            tmposs << std::setw(8) << std::fixed << std::setprecision(2) << value
                               << " ";
                        }
                        tmposs << "\n";
                    }
                }
            }
            if (strlen(tmposs.str().c_str()) <= (GGML_QNN_LOGBUF_LEN - 96)) {
                QNN_LOG_DEBUG("(%dx%d in %dx%d)\n%s\n",  cols, rows, tensor->ne[0], tensor->ne[1], tmposs.str().c_str());
                tmposs.clear();
            }
        }
    }
    //QNN_LOG_DEBUG("\n");
}


//ref: https://github.com/ggerganov/llama.cpp/blob/master/tests/test-backend-ops.cpp#L20
static void init_tensor_uniform(ggml_tensor *tensor, float min = -1.0f, float max = 1.0f) {
    // static RNG initialization (revisit if n_threads stops being constant)
    static const size_t n_threads = std::thread::hardware_concurrency();
    static std::vector<std::default_random_engine> generators = []() {
        std::random_device rd;
        std::vector<std::default_random_engine> vec;
        vec.reserve(n_threads);
        //for (size_t i = 0; i < n_threads; i++) { vec.emplace_back(1234 + i); } // fixed seed
        for (size_t i = 0; i < n_threads; i++) { vec.emplace_back(rd()); }
        return vec;
    }();

    size_t size = ggml_nelements(tensor);
    std::vector<float> data(size);

    auto init_thread = [&](size_t ith, size_t start, size_t end) {
        std::uniform_real_distribution<float> distribution(min, max);
        for (size_t i = start; i < end; i++) {
            data[i] = distribution(generators[ith]);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(n_threads);
    for (size_t i = 0; i < n_threads; i++) {
        size_t start = i * size / n_threads;
        size_t end = (i + 1) * size / n_threads;
        threads.emplace_back(init_thread, i, start, end);
    }
    for (auto &t: threads) {
        t.join();
    }
    if (tensor->type == GGML_TYPE_F32 || tensor->type == GGML_TYPE_I32) {
        //ggml_backend_tensor_set(tensor, data.data(), 0, size * sizeof(float));
        memcpy((char *) tensor->data, data.data(), size * sizeof(float));
    } else if (ggml_is_quantized(tensor->type) || tensor->type == GGML_TYPE_F16 ||
               tensor->type == GGML_TYPE_BF16) {
        GGML_ASSERT(size % ggml_blck_size(tensor->type) == 0);
        std::vector<uint8_t> dataq(ggml_row_size(tensor->type, size));
        std::vector<float> imatrix(tensor->ne[0], 1.0f); // dummy importance matrix
        const float *im = imatrix.data();
        if (!ggml_quantize_requires_imatrix(tensor->type)) {
            // when the imatrix is optional, we want to test both quantization with and without imatrix
            // use one of the random numbers to decide
            if (data[0] > 0.5f * (min + max)) {
                im = nullptr;
            }
        }
        ggml_quantize_chunk(tensor->type, data.data(), dataq.data(), 0, size / tensor->ne[0],
                            tensor->ne[0], im);
        GGML_ASSERT(ggml_validate_row_data(tensor->type, dataq.data(), dataq.size()));
        //ggml_backend_tensor_set(tensor, dataq.data(), 0, dataq.size());
        memcpy((char *) tensor->data, dataq.data(), dataq.size());
    } else if (tensor->type == GGML_TYPE_I8 || tensor->type == GGML_TYPE_I16 ||
               tensor->type == GGML_TYPE_I32) {
        // This is going to create some weird integers though.
        //ggml_backend_tensor_set(tensor, data.data(), 0, ggml_nbytes(tensor));
        memcpy((char *) tensor->data, data.data(), ggml_nbytes(tensor));
    } else {
        GGML_ASSERT(false);
    }
}


//ref: https://github.com/ggerganov/llama.cpp/blob/master/tests/test-backend-ops.cpp#L310
static void initialize_tensors(ggml_context *ctx) {
    for (ggml_tensor *t = ggml_get_first_tensor(ctx);
         t != nullptr; t = ggml_get_next_tensor(ctx, t)) {
        init_tensor_uniform(t);
    }
}


const char *get_ggmltype_str(enum ggml_type wtype) {
    switch (wtype) {
        case GGML_TYPE_Q4_0:
            return "GGML_TYPE_Q4_0";
        case GGML_TYPE_Q4_1:
            return "GGML_TYPE_Q4_1";
        case GGML_TYPE_Q5_0:
            return "GGML_TYPE_Q5_0";
        case GGML_TYPE_Q5_1:
            return "GGML_TYPE_Q5_1";
        case GGML_TYPE_Q8_0:
            return "GGML_TYPE_Q8_0";
        case GGML_TYPE_F16:
            return "GGML_TYPE_F16";
        case GGML_TYPE_F32:
            return "GGML_TYPE_F32";
        default:
            return "unknown";
    }
}


static int qnn_op_ut_automation(int num_threads, int n_backend_type, int n_ggml_op_type) {
    int result = 0;
    int64_t n_begin_time = 0LL;
    int64_t n_end_time = 0LL;
    int64_t n_durtion = 0LL;


    QNN_LOG_DEBUG("enter qnn_ggml_op_automation_ut\n");
    QNN_LOG_DEBUG("num_threads:%d", num_threads);
    QNN_LOG_DEBUG("backend_type:%d(%s)", n_backend_type, get_qnn_backend_name(n_backend_type));
    QNN_LOG_DEBUG("ggml op:%d(%s)", n_ggml_op_type, ggml_op_name((enum ggml_op) n_ggml_op_type));

    n_begin_time = ggml_time_us();

    srand(time(NULL));

    bool support_ops = (n_ggml_op_type == GGML_OP_MUL_MAT || n_ggml_op_type == GGML_OP_MUL ||
                        n_ggml_op_type == GGML_OP_ADD);
    if (!support_ops) {
        QNN_LOG_DEBUG("ggml op %d(%s) not supported  with backend %d(%s)", n_ggml_op_type,
                      ggml_op_name((enum ggml_op) n_ggml_op_type), n_backend_type,
                      get_qnn_backend_name(n_backend_type));
        QNN_LOG_DEBUG("leave qnn_ggml_op UT(unit test)\n");

        return 1;
    }


    char strbuf[256];
    std::string tipString = "";
    std::string s = "";

    const int n_max = 128;

    const std::vector<size_t> sizes = {
            64, 128, 256, 512, 1024, 2048, 4096,
    };

    const size_t N_max = sizes.back();

    // a: N*N*sizeof(float)
    // b: N*N*sizeof(float)
    // c: N*N*sizeof(float)
    // when F16 is used, there is an extra work buffer of size N*N*sizeof(float)
    std::vector<uint8_t> buf(
            3llu * N_max * N_max * sizeof(float) + 3 * ggml_tensor_overhead() +
            ggml_graph_overhead());
    std::vector<uint8_t> work;


    tipString += "\nprepare matrix";

    for (size_t i = 0; i < buf.size(); i++) buf[i] = i;

    for (int j = 0; j < (int) sizes.size(); j++) {
        int n_q4_0 = 0;
        int n_q4_1 = 0;
        int n_q5_0 = 0;
        int n_q5_1 = 0;
        int n_q8_0 = 0;
        int n_fp16 = 0;
        int n_fp32 = 0;

        // GFLOPS/s
        double s_q4_0 = 0.0;
        double s_q4_1 = 0.0;
        double s_q5_0 = 0.0;
        double s_q5_1 = 0.0;
        double s_q8_0 = 0.0;
        double s_fp16 = 0.0;
        double s_fp32 = 0.0;

        const size_t N = sizes[j];

#if 0
        for (int k = 0; k < 7; ++k) {
            const ggml_type wtype =
                    k == 0 ? GGML_TYPE_Q4_0 :
                    k == 1 ? GGML_TYPE_Q4_1 :
                    k == 2 ? GGML_TYPE_Q5_0 :
                    k == 3 ? GGML_TYPE_Q5_1 :
                    k == 4 ? GGML_TYPE_Q8_0 :
                    k == 5 ? GGML_TYPE_F16  : GGML_TYPE_F32;
#else
        for (int k = 4; k < 7; ++k) {
            const ggml_type wtype =
                    k == 4 ? GGML_TYPE_Q8_0 :
                    k == 5 ? GGML_TYPE_F16 : GGML_TYPE_F32;
#endif


            double &s =
                    k == 0 ? s_q4_0 : k == 1 ? s_q4_1 : k == 2 ? s_q5_0 : k == 3 ? s_q5_1 :
                                                                          k == 4 ? s_q8_0 :
                                                                          k == 5 ? s_fp16
                                                                                 : /*k == 6*/ s_fp32;
            int &n = k == 0 ? n_q4_0 : k == 1 ? n_q4_1 : k == 2 ? n_q5_0 : k == 3 ? n_q5_1 :
                                                                           k == 4 ? n_q8_0 :
                                                                           k == 5 ? n_fp16
                                                                                  : /*k == 6*/ n_fp32;

            ggml_backend_t backend = nullptr;
            ggml_backend_buffer_t buffer = nullptr;
#ifdef GGML_USE_QNN
            if (n_backend_type !=
                3) {//3 is fake QNN backend "ggml", just used to compare performance between QNN backend and original GGML
                backend = ggml_backend_qnn_init(n_backend_type,
                                                "/data/local/tmp/"); // the second param can be got by JNI from Java layer
                if (nullptr == backend) {
                    QNN_LOG_DEBUG("create qnn backend %d(%s) failed\n", n_backend_type,
                          get_qnn_backend_name(n_backend_type));
                    return 1;
                }
            }
            num_threads = 1;
#endif
            struct ggml_init_params gparams = {
                    /*.mem_size   =*/ buf.size(),
                    /*.mem_buffer =*/ buf.data(),
                    /*.no_alloc   =*/ false,
            };
#ifdef GGML_USE_QNN
            if (n_backend_type != QNN_BACKEND_GGML) {//QNN_BACKEND_GGML is fake QNN backend "ggml", just used to compare performance between QNN backend and original GGML
                gparams.use_hwaccel = true;
                gparams.no_alloc = true;
            }
#endif
            struct ggml_context *ctx0 = ggml_init(gparams);

            struct ggml_tensor *b = ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, N,
                                                       N); //avoid assert failure in ggml.c
            struct ggml_tensor *a = nullptr;
            if (n_ggml_op_type != GGML_OP_MUL) {
                a = ggml_new_tensor_2d(ctx0, wtype, N, N);
            } else {
                a = ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, N, N); //avoid assert failure in ggml.c
            }
            ggml_set_input(a);
            ggml_set_input(b);

            struct ggml_tensor *c = nullptr;

            switch (n_ggml_op_type) {
                case GGML_OP_ADD:
                    c = ggml_add(ctx0, a, b);
                    break;
                case GGML_OP_MUL:
                    c = ggml_mul(ctx0, a, b);
                    break;
                case GGML_OP_MUL_MAT:
                    c = ggml_mul_mat(ctx0, a, b);
                    break;
            }
            ggml_set_output(c);
#ifdef GGML_USE_QNN
            if (n_backend_type !=
                QNN_BACKEND_GGML) {//QNN_BACKEND_GGML is fake QNN backend "ggml", just used to compare performance between QNN backend and original GGML
                QNN_LOG_DEBUG("creating backend buffer\n");
                buffer = ggml_backend_alloc_ctx_tensors(ctx0, backend);
                if (!buffer) {
                    QNN_LOG_DEBUG("%s: failed to allocate backend buffer\n", __func__);
                    ggml_backend_free(backend);
                    return false;
                }
            }
#endif

            struct ggml_cgraph *gf = ggml_new_graph(ctx0);

            ggml_build_forward_expand(gf, c);

            double tsum = 0.0;

            // heat-up
            ggml_graph_compute_helper(backend, gf, work, num_threads, nullptr, nullptr);

            for (int i = 0; i < n_max; ++i) {
                const int64_t t0 = ggml_time_us();

                //tipString = "calling ggml_graphic_compute_helper:\n";
                tipString = "";
                tipString += "j= " + std::to_string(j) + "(matrix dimension = " +
                             std::to_string(N) + ",n_max=" + std::to_string(n_max) + ")"
                             + ",k=" + std::to_string(k) + "(ggml quant type=" +
                             std::string(get_ggmltype_str(
                                     static_cast<ggml_type>(wtype))) + ")"
                             + ",i=" + std::to_string(i) + "\n";

                //QNN_LOG_DEBUG("%s\n", tipString.c_str());

                ggml_graph_compute_helper(backend, gf, work, num_threads, nullptr, nullptr);

                const int64_t t1 = ggml_time_us();

                tsum += (t1 - t0) * 1e-6;
                n++;

                if (tsum > 1.0 && n >= 3) {
                    break;
                }
            }

            ggml_free(ctx0);
            ggml_backend_buffer_free(buffer);
            ggml_backend_free(backend);

            s = ((2.0 * N * N * N * n) / tsum) * 1e-9;
        }

#if 0
        // Q4_0 | Q4_1
        snprintf(strbuf, sizeof(strbuf),
                 "%4zu x %4zu: Q4_0 %7.1f GFLOPS (%3d runs) | Q4_1 %7.1f GFLOPS (%3d runs)\n",
                 N, N, s_q4_0, n_q4_0, s_q4_1, n_q4_1);
        s += strbuf;


        // Q5_0 | Q5_1 | Q8_0
        snprintf(strbuf, sizeof(strbuf),
                 "%4zu x %4zu: Q5_0 %7.1f GFLOPS (%3d runs) | Q5_1 %7.1f GFLOPS (%3d runs) | Q8_0 %7.1f GFLOPS (%3d runs)\n",
                 N, N, s_q5_0, n_q5_0, s_q5_1, n_q5_1, s_q8_0, n_q8_0);
        s += strbuf;
#endif
        // Q8_0
        snprintf(strbuf, sizeof(strbuf),
                 "%4zu x %4zu: Q8_0 %7.1f GFLOPS (%3d runs)\n",
                 N, N, s_q8_0, n_q8_0);
        s += strbuf;

        // F16 | F32
        snprintf(strbuf, sizeof(strbuf),
                 "%4zu x %4zu: F16  %10.1f GFLOPS (%3d runs) | F32  %8.1f GFLOPS (%3d runs)\n",
                 N, N, s_fp16, n_fp16, s_fp32, n_fp32);
        s += strbuf;


        QNN_LOG_DEBUG("\n%s\n", s.c_str());
    }

    n_end_time = ggml_time_us();
    n_durtion = (n_end_time - n_begin_time) / 1000;
    QNN_LOG_DEBUG(
            "duration of qnn_ggml_op_automation_ut GGML_OP_%s with backend %d(%s) is: %lld milliseconds\n",
            ggml_op_name((enum ggml_op) n_ggml_op_type), n_backend_type,
            get_qnn_backend_name(n_backend_type), n_durtion);
    QNN_LOG_DEBUG("leave qnn_ggml_op_automation_ut(automation unit test)\n");

    return 0;
}


static int qnn_op_ut(int num_threads, int n_backend_type, int n_ggml_op_type) {
    int64_t n_begin_time = 0LL;
    int64_t n_end_time = 0LL;
    int64_t n_duration = 0LL;
    size_t ctx_size = 0;
    int sizey = 384;
    int sizex = 384;

    struct ggml_context *ctx = nullptr;
    struct ggml_cgraph *gf = nullptr;
    struct ggml_tensor *src0 = nullptr;
    struct ggml_tensor *src1 = nullptr;
    struct ggml_tensor *dst = nullptr;
    ggml_backend_t backend = nullptr;
    ggml_backend_buffer_t buffer = nullptr;

    ggml_type qtype = GGML_TYPE_I8;
    qtype = GGML_TYPE_F16;
    qtype = GGML_TYPE_Q8_0;
    qtype = GGML_TYPE_F32;

    std::vector<uint8_t> work_buffer;
    QNN_LOG_DEBUG("enter qnn_ggml_op\n");
    QNN_LOG_DEBUG("ggml op:%d(%s)\n", n_ggml_op_type, ggml_op_name((enum ggml_op) n_ggml_op_type));


    n_begin_time = ggml_time_us();
    srand(time(NULL));

    ctx_size += 1024 * 1024 * 64;
    QNN_LOG_DEBUG("Allocating Memory of size %zi bytes, %zi MB\n", ctx_size,
                  (ctx_size / 1024 / 1024));

    struct ggml_init_params params = {
            /*.mem_size   =*/ ctx_size,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };

    if (n_backend_type != QNN_BACKEND_GGML) {
        params.no_alloc = true;
        backend = ggml_backend_qnn_init(n_backend_type, "/data/local/tmp/");
        if (nullptr == backend) {
            QNN_LOG_ERROR("create qnn backend %d(%s) failed\n", n_backend_type,
                          get_qnn_backend_name(n_backend_type));
            return 1;
        }
    }

    ctx = ggml_init(params);
    if (!ctx) {
        QNN_LOG_ERROR("%s: ggml_init() failed\n");
        return 2;
    }

    QNN_LOG_DEBUG("creating new tensors\n");
    QNN_LOG_DEBUG("ggml_blck_size(%s) %d\n", ggml_type_name(qtype), ggml_blck_size(qtype));
    QNN_LOG_DEBUG("ggml_type_size(%s) %d\n", ggml_type_name(qtype), ggml_type_size(qtype));
    if (ggml_is_quantized(qtype)) {
        sizex = ggml_blck_size(qtype);

        if (n_ggml_op_type == GGML_OP_MUL_MAT) {
            sizex = ggml_blck_size(qtype) * 2;
        }
    }
    QNN_LOG_DEBUG("sizex %d\n", sizex);

    if (n_ggml_op_type == GGML_OP_MUL) {
        src0 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    } else if (n_ggml_op_type == GGML_OP_ADD) {
        if (ggml_is_quantized(qtype)) {
            assert(sizex % ggml_blck_size(qtype) == 0);
        }
        src0 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    } else {
        //MULMAT
        /*
        sizex = 384;
        sizey = 1500;
        if (ggml_is_quantized(qtype)) {
            assert(sizex % ggml_block_size(qtype) == 0);
        }
        src0 = ggml_new_tensor_2d(ctx, qtype, 384, 384);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 384, 1500);
        */
        src0 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 384, 384);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 384, 384);
    }
    ggml_set_input(src0);
    ggml_set_input(src1);

    switch (n_ggml_op_type) {
        case GGML_OP_ADD:
            dst = ggml_add(ctx, src0, src1);
            break;
        case GGML_OP_MUL:
            dst = ggml_mul(ctx, src0, src1);
            break;
        case GGML_OP_MUL_MAT:
            dst = ggml_mul_mat(ctx, src0, src1);
            break;
        default:
            QNN_LOG_WARN("ggml op %d(%s) not supported", n_ggml_op_type,
                         ggml_op_name((enum ggml_op) n_ggml_op_type));
            ggml_free(ctx);
            ggml_backend_free(backend);
            return 3;
    }

    ggml_set_output(dst);
#ifdef GGML_USE_QNN
    if (n_backend_type != QNN_BACKEND_GGML) {
        buffer = ggml_backend_alloc_ctx_tensors(ctx, backend);
        if (!buffer) {
            QNN_LOG_ERROR("%s: failed to allocate backend buffer\n", __func__);
            ggml_free(ctx);
            ggml_backend_free(backend);
            return 4;
        }
    }
#endif

    QNN_LOG_DEBUG("creating compute graph\n");
    gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, dst);

    if (n_backend_type != QNN_BACKEND_GGML) {
        initialize_tensors(ctx);
    } else {
        if (qtype == GGML_TYPE_F32) {
            ggml_set_f32(src0, (rand() % 100 + 1));
        } else {
            initialize_tensors(ctx);
        }
        ggml_set_f32(src1, (rand() % 100 + 1));
        //ggml_set_f32(dst, 0.0f);
    }

    ggml_graph_compute_helper(backend, gf, work_buffer, num_threads, nullptr, nullptr);

    //if (get_tensor_data_size(dst) < (32 * 32))
    if (1)
    {
        QNN_LOG_DEBUG("dump tensors:\n");
        TENSOR_DUMP(src0);
        TENSOR_DUMP(src1);
        TENSOR_DUMP(dst);
    } else {
        QNN_LOG_DEBUG(
                "%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n",
                src0->name,
                src0->type, ggml_type_name(src0->type), src0->ne[0], src0->ne[1], src0->ne[2],
                src0->nb[0], src0->nb[1], src0->nb[2]);
        QNN_LOG_DEBUG(
                "%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n",
                src1->name,
                src1->type, ggml_type_name(src1->type), src1->ne[0], src1->ne[1], src1->ne[2],
                src1->nb[0], src1->nb[1], src1->nb[2]);
        QNN_LOG_DEBUG(
                "%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n",
                dst->name,
                dst->type, ggml_type_name(dst->type), dst->ne[0], dst->ne[1], dst->ne[2],
                dst->nb[0],
                dst->nb[1], dst->nb[2]);
    }

    ggml_free(ctx);
    ggml_backend_buffer_free(buffer);
    ggml_backend_free(backend);
    n_end_time = ggml_time_us();
    n_duration = (n_end_time - n_begin_time) / 1000;
    QNN_LOG_DEBUG("duration of ut GGML_OP_%s using QNN backend %s: %lld milliseconds\n",
                  ggml_op_name((enum ggml_op) n_ggml_op_type), get_qnn_backend_name(n_backend_type),
                  n_duration);
    return 0;
}

#if ENABLE_TEST_WHISPERCPP
#define MAX_PATH_LEN                    512

#define MAX_SAMPLE_SIZE                 (1024 * 8 * 32)

#define MAX_WHISPER_IN_BUFFER_SIZE      (1024 * 1024 * 5)

class whisper_asr;

typedef struct {
    struct whisper_context *p_context;
    struct whisper_full_params *p_params;

    char sz_model_path[MAX_PATH_LEN];
    size_t n_backend;                                //added on 04-17-2024, for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121
    size_t n_threads;

    //03-20-2024,referenced by:https://github.com/futo-org/whisper-acft
    size_t n_decoding_mode;                          // 0:WHISPER_SAMPLING_GREEDY 1:WHISPER_SAMPLING_BEAM_SEARCH

    size_t n_asr_mode;                               // 0: normal transcription  1: asr pressure test 2:benchmark 3: transcription + audio record
    size_t n_benchmark_type;                         // what to benchmark:
    // 0: asr(transcription) 1: memcpy 2: mulmat  3: full/whisper_encode 4: matrix  5: LLAMA inference

    bool b_use_gpu;
    size_t gpu_device;

    bool b_abort_benchmark;                        // TODO: for abort time-consuming task from UI layer. not works as expected

    fifo_buffer_t *asr_fifo;                      // fifo for ASR data producer-consumer

    size_t n_sample_size;

    struct SwrContext *swr_ctx;

    uint8_t p_sample_buffer[MAX_SAMPLE_SIZE];       // temp buffer for convert audio frame
    uint8_t p_audio_buffer[MAX_SAMPLE_SIZE];        // temp buffer for convert audio frame

    pthread_mutex_t mutex;                          // not used since 03-19-2024

    //only for troubleshooting issue
    bool b_pre_convert;
    bool b_enable_dump_16k_data;
} whisper_asr_context;

static whisper_asr_context *p_asr_ctx = NULL;

static std::string whisper_get_time_string() {
    auto to_string = [](const std::chrono::system_clock::time_point &t) -> std::string {
        auto as_time_t = std::chrono::system_clock::to_time_t(t);
        struct tm tm;

        localtime_r(&as_time_t, &tm);

        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                t.time_since_epoch());
        char buf[128];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d %03lld ",
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                 ms.count() % 1000);
        return buf;
    };

    std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
    return to_string(t);
}


static bool ggml_jni_is_valid_utf8(const char *string) {
    if (!string) {
        return true;
    }

    const unsigned char *bytes = (const unsigned char *) string;
    int num;

    while (*bytes != 0x00) {
        if ((*bytes & 0x80) == 0x00) {
            // U+0000 to U+007F
            num = 1;
        } else if ((*bytes & 0xE0) == 0xC0) {
            // U+0080 to U+07FF
            num = 2;
        } else if ((*bytes & 0xF0) == 0xE0) {
            // U+0800 to U+FFFF
            num = 3;
        } else if ((*bytes & 0xF8) == 0xF0) {
            // U+10000 to U+10FFFF
            num = 4;
        } else {
            return false;
        }

        bytes += 1;
        for (int i = 1; i < num; ++i) {
            if ((*bytes & 0xC0) != 0x80) {
                return false;
            }
            bytes += 1;
        }
    }

    return true;
}


static std::string to_timestamp(int64_t t, bool comma = false) {
    int64_t msec = t * 10;
    int64_t hr = msec / (1000 * 60 * 60);
    msec = msec - hr * (1000 * 60 * 60);
    int64_t min = msec / (1000 * 60);
    msec = msec - min * (1000 * 60);
    int64_t sec = msec / 1000;
    msec = msec - sec * 1000;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d%s%03d", (int) hr, (int) min, (int) sec,
             comma ? "," : ".", (int) msec);

    return std::string(buf);
}


static const char *whisper_asr_audio_to_text(const float *pf32_audio_buffer, int num_samples) {
    int result = 0;
    int index = 0;
    const char *text = NULL;
    int64_t begin_time = 0LL;
    int64_t end_time = 0LL;
    int64_t t0_segment = 0;
    int64_t t1_segment = 0;
    int num_segments = 0;

    static std::string asr_result;


    if (NULL == p_asr_ctx) {
        QNN_LOG_WARN("pls check whether asr_ctx already initialized?\n");
        return NULL;
    }

    if ((NULL == pf32_audio_buffer) || (num_samples < 1)) {
        //QNN_LOG_WARN("pls check params\n");
        return NULL;
    }

    if (1 == p_asr_ctx->n_asr_mode) { //ASR pressure test, already handled in whisper_asr_callback
        //QNN_LOG_WARN("are you in ASR pressure test mode?\n");
        return NULL;
    }

    if (2 == p_asr_ctx->n_asr_mode) { //ASR benchmark
        //2024-03-15,16:24
        //there is a special case:launch asr performance test in ASRResearchFragment.java
        //and calling whisper_asr_audio_to_text() directly in whisper_transcribe_from_file
    }

    asr_result = whisper_get_time_string() + " AI subtitle powered by whisper.cpp\n";

    begin_time = ggml_time_ms();
    whisper_reset_timings(p_asr_ctx->p_context);

    //03-20-2024, ref:https://github.com/futo-org/whisper-acft
    p_asr_ctx->p_params->max_tokens = 256;
    p_asr_ctx->p_params->temperature_inc = 0.0f;
    //03-22-2024, don't use this new fine-tune method because it will brings side-effect:app crash randomly
    //p_asr_ctx->p_params->audio_ctx         = std::min(1500, (int)ceil((double)num_samples / (double)(320.0)) + 16);


    //replaced with default value, ref: https://github.com/ggerganov/whisper.cpp/blob/master/whisper.h#L499
    p_asr_ctx->p_params->audio_ctx = 0;

    //03-24-2024, works ok/stable/good performance/... on Xiaomi 14
    p_asr_ctx->p_params->audio_ctx = std::min(1500,
                                              (int) ceil((double) num_samples / (double) (32.0)) +
                                              16);

    //p_asr_ctx->p_params->initial_prompt    = "\" English online TV \"";
    /*
    p_asr_ctx->p_params->abort_callback_user_data = p_asr_ctx;
    p_asr_ctx->p_params->abort_callback = [](void * user_data) -> bool {
        auto *asr_ctx = reinterpret_cast<whisper_asr_context*>(user_data);
        return true;
    };
    */

    p_asr_ctx->n_decoding_mode = WHISPER_SAMPLING_GREEDY;
    if (WHISPER_SAMPLING_GREEDY == p_asr_ctx->n_decoding_mode) {
        p_asr_ctx->p_params->strategy = WHISPER_SAMPLING_GREEDY;
        p_asr_ctx->p_params->greedy.best_of = 1;    //ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L264
    } else {
        p_asr_ctx->p_params->strategy = WHISPER_SAMPLING_BEAM_SEARCH;
        p_asr_ctx->p_params->beam_search.beam_size = 5;    //ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L265
        p_asr_ctx->p_params->greedy.best_of = 5;
    }
    QNN_LOG_DEBUG("decoding_mode=%d, audio_ctx=%d\n", p_asr_ctx->n_decoding_mode,
                  p_asr_ctx->p_params->audio_ctx);

    result = whisper_full(p_asr_ctx->p_context, *p_asr_ctx->p_params, pf32_audio_buffer,
                          num_samples);
    if (0 != result) {
        LOGW("whisper inference failure, pls check why?\n");
        result = -1;
        goto failure;
    }
    end_time = ggml_time_ms();

    QNN_LOG_WARN("whisper inference cost %d ms\n", end_time - begin_time);
    //whisper_print_timings(p_asr_ctx->p_context); // DO NOT uncomment this line

    num_segments = whisper_full_n_segments(p_asr_ctx->p_context);
    for (index = 0; index < num_segments; index++) {
        text = whisper_full_get_segment_text(p_asr_ctx->p_context, index);
        if (NULL == text) {
            LOGW("whisper_full_get_segment_text failure, pls check why\n");
            result = -2;
            goto failure;
        }
        t0_segment = whisper_full_get_segment_t0(p_asr_ctx->p_context, index);
        t1_segment = whisper_full_get_segment_t1(p_asr_ctx->p_context, index);
        /*
        asr_result +=
                "[ " + to_timestamp(t0_segment) + " ---> " + to_timestamp(t1_segment) + "  ]" +
                std::string(text) + "\n";
                */
        asr_result += std::string(text);
        result = 0;

        QNN_LOG_DEBUG("asr result:\n%s\n", asr_result.c_str());
    }

    text = asr_result.c_str();

    failure:
    if (0 != result) {
        asr_result =
                whisper_get_time_string() + "\n" + "whisper inference failure, pls check why?\n";
        text = asr_result.c_str();
    }

    return text;
}


static float *
convert_to_float(uint8_t *in_audio_data, size_t in_audio_size, size_t *out_sample_counts) {
    int result = 0;
    int in_audio_channels = 0;
    int in_sample_counts = 0;
    int in_sample_rates = 0;
    int in_sample_bits = 0;
    int out_audio_channels = 0;
    int out_sample_rates = 0;
    int dst_sample_counts = 0;
    struct SwrContext *swr_ctx = NULL;
    uint8_t *in = NULL;
    uint8_t *payload = NULL;
    size_t payload_len = 0;
    size_t skip_len = 0;
    int i = 0;
    float *out_audio_data = NULL;

    if ((NULL == in_audio_data) || (NULL == out_sample_counts)) {
        QNN_LOG_WARN("pls check params");
        return NULL;
    }

    in = in_audio_data;
    in_audio_channels = (in[23] << 8) | in[22];
    in_sample_rates = (in[27] << 24) | (in[26] << 16) | (in[25] << 8) | in[24];
    in_sample_bits = (in[35] << 8) | in[34];
    skip_len = (in[43] << 24) | (in[42] << 16) | (in[41] << 8) | in[40];
    payload_len = (in[51 + skip_len] << 24) | (in[50 + skip_len] << 16) | (in[49 + skip_len] << 8) |
                  in[48 + skip_len];
    QNN_LOG_DEBUG("%c %c %c %c\n", in[44 + skip_len], in[45 + skip_len], in[46 + skip_len],
                  in[47 + skip_len]);
    QNN_LOG_DEBUG("in_sample_rates:    %d\n", in_sample_rates);
    QNN_LOG_DEBUG("in_audio_channels:  %d\n", in_audio_channels);
    QNN_LOG_DEBUG("in_sample_bits:     %d\n", in_sample_bits);
    QNN_LOG_DEBUG("in_audio_size:      %d\n", in_audio_size);
    QNN_LOG_DEBUG("skip_len:           %d\n", skip_len);
    QNN_LOG_DEBUG("payload_len:        %d\n", payload_len);
    payload = in_audio_data + 44 + skip_len + 8;
    payload_len = in_audio_size - 44 - skip_len - 8;
    QNN_LOG_DEBUG("payload_len:        %d\n", payload_len);
    in_sample_counts = payload_len / 2;
    *out_sample_counts = payload_len / 2;
    out_audio_channels = in_audio_channels;
    out_sample_rates = in_sample_rates;
    QNN_LOG_DEBUG("in_sample_counts:   %d\n", in_sample_counts);
    HEXDUMP(in_audio_data, in_audio_size);
    HEXDUMP(payload, payload_len);

    swr_ctx = swr_alloc();
    if (NULL == swr_ctx) {
        QNN_LOG_WARN("could not allocate FFmpeg resample context\n");
        return NULL;
    }

    av_opt_set_int(swr_ctx, "in_channel_count", in_audio_channels, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", in_sample_rates, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(swr_ctx, "out_channel_count", out_audio_channels, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", out_sample_rates, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
    if ((result = swr_init(swr_ctx)) < 0) {
        QNN_LOG_INFO("Failed to initialize the resampling context\n");
        goto failure;
    }

    out_audio_data = (float *) malloc((in_sample_counts) * sizeof(float));
    if (NULL == out_audio_data) {
        QNN_LOG_WARN("malloc failed\n");
        goto failure;
    }

    dst_sample_counts = av_rescale_rnd(swr_get_delay(swr_ctx, in_sample_rates) + in_sample_counts,
                                       out_sample_rates, in_sample_rates, AV_ROUND_UP);
    QNN_LOG_INFO("dst_sample_counts %d\n", dst_sample_counts);
    if (dst_sample_counts != *out_sample_counts) {
        QNN_LOG_WARN("it shouldn't happen, pls check");
    }

    result = swr_convert(swr_ctx,
                         (uint8_t **) (&out_audio_data),
                         *out_sample_counts,
                         (const uint8_t **) (&payload),
                         in_sample_counts);

    if (result < 0) {
        QNN_LOG_WARN("audio sample convert failure");
        free(out_audio_data);
        out_audio_data = NULL;
    }
    QNN_LOG_DEBUG("audio sample convert's result: %d\n", result);

    failure:
    if (NULL != swr_ctx) {
        swr_free(&swr_ctx);
        swr_ctx = NULL;
    }

    return out_audio_data;
}


static int read_data_from_file(const char *sz_file_name, uint8_t **pp_data, size_t *p_datalen) {
    int result = 0;
    FILE *fp = NULL;
    uint8_t *p_data = NULL;
    uint32_t datalen = 0;

    if ((NULL == sz_file_name) || (NULL == pp_data) || (NULL == p_datalen)) {
        result = -1;
    }

    if (0 == result) {
        QNN_LOG_DEBUG("open file: %s", sz_file_name);
        fp = fopen(sz_file_name, "rb");
        if (NULL == fp) {
            result = -errno;
            LOGD("open file %s failed(reason:%s)", sz_file_name, strerror(errno));
        }
    }

    if (0 == result) {
        fseek(fp, 0, SEEK_END);
        datalen = (uint32_t) ftell(fp);
        QNN_LOG_DEBUG("file size %d\n", datalen);
        if (0 == datalen) {
            QNN_LOG_WARN("pls check why size is zero of file %s\n", sz_file_name);
            result = -2;
        }
    }

    if (0 == result) {
        p_data = (uint8_t *) malloc(datalen);
        if (NULL == p_data) {
            QNN_LOG_WARN("malloc memory failure, pls check why?");
            result = -3;
        }
    }

    if (0 == result) {
        fseek(fp, 0, SEEK_SET);
        datalen = (uint32_t) fread(p_data, 1, datalen, fp);
    }

    if (0 == result) {
        *pp_data = p_data;
        *p_datalen = datalen;
        p_data = NULL;
    }

    if (NULL != fp) {
        fclose(fp);
        fp = NULL;
    }

    //attention here, just for avoid memory leak
    if (NULL != p_data) {
        free(p_data);
        p_data = NULL;
    }

    return result;
}


static int
whisper_asr_init(const char *sz_model_path, int n_threads, int n_asrmode, int n_backend) {
    QNN_LOG_INFO("enter whisper_asr_init\n");
    int result = 0;

    struct whisper_full_params params;
    struct whisper_context_params c_params = whisper_context_default_params();

    if ((NULL == sz_model_path) || (n_asrmode > 3)) {
        QNN_LOG_WARN("invalid param\n");
        return 1;
    }


        //dynamic ISA dectect by RUAPU, prepare for SIMD optimization on Android device. but not used currently
        //not used since v1.3.8 because Tencent's NCNN inference framework was used in this project and ruapu.h already exist in libncnn.a
#if 0
        ruapu_init();
        const char* const* supported = ruapu_rua();
        while (*supported) {
            QNN_LOG_DEBUG("%s\n", *supported);
            supported++;
        }
#endif

    QNN_LOG_INFO("model path:%s\n", sz_model_path);
    QNN_LOG_INFO("thread counts:%d\n", n_threads);
    QNN_LOG_INFO("asr mode:%d\n", n_asrmode);
    QNN_LOG_INFO("backend type:%d\n", n_backend);

    //kantv_asr_callback pfn_asr_callback = whisper_asr_callback;
    //kantv_asr_set_callback(pfn_asr_callback);

    //kantv_inference_callback  pfn_inference_callback = whisper_asr_audio_to_text;
    //kantv_inference_set_callback(pfn_inference_callback);

    if (NULL != p_asr_ctx) {
        QNN_LOG_WARN("asr instance already initialized\n");
        return 2;
    }

    if (NULL == p_asr_ctx) {
        p_asr_ctx = (whisper_asr_context *) malloc(sizeof(whisper_asr_context));
        if (NULL == p_asr_ctx) {
            QNN_LOG_WARN("initialize failure\n");
            return 3;
        }
    }
    memset(p_asr_ctx->sz_model_path, 0, MAX_PATH_LEN);
    memcpy(p_asr_ctx->sz_model_path, sz_model_path, strlen(sz_model_path));
    p_asr_ctx->n_backend = n_backend;//added on 04-17-2024, for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121

    result = pthread_mutex_init(&p_asr_ctx->mutex, NULL);
    if (result != 0) {
        result = 4;
        QNN_LOG_WARN("initialize failure\n");
        goto failure;
    }

    p_asr_ctx->asr_fifo = fifo_new("asr_fifo", 1200, 1024 * 8 * 20);
    if (NULL == p_asr_ctx->asr_fifo) {
        result = 5;
        QNN_LOG_WARN("initialize failure\n");
        goto failure;
    }

    p_asr_ctx->swr_ctx = swr_alloc();      // attention memory leak
    if (NULL == p_asr_ctx->swr_ctx) {
        result = 7;
        QNN_LOG_WARN("initialize failure\n");
        goto failure;
    }

    p_asr_ctx->n_asr_mode = n_asrmode;
    p_asr_ctx->n_threads = n_threads;

    QNN_LOG_DEBUG("calling whisper_init_from_file");
#if 0
    p_asr_ctx->p_context = whisper_init_from_file(sz_model_path);
#else  //04-11-2024, for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121
    // ref:https://github.com/ggerganov/llama.cpp/pull/6022
    // the user could specify the devices that they want to use by name. For example,
    // the user could specify to use devices cpu, sycl_igpu0 and sycl_dgpu0 to select CPU, iGPU and dGPU
    //c_params.gpu_device    = QNN_HTP;//TODO:Failed in loading stub: dlopen failed: library "libQnnHtpV66Stub.so" not found
    c_params.gpu_device = n_backend;
    if (n_backend !=
        QNN_BACKEND_GGML) { // QNN_BACKEND_GGML is a fake QNN backend, just used to compare performance between QNN backend and original GGML
        c_params.use_gpu = true;
    } else {
        c_params.use_gpu = false;
    }
    p_asr_ctx->p_context = whisper_init_from_file_with_params(sz_model_path, c_params);
#endif
    if (nullptr == p_asr_ctx->p_context) {
        result = 8;
        QNN_LOG_WARN("whispercpp initialize failure\n");
        goto failure;
    }
    QNN_LOG_DEBUG("after calling whisper_init_from_file");

    p_asr_ctx->p_params = (struct whisper_full_params *) malloc(sizeof(struct whisper_full_params));
    if (NULL == p_asr_ctx->p_params) {
        result = 9;
        QNN_LOG_WARN("initialize failure\n");
        goto failure;
    }

    params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_realtime = false;
    params.print_progress = false;
    params.print_timestamps = false;
    params.print_special = false;
    params.translate = false; //first step is transcription, the second step is English -> Chinese
    //params.initial_prompt        = "hello,whisper.cpp";
    //params.language                = "en";
    params.n_threads = n_threads;;
    params.offset_ms = 0;
    params.no_context = true;
    params.single_segment = true;
    params.no_timestamps = true;

    params.speed_up = false;
    params.debug_mode = false;
    params.audio_ctx = 0;

    params.suppress_blank = true;
    params.suppress_non_speech_tokens = true;


    //ref: https://github.com/ggerganov/whisper.cpp/issues/1507
    //reduce the maximum context size (--max-context). By default it is 224. Setting it to 64 or 32 can reduce the repetitions significantly. Setting it to 0 will most likely eliminate all repetitions, but the transcription quality can be affected because it will be losing the context from the previous transcript
    params.n_max_text_ctx = 224; //default value
    //params.n_max_text_ctx              = 0;

    //03-20-2024, ref:https://github.com/futo-org/whisper-acft
    p_asr_ctx->n_decoding_mode = WHISPER_SAMPLING_BEAM_SEARCH;


    //params.tdrz_enable                  = false;//whisper complain failed to compute log mel spectrogram when this flag was enabled
    //params.suppress_blank               = true;
    //params.suppress_non_speech_tokens   = true;


    memcpy(p_asr_ctx->p_params, &params, sizeof(struct whisper_full_params));

    p_asr_ctx->b_pre_convert = p_asr_ctx->b_enable_dump_16k_data = false;

    QNN_LOG_INFO("leave whisper_asr_init\n");

    return result;

    failure:

    if (nullptr != p_asr_ctx->p_context) {
        whisper_free(p_asr_ctx->p_context);
    }

    if (NULL != p_asr_ctx->swr_ctx) {
        swr_free(&p_asr_ctx->swr_ctx);
    }

    if (NULL != p_asr_ctx->asr_fifo) {
        p_asr_ctx->asr_fifo->destroy(p_asr_ctx->asr_fifo);
    }

    if (4 != result)
        pthread_mutex_destroy(&p_asr_ctx->mutex);

    if (NULL != p_asr_ctx) {
        free(p_asr_ctx);
    }
    p_asr_ctx = NULL;

    return result;
}


static void whisper_asr_finalize() {
    QNN_LOG_INFO("enter whisper_asr_finalize\n");

    if (NULL == p_asr_ctx) {
        QNN_LOG_WARN("whisper.cpp not initialized or already finalized\n");
        return;
    }
    whisper_free(p_asr_ctx->p_context);

    swr_free(&p_asr_ctx->swr_ctx);

    p_asr_ctx->asr_fifo->destroy(p_asr_ctx->asr_fifo);

    free(p_asr_ctx->p_params);

    pthread_mutex_destroy(&p_asr_ctx->mutex);

    free(p_asr_ctx);
    p_asr_ctx = NULL;

    QNN_LOG_INFO("leave whisper_asr_finalize\n");
}


static int
qnn_test_whispercpp(const char *sz_model_path, const char *sz_audio_path, int num_threads,
                    int n_backend_type) {
    struct whisper_context *context = nullptr;
    struct whisper_full_params whisper_params;
    uint8_t *audio_data = NULL;
    size_t audio_size = 0;
    float *float_audio_data = NULL;
    size_t float_sample_counts = 0;
    int result = 0;
    int num_segments = 0;
    int index = 0;
    int64_t begin_time = 0LL;
    int64_t end_time = 0LL;
    int64_t t0_segment = 0;
    int64_t t1_segment = 0;
    const char *text = NULL;

    //TODO: static variable should not be used in real project
    static std::string asr_result;

    if ((NULL == sz_model_path) || (NULL == sz_audio_path)) {
        QNN_LOG_WARN("pls check params");
        return NULL;
    }

    QNN_LOG_INFO("mode path:   %s\n", sz_model_path);
    QNN_LOG_INFO("audio file:  %s\n", sz_audio_path);
    QNN_LOG_INFO("num threads: %d\n", num_threads);

    if (0 != access(sz_model_path, F_OK)) {
        QNN_LOG_WARN("pls check whether the ggml model file %s is exist", sz_model_path);
        return NULL;
    }

    if (0 != access(sz_audio_path, F_OK)) {
        QNN_LOG_WARN("pls check whether the ggml sample file %s is exist", sz_audio_path);
        return NULL;
    }

    asr_result = "";

    whisper_asr_init(sz_model_path, num_threads, 0, n_backend_type);

    result = read_data_from_file(sz_audio_path, &audio_data, &audio_size);
    if (0 != result) {
        QNN_LOG_WARN("read data from file %s failure,pls check why?\n", sz_audio_path);
        result = -2;
        goto failure;
    }
    QNN_LOG_DEBUG("audio size %d\n", audio_size);
    float_audio_data = convert_to_float(audio_data, audio_size, &float_sample_counts);
    if (NULL == float_audio_data) {
        QNN_LOG_WARN("convert audio sample failure,pls check why?\n");
        result = -3;
        goto failure;
    }
    QNN_LOG_DEBUG("float_sample_counts %d\n", float_sample_counts);

    if (0) {
        text = whisper_asr_audio_to_text(float_audio_data, float_sample_counts);
        if (NULL != text) {
            QNN_LOG_DEBUG("asr reulst:\n%s\n", text);
            asr_result = text;
        } else {
            QNN_LOG_DEBUG("whisper_asr_audio_to_text failed\n");
            result = -1;
            goto failure;
        }
    } else {
        //PoC-S53: fix stability issue during toggle between different backend(QNN CPU/GPU/DSP backend, ggml...) in ggml-qnn.cpp(4th milestone)
        whisper_free(p_asr_ctx->p_context);
        struct whisper_context_params wcp = whisper_context_default_params();
        QNN_LOG_DEBUG("backend %d", n_backend_type);
        wcp.gpu_device = n_backend_type;//added on 04-17-2024, for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121
        if (n_backend_type !=
            QNN_BACKEND_GGML) { // QNN_BACKEND_GGML is a fake QNN backend, just used to compare performance between QNN backend and original GGML
            wcp.use_gpu = true;
        } else {
            wcp.use_gpu = false;
        }
        context = whisper_init_from_file_with_params(sz_model_path, wcp);
        //PoC-S53: fix stability issue during toggle between different backend(QNN CPU/GPU/DSP backend, ggml...) in ggml-qnn.cpp(4th milestone)
        p_asr_ctx->p_context = context;
        if (nullptr == context) {
            QNN_LOG_WARN("whisper_init_from_file_with_params failure, pls check why\n");
            result = -1;
            goto failure;
        }

        whisper_params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        whisper_params.print_realtime = true;
        whisper_params.print_progress = true;
        whisper_params.print_timestamps = true;
        whisper_params.print_special = false;
        whisper_params.translate = false;
        whisper_params.language = "en";
        whisper_params.n_threads = num_threads;
        whisper_params.offset_ms = 0;
        whisper_params.no_context = true;
        whisper_params.single_segment = false;
        whisper_reset_timings(context);

        QNN_LOG_INFO("calling whisper_full\n");
        begin_time = ggml_time_ms();
        result = whisper_full(context, whisper_params, float_audio_data, float_sample_counts);
        if (0 != result) {
            QNN_LOG_WARN("inference failure, pls check why?\n");
            result = -4;
            goto failure;
        }
        QNN_LOG_INFO("whispercpp inference successfully\n");
        num_segments = whisper_full_n_segments(context);
        //QNN_LOG_DEBUG("num_segments:%d\n", num_segments);
        for (index = 0; index < num_segments; index++) {
            text = whisper_full_get_segment_text(context, index);
            if (NULL == text) {
                QNN_LOG_WARN("whisper_full_get_segment_text failure, pls check why\n");
                result = -5;
                goto failure;
            }
            t0_segment = whisper_full_get_segment_t0(context, index);
            t1_segment = whisper_full_get_segment_t1(context, index);

            std::string cur_line = "";
            /*
            QNN_LOG_INFO("text[%d]:[%s --> %s] %s\n",
                  index,
                  to_timestamp(t0_segment).c_str(),
                  to_timestamp(t1_segment).c_str(),
                  text);
            */
            cur_line +=
                    "[ " + to_timestamp(t0_segment) + " ---> " + to_timestamp(t1_segment) + "  ]" +
                    std::string(text);
            asr_result += cur_line + "\n";
            //QNN_LOG_DEBUG("asr result:\n%s\n", cur_line.c_str());
        }
        end_time = ggml_time_ms();
        QNN_LOG_INFO("wshipercpp inference cost %d ms\n", end_time - begin_time);
        //QNN_LOG_INFO("after calling whisper_full\n");
    }

    failure:
    if (NULL != float_audio_data) {
        free(float_audio_data);
        float_audio_data = NULL;
    }

    if (NULL != audio_data) {
        free(audio_data);
        audio_data = NULL;
    }

    if (nullptr != context) {
        whisper_free(context);
        p_asr_ctx->p_context = nullptr;
    }

    whisper_asr_finalize();
    QNN_LOG_DEBUG("whispercpp UT using QNN backend %s: %lld milliseconds\n",
                  get_qnn_backend_name(n_backend_type), (end_time - begin_time));

    if (0 == result) {
        if (num_segments != 0) {
            QNN_LOG_DEBUG("whisper ASR result:\n%s", asr_result.c_str());
            return 0;
        } else {
            asr_result = "pls check why whisper.cpp inference failure\n";
            QNN_LOG_DEBUG("%s", asr_result.c_str());
            return 1;
        }
    } else
        return 2;
}
#endif

// ==================== verify/develop QNN NPU backend(RPC,....)==================================
class qnn_interface_test {

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

    friend class qnn_implementation_test;

public:
    qnn_interface_test() = default;

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

    void set_qnn_interface(const QnnInterface_t *qnn_interface) {
        _qnn_interface = qnn_interface;
    }

    void set_qnn_system_interface(const QnnSystemInterface_t *qnn_sys_interface) {
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

class qnn_io_tensor_test {
public:
    // Setup details for all input and output tensors for graph execution.
    int setupInputAndOutputTensors(
            Qnn_Tensor_t **inputs, Qnn_Tensor_t **outputs, GraphInfo_t graphInfo) {
        int returnStatus = 0;
        if (0 !=
            setupTensors(inputs, graphInfo.numInputTensors, (graphInfo.inputTensors))) {
            LOGGW("failure in setting up input tensors");
            returnStatus = 1;
        } else {
            LOGGI("succeed in setting up input tensors");
        }

        if (0 !=
            setupTensors(outputs, graphInfo.numOutputTensors, (graphInfo.outputTensors))) {
            LOGGW("failure in setting up output tensors");
            returnStatus = 1;
        } else {
            LOGGI("succeed in setting up output tensors");
        }

        if (0 != returnStatus) {
            if (nullptr != *inputs) {
                LOGGD("cleaning up input tensors");
                tearDownTensors(*inputs, graphInfo.numInputTensors);
                *inputs = nullptr;
            }
            if (nullptr != *outputs) {
                LOGGD("cleaning up output tensors");
                tearDownTensors(*outputs, graphInfo.numOutputTensors);
                *outputs = nullptr;
            }
            LOGGW("failure in setupInputAndOutputTensors, done cleaning up resources");
        }

        return returnStatus;
    }



    // write out all output tensors to files. If output_data_type is float,
// then all outputs will be raw floats regardless of what the model outputs.
// If the output_data_type is native, then output is written as produced by the model.
// Also, for native option, a json with quantization parameters is written out.
// If output_data_type is float_and_native, both above are done.
// If the output in the graph is float, then output_data_type has no effect.
    int writeOutputTensors(uint32_t graphIdx,
                                               size_t startIdx,
                                               char *graphName,
                                               Qnn_Tensor_t *outputs,
                                               uint32_t numOutputs,
                                               OutputDataType outputDatatype,
                                               uint32_t graphsCount,
                                               std::string outputPath,
                                               size_t numInputFilesPopulated,
                                               size_t outputBatchSize) {
        if (nullptr == outputs) {
            LOGGW("Received nullptr");
            return 1;
        }
        if (graphsCount > 1) {
            if (nullptr != graphName && strlen(graphName) > 0) {
                outputPath += ("/" + std::string(graphName));
            } else {
                outputPath += ("/" + std::string("Graph_") +
                               std::to_string(graphIdx));
            }
        }
        auto returnStatus = 0;
        std::vector<std::string> outputPaths;
        for (size_t idx = 0; idx < numInputFilesPopulated; idx++) {
            std::string output = outputPath + ("/" + std::string("Result_") +
                                               std::to_string(startIdx + idx));
            outputPaths.push_back(output);
        }
        for (size_t outputIdx = 0; outputIdx < numOutputs; outputIdx++) {
            LOGGD("Writing output for outputIdx: %d", outputIdx);
            LOGGD("Writing output for outputIdx: %d", outputIdx);
            std::string outputFilePrefix;
            if (nullptr != QNN_TENSOR_GET_NAME(outputs[outputIdx]) &&
                strlen(QNN_TENSOR_GET_NAME(outputs[outputIdx])) > 0) {
                outputFilePrefix = std::string(QNN_TENSOR_GET_NAME(outputs[outputIdx]));
            } else {
                outputFilePrefix = std::string("Output_") + std::to_string(outputIdx);
            }
            auto outputFile = outputFilePrefix + std::string(".raw");
            auto outputFileNative = outputFilePrefix + std::string("_native.raw");
            if (QNN_TENSOR_GET_DATA_TYPE(outputs[outputIdx]) == QNN_DATATYPE_FLOAT_32) {
                LOGGD("Writing in output->dataType == QNN_DATATYPE_FLOAT_32");
                returnStatus =
                        writeOutputTensor(&(outputs[outputIdx]), outputPaths, outputFile,
                                          outputBatchSize);
            } else if (outputDatatype == OutputDataType::FLOAT_ONLY) {
                LOGGD("Writing in output->dataType == OutputDataType::FLOAT_ONLY");
                returnStatus = convertAndWriteOutputTensorInFloat(
                        &(outputs[outputIdx]), outputPaths, outputFile, outputBatchSize);
            } else if (outputDatatype == OutputDataType::NATIVE_ONLY) {
                LOGGD("Writing in output->dataType == OutputDataType::NATIVE_ONLY");
                returnStatus =
                        writeOutputTensor(&(outputs[outputIdx]), outputPaths, outputFileNative,
                                          outputBatchSize);
            } else if (outputDatatype == OutputDataType::FLOAT_AND_NATIVE) {
                LOGGD("Writing in output->dataType == OutputDataType::FLOAT_AND_NATIVE");
                returnStatus = convertAndWriteOutputTensorInFloat(
                        &(outputs[outputIdx]), outputPaths, outputFile, outputBatchSize);
                if (0 == returnStatus) {
                    returnStatus = writeOutputTensor(
                            &(outputs[outputIdx]), outputPaths, outputFileNative, outputBatchSize);
                }
            }
        }
        return returnStatus;
    }

    PopulateInputTensorsRetType_t populateInputTensors(
            uint32_t graphIdx,
            const std::vector<std::vector<std::string>> &filePathsVector,
            const size_t filePathsIndexOffset,
            const bool loopBackToStart,
            const std::unordered_map<std::string, uint32_t> &inputNameToIndex,
            Qnn_Tensor_t *inputs,
            GraphInfo_t graphInfo,
            InputDataType inputDataType);



    // Clean up all input and output tensors after execution.
    int tearDownInputAndOutputTensors(Qnn_Tensor_t *inputs,
                                                          Qnn_Tensor_t *outputs,
                                                          size_t numInputTensors,
                                                          size_t numOutputTensors) {
        if (nullptr != inputs) {
            //LOGGD("cleaning up resources for input tensors");
            //LOGGD("cleaning up resources for input tensors");
            tearDownTensors(inputs, numInputTensors);
            inputs = nullptr;
        }
        if (nullptr != outputs) {
            //LOGGD("cleaning up resources for output tensors");
            //LOGGD("cleaning up resources for output tensors");
            tearDownTensors(outputs, numOutputTensors);
            outputs = nullptr;
        }
        return 0;
    }



    // Helper method to read data from files to a buffer.
    int readDataAndAllocateBuffer(
            std::queue<std::string> &filePaths,
            std::vector<size_t> dims,
            Qnn_DataType_t dataType,
            uint8_t **bufferToCopy) {
        int returnStatus = 0;
        *bufferToCopy = nullptr;
        returnStatus = allocateBuffer(bufferToCopy, dims, dataType);
        if (0 == returnStatus) {
            int status = 0;
            std::tie(status, m_numFilesPopulated, m_batchSize) = readBatchDataAndUpdateQueue(
                    filePaths, dims, dataType, reinterpret_cast<uint8_t *>(*bufferToCopy));
            if (0 != status) {
                LOGGW("failed to readBatchDataAndUpdateQueue");
                returnStatus = 1;
            }
        }
        if (0 != returnStatus) {
            if (nullptr != *bufferToCopy) {
                free(*bufferToCopy);
                *bufferToCopy = nullptr;
            }
        }

        return returnStatus;
    }


    // Helper method to populate an input tensor in the graph during execution.
// It relies on reading data from files provided during app creation.
    int populateInputTensor(
            std::queue<std::string> &filePaths,
            Qnn_Tensor_t *input,
            InputDataType inputDataType) {
        if (nullptr == input) {
            LOGGW("input is nullptr");
            return 1;
        }

        int returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(input), QNN_TENSOR_GET_RANK(input));

        if (inputDataType == InputDataType::FLOAT &&
            QNN_TENSOR_GET_DATA_TYPE(input) != QNN_DATATYPE_FLOAT_32) {
            uint8_t *fileToBuffer = nullptr;
            returnStatus = readDataAndAllocateBuffer(filePaths, dims, QNN_DATATYPE_FLOAT_32,
                                                     &fileToBuffer);
            if (0 == returnStatus) {
                LOGGW("readDataFromFileToBuffer successful");
                returnStatus = copyFromFloatToNative(reinterpret_cast<float *>(fileToBuffer),
                                                     input);
            }
            if (nullptr != fileToBuffer) {
                free(fileToBuffer);
                fileToBuffer = nullptr;
            }
        } else {
            int status = 0;
            std::tie(status, m_numFilesPopulated, m_batchSize) = readBatchDataAndUpdateQueue(
                    filePaths,
                    dims,
                    QNN_TENSOR_GET_DATA_TYPE(input),
                    static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(input).data));
            if (0 != status) {
                LOGGW("Failure in datautil::readBatchDataAndUpdateQueue");
                returnStatus = 1;
            }
        }
        return returnStatus;
    }


    // Helper method to populate an input tensor in the graph during execution.
    // It relies on reading data from buffer provided during executeGraph() call.
    int populateInputTensor(
            uint8_t *buffer, Qnn_Tensor_t *input, InputDataType inputDataType) {
        if (nullptr == input) {
            LOGGW("input is nullptr");
            return 1;
        }
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(input), QNN_TENSOR_GET_RANK(input));
        if (inputDataType == InputDataType::FLOAT &&
            QNN_TENSOR_GET_DATA_TYPE(input) != QNN_DATATYPE_FLOAT_32) {
            LOGGW("Received FLOAT input, but model needs non-float input");
            if (0 != copyFromFloatToNative(reinterpret_cast<float *>(buffer), input)) {
                LOGGW("copyFromFloatToNative failure");
                return 1;
            }
        } else {
            size_t length;
            int returnStatus;
            std::tie(returnStatus, length) = calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE(input));
            if (0 != returnStatus) {
                LOGGW("failed to calculate length");
                return 1;
            }
            memscpy(reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(input).data), length,
                    buffer, length);
        }
        return 0;
    }

    // Helper method to populate all input tensors.
    int populateInputTensors(
            uint32_t graphIdx,
            std::vector<uint8_t *> inputBuffers,
            Qnn_Tensor_t *inputs,
            GraphInfo_t graphInfo,
            InputDataType inputDataType) {
        if (nullptr == inputs) {
            LOGGW("inputs is nullptr");
            return 1;
        }
        auto inputCount = graphInfo.numInputTensors;
        if (inputBuffers.size() != inputCount) {
            LOGGW("Incorrect amount of Input Buffers for graphIdx: %d. Expected: %d, received: %d",
                  graphIdx,
                  inputCount,
                  inputBuffers.size());
            return 1;
        }
        for (size_t inputIdx = 0; inputIdx < inputCount; inputIdx++) {
            if (0 !=
                populateInputTensor(inputBuffers[inputIdx], &(inputs[inputIdx]), inputDataType)) {
                LOGGW("populateInputTensor() failure for input: %d", inputIdx);
                return 1;
            }
        }
        return 0;
    }

    // Helper method to allocate a buffer.
    template<typename T>
    int allocateBuffer(T **buffer, size_t &elementCount) {
        LOGGD("elementCount: %d, sizeof(T): %d, total size: %d",
              elementCount,
              sizeof(T),
              elementCount * sizeof(T));
        *buffer = (T *) malloc(elementCount * sizeof(T));
        if (nullptr == *buffer) {
            LOGGW("mem alloc failed for *buffer");
            return 1;
        }

        return 0;
    }

    //Helper method convert output to floatbuffer
    int converQnntensortoFloatBuffer(Qnn_Tensor_t *output, float **floatBuffer) {
        int result = 0;
        if (nullptr == output) {
            return 1;
        }

        result = convertToFloat(floatBuffer, output);
        if (0 != result) {
            return 2;
        }
        return result;
    }

    // Helper method to convert Output tensors to float and write them to files.
    int convertAndWriteOutputTensorInFloat(
            Qnn_Tensor_t *output, std::vector<std::string> outputPaths, std::string fileName) {
        if (nullptr == output) {
            return 1;
        }

        auto returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
        float *floatBuffer = nullptr;
        returnStatus = convertToFloat(&floatBuffer, output);
        if (0 != returnStatus) {
            LOGGD("failure in convertToFloat");
            return 1;
        }
        uint8_t *bufferToWrite = reinterpret_cast<uint8_t *>(floatBuffer);

        if (nullptr != floatBuffer) {
            LOGGD("freeing floatBuffer");
            free(floatBuffer);
            floatBuffer = nullptr;
        }
        return returnStatus;
    }


    // Helper method to write out output. There is no de-quantization here.
    // Just write output as is to files
    int writeOutputTensor(Qnn_Tensor_t *output,
                          std::vector<std::string> outputPaths,
                          std::string &fileName) {
        if (nullptr == output) {
            LOGGW("output is nullptr");
            return 1;
        }
        int returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
        uint8_t *bufferToWrite = reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(
                output).data);

        return returnStatus;
    }


private:
    size_t m_batchSize = 0;
    size_t m_numFilesPopulated = 0;

// helper method to copy a float buffer, quantize it, and copy it to a tensor (Qnn_Tensor_t) buffer.
    int copyFromFloatToNative(float *floatBuffer, Qnn_Tensor_t *tensor) {
        if (nullptr == floatBuffer || nullptr == tensor) {
            LOGGW("copyFromFloatToNative(): received a nullptr");
            return 1;
        }

        int returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(tensor), QNN_TENSOR_GET_RANK(tensor));

        switch (QNN_TENSOR_GET_DATA_TYPE(tensor)) {
            case QNN_DATATYPE_UFIXED_POINT_8:
                floatToTfN<uint8_t>(
                        static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        floatBuffer,
                        QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                        QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                        calculateElementCount(dims));
                break;

            case QNN_DATATYPE_UFIXED_POINT_16:
                floatToTfN<uint16_t>(
                        static_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        floatBuffer,
                        QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                        QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                        calculateElementCount(dims));
                break;

            case QNN_DATATYPE_UINT_8:
                if (0 !=
                    castFromFloat<uint8_t>(
                            static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            floatBuffer,
                            calculateElementCount(dims))) {
                    LOGGW("failure in castFromFloat<uint8_t>");
                    returnStatus = 1;
                }
                break;

            case QNN_DATATYPE_UINT_16:
                if (0 !=
                    castFromFloat<uint16_t>(
                            static_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            floatBuffer,
                            calculateElementCount(dims))) {
                    LOGGW("failure in castFromFloat<uint16_t>");
                    returnStatus = 1;
                }
                break;

            case QNN_DATATYPE_UINT_32:
                if (0 !=
                    castFromFloat<uint32_t>(
                            static_cast<uint32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            floatBuffer,
                            calculateElementCount(dims))) {
                    LOGGW("failure in castFromFloat<uint32_t>");
                    returnStatus = 1;
                }
                break;

            case QNN_DATATYPE_INT_8:
                if (0 !=
                    castFromFloat<int8_t>(
                            static_cast<int8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            floatBuffer,
                            calculateElementCount(dims))) {
                    LOGGW("failure in castFromFloat<int8_t>");
                    returnStatus = 1;
                }
                break;

            case QNN_DATATYPE_INT_16:
                if (0 !=
                    castFromFloat<int16_t>(
                            static_cast<int16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            floatBuffer,
                            calculateElementCount(dims))) {
                    LOGGW("failure in castFromFloat<int16_t>");
                    returnStatus = 1;
                }
                break;

            case QNN_DATATYPE_INT_32:
                if (0 !=
                    castFromFloat<int32_t>(
                            static_cast<int32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            floatBuffer,
                            calculateElementCount(dims))) {
                    LOGGW("failure in castFromFloat<int32_t>");
                    returnStatus = 1;
                }
                break;

            case QNN_DATATYPE_BOOL_8:
                if (0 !=
                    castFromFloat<uint8_t>(
                            static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            floatBuffer,
                            calculateElementCount(dims))) {
                    LOGGW("failure in castFromFloat<bool>");
                    returnStatus = 1;
                }
                break;

            default:
                LOGGW("Datatype not supported yet!");
                returnStatus = 1;
                break;
        }
        return returnStatus;
    }


// Setup details for Qnn_Tensor_t for execution
// based on information in Qnn_TensorWrapper_t provided by model.so.
    int setupTensors(Qnn_Tensor_t **tensors, uint32_t tensorCount,
                                         Qnn_Tensor_t *tensorWrappers) {
        int result = 0;

        if (nullptr == tensorWrappers) {
            LOGGW("tensorWrappers is nullptr");
            return 1;
        }
        if (0 == tensorCount) {
            LOGGI("why tensor count is 0, pls check\n");
            return 2;
        }

        *tensors = (Qnn_Tensor_t *) calloc(1, tensorCount * sizeof(Qnn_Tensor_t));
        if (nullptr == *tensors) {
            LOGGW("mem alloc failed for *tensors");
            return 3;
        }

        for (size_t tensorIdx = 0; tensorIdx < tensorCount; tensorIdx++) {
            Qnn_Tensor_t wrapperTensor = tensorWrappers[tensorIdx];
            std::vector<size_t> dims;
            fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(wrapperTensor),
                     QNN_TENSOR_GET_RANK(wrapperTensor));
            if (0 == result) {
                LOGGD("allocateBuffer successful");
                (*tensors)[tensorIdx] = QNN_TENSOR_INIT;
                result = (deepCopyQnnTensorInfo(((*tensors) + tensorIdx), &wrapperTensor) ==
                          true
                          ? 0
                          : 1);
            }

            if (0 == result) {
                LOGGD("deepCopyQnnTensorInfo successful");
                QNN_TENSOR_SET_MEM_TYPE(((*tensors) + tensorIdx), QNN_TENSORMEMTYPE_RAW);
            }

            Qnn_ClientBuffer_t clientBuffer = QNN_CLIENT_BUFFER_INIT;
            result = allocateBuffer(reinterpret_cast<uint8_t **>(&clientBuffer.data),
                                    dims,
                                    QNN_TENSOR_GET_DATA_TYPE((*tensors) + tensorIdx));
            int status = 0;
            size_t length = 0;
            std::tie(status, length) =
                    calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE((*tensors) + tensorIdx));
            if (status != 0) {
                result = 1;
            }
            clientBuffer.dataSize = length;
            QNN_TENSOR_SET_CLIENT_BUF(((*tensors) + tensorIdx), clientBuffer);

            if (0 != result) {
                LOGGW("failure in setupTensors, cleaning up resources");
                if (nullptr != (QNN_TENSOR_GET_CLIENT_BUF((*tensors) + tensorIdx)).data) {
                    free(QNN_TENSOR_GET_CLIENT_BUF((*tensors) + tensorIdx).data);
                }
                tearDownTensors(*tensors, tensorIdx);
                *tensors = nullptr;
                result = 1;
                LOGGW("failure in setupTensors, done cleaning up resources");
                return result;
            }
        }

        return result;
    }


// Clean up all tensors related data after execution.
    int tearDownTensors(Qnn_Tensor_t *tensors, uint32_t tensorCount) {
        for (size_t tensorIdx = 0; tensorIdx < tensorCount; tensorIdx++) {
            //LOGGD("freeing resources for tensor: %d", tensorIdx);
            //LOGGD("freeing resources for tensor: %d", tensorIdx);
            if (nullptr != QNN_TENSOR_GET_DIMENSIONS(tensors[tensorIdx])) {
                //LOGGD("freeing dimensions");
                //LOGGD("freeing dimensions");
                free(QNN_TENSOR_GET_DIMENSIONS(tensors[tensorIdx]));
            }
            if (nullptr != QNN_TENSOR_GET_CLIENT_BUF(tensors[tensorIdx]).data) {
                //LOGGD("freeing clientBuf.data");
                //LOGGD("freeing clientBuf.data");
                free(QNN_TENSOR_GET_CLIENT_BUF(tensors[tensorIdx]).data);
            }
        }
        free(tensors);
        return 0;
    }


// Helper method to allocate a buffer.
    int allocateBuffer(uint8_t **buffer, std::vector<size_t> dims, Qnn_DataType_t dataType) {
        size_t elementCount = calculateElementCount(dims);
        auto returnStatus = 0;
        switch (dataType) {
            case QNN_DATATYPE_FLOAT_32:
                LOGGD("allocating float buffer");
                returnStatus =
                        allocateBuffer<float>(reinterpret_cast<float **>(buffer), elementCount);
                break;

            case QNN_DATATYPE_UINT_8:
            case QNN_DATATYPE_UFIXED_POINT_8:
                LOGGD("allocating uint8_t buffer");
                returnStatus =
                        allocateBuffer<uint8_t>(reinterpret_cast<uint8_t **>(buffer), elementCount);
                break;

            case QNN_DATATYPE_UINT_16:
            case QNN_DATATYPE_UFIXED_POINT_16:
                LOGGD("allocating uint16_t buffer");
                returnStatus = allocateBuffer<uint16_t>
                        (reinterpret_cast<uint16_t **>(buffer), elementCount);
                break;

            case QNN_DATATYPE_UINT_32:
                LOGGD("allocating uint32_t buffer");
                returnStatus = allocateBuffer<uint32_t>
                        (reinterpret_cast<uint32_t **>(buffer), elementCount);
                break;

            case QNN_DATATYPE_INT_8:
                LOGGD("allocating int8_t buffer");
                returnStatus =
                        allocateBuffer<int8_t>(reinterpret_cast<int8_t **>(buffer), elementCount);
                break;

            case QNN_DATATYPE_INT_16:
                LOGGD("allocating int16_t buffer");
                returnStatus =
                        allocateBuffer<int16_t>(reinterpret_cast<int16_t **>(buffer), elementCount);
                break;

            case QNN_DATATYPE_INT_32:
                LOGGD("allocating int32_t buffer");
                returnStatus =
                        allocateBuffer<int32_t>(reinterpret_cast<int32_t **>(buffer), elementCount);
                break;

            case QNN_DATATYPE_BOOL_8:
                LOGGD("allocating bool buffer");
                returnStatus =
                        allocateBuffer<uint8_t>(reinterpret_cast<uint8_t **>(buffer), elementCount);
                break;

            default:
                LOGGW("Datatype not supported yet!");
                returnStatus = 1;
                break;
        }
        return returnStatus;
    }


// Convert data to float or de-quantization. This is used when
// user requests for float output and the model produces
// non-float output.
    int convertToFloat(float **out, Qnn_Tensor_t *tensor) {
        if (nullptr == tensor) {
            LOGGW("tensors is nullptr");
            return 1;
        }
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(tensor), QNN_TENSOR_GET_RANK(tensor));
        int result = 0;
        size_t elementCount = calculateElementCount(dims);
        result = allocateBuffer<float>(out, elementCount);
        if (0 != result) {
            LOGGW("failure in allocateBuffer<float>");
            return result;
        }
        Qnn_DataType_t data_type = QNN_TENSOR_GET_DATA_TYPE(tensor);
        LOGGI("tensor data type %d\n", data_type);
        switch (data_type) {
            case QNN_DATATYPE_UFIXED_POINT_8:
                if (0 !=
                    tfNToFloat<uint8_t>(
                            *out,
                            reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                            QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                            elementCount)) {
                    LOGGW("failure in tfNToFloat<uint8_t>");
                    result = 1;
                }
                break;

            case QNN_DATATYPE_UFIXED_POINT_16:
                if (0 !=
                    tfNToFloat<uint16_t>(
                            *out,
                            reinterpret_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                            QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                            elementCount)) {
                    LOGGW("failure in tfNToFloat<uint8_t>");
                    result = 1;
                }
                break;

            case QNN_DATATYPE_UINT_8:
                if (0 !=
                    castToFloat<uint8_t>(
                            *out,
                            reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            elementCount)) {
                    LOGGW("failure in castToFloat<uint8_t>");
                    result = 1;
                }
                break;

            case QNN_DATATYPE_UINT_16:
                if (0 !=
                    castToFloat<uint16_t>(
                            *out,
                            reinterpret_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            elementCount)) {
                    LOGGW("failure in castToFloat<uint16_t>");
                    result = 1;
                }
                break;

            case QNN_DATATYPE_UINT_32:
                if (0 !=
                    castToFloat<uint32_t>(
                            *out,
                            reinterpret_cast<uint32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            elementCount)) {
                    LOGGW("failure in castToFloat<uint32_t>");
                    result = 1;
                }
                break;

            case QNN_DATATYPE_INT_8:
                if (0 !=
                    castToFloat<int8_t>(
                            *out,
                            reinterpret_cast<int8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            elementCount)) {
                    LOGGW("failure in castToFloat<int8_t>");
                    result = 1;
                }
                break;

            case QNN_DATATYPE_INT_16:
                if (0 !=
                    castToFloat<int16_t>(
                            *out,
                            reinterpret_cast<int16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            elementCount)) {
                    LOGGW("failure in castToFloat<int16_t>");
                    result = 1;
                }
                break;

            case QNN_DATATYPE_INT_32:
                if (0 !=
                    castToFloat<int32_t>(
                            *out,
                            reinterpret_cast<int32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            elementCount)) {
                    LOGGW("failure in castToFloat<int32_t>");
                    result = 1;
                }
                break;

            case QNN_DATATYPE_BOOL_8:
                if (0 !=
                    castToFloat<uint8_t>(
                            *out,
                            reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                            elementCount)) {
                    LOGGW("failure in castToFloat<bool>");
                    result = 1;
                }
                break;

            case QNN_DATATYPE_FLOAT_32: {
                float *src = (float *) (QNN_TENSOR_GET_CLIENT_BUF(tensor).data);
                float *dst = (*out);
                LOGGI("dump output tensor:\n");
                for (size_t i = 0; i < elementCount; i++) {
                    LOGGI("%.2f\t", src[i]);
                    dst[i] = src[i];
                }
            }
                break;

            default:
                LOGGW("datatype not supported yet!");
                result = 1;
                break;
        }
        if (0 != result) {
            LOGGD("freeing *out");
            if (*out != nullptr) {
                free(*out);
                *out = nullptr;
            }
        }

        return result;
    }


// Helper method to convert Output tensors to float and write them
// out to files.
    int convertAndWriteOutputTensorInFloat(
            Qnn_Tensor_t *output,
            std::vector<std::string> outputPaths,
            std::string fileName,
            size_t outputBatchSize) {
        if (nullptr == output) {
            LOGGW("output is nullptr");
            return 1;
        }

        auto returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
        float *floatBuffer = nullptr;
        returnStatus = convertToFloat(&floatBuffer, output);
        if (0 != returnStatus) {
            LOGGW("failure in convertToFloat");
            return 1;
        }
        uint8_t *bufferToWrite = reinterpret_cast<uint8_t *>(floatBuffer);

        if (nullptr != floatBuffer) {
            LOGGD("freeing floatBuffer");
            free(floatBuffer);
            floatBuffer = nullptr;
        }
        return returnStatus;
    }


// Helper method to write out output. There is no de-quantization here.
// Just write output as is to files.
    int writeOutputTensor(Qnn_Tensor_t *output,
                                              std::vector<std::string> outputPaths,
                                              std::string fileName,
                                              size_t outputBatchSize) {
        if (nullptr == output) {
            LOGGW("output is nullptr");
            return 1;
        }
        auto returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
        uint8_t *bufferToWrite = reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(output).data);

        return returnStatus;
    }





// Helper method to allocate a buffer and copy data to it.
    int allocateAndCopyBuffer(uint8_t **buffer, Qnn_Tensor_t *tensor) {
        if (nullptr == tensor) {
            return 1;
        }
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(tensor), QNN_TENSOR_GET_RANK(tensor));
        int datautilStatus = 0;
        size_t length;
        std::tie(datautilStatus, length) =
                calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE(tensor));
        if (datautilStatus != 0) {
            return 1;
        }
        if (0 != allocateBuffer(buffer, dims, QNN_TENSOR_GET_DATA_TYPE(tensor))) {
            LOGGW("failure in allocateBuffer");
            return 1;
        }
        memscpy(*buffer,
                length * sizeof(uint8_t),
                QNN_TENSOR_GET_CLIENT_BUF(tensor).data,
                length * sizeof(uint8_t));
        return 0;
    }


    int fillDims(std::vector<size_t> &dims, uint32_t *inDimensions, uint32_t rank) {
        if (nullptr == inDimensions) {
            LOGGW("input dimensions is nullptr\n");
            return 1;
        }
        LOGGD("rank %d\n", rank);
        for (size_t r = 0; r < rank; r++) {
            dims.push_back(inDimensions[r]);
        }
        return 0;
    }
};

class qnn_implementation_test {
public:
    using BackendIdType = decltype(QnnInterface_t{}.backendId);

    explicit qnn_implementation_test(const std::string &lib_path, const std::string &backend_name,
                                const std::string &model_name) :
            _lib_path(std::move(lib_path)),
            _backend_name(std::move(backend_name)),
            _model_name(std::move(model_name)) {};

    ~qnn_implementation_test() {
        //TODO:
        _input_tensors.clear();
        _output_tensors.clear();
    }

    const qnn_interface_test &get_qnn_interface() {
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


    int init_qnn_model(const char *graph_name, bool debug, uint8_t do_node_validation,
                                       const QnnGraph_Config_t **graph_configs) {
        int result = 0;

        ENTER_FUNC();

        if (nullptr == graph_name) {
            LOGGW("graph name is null\n");
            return 1;
        }

        if (!_graph_name.empty()) {
            //TODO: only one graph is allowed
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


    int finalize_qnn_model() {
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

        free_cached_tensors();

        LEAVE_FUNC();

        return 0;
    }

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

            // set RPC Control Latency
            QnnHtpPerfInfrastructure_PowerConfig_t rpcControlLatency;            // refer QnnHtpPerfInfrastructure.h
            memset(&rpcControlLatency, 0, sizeof(rpcControlLatency));
            rpcControlLatency.option = QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_RPC_CONTROL_LATENCY;
            rpcControlLatency.rpcControlLatencyConfig = 40;         // use rpc control latency recommended 100 us, refer hexagon sdk

            const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = {&rpc_pollingTime, &rpcControlLatency, nullptr};
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
        const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = {&powerConfig, NULL};

        _qnn_htp_perfinfra->setPowerConfig(_qnn_power_configid, powerConfigs);

        return 0;
    }

    /**
     * @param node_name        Lookup name for node/layer
     * @param p_tensor         A pointer to a struct containing information on the tensor
     * @param b_save_tensor    Flag to indicate if tensor should be saved in object for later retrieval
     *                         with class getter functions
     * @return
     */
    int add_tensor(const char *node_name, Qnn_Tensor_t *p_tensor, bool b_save_tensor = true) {
        Qnn_ErrorHandle_t error = QNN_SUCCESS;
        int counts = 0;
        std::string tensor_name;

        if (nullptr == node_name || nullptr == p_tensor) {
            LOGGW("invalid param\n");
            return 1;
        }
        VALIDATE_TENSOR_VERSION((*p_tensor), error);

        std::string map_entry = std::string((QNN_VER_PTR(*p_tensor))->name);
        if (_tensors_map.find(map_entry) != _tensors_map.end()) {
            LOGGW("tensor %s already exists on node %s\n", map_entry.c_str(), node_name);
            return 2;
        }

        const std::map<Qnn_DataType_t, float> data_type_to_size = {
                {QNN_DATATYPE_INT_8,           1},
                {QNN_DATATYPE_INT_16,          2},
                {QNN_DATATYPE_INT_32,          4},
                {QNN_DATATYPE_INT_64,          8},
                {QNN_DATATYPE_UINT_8,          1},
                {QNN_DATATYPE_UINT_16,         2},
                {QNN_DATATYPE_UINT_32,         4},
                {QNN_DATATYPE_UINT_64,         8},
                {QNN_DATATYPE_FLOAT_16,        2},
                {QNN_DATATYPE_FLOAT_32,        4},
                {QNN_DATATYPE_FLOAT_64,        8},
                {QNN_DATATYPE_BOOL_8,          1},
                {QNN_DATATYPE_SFIXED_POINT_4,  0.5},
                {QNN_DATATYPE_SFIXED_POINT_8,  1},
                {QNN_DATATYPE_SFIXED_POINT_16, 2},
                {QNN_DATATYPE_SFIXED_POINT_32, 4},
                {QNN_DATATYPE_UFIXED_POINT_4,  0.5},
                {QNN_DATATYPE_UFIXED_POINT_8,  1},
                {QNN_DATATYPE_UFIXED_POINT_16, 2},
                {QNN_DATATYPE_UFIXED_POINT_32, 4},
        };
        if (data_type_to_size.find((QNN_VER_PTR(*p_tensor))->dataType) == data_type_to_size.end()) {
            LOGGW("invalid tensor type\n");
            return 3;
        }

        if ((QNN_VER_PTR(*p_tensor))->type == QNN_TENSOR_TYPE_STATIC) {
            if ((QNN_VER_PTR(*p_tensor))->memType != QNN_TENSORMEMTYPE_RAW) {
                LOGGW("mismatch between tensor type and tensor memtype\n");
                return 4;
            }

            // verify size expressed by the dims matches the raw tensor size
            uint32_t qnn_tensor_size =
                    std::lround(std::accumulate(
                            (QNN_VER_PTR(*p_tensor))->dimensions,
                            (uint32_t *) ((QNN_VER_PTR(*p_tensor))->dimensions) +
                            (QNN_VER_PTR(*p_tensor))->rank,
                            data_type_to_size.find((QNN_VER_PTR(*p_tensor)->dataType))->second,
                            std::multiplies<float>()));
            LOGGD("qnn tensor size %d\n", qnn_tensor_size);
            qnn_tensor_size =
                    std::lround(std::accumulate(QNN_TENSOR_GET_DIMENSIONS(p_tensor),
                                                QNN_TENSOR_GET_DIMENSIONS(p_tensor) +
                                                QNN_TENSOR_GET_RANK(p_tensor),
                                                data_type_to_size.find(
                                                        QNN_TENSOR_GET_DATA_TYPE(p_tensor))->second,
                                                std::multiplies<float>()));

            LOGGD("qnn tensor size %d\n", qnn_tensor_size);
            if (qnn_tensor_size != ((QNN_VER_PTR(*p_tensor)->clientBuf.dataSize))) {
                LOGGW("this is a static tensor %s but length mismatch\n", QNN_VER_PTR(*p_tensor)->name);
                LOGGW("adding STATIC tensor, length mismatch between clientBuf"
                      "size and tensor Dims(dim * rank * sizeof(datatype) for nodeName: %s, tensorName: %s,"
                      "got tensorSize: %d, tensor.clientBuf.dataSize: %d\n",
                      node_name,
                      QNN_TENSOR_GET_NAME(p_tensor),
                      qnn_tensor_size,
                      QNN_TENSOR_GET_CLIENT_BUF(p_tensor).dataSize);

                return 5;
            }
        }

        if (_debug_tensor && QNN_TENSOR_GET_TYPE(*p_tensor) == QNN_TENSOR_TYPE_NATIVE) {
            // for debug, make all tensors accessible by client
            QNN_TENSOR_SET_TYPE(*p_tensor, QNN_TENSOR_TYPE_APP_READ);
        }

        error = _qnn_interface.qnn_tensor_create_graph_tensor(_qnn_graph_handle, p_tensor);
        //error = _qnn_interface.qnn_tensor_create_context_tensor(_qnn_context_handle, p_tensor);
        if (error == QNN_TENSOR_ERROR_NAME_HASH_COLLISION) {
            LOGGW("tensor name \"qnn_matrix\" hash collision\n");
            return 6;
        }

        if (error != QNN_TENSOR_NO_ERROR) {
            LOGGW("add tensor failure\n");
            return 7;
        } else {
            LOGGD("add tensor %s successfully\n", (QNN_VER_PTR(*p_tensor)->name));
        }

        if (b_save_tensor) {
            Qnn_Tensor_t tensor_copy;
            VALIDATE(deepCopyQnnTensors(*p_tensor, tensor_copy), error);

            if ((QNN_VER_PTR(*p_tensor))->type == QNN_TENSOR_TYPE_APP_WRITE) {
                _input_tensors.push_back(tensor_copy);
            } else if ((QNN_VER_PTR(*p_tensor))->type == QNN_TENSOR_TYPE_APP_READ) {
                _output_tensors.push_back(tensor_copy);
            }

            _tensors_map[map_entry] = tensor_copy;
        }


        return 0;
    }

    int add_tensor(const char *node_name, Qnn_Tensor_t tensor, bool b_save_tensor = true) {
        return add_tensor(node_name, &tensor, b_save_tensor);
    }


    /**
     * @param version      version The QNN version for Op_Config_t structure to use,QNN_OPCONFIG_VERSION_1
     * @param name         The node name to use
     * @param packageName  The node package name (e.g. qti.aisw)
     * @param type         The QNN_OP_QNN_OP_H node type (e.g. QNN_OP_ARGMAX)
     * @param params       A struct object containing all the params for the node to be added. For
     *                     tensorParam case. The tensor will be created within the function and the
     *                     data will be retrieved from the binary blob to set the tensor data
     * @param numOfParams  The number of elements in above params object
     * @param inputNames   List of tensor names for inputs to node. Note: the corresponding qnn
     * tensor objects must be created within this instance prior to being listed as input to a node
     * @param numOfInputs  The number of elements in above inputNames object
     * @param outputTensors List of Qnn_Tensor_t objects for outputs from node.
     *                      Note1: the corresponding qnn tensor objects will be created in function
     *                      and must not already exist.
     *                      Note2: the output names must be unique per graph
     * @param numOfOutputs  The number of elements in above outputs object
     * @return
     */
    int add_node(Qnn_OpConfigVersion_t version,
                                     const char *name,
                                     const char *packageName,
                                     const char *type,
                                     Qnn_Param_t *params,
                                     uint32_t numOfParams,
                                     const char **inputNames,
                                     uint32_t numOfInputs,
                                     Qnn_Tensor_t *outputTensors,
                                     uint32_t numOfOutputs) {
        int nodeError = 0;
        Qnn_OpConfig_t opDefinition = QNN_OPCONFIG_INIT;
        opDefinition.version = version;
        VALIDATE_OP_CONFIG_VERSION((opDefinition), nodeError);

        // populate Qnn param for node
        Qnn_Param_t *nodeParams = (Qnn_Param_t *) malloc(numOfParams * sizeof(Qnn_Param_t));

        // populate input tensors for node
        Qnn_Tensor_t *inputs = (Qnn_Tensor_t *) malloc(numOfInputs * sizeof(Qnn_Tensor_t));

        // populate output tensors of node
        Qnn_Tensor_t *outputs = (Qnn_Tensor_t *) malloc(numOfOutputs * sizeof(Qnn_Tensor_t));

        if (nodeParams == nullptr || inputs == nullptr || outputs == nullptr) {
            LOGGW("failed for allocate memory for creating QNN OpConfig for node %s\n", name);
            FREE_MEMORY(nodeParams, inputs, outputs);
            return 1;
        }

        uint32_t nodeParamsCounter = 0;
        for (size_t i = 0; i < numOfParams; i++) {
            switch (params[i].paramType) {
                case QNN_PARAMTYPE_TENSOR: {
                    Qnn_Tensor_t &tensor = params[i].tensorParam;
                    //TODO: set b_save_tensor to false as no need to save tensor because this function call just for setup params
                    nodeError = add_tensor(name, &tensor, false);
                    if (nodeError != 0) {
                        if (nodeError != 2) {
                            LOGGW("failed to add tensor %s on node %s\n",
                                  QNN_TENSOR_GET_NAME(tensor), name);
                            FREE_MEMORY(nodeParams, inputs, outputs);
                            return nodeError;
                        }
                    }
                    LOGGI("succeed to add tensor %s on node %s\n", QNN_TENSOR_GET_NAME(tensor), name);
                    nodeParams[nodeParamsCounter].paramType = QNN_PARAMTYPE_TENSOR;
                    nodeParams[nodeParamsCounter].name = params[i].name;
                    nodeParams[nodeParamsCounter++].tensorParam = tensor;
                    break;
                }
                case QNN_PARAMTYPE_SCALAR: {
                    nodeParams[nodeParamsCounter].paramType = QNN_PARAMTYPE_SCALAR;
                    nodeParams[nodeParamsCounter].name = params[i].name;
                    nodeParams[nodeParamsCounter++].scalarParam = params[i].scalarParam;
                    break;
                }
                default: {
                    LOGGW("unknown param type passed for param %s on node %s\n", params[i].name, name);
                    FREE_MEMORY(nodeParams, inputs, outputs);
                    return 2;
                }
            }
        }

        size_t inputsCounter = 0;
        for (size_t j = 0; j < numOfInputs; j++) {
            nodeError = get_tensor(name, inputNames[j], inputs[inputsCounter++]);
            if (nodeError != 0) {
                LOGGW("get_tensor() failed for input tensor %s on node %s\n", inputNames[j], name);
                FREE_MEMORY(nodeParams, inputs, outputs);
                return nodeError;
            }
        }

        size_t outputsCounter = 0;
        _output_tensor_map[name] = {};
        for (size_t k = 0; k < numOfOutputs; k++) {
            // create node output tensors
            nodeError = add_tensor(name, outputTensors[k]);
            if (nodeError != 0) {
                LOGGW("add_tensor() failed for output tensor %s on node %s\n",
                      QNN_TENSOR_GET_NAME(outputTensors[k]), name);
                FREE_MEMORY(nodeParams, inputs, outputs);
                return nodeError;
            } else {
                LOGGI("add_tensor() ok for output tensor %s on node %s\n",
                      QNN_TENSOR_GET_NAME(outputTensors[k]), name);
            }
            const char *outTensorName = QNN_TENSOR_GET_NAME(outputTensors[k]);
            _output_tensor_map[name].push_back(outTensorName);
            nodeError = get_tensor(name, outTensorName, outputs[outputsCounter++]);
            if (nodeError != 0) {
                LOGGW("get_tensor() failed for tensor %s on node %s\n", outTensorName, name);
                FREE_MEMORY(nodeParams, inputs, outputs);
                return nodeError;
            }
        }

        // define and add node to graph
        QNN_OP_CFG_SET_NAME(opDefinition, name);
        QNN_OP_CFG_SET_PACKAGE_NAME(opDefinition, packageName);
        QNN_OP_CFG_SET_TYPE_NAME(opDefinition, type);
        QNN_OP_CFG_SET_PARAMS(opDefinition, numOfParams, nodeParams);
        QNN_OP_CFG_SET_INPUTS(opDefinition, numOfInputs, inputs);
        QNN_OP_CFG_SET_OUTPUTS(opDefinition, numOfOutputs, outputs);

        if (_do_node_validations) {
            auto validationStatus = _qnn_interface.qnn_backend_validate_op_config(_qnn_backend_handle,
                                                                                  opDefinition); //TODO: not work
            if (validationStatus == QNN_BACKEND_ERROR_NOT_SUPPORTED) {
                LOGGD("validation API not supported\n");
            } else if (validationStatus != QNN_SUCCESS) {
                LOGGW("validating op config %s failed\n",
                      name); //TODO:[ ERROR ] OpConfig validation failed for ElementWiseAdd
                //FREE_MEMORY(nodeParams, inputs, outputs);
                //return 3;
            }
        }

        if (_qnn_interface.qnn_graph_add_node(_qnn_graph_handle, opDefinition) != QNN_GRAPH_NO_ERROR) {
            LOGGW("adding node %s failed\n", name);
            FREE_MEMORY(nodeParams, inputs, outputs);
            return 4;
        } else {
            LOGGW("adding qnn graph node %s succesfully\n", name);
        }

        FREE_MEMORY(nodeParams, inputs, outputs);

        return 0;
    }

    int get_tensor(const char *&node_name, const char *&tensor_name, Qnn_Tensor_t &tensor) {
        std::string map_entry = std::string(tensor_name);
        if (_tensors_map.find(tensor_name) == _tensors_map.end()) {
            LOGGW("tensor %s not found on node %s\n", map_entry.c_str(), node_name);
            return 1;
        }

        tensor = _tensors_map[map_entry];
        return 0;
    }

    int add_node(const Qnn_OpConfig_t &op_config) {
        return _qnn_interface.qnn_graph_add_node(_qnn_graph_handle, op_config);
    };

    int get_graphinfo_from_model(GraphInfoPtr_t **graphs_info, uint32_t *num_graphsinfo);


    // compose all QNN graphs from prebuilt Qualcomm's dedicated MODEL.so which generated by Qualcomm's dedicated tool
    int compose_special_graph();

    int finalize_graph() {
        LOGGI("enter %s\n", __func__);

        int error = 0;
        LOGGD("qnn graph handle %s, %p\n", get_qnn_graph_name().c_str(), get_qnn_graph_handle());
        for (size_t graph_idx = 0; graph_idx < _graphs_count; graph_idx++) {
            LOGGD("graph handle %p\n", (*_graphs_info)[graph_idx].graph);
            error = _qnn_raw_interface.graphFinalize((*_graphs_info)[graph_idx].graph,
                                                     _qnn_profile_handle, nullptr);
            if (QNN_GRAPH_NO_ERROR != error) {
                LOGGW("qnn graph finalize failure, error %d\n", error);
                LOGGD("qnn graph finalize failure, error %d\n", error);
                //return 1;
            } else {
                LOGGW("qnn graph finalize successfully\n");
                LOGGD("qnn graph finalize successfully\n");
            }
        }

        if (ggml_qnn_profile_level::profile_off != _profile_level) {
            //TODO: handle profile info with _qnn_profile_handle
        }

        LOGGD("qnn graph handle %p\n", _qnn_graph_handle);

        LOGGI("leave %s\n", __func__);

        return 0;
    }

    int execute_graph(
            const std::vector<Qnn_Tensor_t> &input_tensor_structs,
            std::vector<Qnn_Tensor_t> &output_tensor_structs) {
        return _qnn_interface.qnn_graph_execute(
                _qnn_graph_handle,
                input_tensor_structs.data(),
                input_tensor_structs.size(),
                output_tensor_structs.data(),
                output_tensor_structs.size(),
                _qnn_profile_handle, nullptr);
    }

    int execute_graph() {
        int result = 0;
        auto before_exec = std::chrono::high_resolution_clock::now();
        result = _qnn_interface.qnn_graph_execute(_qnn_graph_handle,
                                                  _input_tensors.data(),
                                                  _input_tensors.size(),
                                                  _output_tensors.data(),
                                                  _output_tensors.size(),
                                                  _qnn_profile_handle, nullptr);

        auto after_exec = std::chrono::high_resolution_clock::now();
        double exec_duration =
                std::chrono::duration_cast<std::chrono::microseconds>(
                        after_exec - before_exec)
                        .count() /
                1000.0;

        LOGGD("matrix operation cost %.2f ms\n", exec_duration);

        if (1) {
            std::string dir = "/sdcard/kantv/qnn/debug/";
            ggml_qnn_create_directory(dir);
            LOGGD("dump output tensor to the path: %s\n", dir.c_str());
            for (std::size_t out_idx = 0; out_idx < _output_tensors.size();
                 ++out_idx) {
                const Qnn_Tensor_t &output_tensor = _output_tensors[out_idx];

                std::string output_path =
                        dir + QNN_VER_PTR(output_tensor)->name + "_tensor.raw";

                std::ofstream fout(output_path, std::ios::binary);
                if (fout.fail()) {
                    LOGGW("dump tensor name: %s failure\n", QNN_VER_PTR(output_tensor)->name);
                    return 1;
                }
                fout.write(static_cast<const char *>(QNN_VER_PTR(output_tensor)->clientBuf.data),
                           QNN_VER_PTR(output_tensor)->clientBuf.dataSize);
            }
        }

        return result;
    }

    int execute_special_graph();

    int execute_common_graph(const std::vector<uint8_t *> &input_node_values,
                             std::vector<float *> &output_node_values,
                             std::vector<size_t> output_node_sizes);

    int run_qnn_matrix();

    std::string get_qnn_graph_name() { return _graph_name; }

    std::vector<Qnn_Tensor_t> get_graph_input_tensors() { return _input_tensors; }

    std::vector<Qnn_Tensor_t> get_graph_output_tensors() { return _output_tensors; }

    std::map<std::string, std::vector<std::string>> get_output_tensormap() {
        return _output_tensor_map;
    }

    bool is_rpcmem_initialized() {
        return _rpcmem_initialized;
    }

    void set_rpcmem_initialized(bool initialized) {
        _rpcmem_initialized = initialized;
    }

    bool is_rpcmem_registered(Qnn_MemHandle_t handle) {
        return _qnn_mem_set.count(handle) != 0U;
    }

    void *alloc_rpcmem(size_t bytes, size_t alignment) {
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

        auto aligned_buf = reinterpret_cast<void *>(alignTo(alignment,
                                                            reinterpret_cast<intptr_t>(buf)));
        bool status = _rpcmem_store_map.insert(std::pair<void *, void *>(aligned_buf, buf)).second;
        if (!status) {
            LOGGW("failed to allocate rpc memory\n");
            _pfn_rpc_mem_free(buf);
        }

        return aligned_buf;
    }


    void free_rpcmem(void *buf) {
        if (!_rpcmem_initialized) {
            LOGGW("rpc memory not initialized\n");
        } else if (0 == _rpcmem_store_map.count(buf)) {
            LOGGW("no allocated tensor\n");
        } else {
            _pfn_rpc_mem_free(_rpcmem_store_map[buf]);
            _rpcmem_store_map.erase(buf);
        }
    }


    int32_t rpcmem_to_fd(void *buf) {
        int32_t mem_fd = -1;
        if (!is_rpcmem_initialized()) {
            LOGGW("rpc memory not initialized\n");
        } else {
            mem_fd = _pfn_rpc_mem_to_fd(buf);
        }

        return mem_fd;
    }


    int register_rpcmem(void *p_data, Qnn_Tensor_t *p_tensor) {
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
                /*numDescriptors=*/1u,
                &handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to register shared memory, error %d, %s\n", QNN_GET_ERROR_CODE(error),
                  strerror(error));
            return 6;
        } else {
            LOGGI("tensor %s successfully register shared memory\n", (QNN_VER_PTR(*p_tensor)->name));
        }
        QNN_VER_PTR(*p_tensor)->memHandle = handle;
        //QNN_VER_PTR(*p_tensor)->memType = QNN_TENSORMEMTYPE_MEMHANDLE;
        _qnn_mem_set.insert(handle);

        return 0;
    }


    void unregister_rpcmem() {
        Qnn_ErrorHandle_t error = QNN_SUCCESS;

        if (_qnn_mem_set.empty()) {
            LOGGW("no rpcmem registered\n");
        }

        for (auto &mem_handle: _qnn_mem_set) {
            error = _qnn_interface.qnn_mem_de_register(&mem_handle, 1);
            if (error != QNN_SUCCESS) {
                LOGGW("failed to unregister shared memory, error %d\n", QNN_GET_ERROR_CODE(error));
            }
        }
        _qnn_mem_set.clear();
    }


    bool is_rpcmem_allocated(void *buf) {
        return _rpcmem_store_map.count(buf) != 0U;
    }




    static void qnntest_logcallback(const char *fmt,
                                    QnnLog_Level_t level,
                                    uint64_t timestamp,
                                    va_list argp) {
#if ENABLE_QNN_LOG
        //don't care static variable in PoC stage
        static std::mutex _log_mutex;
        static unsigned char s_qnn_jni_buf[JNI_BUF_LEN];

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
            memset(s_qnn_jni_buf, 0, JNI_BUF_LEN);
            len_content = vsnprintf(reinterpret_cast<char *const>(s_qnn_jni_buf), JNI_BUF_LEN, fmt,
                                    argp);
            //snprintf(reinterpret_cast<char *const>(s_qnn_jni_buf + len_content), JNI_BUF_LEN - len_content, "\n");
            LOGGD("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_jni_buf);
            //if (level <= QNN_LOG_LEVEL_INFO)
            {
                LOGGD("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_jni_buf);
            }
        }
#endif
    }


//TODO:error handle & memory leak handle
    int qnn_init(const QnnSaver_Config_t **saver_config) {
        BackendIdType backend_id = QNN_BACKEND_ID_NULL;
        LOGGD("enter qni_init\n");

        const std::lock_guard<std::mutex> lock(_init_mutex);

        if (0 != load_system()) {
            LOGGW("can not load QNN system lib, pls check why?\n");
            return 1;
        } else {
            LOGGD("load QNN system lib succesfully\n");
        }

        if (0 != load_prebuit_qnn_model()) {
            LOGGW("can not load prebuilt QNN model, pls check why?\n");
            return 1;
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

        _qnn_interface.qnn_log_create(qnntest_logcallback, _qnn_log_level, &_qnn_log_handle);
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

        if (nullptr != _qnn_raw_interface.propertyHasCapability) {
            auto qnnStatus = _qnn_raw_interface.propertyHasCapability(QNN_PROPERTY_GROUP_DEVICE);
            if (QNN_PROPERTY_NOT_SUPPORTED == qnnStatus) {
                LOGGW("device property is not supported\n");
            }
            if (QNN_PROPERTY_ERROR_UNKNOWN_KEY == qnnStatus) {
                LOGGW("device property is not known to backend\n");
            }
        }

        QnnHtpDevice_Arch_t htp_arch;
        QnnHtpDevice_OnChipDeviceInfoExtension_t chipinfo;
        if (_backend_name.find("Htp") != std::variant_npos) {
            const QnnDevice_PlatformInfo_t *p_info = nullptr;
            _qnn_raw_interface.deviceGetPlatformInfo(nullptr, &p_info);
            QNN_LOG_INFO("device counts %d\n", p_info->v1.numHwDevices);
            QnnDevice_HardwareDeviceInfo_t *infos = p_info->v1.hwDevices;
            for (int i = 0; i < p_info->v1.numHwDevices; i++) {
                QNN_LOG_INFO("deviceID:%d, deviceType:%d, numCores %d\n", infos[i].v1.deviceId,
                             infos[i].v1.deviceType, infos[i].v1.numCores);
                QnnDevice_DeviceInfoExtension_t devinfo = infos[i].v1.deviceInfoExtension;
                chipinfo = devinfo->onChipDevice;
                htp_arch = chipinfo.arch;
                QNN_LOG_INFO("htp_type:%d(%s)\n", devinfo->devType,
                             (devinfo->devType == QNN_HTP_DEVICE_TYPE_ON_CHIP) ? "ON_CHIP" : "");
                QNN_LOG_INFO("qualcomm soc_model:%d(%s), htp_arch:%d(%s), vtcm_size:%d MB\n", \
                             chipinfo.socModel, qnn_get_chipset_desc(chipinfo.socModel), \
                             htp_arch, qnn_get_htparch_desc(htp_arch), chipinfo.vtcmSize);

            }
            _qnn_raw_interface.deviceFreePlatformInfo(nullptr, p_info);
        }

        int qnnStatus = 0;
        if (_backend_name.find("Htp") != std::variant_npos) {
            QnnHtpDevice_CustomConfig_t soc_customConfig;
            soc_customConfig.option = QNN_HTP_DEVICE_CONFIG_OPTION_SOC;
            soc_customConfig.socModel = chipinfo.socModel;
            QnnDevice_Config_t soc_devConfig;
            soc_devConfig.option = QNN_DEVICE_CONFIG_OPTION_CUSTOM;
            soc_devConfig.customConfig = &soc_customConfig;

            QnnHtpDevice_CustomConfig_t arch_customConfig;
            arch_customConfig.option    = QNN_HTP_DEVICE_CONFIG_OPTION_ARCH;
            arch_customConfig.arch.arch = chipinfo.arch;
            arch_customConfig.arch.deviceId = 0;  // Id of device to be used. If single device is used by default 0.
            QnnDevice_Config_t arch_devConfig;
            arch_devConfig.option = QNN_DEVICE_CONFIG_OPTION_CUSTOM;
            arch_devConfig.customConfig = &arch_customConfig;
            const QnnDevice_Config_t* pDeviceConfig[] = {&soc_devConfig, &soc_devConfig, NULL};
            qnnStatus = _qnn_raw_interface.deviceCreate(_qnn_log_handle, pDeviceConfig, &_qnn_device_handle);
        } else {
            qnnStatus = _qnn_raw_interface.deviceCreate(_qnn_log_handle, nullptr, &_qnn_device_handle);
        }

        if (QNN_SUCCESS != qnnStatus && QNN_DEVICE_ERROR_UNSUPPORTED_FEATURE != qnnStatus) {
            LOGGW("failed to create QNN device\n");
            LOGGD("failed to create QNN device\n");
        } else {
            LOGGI("create device successfully\n");
        }

        /*
        std::vector<const QnnDevice_Config_t*> temp_device_config;  //TODO:now is empty because I don't know how to use QnnDevice_Config_t currently
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

        _rpc_lib_handle = dlopen("libcdsprpc.so", RTLD_NOW | RTLD_LOCAL);
        if (nullptr == _rpc_lib_handle) {
            LOGGW("failed to load qualcomm's rpc lib, error:%s\n", dlerror());
            return 9;
        } else {
            LOGGD("load rpcmem lib successfully\n");
            set_rpcmem_initialized(true);
        }
        _pfn_rpc_mem_init = reinterpret_cast<pfn_rpc_mem_init>(dlsym(_rpc_lib_handle, "rpcmem_init"));
        _pfn_rpc_mem_deinit = reinterpret_cast<pfn_rpc_mem_deinit>(dlsym(_rpc_lib_handle,
                                                                         "rpcmem_deinit"));
        _pfn_rpc_mem_alloc = reinterpret_cast<pfn_rpc_mem_alloc>(dlsym(_rpc_lib_handle,
                                                                       "rpcmem_alloc"));
        _pfn_rpc_mem_free = reinterpret_cast<pfn_rpc_mem_free>(dlsym(_rpc_lib_handle, "rpcmem_free"));
        _pfn_rpc_mem_to_fd = reinterpret_cast<pfn_rpc_mem_to_fd>(dlsym(_rpc_lib_handle,
                                                                       "rpcmem_to_fd"));
        if (nullptr == _pfn_rpc_mem_init || nullptr == _pfn_rpc_mem_deinit
            || nullptr == _pfn_rpc_mem_alloc || nullptr == _pfn_rpc_mem_free
            || nullptr == _pfn_rpc_mem_to_fd) {
            LOGGW("unable to access symbols in QNN RPC lib. dlerror(): %s", dlerror());
            dlclose(_rpc_lib_handle);
            return 10;
        }

        _pfn_rpc_mem_init();

        std::vector<const QnnContext_Config_t *> temp_context_config; //TODO:now is empty because I don't know how to use QnnContext_Config_t currently
        _qnn_interface.qnn_context_create(_qnn_backend_handle, _qnn_device_handle,
                                          temp_context_config.empty() ? nullptr
                                                                      : temp_context_config.data(),
                                          &_qnn_context_handle);
        if (nullptr == _qnn_context_handle) {
            LOGGW("why failed to initialize qnn context\n");
            LOGGD("why failed to initialize qnn context\n");
            return 8;
        } else {
            LOGGD("initialize qnn context successfully\n");
        }

        QnnDevice_Infrastructure_t device_infra = nullptr;
        Qnn_ErrorHandle_t error = _qnn_raw_interface.deviceGetInfrastructure(&device_infra);
        if (error != QNN_SUCCESS) {
            LOGGW("HTP backend perf_infrastructure creation failed. Error %d", QNN_GET_ERROR_CODE(error));
            //return 9;
        } else {
            LOGGW("HTP backend perf_infrastructure creation ok\n");
            auto* htp_infra = static_cast<QnnHtpDevice_Infrastructure_t*>(device_infra);
            if (htp_infra->infraType != QNN_HTP_DEVICE_INFRASTRUCTURE_TYPE_PERF) {
                LOGGW("HTP infra type = %d, which is not perf infra type", htp_infra->infraType);
            } else {
                LOGGI("HTP infra type = %d\n", htp_infra->infraType);
                QnnHtpDevice_PerfInfrastructure_t  p_out = htp_infra->perfInfra;
            }
        }

#if 1
            if (init_htp_perfinfra()) {
                QNN_LOG_WARN("initialize HTP performance failure\n");
            }
            if (set_rpc_polling()) {
                QNN_LOG_WARN("set RPC polling failure\n");
            }
            if (set_high_performance_mode()) {
                LOGGI("set HTP high performance mode failure\n");
            }
#endif

        LOGGD("leave qni_init\n");

        return 0;
    }


    int qnn_finalize() {
        int ret_status = 0;
        Qnn_ErrorHandle_t error = QNN_SUCCESS;
        ENTER_FUNC();

        _pfn_rpc_mem_deinit();

        if (dlclose(_rpc_lib_handle) != 0) {
            LOGGW("failed to unload qualcomm's rpc lib, error:%s\n", dlerror());
        } else {
            LOGGD("succeed to close rpcmem lib\n");
        }

        VALIDATE(free_cached_tensors(), error);

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

        unload_prebuilt_qnn_model();

        LEAVE_FUNC();

        return ret_status;
    }

private:
    int load_backend(std::string &lib_path, const QnnSaver_Config_t **saver_config) {
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
            //return 4;
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
#if 0 //remove libQnnCPU.so and libQNNSaver.so and reduce size of apk
        QnnSaver_Config_t outputdir_cfg;
    outputdir_cfg.option = QNN_SAVER_CONFIG_OPTION_OUTPUT_DIRECTORY;
    outputdir_cfg.outputDirectory = "/data/data/com.cdeos.kantv/qnn/";

    QnnSaver_Config_t backendid_cfg;
    backendid_cfg.option = QNN_SAVER_CONFIG_OPTION_BACKEND_ID;
    backendid_cfg.backendId = _backend_id;
    const QnnSaver_Config_t *saverCfg[] = {&outputdir_cfg, &backendid_cfg, NULL};
    if (0 == QnnSaver_initialize(saverCfg)) {
        LOGGI("QnnSaver_initialize successfully");
    } else {
        LOGGI("QnnSaver_initialize failure");
    }

    // saver_initialize must be set before backend initialization
    auto saver_initialize = load_qnn_functionpointers<_pfn_QnnSaver_initialize *>(_loaded_lib_handle[backend_id], "QnnSaver_initialize");
    if (nullptr != saver_initialize) {
        error = saver_initialize(saver_config);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to saver_initialize，error %d", QNN_GET_ERROR_CODE(error));
            return 7;
        }
    } else {
        LOGGW("saver_initialize is null\n");
    }
#endif

        return 0;
    }

    int unload_backend() {
        ENTER_FUNC();
        int dlclose_error = 0;
        for (auto &it: _loaded_lib_handle) {
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


    int load_system() {
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
        const QnnSystemInterface_t **provider_list = nullptr;
        error = get_providers(&provider_list, &num_providers);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to get providers, error %d\n", QNN_GET_ERROR_CODE(error));
            return 3;
        }

        if (num_providers != _required_num_providers) {
            //LOGGW("providers is %d instead of required %d\n", num_providers, _required_num_providers);
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


    int unload_system() {
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


    int load_prebuit_qnn_model() {

        LOGGD("model name %s\n", _model_name.c_str());
        if (_model_name.empty()) {
            LOGGD("model name is empty\n");
            return 0;
        }

        void *model_handle = dlopen(_model_name.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (nullptr == model_handle) {
            LOGGW("failed to load prebuilt qnn model:%s, error:%s\n", _model_name.c_str(), dlerror());
            return 1;
        }

        _model_lib_handle = model_handle;

        _qnn_composegraphs_handle = (ComposeGraphsHandleType_t) dlsym(_model_lib_handle,
                                                                      "QnnModel_composeGraphs");
        if (nullptr == _qnn_composegraphs_handle) {
            LOGGW("failed to load prebuilt qnn model:%s, error:%s\n", _model_name.c_str(), dlerror());
            dlclose(_model_lib_handle);
            _model_lib_handle = nullptr;
            return 2;
        }

        _qnn_freegraphs_handle = (FreeGraphsHandleType_t) dlsym(_model_lib_handle,
                                                                "QnnModel_freeGraphsInfo");
        if (nullptr == _qnn_freegraphs_handle) {
            LOGGW("failed to load prebuilt qnn model:%s, error:%s\n", _model_name.c_str(), dlerror());
            dlclose(_model_lib_handle);
            _model_lib_handle = nullptr;
            return 3;
        }

        return 0;
    }


    int unload_prebuilt_qnn_model() {
        if (nullptr != _model_lib_handle) {
            dlclose(_model_lib_handle);
            _model_lib_handle = nullptr;
            _qnn_composegraphs_handle = nullptr;
            _qnn_freegraphs_handle = nullptr;
        }

        return 0;
    }


    void set_qnn_raw_interface(QNN_INTERFACE_VER_TYPE &raw_interface) {
        _qnn_raw_interface = raw_interface;
    }

    void set_qnn_raw_system_interface(QNN_SYSTEM_INTERFACE_VER_TYPE &raw_interface) {
        _qnn_raw_system_interface = raw_interface;
    }

    int free_cached_tensors() {
        int error = 0;

        ENTER_FUNC();
        for (std::map<std::string, Qnn_Tensor_t>::iterator tensorIt = _tensors_map.begin();
             tensorIt != _tensors_map.end();) {
            Qnn_Tensor_t &tensor = tensorIt->second;
            if (QNN_TENSOR_GET_TYPE(tensor) != QNN_TENSOR_TYPE_APP_WRITE &&
                QNN_TENSOR_GET_TYPE(tensor) != QNN_TENSOR_TYPE_APP_READ) {
                VALIDATE(freeQnnTensor(tensor), error);
                tensorIt = _tensors_map.erase(tensorIt);
            } else {
                tensorIt++;
            }
        }

        _tensors_map.clear();
        LEAVE_FUNC();

        return error;
    }


private:
    static constexpr const int _required_num_providers = 1;

private:
    std::string _lib_path;
    std::string _backend_name;
    std::string _model_name;                                 // prebuilt QNN model name
    BackendIdType _backend_id;

    bool _debug_tensor = false; // flag to indicate if requested graph is to be run in debug mode
    bool _do_node_validations = true;  // flag to indicate whether all add_node calls need to be validated
    QnnLog_Level_t _qnn_log_level = QNN_LOG_LEVEL_DEBUG;
    //QnnLog_Level_t _qnn_log_level                   = QNN_LOG_LEVEL_VERBOSE;

    ggml_qnn_profile_level _profile_level = ggml_qnn_profile_level::profile_detail;

    qnn_interface_test _qnn_interface;

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

    ComposeGraphsHandleType_t _qnn_composegraphs_handle = nullptr;
    FreeGraphsHandleType_t _qnn_freegraphs_handle = nullptr;

    std::unordered_set<Qnn_MemHandle_t> _qnn_mem_set;

    GraphInfo_t **_graphs_info = nullptr;
    uint32_t _graphs_count = 0;

    std::vector<Qnn_Tensor_t> _input_tensors;
    std::vector<Qnn_Tensor_t> _output_tensors;
    std::map<std::string, Qnn_Tensor_t> _tensors_map;
    std::map<std::string, std::vector<std::string>> _output_tensor_map;
    std::string _graph_name;

    std::vector<const char *> input_node_names;
    std::vector<std::vector<int64_t>> input_node_dims;
    std::vector<size_t> input_node_sizes;

    std::vector<const char *> output_node_names;
    std::vector<std::vector<int64_t>> output_node_dims;
    std::vector<size_t> output_node_sizes;

    std::vector<uint8_t *> input_node_values;
    std::vector<float *> output_node_values;

    std::mutex _init_mutex;
    std::unordered_map<BackendIdType, void *> _loaded_lib_handle;
    std::unordered_map<std::string, BackendIdType> _lib_path_to_backend_id;
    std::unordered_map<BackendIdType, const QnnInterface_t *> _loaded_backend;

    void * _rpc_lib_handle = nullptr;
    std::atomic_bool _rpcmem_initialized{false};
    pfn_rpc_mem_alloc _pfn_rpc_mem_alloc;
    pfn_rpc_mem_free _pfn_rpc_mem_free;
    pfn_rpc_mem_to_fd _pfn_rpc_mem_to_fd;
    pfn_rpc_mem_init _pfn_rpc_mem_init;
    pfn_rpc_mem_deinit _pfn_rpc_mem_deinit;
    std::unordered_map<void *, void *> _rpcmem_store_map;

    qnn_io_tensor_test _io_tensor;

};

static Qnn_DataType_t qnn_datatype_from_ggml_datatype(enum ggml_type ggmltype) {
    switch (ggmltype) {
        case GGML_TYPE_F16:
            return QNN_DATATYPE_FLOAT_16;
        case GGML_TYPE_F32:
            return QNN_DATATYPE_FLOAT_32;
        case GGML_TYPE_I8:
            return QNN_DATATYPE_INT_8;
        case GGML_TYPE_Q8_0:
            return QNN_DATATYPE_SFIXED_POINT_8;
        default:
            break;

    }
    return QNN_DATATYPE_UNDEFINED;
}

#define ENABLE_MIX_GGML_QNN 1
static int qnn_fuzz(int n_backend_type, int n_ggml_op_type) {
    int result = 0;
    const char *qnn_backend_lib = "libQnnCpu.so";
    int64_t n_begin_time = 0LL;
    int64_t n_end_time = 0LL;
    int64_t n_durtion = 0LL;
    uint8_t *qnn_buffer = nullptr;

    srand(time(NULL));
    ggml_time_init();
    n_begin_time = ggml_time_us();

    LOGGD("enter qnn_rpc_test backend type:%d(%s), ggml op type:%d\n",
          n_backend_type, get_qnn_backend_name(n_backend_type), n_ggml_op_type);

    switch (n_backend_type) {
        case QNN_BACKEND_CPU:
            qnn_backend_lib = "libQnnCpu.so";
            break;

        case QNN_BACKEND_GPU:
            qnn_backend_lib = "libQnnGpu.so";
            break;

        case QNN_BACKEND_NPU: {
            qnn_backend_lib = "libQnnHtp.so";
            std::string path = "/data/local/tmp/";
            LOGGI("path:%s\n", path.c_str());
            LOGGI("qnn backend lib:%s\n", qnn_backend_lib);
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
            break;
        }
        default:
            LOGGW("backend type %d not supported\n", n_backend_type);
            break;
    }
    LOGGD("qnn_backend:%s\n", qnn_backend_lib);
    qnn_implementation_test qnn_backend = qnn_implementation_test("/data/local/tmp/",qnn_backend_lib, "");
    result = qnn_backend.qnn_init(nullptr);
    if (0 != result) {
        LOGGW("init qnn subsystem failed with qnn backend %s, pls check why\n",
              get_qnn_backend_name(n_backend_type));
        LOGGD("init qnn subsystem failed with qnn backend %s, pls check why\n",
              get_qnn_backend_name(n_backend_type));
        result = 1;
        return 1;
    }
    QNN_INTERFACE_VER_TYPE qnn_raw_interface = qnn_backend.get_qnn_raw_interface();
    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface = qnn_backend.get_qnn_raw_system_interface();
    qnn_interface_test qnn_interface = qnn_backend.get_qnn_interface();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        result = 2;
        goto failure;
    }
    {
        QnnDevice_PlatformInfo_t platform_info = {};
        const QnnDevice_PlatformInfo_t * p_info = &platform_info;
        qnn_raw_interface.deviceGetInfo(qnn_backend.get_qnn_device_handle(), &p_info);
        LOGGI("device counts %d", platform_info.v1.numHwDevices);
        QnnDevice_HardwareDeviceInfo_t *infos = platform_info.v1.hwDevices;
        for (int i = 0; i < platform_info.v1.numHwDevices; i++) {
            LOGGI("deviceID:%d, deviceType:%d, numCores %d", infos[i].v1.deviceId,
                  infos[i].v1.deviceType, infos[i].v1.numCores);
        }
    }
    {
        const QnnDevice_PlatformInfo_t * p_info = nullptr;
        qnn_raw_interface.deviceGetPlatformInfo(nullptr, &p_info);
        LOGGI("device counts %d", p_info->v1.numHwDevices);
        QnnDevice_HardwareDeviceInfo_t * infos = p_info->v1.hwDevices;
        for (int i = 0; i < p_info->v1.numHwDevices; i++) {
            LOGGI("deviceID:%d, deviceType:%d, numCores %d", infos[i].v1.deviceId, infos[i].v1.deviceType, infos[i].v1.numCores);
            QnnDevice_DeviceInfoExtension_t devinfo = infos[i].v1.deviceInfoExtension;
            QnnHtpDevice_OnChipDeviceInfoExtension_t chipinfo = devinfo->onChipDevice;
            QnnHtpDevice_Arch_t chiparch = chipinfo.arch;//QNN_HTP_DEVICE_ARCH_V75
            LOGGI("%d", devinfo->devType);//QNN_HTP_DEVICE_TYPE_ON_CHIP
            LOGGI("soc_model:%d(%s), htp_arch:%d, vtcm_size_in_mb:%d MB", chipinfo.socModel, qnn_get_chipset_desc(chipinfo.socModel), chiparch, chipinfo.vtcmSize);
            //Got soc_model=SM8650, arch=75, vtcm=8,
        }
        qnn_raw_interface.deviceFreePlatformInfo(nullptr, p_info);
    }

    if (QNN_BACKEND_NPU == n_backend_type) {
        const int MB_SIZE = (1 << 20);
        size_t rpc_size = 0;
        while (1) {
            qnn_buffer = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(rpc_size, 4));
            if (nullptr == qnn_buffer) {
                //[test-qnn-ops.cpp, qnn_fuzz, 5274]: alloc rpcmem 2048 (MB) failure, No such file or directory
                LOGGW("alloc rpcmem %d (MB) failure, %s\n",  rpc_size / MB_SIZE, strerror(errno));
                goto failure;
            } else {
                //[test-qnn-ops.cpp, qnn_fuzz, 5277]: alloc rpcmem 2044 (MB) successfully on Xiaomi 14
                LOGGD("alloc rpcmem %d (MB) successfully\n", rpc_size / MB_SIZE);
                if (nullptr != qnn_buffer) {
                    qnn_backend.free_rpcmem(qnn_buffer);
                    qnn_buffer = nullptr;
                }
            }
            rpc_size += (256 * MB_SIZE);
        }
    }

failure:
    qnn_backend.qnn_finalize();
    n_end_time = ggml_time_us();
    n_durtion = (n_end_time - n_begin_time) / 1000;
    LOGGD("duration of qnn_fuzz is: %lld milliseconds\n", n_durtion);
    LOGGD("leave qnn_fuzz\n");
    return result;
}


static int qnn_test_rpc_1(int n_backend_type, int n_ggml_op_type) {
    int result = 0;
    std::string graph_name = "qnn_rpc_test1";
    const char *qnn_backend_lib = "libQnnCpu.so";
    uint32_t matrix_input_0[] = {1, 2, 3, 4};
    uint32_t matrix_input_1[] = {1, 2, 3, 4};

    int64_t n_begin_time = 0LL;
    int64_t n_end_time = 0LL;
    int64_t n_durtion = 0LL;

    auto is_io_tensor = [](Qnn_TensorType_t type) {
        return type < QNN_TENSOR_TYPE_STATIC;
    };
    uint8_t *qnn_buffer_0 = nullptr;
    uint8_t *qnn_buffer_1 = nullptr;
    uint8_t *qnn_buffer_2 = nullptr;
    Qnn_Tensor_t tensor_0 = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_1 = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_2 = QNN_TENSOR_INIT;
    Qnn_QuantizeParams_t quantize_param = QNN_QUANTIZE_PARAMS_INIT;
    Qnn_OpConfig_t qnn_opconfig = QNN_OPCONFIG_INIT;
    const char * qnn_op_name = QNN_OP_ELEMENT_WISE_ADD;

    srand(time(NULL));
    ggml_time_init();
    n_begin_time = ggml_time_us();

    LOGGD("enter qnn_rpc_test backend type:%d(%s), ggml op type:%d\n",
          n_backend_type, get_qnn_backend_name(n_backend_type), n_ggml_op_type);

    switch (n_backend_type) {
        case QNN_BACKEND_CPU:
            qnn_backend_lib = "libQnnCpu.so";
            break;

        case QNN_BACKEND_GPU:
            qnn_backend_lib = "libQnnGpu.so";
            break;

        case QNN_BACKEND_NPU: {
            qnn_backend_lib = "libQnnHtp.so";
            std::string path = "/data/local/tmp/";
            LOGGI("path:%s\n", path.c_str());
            LOGGI("qnn backend lib:%s\n", qnn_backend_lib);
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
            break;
        }
        default:
            LOGGW("backend type %d not supported\n", n_backend_type);
            break;
    }

    size_t ctx_size = 0;
    int sizex = 2;
    int sizey = 2;
    struct ggml_context *ctx = nullptr;
    struct ggml_cgraph *gf = nullptr;
    struct ggml_tensor *src0 = nullptr;
    struct ggml_tensor *src1 = nullptr;
    struct ggml_tensor *dst = nullptr;
    ggml_backend_t backend = nullptr;
    ggml_backend_buffer_t buffer = nullptr;
    ggml_type qtype = GGML_TYPE_I8;
    qtype = GGML_TYPE_F16;
    qtype = GGML_TYPE_F32;
    qtype = GGML_TYPE_Q8_0;
    qtype = GGML_TYPE_F32;
    ctx_size += 1024 * 1024 * 32;
    QNN_LOG_DEBUG("Allocating Memory of size %zi bytes, %zi MB\n", ctx_size, (ctx_size / 1024 / 1024));
    struct ggml_init_params params = {
            /*.mem_size   =*/ ctx_size,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };
    ctx = ggml_init(params);
    if (!ctx) {
        QNN_LOG_ERROR("%s: ggml_init() failed\n");
        return 1;
    }
    QNN_LOG_DEBUG("creating new tensors\n");
    QNN_LOG_DEBUG("ggml_blck_size(%s) %d\n", ggml_type_name(qtype), ggml_blck_size(qtype));
    QNN_LOG_DEBUG("ggml_type_size(%s) %d\n", ggml_type_name(qtype), ggml_type_size(qtype));
    if (ggml_is_quantized(qtype)) {
        int blksize = ggml_blck_size(qtype);
        if (sizex % blksize  != 0) {
            sizex = sizex / blksize * blksize + blksize;
            if (n_ggml_op_type == GGML_OP_MUL_MAT) {
                //sizex = blksize * 2;
            }
        }
    }
    QNN_LOG_DEBUG("sizex %d\n", sizex);
    if (n_ggml_op_type == GGML_OP_MUL) {
        src0 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    } else if (n_ggml_op_type == GGML_OP_ADD) {
        if (ggml_is_quantized(qtype)) {
            assert(sizex % ggml_blck_size(qtype) == 0);
        }
        src0 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    } else {
        //MULMAT
        if (ggml_is_quantized(qtype)) {
            assert(sizex % ggml_blck_size(qtype) == 0);
        }
        src0 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    }
    ggml_set_input(src0);
    ggml_set_input(src1);
    switch (n_ggml_op_type) {
        case GGML_OP_ADD:
            dst = ggml_add(ctx, src0, src1);
            qnn_op_name = QNN_OP_ELEMENT_WISE_ADD;
            break;
        case GGML_OP_MUL:
            dst = ggml_mul(ctx, src0, src1);
            qnn_op_name = QNN_OP_ELEMENT_WISE_MULTIPLY;
            break;
        case GGML_OP_MUL_MAT:
            dst = ggml_mul_mat(ctx, src0, src1);
            qnn_op_name = QNN_OP_MAT_MUL;
            break;
        default:
            QNN_LOG_WARN("ggml op %d(%s) not supported", n_ggml_op_type, ggml_op_name((enum ggml_op) n_ggml_op_type));
            ggml_free(ctx);
            ggml_backend_free(backend);
            return 2;
    }
    ggml_set_output(dst);
    QNN_LOG_DEBUG("creating compute graph\n");
    gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, dst);
    if (n_backend_type != QNN_BACKEND_GGML) {
        initialize_tensors(ctx);
    } else {
        if (qtype == GGML_TYPE_F32) {
            ggml_set_f32(src0, (rand() % 100 + 1));
            ggml_set_f32(src1, (rand() % 100 + 1));
        } else {
            initialize_tensors(ctx);
        }
    }


    LOGGD("qnn_backend:%s\n", qnn_backend_lib);
    qnn_implementation_test qnn_backend = qnn_implementation_test("/data/local/tmp/",qnn_backend_lib, "");
    result = qnn_backend.qnn_init(nullptr);
    if (0 != result) {
        LOGGW("init qnn subsystem failed with qnn backend %s, pls check why\n",
              get_qnn_backend_name(n_backend_type));
        LOGGD("init qnn subsystem failed with qnn backend %s, pls check why\n",
              get_qnn_backend_name(n_backend_type));
        result = 1;
        return 1;
    }
    QNN_INTERFACE_VER_TYPE qnn_raw_interface = qnn_backend.get_qnn_raw_interface();
    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface = qnn_backend.get_qnn_raw_system_interface();
    qnn_interface_test qnn_interface = qnn_backend.get_qnn_interface();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        result = 2;
        goto failure;
    }
    {
        QnnDevice_PlatformInfo_t platform_info = {};
        const QnnDevice_PlatformInfo_t *p_info = &platform_info;
        qnn_raw_interface.deviceGetInfo(qnn_backend.get_qnn_device_handle(), &p_info);
        LOGGI("device counts %d", platform_info.v1.numHwDevices);
    }
    { //the following workaround method works fine as expect
        int error = 0;
        Qnn_GraphHandle_t graph_handle = nullptr;
        error = qnn_raw_interface.graphCreate(qnn_backend.get_qnn_context_handle(), graph_name.c_str(), nullptr, &graph_handle);
        LOGGI("error = %d\n", error);

#if ENABLE_MIX_GGML_QNN
        Qnn_DataType_t src0_qnn_type                = QNN_DATATYPE_FLOAT_32;
        Qnn_DataType_t src1_qnn_type                = QNN_DATATYPE_FLOAT_32;
        Qnn_DataType_t dst_qnn_type                 = QNN_DATATYPE_FLOAT_32;

        uint32_t dimensions_input_0[] = {(uint32_t) src0->ne[0], (uint32_t) src0->ne[1],
                                         (uint32_t) src0->ne[2], (uint32_t) src0->ne[3]};
        uint32_t dimensions_input_1[] = {(uint32_t) src1->ne[0], (uint32_t) src1->ne[1],
                                         (uint32_t) src1->ne[2], (uint32_t) src1->ne[3]};
        uint32_t dimensions_output[]  = {(uint32_t) dst->ne[0], (uint32_t) dst->ne[1],
                                         (uint32_t) dst->ne[2], (uint32_t) dst->ne[3]};
#else
        uint32_t dimensions_input_0[] = {2, 2};
        uint32_t dimensions_input_1[] = {2, 2};
        uint32_t dimensions_output[] = {2, 2};
        float input_matrix[2][4] = {{1, 1, 1, 1},
                                    {2, 2, 2, 2}};
        float output_matrix[1][4] = {{1.0, 1.0, 1.0, 1.0}};
#endif


        tensor_0 = {
                .version= QNN_TENSOR_VERSION_1,
                {.v1= {
                        .id=0,
                        .name= "tensor_0",
                        .type= QNN_TENSOR_TYPE_APP_WRITE,
                        .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                        .dataType= QNN_DATATYPE_FLOAT_32,
                        .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                          QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                          {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= 2,
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
                        .dataType= QNN_DATATYPE_FLOAT_32,
                        .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                          QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                          {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= 2,
                        .dimensions=dimensions_input_1,
                        .memType= QNN_TENSORMEMTYPE_RAW,
                        {.clientBuf= {.data=nullptr,
                                .dataSize=0}}}}
        };

        Qnn_Tensor_t tensor_2 = (Qnn_Tensor_t) {
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
                        .rank= 2,
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

#if ENABLE_MIX_GGML_QNN
        src0_qnn_type                = qnn_datatype_from_ggml_datatype(src0->type);
        src1_qnn_type                = qnn_datatype_from_ggml_datatype(src1->type);
        dst_qnn_type                 = qnn_datatype_from_ggml_datatype(dst->type);
        QNN_VER_PTR(tensor_0)->dimensions = dimensions_input_0;
        QNN_VER_PTR(tensor_0)->rank = get_tensor_rank(src0);
        QNN_VER_PTR(tensor_0)->dataType = src0_qnn_type;
        QNN_VER_PTR(tensor_1)->dimensions = dimensions_input_1;
        QNN_VER_PTR(tensor_1)->rank = get_tensor_rank(src1);
        QNN_VER_PTR(tensor_1)->dataType = src1_qnn_type;
        QNN_VER_PTR(tensor_2)->dimensions = dimensions_output;
        QNN_VER_PTR(tensor_2)->rank = get_tensor_rank(dst);
        QNN_VER_PTR(tensor_2)->dataType = dst_qnn_type;
#endif

        if (QNN_BACKEND_NPU == n_backend_type) {
            qnn_buffer_0 = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(ggml_nbytes(src0), 4));
            if (nullptr == qnn_buffer_0) {
                LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
                goto failure;
            } else {
                LOGGD("alloc rpcmem successfully\n");
            }
            qnn_backend.register_rpcmem(qnn_buffer_0, &tensor_0);
#if ENABLE_MIX_GGML_QNN
            memcpy(qnn_buffer_0, src0->data, ggml_nbytes(src0));
            QNN_VER_PTR(tensor_0)->clientBuf = {qnn_buffer_0, static_cast<uint32_t>(ggml_nbytes(src0))};
#else
            memcpy(qnn_buffer_0, input_matrix[0], 16);
            QNN_VER_PTR(tensor_0)->clientBuf = {qnn_buffer_0, 16};
#endif

            qnn_buffer_1 = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(ggml_nbytes(src1), 4));
            if (nullptr == qnn_buffer_1) {
                LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
                goto failure;
            } else {
                LOGGD("alloc rpcmem successfully\n");
            }
            qnn_backend.register_rpcmem(qnn_buffer_1, &tensor_1);
#if ENABLE_MIX_GGML_QNN
            memcpy(qnn_buffer_1, src1->data, ggml_nbytes(src1));
            QNN_VER_PTR(tensor_1)->clientBuf = {qnn_buffer_1, static_cast<uint32_t>(ggml_nbytes(src1))};
#else
            memcpy(qnn_buffer_1, input_matrix[1], 16);
            QNN_VER_PTR(tensor_1)->clientBuf = {qnn_buffer_1, 16};
#endif


            qnn_buffer_2 = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(ggml_nbytes(dst), 4));
            if (nullptr == qnn_buffer_2) {
                LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
                goto failure;
            } else {
                LOGGD("alloc rpcmem successfully\n");
            }
            qnn_backend.register_rpcmem(qnn_buffer_2, &tensor_2);
#if ENABLE_MIX_GGML_QNN
            memcpy(qnn_buffer_2, dst->data, ggml_nbytes(dst));
            QNN_VER_PTR(tensor_2)->clientBuf = {qnn_buffer_2, static_cast<uint32_t>(ggml_nbytes(dst))};
#else
            QNN_VER_PTR(tensor_2)->clientBuf = {qnn_buffer_2, 16};
            QNN_VER_PTR(tensor_2)->clientBuf = {qnn_buffer_2, 16};
#endif

        } else {
            //here is similar to OpenMAX IL
#if ENABLE_MIX_GGML_QNN
            QNN_VER_PTR(tensor_0)->clientBuf = {src0->data, static_cast<uint32_t>(ggml_nbytes(src0))};
            QNN_VER_PTR(tensor_1)->clientBuf = {src1->data, static_cast<uint32_t>(ggml_nbytes(src1))};
            QNN_VER_PTR(tensor_2)->clientBuf = {dst->data, static_cast<uint32_t>(ggml_nbytes(dst))};

#else
            QNN_VER_PTR(tensor_0)->clientBuf = {input_matrix[0], 16};
            QNN_VER_PTR(tensor_1)->clientBuf = {input_matrix[1], 16};
            QNN_VER_PTR(tensor_2)->clientBuf = {output_matrix[0], 16};
#endif
        }

        //for this single computation graph which contains three nodes(every node is a tensor,
        //the GGML_OP_ADD is the edge of this single computation graph), nullptr is ok
        //
        //tensor_2 = tensor_0 + tensor_1
        //
        // tensor_0
        //          + (GGML_OP_ADD)    =   tensor_2
        // tensor_1
        //
        Qnn_Param_t params[] = {};
        Qnn_Tensor_t tensor_inputs[] = {
                tensor_0,
                tensor_1
        };

        Qnn_Tensor_t tensor_outputs[] = {
                tensor_2
        };

        Qnn_OpConfig_t opconfig = {
                (Qnn_OpConfigVersion_t) 1, .v1 = {
                        "qnn_rpc_test",
                        QNN_OP_PACKAGE_NAME_QTI_AISW,
                        qnn_op_name,
                        0,
                        params,
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
#if ENABLE_MIX_GGML_QNN
        LOGGD("dump tensor:\n");
        TENSOR_DUMP(src0);
        TENSOR_DUMP(src1);
#else
        if (QNN_BACKEND_NPU != n_backend_type) {
            for (size_t i = 0; i < 2; i++) {
                if (0 == i) {
                    LOGGD("input matrix A:");
                } else if (1 == i) {
                    LOGGD("input matrix B:");
                }
                float *temp = input_matrix[i];
                LOGGD("%.2f \t %.2f\n", temp[0], temp[1]);
                LOGGD("%.2f \t %.2f\n", temp[2], temp[3]);
            }

        } else {
            float *temp = nullptr;
            for (size_t i = 0; i < 2; i++) {
                if (0 == i) {
                    LOGGD("input matrix A:");
                    temp = (float*)qnn_buffer_0;
                } else if (1 == i) {
                    LOGGD("input matrix B:");
                    temp = (float*)qnn_buffer_1;
                }
                LOGGD("%.2f \t %.2f\n", temp[0], temp[1]);
                LOGGD("%.2f \t %.2f\n", temp[2], temp[3]);
            }

        }
#endif
        error = qnn_raw_interface.graphExecute(graph_handle, tensor_inputs, 2, tensor_outputs, 1,
                                               nullptr, nullptr);
        LOGGI("error = %d\n", error);
        if (0 == error) {
            float *temp = (float *)((QNN_VER_PTR(tensor_2)->clientBuf.data));
            LOGGD("output tensor:%.2f %.2f %.2f %.2f\n", temp[0], temp[1], temp[2], temp[3]);

            if (0 == result) {
                LOGGD("output matrix:");
#if ENABLE_MIX_GGML_QNN
                if (QNN_BACKEND_NPU == n_backend_type) {
                    memcpy(dst->data, qnn_buffer_2, ggml_nbytes(dst));
                }
                TENSOR_DUMP(dst);
#else
                for (size_t i = 0; i < 1; i++) {
                    float *temp = nullptr;
                    if (QNN_BACKEND_NPU == n_backend_type)
                        temp = (float *) qnn_buffer_2;
                    else
                        temp = output_matrix[i];
                    LOGGD("%.2f \t %.2f\n", temp[0], temp[1]);
                    LOGGD("%.2f \t %.2f\n", temp[2], temp[3]);
                }
#endif
            }
        }
    }

failure:
    qnn_backend.unregister_rpcmem();
    if (nullptr != qnn_buffer_0)
        qnn_backend.free_rpcmem(qnn_buffer_0);
    if (nullptr != qnn_buffer_1)
        qnn_backend.free_rpcmem(qnn_buffer_1);
    if (nullptr != qnn_buffer_2)
        qnn_backend.free_rpcmem(qnn_buffer_2);

    qnn_backend.qnn_finalize();

    ggml_free(ctx);

    n_end_time = ggml_time_us();
    n_durtion = (n_end_time - n_begin_time) / 1000;
    LOGGD("duration of qnn_rpc_test is: %lld milliseconds\n", n_durtion);

    LOGGD("leave qnn_rpc_test\n");

    return result;
}


static int qnn_test_rpc_2(int n_backend_type, int n_ggml_op_type) {
    int result = 0;
    std::string graph_name = "qnn_rpc_test2";
    const char *qnn_backend_lib = "libQnnCpu.so";

    auto is_io_tensor = [](Qnn_TensorType_t type) {
        return type < QNN_TENSOR_TYPE_STATIC;
    };
    uint8_t *qnn_buffer_0 = nullptr;
    uint8_t *qnn_buffer_1 = nullptr;
    uint8_t *qnn_buffer_2 = nullptr;
    Qnn_Tensor_t tensor_0 = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_1 = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_2 = QNN_TENSOR_INIT;
    Qnn_QuantizeParams_t quantize_param = QNN_QUANTIZE_PARAMS_INIT;
    Qnn_OpConfig_t qnn_opconfig = QNN_OPCONFIG_INIT;
    const char * qnn_op_name = QNN_OP_ELEMENT_WISE_ADD;

    srand(time(NULL));
    qnn_perf perf("qnn_rpc_test2");
    perf.start();

    LOGGD("enter qnn_rpc_test backend type:%d(%s), ggml op type:%d\n",
          n_backend_type, get_qnn_backend_name(n_backend_type), n_ggml_op_type);

    switch (n_backend_type) {
        case QNN_BACKEND_CPU:
            qnn_backend_lib = "libQnnCpu.so";
            break;

        case QNN_BACKEND_GPU:
            qnn_backend_lib = "libQnnGpu.so";
            break;

        case QNN_BACKEND_NPU: {
            qnn_backend_lib = "libQnnHtp.so";
            std::string path = "/data/local/tmp/";
            LOGGI("path:%s\n", path.c_str());
            LOGGI("qnn backend lib:%s\n", qnn_backend_lib);
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
            break;
        }
        default:
            LOGGW("backend type %d not supported\n", n_backend_type);
            break;
    }

    size_t ctx_size = 0;
    int sizex = 384;
    int sizey = 384;
    struct ggml_context *ctx = nullptr;
    struct ggml_cgraph *gf = nullptr;
    struct ggml_tensor *src0 = nullptr;
    struct ggml_tensor *src1 = nullptr;
    struct ggml_tensor *dst = nullptr;
    ggml_backend_t backend = nullptr;
    ggml_backend_buffer_t buffer = nullptr;
    ggml_type qtype = GGML_TYPE_I8;
    qtype = GGML_TYPE_F16;
    qtype = GGML_TYPE_F32;
    qtype = GGML_TYPE_Q8_0;
    qtype = GGML_TYPE_F32;
    ctx_size += sizex * sizey * 32 * 4;
    QNN_LOG_DEBUG("Allocating Memory of size %zi bytes, %zi MB\n", ctx_size, (ctx_size / 1024 / 1024));
    struct ggml_init_params params = {
            /*.mem_size   =*/ ctx_size,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };
    ctx = ggml_init(params);
    if (!ctx) {
        QNN_LOG_ERROR("%s: ggml_init() failed\n");
        return 1;
    }
    QNN_LOG_DEBUG("creating new tensors\n");
    QNN_LOG_DEBUG("ggml_blck_size(%s) %d\n", ggml_type_name(qtype), ggml_blck_size(qtype));
    QNN_LOG_DEBUG("ggml_type_size(%s) %d\n", ggml_type_name(qtype), ggml_type_size(qtype));
    if (ggml_is_quantized(qtype)) {
        int blksize = ggml_blck_size(qtype);
        if (sizex % blksize  != 0) {
            sizex = sizex / blksize * blksize + blksize;
            if (n_ggml_op_type == GGML_OP_MUL_MAT) {
                //sizex = blksize * 2;
            }
        }
    }
    QNN_LOG_DEBUG("sizex %d\n", sizex);
    if (n_ggml_op_type == GGML_OP_MUL) {
        src0 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    } else if (n_ggml_op_type == GGML_OP_ADD) {
        if (ggml_is_quantized(qtype)) {
            assert(sizex % ggml_blck_size(qtype) == 0);
        }
        src0 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    } else {
        //MULMAT
        if (ggml_is_quantized(qtype)) {
            assert(sizex % ggml_blck_size(qtype) == 0);
        }
        src0 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
        src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    }
    ggml_set_input(src0);
    ggml_set_input(src1);
    switch (n_ggml_op_type) {
        case GGML_OP_ADD:
            dst = ggml_add(ctx, src0, src1);
            qnn_op_name = QNN_OP_ELEMENT_WISE_ADD;
            break;
        case GGML_OP_MUL:
            dst = ggml_mul(ctx, src0, src1);
            qnn_op_name = QNN_OP_ELEMENT_WISE_MULTIPLY;
            break;
        case GGML_OP_MUL_MAT:
            dst = ggml_mul_mat(ctx, src0, src1);
            qnn_op_name = QNN_OP_MAT_MUL;
            break;
        default:
            QNN_LOG_WARN("ggml op %d(%s) not supported", n_ggml_op_type, ggml_op_name((enum ggml_op) n_ggml_op_type));
            ggml_free(ctx);
            ggml_backend_free(backend);
            return 2;
    }
    ggml_set_output(dst);
    QNN_LOG_DEBUG("creating compute graph\n");
    gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, dst);
    if (n_backend_type != QNN_BACKEND_GGML) {
        initialize_tensors(ctx);
    } else {
        if (qtype == GGML_TYPE_F32) {
            ggml_set_f32(src0, (rand() % 100 + 1));
            ggml_set_f32(src1, (rand() % 100 + 1));
        } else {
            initialize_tensors(ctx);
        }
    }


    LOGGD("qnn_backend:%s\n", qnn_backend_lib);
    qnn_implementation_test qnn_backend = qnn_implementation_test("/data/local/tmp/",qnn_backend_lib, "");
    result = qnn_backend.qnn_init(nullptr);
    if (0 != result) {
        LOGGW("init qnn subsystem failed with qnn backend %s, pls check why\n",
              get_qnn_backend_name(n_backend_type));
        LOGGD("init qnn subsystem failed with qnn backend %s, pls check why\n",
              get_qnn_backend_name(n_backend_type));
        result = 1;
        return 1;
    }
    QNN_INTERFACE_VER_TYPE qnn_raw_interface = qnn_backend.get_qnn_raw_interface();
    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface = qnn_backend.get_qnn_raw_system_interface();
    qnn_interface_test qnn_interface = qnn_backend.get_qnn_interface();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        result = 2;
        goto failure;
    }
    {
        QnnDevice_PlatformInfo_t platform_info = {};
        const QnnDevice_PlatformInfo_t *p_info = &platform_info;
        qnn_raw_interface.deviceGetInfo(qnn_backend.get_qnn_device_handle(), &p_info);
        LOGGI("device counts %d", platform_info.v1.numHwDevices);
    }
    {
        int error = 0;
        Qnn_GraphHandle_t graph_handle = nullptr;
        if (n_backend_type == QNN_BACKEND_NPU) {
            QnnHtpGraph_CustomConfig_t custom_config;
            custom_config.option = QNN_HTP_GRAPH_CONFIG_OPTION_NUM_HVX_THREADS;
            custom_config.numHvxThreads = 8; // set a number. MAX = number of HVX HW blocks for that SoC
            QnnGraph_Config_t graph_config;
            graph_config.option = QNN_GRAPH_CONFIG_OPTION_CUSTOM;
            graph_config.customConfig = &custom_config;


             QnnHtpGraph_CustomConfig_t customConfig;
            customConfig.option = QNN_HTP_GRAPH_CONFIG_OPTION_OPTIMIZATION;
            customConfig.optimizationOption.type = QNN_HTP_GRAPH_OPTIMIZATION_TYPE_ENABLE_DLBC;
            customConfig.optimizationOption.floatValue = 0; // set to 0 to turn off

            QnnGraph_Config_t graphConfig;
            graphConfig.option       = QNN_GRAPH_CONFIG_OPTION_CUSTOM;
            graphConfig.customConfig = &customConfig;

            const QnnGraph_Config_t *p_graphconfig[] = {&graph_config, &graphConfig, NULL};
            error = qnn_raw_interface.graphCreate(qnn_backend.get_qnn_context_handle(),graph_name.c_str(), p_graphconfig, &graph_handle);
        } else {
            error = qnn_raw_interface.graphCreate(qnn_backend.get_qnn_context_handle(),graph_name.c_str(), nullptr, &graph_handle);
        }
        LOGGI("error = %d\n", error);

        Qnn_DataType_t src0_qnn_type                = QNN_DATATYPE_FLOAT_32;
        Qnn_DataType_t src1_qnn_type                = QNN_DATATYPE_FLOAT_32;
        Qnn_DataType_t dst_qnn_type                 = QNN_DATATYPE_FLOAT_32;

        uint32_t dimensions_input_0[] = {(uint32_t) src0->ne[0], (uint32_t) src0->ne[1],
                                         (uint32_t) src0->ne[2], (uint32_t) src0->ne[3]};
        uint32_t dimensions_input_1[] = {(uint32_t) src1->ne[0], (uint32_t) src1->ne[1],
                                         (uint32_t) src1->ne[2], (uint32_t) src1->ne[3]};
        uint32_t dimensions_output[]  = {(uint32_t) dst->ne[0], (uint32_t) dst->ne[1],
                                         (uint32_t) dst->ne[2], (uint32_t) dst->ne[3]};


        tensor_0 = {
                .version= QNN_TENSOR_VERSION_1,
                {.v1= {
                        .id=0,
                        .name= "tensor_0",
                        .type= QNN_TENSOR_TYPE_APP_WRITE,
                        .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                        .dataType= QNN_DATATYPE_FLOAT_32,
                        .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                          QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                          {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= get_tensor_rank(src0),
                        .dimensions=dimensions_input_0,
                        .memType= QNN_TENSORMEMTYPE_MEMHANDLE,
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
                        .dataType= QNN_DATATYPE_FLOAT_32,
                        .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                          QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                          {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= get_tensor_rank(src1),
                        .dimensions=dimensions_input_1,
                        .memType= QNN_TENSORMEMTYPE_MEMHANDLE,
                        {.clientBuf= {.data=nullptr,
                                .dataSize=0}}}}
        };

        Qnn_Tensor_t tensor_2 = (Qnn_Tensor_t) {
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
                        .memType= QNN_TENSORMEMTYPE_MEMHANDLE,
                        {.clientBuf= {.data=nullptr,
                                .dataSize=0}}}}};


        error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_0);
        LOGGI("error = %d\n", error);
        error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_1);
        LOGGI("error = %d\n", error);
        error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_2);
        LOGGI("error = %d\n", error);

        src0_qnn_type                = qnn_datatype_from_ggml_datatype(src0->type);
        src1_qnn_type                = qnn_datatype_from_ggml_datatype(src1->type);
        dst_qnn_type                 = qnn_datatype_from_ggml_datatype(dst->type);
        QNN_VER_PTR(tensor_0)->dimensions = dimensions_input_0;
        QNN_VER_PTR(tensor_0)->rank = get_tensor_rank(src0);
        QNN_VER_PTR(tensor_0)->dataType = src0_qnn_type;
        QNN_VER_PTR(tensor_1)->dimensions = dimensions_input_1;
        QNN_VER_PTR(tensor_1)->rank = get_tensor_rank(src1);
        QNN_VER_PTR(tensor_1)->dataType = src1_qnn_type;
        QNN_VER_PTR(tensor_2)->dimensions = dimensions_output;
        QNN_VER_PTR(tensor_2)->rank = get_tensor_rank(dst);
        QNN_VER_PTR(tensor_2)->dataType = dst_qnn_type;

        if (QNN_BACKEND_NPU == n_backend_type) {
            qnn_buffer_0 = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(ggml_nbytes(src0), 4));
            if (nullptr == qnn_buffer_0) {
                LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
                goto failure;
            } else {
                LOGGD("alloc rpcmem successfully\n");
            }
            qnn_backend.register_rpcmem(qnn_buffer_0, &tensor_0);
            memcpy(qnn_buffer_0, src0->data, ggml_nbytes(src0));
            //QNN_VER_PTR(tensor_0)->clientBuf = {qnn_buffer_0, static_cast<uint32_t>(ggml_nbytes(src0))};

            qnn_buffer_1 = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(ggml_nbytes(src1), 4));
            if (nullptr == qnn_buffer_1) {
                LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
                goto failure;
            } else {
                LOGGD("alloc rpcmem successfully\n");
            }
            qnn_backend.register_rpcmem(qnn_buffer_1, &tensor_1);
            memcpy(qnn_buffer_1, src1->data, ggml_nbytes(src1));
            //QNN_VER_PTR(tensor_1)->clientBuf = {qnn_buffer_1, static_cast<uint32_t>(ggml_nbytes(src1))};

            qnn_buffer_2 = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(ggml_nbytes(dst), 4));
            if (nullptr == qnn_buffer_2) {
                LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
                goto failure;
            } else {
                LOGGD("alloc rpcmem successfully\n");
            }
            qnn_backend.register_rpcmem(qnn_buffer_2, &tensor_2);
            //memcpy(qnn_buffer_2, dst->data, ggml_nbytes(dst));
            //QNN_VER_PTR(tensor_2)->clientBuf = {qnn_buffer_2, static_cast<uint32_t>(ggml_nbytes(dst))};

        } else {
            //here is similar to OpenMAX IL
            QNN_VER_PTR(tensor_0)->clientBuf = {src0->data, static_cast<uint32_t>(ggml_nbytes(src0))};
            QNN_VER_PTR(tensor_1)->clientBuf = {src1->data, static_cast<uint32_t>(ggml_nbytes(src1))};
            QNN_VER_PTR(tensor_2)->clientBuf = {dst->data, static_cast<uint32_t>(ggml_nbytes(dst))};
        }

        Qnn_Param_t params[] = {};
        Qnn_Tensor_t tensor_inputs[] = {
                tensor_0,
                tensor_1
        };

        Qnn_Tensor_t tensor_outputs[] = {
                tensor_2
        };

        Qnn_OpConfig_t opconfig = {
                (Qnn_OpConfigVersion_t) 1, .v1 = {
                        "qnn_rpc_test",
                        QNN_OP_PACKAGE_NAME_QTI_AISW,
                        qnn_op_name,
                        0,
                        params,
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
        LOGGD("dump tensor:\n");
        TENSOR_DUMP(src0);
        TENSOR_DUMP(src1);
        error = qnn_raw_interface.graphExecute(graph_handle, tensor_inputs, 2, tensor_outputs, 1,
                                               nullptr, nullptr);
        LOGGI("error = %d\n", error);
        if (0 == error) {
            if (0 == result) {
                LOGGD("output matrix:");
                if (QNN_BACKEND_NPU == n_backend_type) {
                    memcpy(dst->data, qnn_buffer_2, ggml_nbytes(dst));
                }
                TENSOR_DUMP(dst);
            }
        }
    }

    failure:
    qnn_backend.unregister_rpcmem();
    if (nullptr != qnn_buffer_0)
        qnn_backend.free_rpcmem(qnn_buffer_0);
    if (nullptr != qnn_buffer_1)
        qnn_backend.free_rpcmem(qnn_buffer_1);
    if (nullptr != qnn_buffer_2)
        qnn_backend.free_rpcmem(qnn_buffer_2);

    qnn_backend.qnn_finalize();

    ggml_free(ctx);

    perf.info();
    
    LOGGD("leave qnn_rpc_test\n");

    return result;
}
// ==================== verify/develop QNN NPU backend(RPC,....)==================================

extern int llama_inference_main(int argc, char *argv[], int backend);

static int llama_inference_ut(const char *sz_model_path, const char *sz_user_data,
                       int n_threads, int n_backend_type) {
    int ret = 0;
    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("num_threads:%d\n", n_threads);
    LOGGD("backend type:%d\n", n_backend_type);

    if (nullptr == sz_model_path || nullptr == sz_user_data) {
        LOGGD("pls check params\n");
        return 1;
    }
    //TODO: this is a lazy/dirty/quick method, just for merge latest source codes of llama.cpp from upstream quickly
    int argc = 7;
    const char *argv[] = {"llama-inference-main",
                          "-m", sz_model_path,
                          "-p", sz_user_data,
                          "-t", std::to_string(n_threads).c_str()
    };
    ret = llama_inference_main(argc, const_cast<char **>(argv), n_backend_type);

    return ret;
}


#ifndef TARGET_ANDROID
static void show_usage() {
    printf(" " \
        "\nUsage: test_qnn_ops [options]\n" \
        "\n" \
        "Options:\n" \
        " -t 0(simple UT) 1(automation UT) 2(whisper) 3(fuzz) 4(rpc)\n" \
        " -o ADD / MUL / MULMAT\n" \
        " -b 0(QNN_CPU) 1(QNN_GPU) 2(QNN_NPU) 3(ggml)\n" \
        " ?/h print usage infomation\n\n"
    );
}

int main(int argc, char *argv[]) {
    int num_threads = 4;
    int n_test_type = TEST_SIMPLE_UT;
    int n_backend_type = QNN_BACKEND_CPU;
    int n_ggml_op_type = GGML_OP_ADD;

    for (int i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], "-t")) { // test type
            if (i + 1 < argc) {
                int type = atoi(argv[i + 1]);
                if (type < UT_COUNTS)
                    n_test_type = type;
                i++;
            }
        } else if (0 == strcmp(argv[i], "-o")) { //GGML OPs
            if (i + 1 < argc) {
                if (0 == memcmp(argv[i + 1], "ADD", 3)) {
                    n_ggml_op_type = GGML_OP_ADD;
                } else if (0 == memcmp(argv[i + 1], "MULMAT", 6)) {
                    n_ggml_op_type = GGML_OP_MUL_MAT;
                } else if (0 == memcmp(argv[i + 1], "MUL", 3)) {
                    n_ggml_op_type = GGML_OP_MUL;
                } else {
                    show_usage();
                    return 1;
                }
                i++;
            }
        } else if (0 == strcmp(argv[i], "-b")) { //QNN backend
            if (i + 1 < argc) {
                int backend = atoi(argv[i + 1]);
                if (backend <= QNN_BACKEND_GGML)
                    n_backend_type = backend;
                else {
                    show_usage();
                    return 1;
                }
                i++;
            }
        } else {
            show_usage();
            return 1;
        }
    }

    QNN_LOG_DEBUG("type %d, backend %d, op %d\n", n_test_type, n_backend_type, n_ggml_op_type);
    switch (n_test_type) {
        case TEST_SIMPLE_UT:
            qnn_op_ut(num_threads, n_backend_type, n_ggml_op_type);
            break;

        case TEST_AUTOMATION_UT:
            qnn_op_ut_automation(num_threads, n_backend_type, n_ggml_op_type);
            break;

        case TEST_WHISPERCPP:
#if ENABLE_TEST_WHISPERCPP
            qnn_test_whispercpp("/sdcard/kantv/models/ggml-tiny.en-q8_0.bin",
                                "/sdcard/kantv/jfk.wav",
                                num_threads,
                                n_backend_type);
#else
            LOGGD("whispercpp UT disabled\n");
#endif
            break;
#if ENABLE_TEST_LLM_
        case TEST_LLM:
            llama_inference_ut("/sdcard/kantv/models/qwen1_5-1_8b-chat-q4_0.gguf",
                       "what's the population of China, less then 100 words",
                       num_threads,
                       n_backend_type);
#else
            LOGGD("LLM UT disabled\n");
#endif
        case TEST_QNN_FUZZ:
            qnn_fuzz(n_backend_type, n_ggml_op_type);
            break;
        case TEST_QNN_RPC:
            //qnn_test_rpc_1(n_backend_type, n_ggml_op_type);
            qnn_test_rpc_2(n_backend_type, n_ggml_op_type);
            break;
        default:
            break;
    }

    QNN_LOG_DEBUG("exit main\n");
    //FIXME:why report "Bus error" here after this Android command line program executed on Android phone?
    return 0;
}
#endif
