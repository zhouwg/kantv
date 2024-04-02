/*
 * MIT license
 * Copyright (C) 2024 KanTV Authors
 * SPDX-License-Identifier: MIT
 *
 * The most important two references are:
 *
 * (1) https://github.com/pytorch/executorch/tree/main/backends/qualcomm
 *
 * (2) /opt/qcom/aistack/qnn/2.20.0.240223/examples/Models/InceptionV3/model/Inception_v3.cpp
 *
 *     Inception_v3.cpp is generated automatically by Qualcomm's dedicated tool and it contains more then 20,000 lines C++ code
 *
 *
 * this implementation is for
 *
 * PoC#121:Add Qualcomm mobile SoC native backend for GGML(https://github.com/zhouwg/kantv/issues/121) in Project KanTV
 *
 * and will submit to upstream GGML community after this PoC is finished
 *
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

#include "ggml-qnn.h"

#include "ggml-jni.h"  //should be removed after finished this PoC for purpose of submit to upstream GGML community




// =================================================================================================
//
// self-defined structure / class / macro / const / pfn
//
// =================================================================================================
#define RPCMEM_DEFAULT_FLAGS                            1
#define RPCMEM_HEAP_ID_SYSTEM                           25
#define QNN_VER_PTR(x)                                  (&((x).v1))
#define TENSOR_DUMP(tensor)                             tensor_dump(tensor, #tensor)
#define QNN_OP_CFG_VALID(opConfig)                      ((opConfig).version == QNN_OPCONFIG_VERSION_1)

#define VALIDATE(value, status)                         \
  do {                                                  \
    status = value;                                     \
    if (status != QNN_SUCCESS) {                        \
      LOGGW("%s expected QNN_SUCCESS\n", #value);       \
      return status;                                    \
    }                                                   \
  } while (0)

#define VALIDATE_TENSOR_VERSION(tensor, err)            VALIDATE(validateTensorVersion(tensor), err)

#define VALIDATE_OP_CONFIG_VERSION(op, err)             VALIDATE(validateOpConfigVersion(op), err)


#define BINVARSTART(NAME)                                            \
  ({                                                                 \
    extern const uint8_t _binary_obj_binary_##NAME##_raw_start[];    \
    (void *)_binary_obj_binary_##NAME##_raw_start;                   \
  })


#define BINVAREND(NAME)                                              \
  ({                                                                 \
    extern const uint8_t _binary_obj_binary_##NAME##_raw_end[];      \
    (void *)_binary_obj_binary_##NAME##_raw_end;                     \
  })

#define BINLEN(NAME)                                                                             \
  ({                                                                                             \
    extern const uint8_t _binary_obj_binary_##NAME##_raw_start[];                                \
    extern const uint8_t _binary_obj_binary_##NAME##_raw_end[];                                  \
    (uint32_t)((_binary_obj_binary_##NAME##_raw_end) - (_binary_obj_binary_##NAME##_raw_start)); \
  })


#define FREE_MEMORY(ptr1, ptr2, ptr3) \
  do {                                \
    free(ptr1);                       \
    free(ptr2);                       \
    free(ptr3);                       \
  } while (0)


enum class ggml_qnn_profile_level {
    profile_off         = 0,
    profile_basic       = 1,
    profile_detail      = 2
};


using pfn_rpc_mem_alloc                     = void * (*)(int, uint32_t, int);
using pfn_rpc_mem_free                      = void (*)(void *);
using pfn_rpc_mem_to_fd                     = int (*)(void *);

using _pfn_QnnSaver_initialize              = decltype(QnnSaver_initialize);
using _pfn_QnnInterface_getProviders        = decltype(QnnInterface_getProviders);
using _pfn_QnnSystemInterface_getProviders  = decltype(QnnSystemInterface_getProviders);


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


inline const char* getQnnOpConfigName(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.name;
    }
    return NULL;
}


inline const char* getQnnOpConfigName(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigName(*opConfig);
}


inline const char* getQnnOpConfigPackageName(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.packageName;
    }
    return NULL;
}


inline const char* getQnnOpConfigPackageName(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigPackageName(*opConfig);
}

inline const char* getQnnOpConfigTypeName(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.typeName;
    }
    return NULL;
}


inline const char* getQnnOpConfigTypeName(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigTypeName(*opConfig);
}

inline uint32_t getQnnOpConfigNumParams(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.numOfParams;
    }
    return 0u;
}


inline uint32_t getQnnOpConfigNumParams(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigNumParams(*opConfig);
}


inline const Qnn_Param_t* getQnnOpConfigParams(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.params;
    }
    return NULL;
}


inline const Qnn_Param_t* getQnnOpConfigParams(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigParams(*opConfig);
}


inline uint32_t getQnnOpConfigNumInputs(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.numOfInputs;
    }
    return 0u;
}


inline uint32_t getQnnOpConfigNumInputs(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigNumInputs(*opConfig);
}


inline const Qnn_Tensor_t* getQnnOpConfigInputs(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.inputTensors;
    }
    return NULL;
}

inline const Qnn_Tensor_t* getQnnOpConfigInputs(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigInputs(*opConfig);
}


inline uint32_t getQnnOpConfigNumOutputs(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.numOfOutputs;
    }
    return 0u;
}


inline uint32_t getQnnOpConfigNumOutputs(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigNumOutputs(*opConfig);
}


inline const Qnn_Tensor_t* getQnnOpConfigOutputs(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.outputTensors;
    }
    return NULL;
}


inline const Qnn_Tensor_t* getQnnOpConfigOutputs(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigOutputs(*opConfig);
}

inline void setQnnOpConfigName(Qnn_OpConfig_t& opConfig, const char* name) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.name = name;
    }
}


inline void setQnnOpConfigName(Qnn_OpConfig_t* opConfig, const char* name) {
    setQnnOpConfigName(*opConfig, name);
}

inline void setQnnOpConfigPackageName(Qnn_OpConfig_t& opConfig, const char* packageName) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.packageName = packageName;
    }
}


inline void setQnnOpConfigPackageName(Qnn_OpConfig_t* opConfig, const char* packageName) {
    setQnnOpConfigPackageName(*opConfig, packageName);
}


inline void setQnnOpConfigTypeName(Qnn_OpConfig_t& opConfig, const char* typeName) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.typeName = typeName;
    }
}


inline void setQnnOpConfigTypeName(Qnn_OpConfig_t* opConfig, const char* typeName) {
    setQnnOpConfigTypeName(*opConfig, typeName);
}


inline void setQnnOpConfigParams(Qnn_OpConfig_t& opConfig,
                                 uint32_t numOfParams,
                                 Qnn_Param_t* params) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.numOfParams = numOfParams;
        opConfig.v1.params      = params;
    }
}


inline void setQnnOpConfigParams(Qnn_OpConfig_t* opConfig,
                                 uint32_t numOfParams,
                                 Qnn_Param_t* params) {
    setQnnOpConfigParams(*opConfig, numOfParams, params);
}


inline void setQnnOpConfigInputs(Qnn_OpConfig_t& opConfig,
                                 uint32_t numOfInputs,
                                 Qnn_Tensor_t* inputTensors) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.numOfInputs  = numOfInputs;
        opConfig.v1.inputTensors = inputTensors;
    }
}


inline void setQnnOpConfigInputs(Qnn_OpConfig_t* opConfig,
                                 uint32_t numOfInputs,
                                 Qnn_Tensor_t* inputTensors) {
    setQnnOpConfigInputs(*opConfig, numOfInputs, inputTensors);
}


inline void setQnnOpConfigOutputs(Qnn_OpConfig_t& opConfig,
                                  uint32_t numOfOutputs,
                                  Qnn_Tensor_t* outputTensors) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.numOfOutputs  = numOfOutputs;
        opConfig.v1.outputTensors = outputTensors;
    }
}


inline void setQnnOpConfigOutputs(Qnn_OpConfig_t* opConfig,
                                  uint32_t numOfOutputs,
                                  Qnn_Tensor_t* outputTensors) {
    setQnnOpConfigOutputs(*opConfig, numOfOutputs, outputTensors);
}

inline uint32_t getQnnTensorId(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.id;
    }
    return 0u;
}


inline uint32_t getQnnTensorId(const Qnn_Tensor_t* tensor) { return getQnnTensorId(*tensor); }

inline const char* getQnnTensorName(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.name;
    }
    return 0u;
}


inline const char* getQnnTensorName(const Qnn_Tensor_t* tensor) {
    return getQnnTensorName(*tensor);
}

inline Qnn_TensorType_t getQnnTensorType(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.type;
    }
    return QNN_TENSOR_TYPE_UNDEFINED;
}

inline Qnn_TensorType_t getQnnTensorType(const Qnn_Tensor_t* tensor) {
    return getQnnTensorType(*tensor);
}


inline Qnn_TensorDataFormat_t getQnnTensorDataFormat(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.dataFormat;
    }
    return QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER;
}


inline Qnn_TensorDataFormat_t getQnnTensorDataFormat(const Qnn_Tensor_t* tensor) {
    return getQnnTensorDataFormat(*tensor);
}

inline Qnn_DataType_t getQnnTensorDataType(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.dataType;
    }
    return QNN_DATATYPE_UNDEFINED;
}

inline Qnn_DataType_t getQnnTensorDataType(const Qnn_Tensor_t* tensor) {
    return getQnnTensorDataType(*tensor);
}

inline Qnn_QuantizeParams_t getQnnTensorQuantParams(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.quantizeParams;
    }
    return QNN_QUANTIZE_PARAMS_INIT;
}

inline Qnn_QuantizeParams_t getQnnTensorQuantParams(const Qnn_Tensor_t* tensor) {
    return getQnnTensorQuantParams(*tensor);
}

inline uint32_t getQnnTensorRank(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.rank;
    }
    return 0u;
}

inline uint32_t getQnnTensorRank(const Qnn_Tensor_t* tensor) { return getQnnTensorRank(*tensor); }


inline uint32_t* getQnnTensorDimensions(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.dimensions;
    }
    return NULL;
}


inline uint32_t* getQnnTensorDimensions(const Qnn_Tensor_t* tensor) {
    return getQnnTensorDimensions(*tensor);
}


inline Qnn_TensorMemType_t getQnnTensorMemType(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.memType;
    }
    return QNN_TENSORMEMTYPE_UNDEFINED;
}


inline Qnn_TensorMemType_t getQnnTensorMemType(const Qnn_Tensor_t* tensor) {
    return getQnnTensorMemType(*tensor);
}

inline Qnn_ClientBuffer_t getQnnTensorClientBuf(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.clientBuf;
    }
    return QNN_CLIENT_BUFFER_INIT;
}

inline Qnn_ClientBuffer_t getQnnTensorClientBuf(const Qnn_Tensor_t* tensor) {
    return getQnnTensorClientBuf(*tensor);
}


inline Qnn_MemHandle_t getQnnTensorMemHandle(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.memHandle;
    }
    return NULL;
}


inline Qnn_MemHandle_t getQnnTensorMemHandle(const Qnn_Tensor_t* tensor) {
    return getQnnTensorMemHandle(*tensor);
}


inline void setQnnTensorId(Qnn_Tensor_t& tensor, uint32_t id) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.id = id;
    }
}


inline void setQnnTensorId(Qnn_Tensor_t* tensor, uint32_t id) { setQnnTensorId(*tensor, id); }

inline void setQnnTensorName(Qnn_Tensor_t& tensor, const char* name) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.name = name;
    }
}


inline void setQnnTensorName(Qnn_Tensor_t* tensor, const char* name) {
    setQnnTensorName(*tensor, name);
}


inline void setQnnTensorType(Qnn_Tensor_t& tensor, Qnn_TensorType_t type) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.type = type;
    }
}


inline void setQnnTensorType(Qnn_Tensor_t* tensor, Qnn_TensorType_t type) {
    setQnnTensorType(*tensor, type);
}


inline void setQnnTensorDataFormat(Qnn_Tensor_t& tensor, Qnn_TensorDataFormat_t format) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.dataFormat = format;
    }
}


inline void setQnnTensorDataFormat(Qnn_Tensor_t* tensor, Qnn_TensorDataFormat_t format) {
    setQnnTensorDataFormat(*tensor, format);
}


inline void setQnnTensorDataType(Qnn_Tensor_t& tensor, Qnn_DataType_t dataType) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.dataType = dataType;
    }
}


inline void setQnnTensorDataType(Qnn_Tensor_t* tensor, Qnn_DataType_t dataType) {
    setQnnTensorDataType(*tensor, dataType);
}


inline void setQnnTensorQuantParams(Qnn_Tensor_t& tensor, Qnn_QuantizeParams_t params) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.quantizeParams = params;
    }
}


inline void setQnnTensorQuantParams(Qnn_Tensor_t* tensor, Qnn_QuantizeParams_t params) {
    setQnnTensorQuantParams(*tensor, params);
}


inline void setQnnTensorRank(Qnn_Tensor_t& tensor, uint32_t rank) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.rank = rank;
    }
}


inline void setQnnTensorRank(Qnn_Tensor_t* tensor, uint32_t rank) {
    setQnnTensorRank(*tensor, rank);
}


inline void setQnnTensorDimensions(Qnn_Tensor_t& tensor, uint32_t* dims) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.dimensions = dims;
    }
}


inline void setQnnTensorDimensions(Qnn_Tensor_t* tensor, uint32_t* dims) {
    setQnnTensorDimensions(*tensor, dims);
}


inline void setQnnTensorMemType(Qnn_Tensor_t& tensor, Qnn_TensorMemType_t memType) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.memType = memType;
    }
}


inline void setQnnTensorMemType(Qnn_Tensor_t* tensor, Qnn_TensorMemType_t memType) {
    setQnnTensorMemType(*tensor, memType);
}


inline void setQnnTensorClientBuf(Qnn_Tensor_t& tensor, Qnn_ClientBuffer_t clientBuf) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.clientBuf = clientBuf;
    }
}


inline void setQnnTensorClientBuf(Qnn_Tensor_t* tensor, Qnn_ClientBuffer_t clientBuf) {
    setQnnTensorClientBuf(*tensor, clientBuf);
}


inline void setQnnTensorMemHandle(Qnn_Tensor_t& tensor, Qnn_MemHandle_t handle) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.memHandle = handle;
    }
}


inline void setQnnTensorMemHandle(Qnn_Tensor_t* tensor, Qnn_MemHandle_t handle) {
    setQnnTensorMemHandle(*tensor, handle);
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



// =================================================================================================
//
//  internal helper function
//
// =================================================================================================
static size_t memscpy(void *dst, size_t dstSize, const void *src, size_t copySize) {
    if (!dst || !src || !dstSize || !copySize) return 0;

    size_t minSize = dstSize < copySize ? dstSize : copySize;

    memcpy(dst, src, minSize);

    return minSize;
}


static char *ggml_qnn_strndup(const char *source, size_t maxlen) { return ::strndup(source, maxlen); }

static int deepCopyQnnTensors(Qnn_Tensor_t &src, Qnn_Tensor_t &dst) {
    int err = 0;
    VALIDATE_TENSOR_VERSION(src, err);

    dst.version = src.version;
    QNN_TENSOR_SET_NAME(
            dst, ggml_qnn_strndup(QNN_TENSOR_GET_NAME(src), std::string(QNN_TENSOR_GET_NAME(src)).size()));
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

    Qnn_QuantizeParams_t srcQParam      = QNN_TENSOR_GET_QUANT_PARAMS(src);
    Qnn_QuantizationEncoding_t encoding = srcQParam.quantizationEncoding;
    if (encoding == QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET) {
        // need to allocate and copy memory for scaleOffset as it is a pointer array
        Qnn_QuantizeParams_t srcQParamCpy      = srcQParam;
        Qnn_AxisScaleOffset_t &axisScaleOffset = srcQParamCpy.axisScaleOffsetEncoding;
        Qnn_ScaleOffset_t **scaleOffset        = &axisScaleOffset.scaleOffset;
        size_t scaleOffsetSize = axisScaleOffset.numScaleOffsets * sizeof(Qnn_ScaleOffset_t);
        *scaleOffset           = (Qnn_ScaleOffset_t *)malloc(scaleOffsetSize);
        memscpy(*scaleOffset,
                scaleOffsetSize,
                srcQParam.axisScaleOffsetEncoding.scaleOffset,
                scaleOffsetSize);
        QNN_TENSOR_SET_QUANT_PARAMS(dst, srcQParamCpy);
    } else if (encoding == QNN_QUANTIZATION_ENCODING_BW_AXIS_SCALE_OFFSET) {
        // need to allocate and copy memory for scaleOffset as it is a pointer array
        Qnn_QuantizeParams_t srcQParamCpy          = srcQParam;
        Qnn_BwAxisScaleOffset_t &bwAxisScaleOffset = srcQParamCpy.bwAxisScaleOffsetEncoding;
        size_t scaleSize                           = bwAxisScaleOffset.numElements * sizeof(float);
        float **scales                             = &bwAxisScaleOffset.scales;
        int32_t **offsets                          = &bwAxisScaleOffset.offsets;
        *scales                                    = (float *)malloc(scaleSize);
        memscpy(*scales, scaleSize, srcQParam.bwAxisScaleOffsetEncoding.scales, scaleSize);

        // Only copy offsets if present, nullptr implies all offsets are 0
        if (bwAxisScaleOffset.offsets != nullptr) {
            size_t offsetSize = bwAxisScaleOffset.numElements * sizeof(int32_t);
            *offsets          = (int32_t *)malloc(offsetSize);
            memscpy(*offsets, offsetSize, srcQParam.bwAxisScaleOffsetEncoding.offsets, offsetSize);
        }
        QNN_TENSOR_SET_QUANT_PARAMS(dst, srcQParamCpy);
    } else {
        QNN_TENSOR_SET_QUANT_PARAMS(dst, srcQParam);
    }

    // need to allocate and copy memory for all the pointer members
    uint32_t rank = QNN_TENSOR_GET_RANK(src);
    QNN_TENSOR_SET_RANK(dst, rank);
    size_t dimSize       = rank * sizeof(uint32_t);
    uint32_t *dimensions = (uint32_t *)malloc(dimSize);
    if (dimensions == nullptr) {
        LOGGW("deepCopyQnnTensors() allocation error while copying tensor %s\n", QNN_TENSOR_GET_NAME(src));
        return 1;
    }
    memscpy(dimensions, dimSize, QNN_TENSOR_GET_DIMENSIONS(src), dimSize);
    QNN_TENSOR_SET_DIMENSIONS(dst, dimensions);

    return err;
}

static int freeQnnTensor(Qnn_Tensor_t &tensor) {
    int err = 0;
    VALIDATE_TENSOR_VERSION(tensor, err);

    free((void *)QNN_TENSOR_GET_NAME(tensor));
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



static intptr_t alignTo(size_t alignment, intptr_t offset) {
    return offset % alignment == 0 ? offset
                                   : offset +
                                     (static_cast<intptr_t>(alignment) -
                                      offset % static_cast<intptr_t>(alignment));
}


static void ggml_graph_compute_helper(std::vector<uint8_t> & buf, ggml_cgraph * graph, int n_threads) {
    struct ggml_cplan plan = ggml_graph_plan(graph, n_threads);

    if (plan.work_size > 0) {
        buf.resize(plan.work_size);
        plan.work_data = buf.data();
    }

    ggml_graph_compute(graph, &plan);
}


static float tensor_sum_elements(const ggml_tensor * tensor) {
    double sum = 0;
    if (tensor->type == GGML_TYPE_F32) {
        for (int j = 0; j < tensor->ne[1]; j++) {
            for (int k = 0; k < tensor->ne[0]; k++) {
                sum += ((float *) tensor->data)[j*tensor->ne[0] + k];
                LOGGD("[%d][%d]%.2f \t", j, k, ((float*)tensor->data)[j*tensor->ne[0] + k]);
            }
            LOGGD("\n");
        }
    }
    LOGGD("\n");
    return sum;
}


static void tensor_dump(const ggml_tensor * tensor, const char * name) {
    LOGGD("dump tensor %s\n", name);
    LOGGD("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi) - \n", name,
            tensor->type, ggml_type_name(tensor->type),
            tensor->ne[0], tensor->ne[1], tensor->ne[2], tensor->nb[0], tensor->nb[1], tensor->nb[2]);
    float sum = tensor_sum_elements(tensor);

    LOGGD("\n");
    LOGGD("Sum of tensor %s is %6.2f\n", name, sum);
}


template<typename Fn>
        Fn load_qnn_functionpointers(void * handle, const char * function_name) {
    return reinterpret_cast<Fn>(dlsym(handle, function_name));
}


static void ggml_qnn_create_directory(const std::string& path) {
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

    friend class qnn_implementation;

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
    const QnnInterface_t * _qnn_interface           = nullptr;

    const QnnSystemInterface_t * _qnn_sys_interface = nullptr;
};


class qnn_implementation {
public:
    using BackendIdType = decltype(QnnInterface_t{}.backendId);

    explicit qnn_implementation(const std::string & lib_path) : _lib_path(std::move(lib_path)) {};

    ~qnn_implementation() {
        //TODO:
        _input_tensors.clear();
        _output_tensors.clear();
    }

    int qnn_init(const QnnSaver_Config_t ** saver_config);

    int qnn_finalize();

    const qnn_interface & get_qnn_interface() const;

    const Qnn_LogHandle_t get_qnn_log_handle() { return _qnn_log_handle; }

    const Qnn_ProfileHandle_t get_qnn_profile_handle() { return _qnn_profile_handle; }

    const Qnn_DeviceHandle_t get_qnn_device_handle() { return _qnn_device_handle; }

    const Qnn_BackendHandle_t get_qnn_backend_handle() { return _qnn_backend_handle; }

    const Qnn_ContextHandle_t get_qnn_context_handle() { return _qnn_context_handle; }

    const QnnSystemContext_Handle_t get_qnn_system_handle() { return _qnn_system_handle; }

    const Qnn_GraphHandle_t get_qnn_graph_handle() { return _qnn_graph_handle; }


    int add_tensor(const char * node_name, Qnn_Tensor_t * p_tensor, bool b_save_tensor = true);

    int add_tensor(const char *node_name, Qnn_Tensor_t tensor, bool b_save_tensor = true) {
        return add_tensor(node_name, &tensor, b_save_tensor);
    }

    int get_tensor(const char *& node_name, const char *& tensor_name, Qnn_Tensor_t & tensor) {
        std::string map_entry = std::string(tensor_name);
        if (_tensors_map.find(tensor_name) == _tensors_map.end()) {
            LOGGW("tensor %s not found on node %s\n", map_entry.c_str(), node_name);
            return 1;
        }

        tensor = _tensors_map[map_entry];
        return 0;
    }

    int add_node(Qnn_OpConfigVersion_t version,
                 const char * name,
                 const char * packageName,
                 const char * type,
                 Qnn_Param_t * params,
                 uint32_t numOfParams,
                 const char ** inputNames,
                 uint32_t numOfInputs,
                 Qnn_Tensor_t * outputTensors,
                 uint32_t numOfOutputs);

    int add_node(const Qnn_OpConfig_t & op_config) {
        return _qnn_interface.qnn_graph_add_node(_qnn_graph_handle, op_config);
    };


    int execute_graph(
            const std::vector<Qnn_Tensor_t>& input_tensor_structs,
            std::vector<Qnn_Tensor_t>& output_tensor_structs) {
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
                const Qnn_Tensor_t& output_tensor = _output_tensors[out_idx];

                std::string output_path =
                        dir + QNN_VER_PTR(output_tensor)->name + "_tensor.raw";

                std::ofstream fout(output_path, std::ios::binary);
                if (fout.fail()) {
                    LOGGW("dump tensor name: %s failure\n", QNN_VER_PTR(output_tensor)->name);
                    return 1;
                }
                fout.write(static_cast<const char*>(QNN_VER_PTR(output_tensor)->clientBuf.data),
                        QNN_VER_PTR(output_tensor)->clientBuf.dataSize);
            }
        }

        return result;
    }

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

    int32_t rpcmem_to_fd(void * buf);

    int  register_rpcmem(void * p_data, Qnn_Tensor_t * p_tensor);

    void unregister_rpcmem();

    void * alloc_rpcmem(size_t bytes, size_t alignment);

    void free_rpcmem(void * buf);

    bool is_rpcmem_allocated(void * buf);

    bool is_rpcmem_registered(Qnn_MemHandle_t handle) {
        return _qnn_mem_set.count(handle) != 0U;
    }

private:
    int load_system(const std::string & system_lib_path);
    int unload_system();

    int free_cached_tensors() {
        int err = 0;

        for (std::map<std::string, Qnn_Tensor_t>::iterator tensorIt = _tensors_map.begin();
             tensorIt != _tensors_map.end();) {
            Qnn_Tensor_t &tensor = tensorIt->second;
            if (QNN_TENSOR_GET_TYPE(tensor) != QNN_TENSOR_TYPE_APP_WRITE &&
                QNN_TENSOR_GET_TYPE(tensor) != QNN_TENSOR_TYPE_APP_READ) {
                VALIDATE(freeQnnTensor(tensor), err);
                tensorIt = _tensors_map.erase(tensorIt);
            } else {
                tensorIt++;
            }
        }

        return err;
    }


private:
    static constexpr const int _required_num_providers = 1;

    static int load_backend(
            const std::string &lib_path,
            const QnnSaver_Config_t ** saver_config);

    static int init_backend(
            void * const lib_handle,
            const QnnSaver_Config_t ** saver_config);

private:
    std::string _lib_path;

    bool _debug_tensor                              = false; // flag to indicate if requested graph is to be run in debug mode
    bool _do_node_validations                       = true;  // flag to indicate whether all add_node calls need to be validated
    QnnLog_Level_t _qnn_log_level                   = QNN_LOG_LEVEL_DEBUG;

    ggml_qnn_profile_level _profile_level           = ggml_qnn_profile_level::profile_detail;

    qnn_interface _qnn_interface;

    void * _system_lib_handle                       = nullptr;

    Qnn_GraphHandle_t _qnn_graph_handle             = nullptr;

    Qnn_LogHandle_t _qnn_log_handle                 = nullptr;

    Qnn_ProfileHandle_t _qnn_profile_handle         = nullptr;

    Qnn_DeviceHandle_t _qnn_device_handle           = nullptr;

    Qnn_BackendHandle_t _qnn_backend_handle         = nullptr;

    Qnn_ContextHandle_t _qnn_context_handle         = nullptr;

    QnnSystemContext_Handle_t _qnn_system_handle    = nullptr;

    std::unordered_set<Qnn_MemHandle_t> _qnn_mem_set;

    std::vector<Qnn_Tensor_t> _input_tensors;
    std::vector<Qnn_Tensor_t> _output_tensors;
    std::map<std::string, Qnn_Tensor_t> _tensors_map;
    std::map<std::string, std::vector<std::string>> _output_tensor_map;

    static std::mutex _init_mutex;
    static std::unordered_map<BackendIdType, void *> _loaded_lib_handle;
    static std::unordered_map<std::string, BackendIdType> _lib_path_to_backend_id;
    static std::unordered_map<BackendIdType, const QnnInterface_t *> _loaded_backend;

    void * _rpc_lib_handle                          = nullptr;
    std::atomic_bool _rpcmem_initialized            { false };
    pfn_rpc_mem_alloc _pfn_rpc_mem_alloc;
    pfn_rpc_mem_free  _pfn_rpc_mem_free;
    pfn_rpc_mem_to_fd _pfn_rpc_mem_to_fd;
    std::unordered_map<void *, void *> _rpcmem_store_map;
};


std::mutex qnn_implementation::_init_mutex;

std::unordered_map<qnn_implementation::BackendIdType, void *> qnn_implementation::_loaded_lib_handle;

std::unordered_map<std::string, qnn_implementation::BackendIdType> qnn_implementation::_lib_path_to_backend_id;

std::unordered_map<qnn_implementation::BackendIdType, const QnnInterface_t *> qnn_implementation::_loaded_backend;


void * qnn_implementation::alloc_rpcmem(size_t bytes, size_t alignment) {
    if (!_rpcmem_initialized) {
        LOGGW("rpc memory not initialized\n");
        return nullptr;
    }

    auto allocate_bytes = static_cast<int32_t>(bytes + alignment);
    void* buf = _pfn_rpc_mem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, allocate_bytes);
    if (buf == nullptr) {
        LOGGW("failed to allocate rpc memory\n");
        return nullptr;
    }

    auto aligned_buf = reinterpret_cast<void*>(alignTo(alignment, reinterpret_cast<intptr_t>(buf)));
    bool status = _rpcmem_store_map.insert(std::pair<void*, void*>(aligned_buf, buf)).second;
    if (!status) {
        LOGGW("failed to allocate rpc memory\n");
        _pfn_rpc_mem_free(buf);
    }

    return aligned_buf;
}


void qnn_implementation::free_rpcmem(void * buf) {
    if (!_rpcmem_initialized) {
        LOGGW("rpc memory not initialized\n");
    } else if (0 == _rpcmem_store_map.count(buf)) {
        LOGGW("no allocated tensor\n");
    } else {
        _pfn_rpc_mem_free(_rpcmem_store_map[buf]);
        _rpcmem_store_map.erase(buf);
    }
}


int32_t qnn_implementation::rpcmem_to_fd(void * buf) {
    int32_t mem_fd = -1;
    if (!_rpcmem_initialized) {
        LOGGW("rpc memory not initialized\n");
    } else {
        mem_fd = _pfn_rpc_mem_to_fd(buf);
    }

    return mem_fd;
}


int qnn_implementation::register_rpcmem(void * p_data, Qnn_Tensor_t * p_tensor) {
    if (nullptr == p_data || (nullptr == p_tensor)) {
        LOGGW("invalid param\n");
        return 1;
    }

    //if (is_rpcmem_allocated(p_data)) {
        //LOGGW("rpc memory already allocated\n");
        //return 2;
    //}
    if (is_rpcmem_registered((QNN_VER_PTR(*p_tensor)->memHandle))) {
        LOGGW("tensor %s has been registered shared memory\n", (QNN_VER_PTR(*p_tensor)->name));
        return 3;
    }

    int32_t mem_fd = rpcmem_to_fd(p_data);
    if (-1 == mem_fd) {
        LOGGW("failed to get file descriptor\n");
        return 4;
    }
    LOGGD("mem_fd %d\n", mem_fd);
    Qnn_MemDescriptor_t descriptor = {
            {QNN_VER_PTR(*p_tensor)->rank, QNN_VER_PTR(*p_tensor)->dimensions, nullptr},
            QNN_VER_PTR(*p_tensor)->dataType,
            QNN_MEM_TYPE_ION,
            {{mem_fd}}};
    Qnn_MemHandle_t handle = nullptr;
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    error = _qnn_interface.qnn_mem_register(
            _qnn_context_handle,
            &descriptor,
            /*numDescriptors=*/1,
            &handle);
    if (error != QNN_SUCCESS) {
        LOGGW("failed to register shared memory, error %d, %s\n", QNN_GET_ERROR_CODE(error), strerror(error));
        return 1;
    } else {
        LOGGI("tensor %s successfully register shared memory\n", (QNN_VER_PTR(*p_tensor)->name));
    }
    QNN_VER_PTR(*p_tensor)->memHandle = handle;
    _qnn_mem_set.insert(handle);

    return 0;
}


void qnn_implementation::unregister_rpcmem() {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;

    if (_qnn_mem_set.empty()) {
        LOGGW("no rpcmem registered\n");
    }

    for (auto& mem_handle : _qnn_mem_set) {
        error = _qnn_interface.qnn_mem_de_register(&mem_handle, 1);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to unregister shared memory, error %d\n", QNN_GET_ERROR_CODE(error));
        }
    }
    _qnn_mem_set.clear();
}


bool qnn_implementation::is_rpcmem_allocated(void * buf) {
    return _rpcmem_store_map.count(buf) != 0U;
}


int qnn_implementation::init_backend(void * const lib_handle, const QnnSaver_Config_t ** saver_config) {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;

    // saver_config must be set before backend initialization
    auto saver_initialize = load_qnn_functionpointers<_pfn_QnnSaver_initialize *>(lib_handle, "QnnSaver_initialize");
    if (nullptr != saver_initialize) {
        error = saver_initialize(saver_config);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to saver_initialize，error %d", QNN_GET_ERROR_CODE(error));
            return 1;
        }
    }

    return 0;
}


int qnn_implementation::load_backend(const std::string & lib_path, const QnnSaver_Config_t ** saver_config) {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    void *lib_handle = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_GLOBAL);

    if (nullptr == lib_handle) {
        LOGGW("can not open QNN library %s, with error: %s", lib_path.c_str(), dlerror());
        return 1;
    }

    // load get_provider function
    auto get_providers = load_qnn_functionpointers<_pfn_QnnInterface_getProviders *>(lib_handle, "QnnInterface_getProviders");
    if (nullptr == get_providers) {
        LOGGW("can not load symbol QnnInterface_getProviders : %s", dlerror());
        return 2;
    }

    // get QnnInterface Providers
    std::uint32_t num_providers = 0;
    const QnnInterface_t ** provider_list = nullptr;
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

    int is_init_ok = init_backend(_loaded_lib_handle[backend_id], saver_config);
    if (is_init_ok != 0) {
        // backend init fails. clear things
        _lib_path_to_backend_id.erase(lib_path);
        _loaded_backend.erase(backend_id);

        int dlclose_error = dlclose(_loaded_lib_handle[backend_id]);
        if (dlclose_error != 0) {
            LOGGW("fail to close %p after backend-init failure, with error %s",
                    _loaded_lib_handle[backend_id], dlerror());
        }

        _loaded_lib_handle.erase(backend_id);
        return is_init_ok;
    }

    return 0;
}


int qnn_implementation::load_system(const std::string & system_lib_path) {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;

    _system_lib_handle = dlopen(system_lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (nullptr == _system_lib_handle) {
        LOGGW("can not pen QNN library %s, error: %s\n", system_lib_path.c_str(), dlerror());
        return 1;
    }

    auto * get_providers = reinterpret_cast<_pfn_QnnSystemInterface_getProviders *>(dlsym(_system_lib_handle, "QnnSystemInterface_getProviders"));
    if (nullptr == get_providers) {
        LOGGW("can not load QNN symbol QnnSystemInterface_getProviders: %s\n", dlerror());
        return 2;
    }

    std::uint32_t num_providers;
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

    _qnn_interface.set_qnn_system_interface(provider_list[0]);

    _qnn_interface.qnn_system_context_create(&_qnn_system_handle);
    if (nullptr == _qnn_system_handle) {
        LOGW("can not create QNN system contenxt\n");
    } else {
        LOGGD("initialize qnn system successfully\n");
    }

    return 0;
}


int qnn_implementation::unload_system() {
    int result = 0;
    if (nullptr == _system_lib_handle)
        return 1;

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

    return 0;
}


static void ggml_qnn_logcallback(const char* fmt,
                                        QnnLog_Level_t level,
                                        uint64_t timestamp,
                                        va_list argp) {

    //don't care static variable in PoC stage
    static std::mutex _log_mutex;
    static unsigned char s_qnn_jni_buf[JNI_BUF_LEN];

    const char * levelStr = "";
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

    double ms = (double)timestamp / 1000000.0;

    {
        std::lock_guard<std::mutex> lock(_log_mutex);

        int len_content = 0;
        memset(s_qnn_jni_buf, 0, JNI_BUF_LEN);
        len_content = vsnprintf(reinterpret_cast<char *const>(s_qnn_jni_buf), JNI_BUF_LEN, fmt, argp);
        snprintf(reinterpret_cast<char *const>(s_qnn_jni_buf + len_content), JNI_BUF_LEN - len_content, "\n");
        LOGGD("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_jni_buf);
        //if (level <= QNN_LOG_LEVEL_INFO)
        {
            GGML_JNI_NOTIFY("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_jni_buf);
        }
    }
}


//TODO:error handle & memory leak handle
int qnn_implementation::qnn_init(const QnnSaver_Config_t ** saver_config) {
    BackendIdType backend_id = QNN_BACKEND_ID_NULL;
    LOGGD("enter qni_init\n");

    const std::lock_guard<std::mutex> lock(_init_mutex);

    if (0 != load_system("/data/data/com.cdeos.kantv/libQnnSystem.so")) { //TODO:hardcode
        LOGGW("can not load QNN system lib, pls check why?\n");
        return 1;
    } else {
        LOGGD("load QNN system lib succesfully\n");
    }

    if (0 == _lib_path_to_backend_id.count(_lib_path)) {
        int is_load_ok = load_backend(_lib_path, saver_config);
        if (0 != is_load_ok) {
            LOGGW("failed to load QNN backend\n");
            return 2;
        }
    }

    backend_id = _lib_path_to_backend_id[_lib_path];
    if (0 == _loaded_backend.count(backend_id) ||
        0 == _loaded_lib_handle.count(backend_id)) {
        LOGGW("library %s is loaded but loaded backend count=%zu, loaded lib_handle count=%zu\n",
              _lib_path.c_str(),
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

    std::vector<const QnnBackend_Config_t*> temp_backend_config; //TODO:now is empty because I don't know how to use QnnBackend_Config_t currently
    _qnn_interface.qnn_backend_create(_qnn_log_handle, temp_backend_config.empty() ? nullptr : temp_backend_config.data(), &_qnn_backend_handle);
    if (nullptr == _qnn_backend_handle) {
        LOGGW("why failed to initialize qnn backend\n");
        return 5;
    } else {
        LOGGD("initialize qnn backend successfully\n");
    }

    std::vector<const QnnDevice_Config_t*> temp_device_config;  //TODO:now is empty because I don't know how to use QnnDevice_Config_t currently
    _qnn_interface.qnn_device_create(_qnn_log_handle, temp_device_config.empty() ? nullptr : temp_device_config.data(), &_qnn_device_handle);
    if (nullptr == _qnn_device_handle) {
        LOGGW("why failed to initialize qnn device\n");
        //return 6;
    }

    _qnn_interface.qnn_profile_create(_qnn_backend_handle, static_cast<int>(_profile_level), &_qnn_profile_handle);
    if (nullptr == _qnn_profile_handle) {
        LOGGW("why failed to initialize qnn profile\n");
        return 7;
    } else {
        LOGGD("initialize qnn profile successfully\n");
    }

    std::vector<const QnnContext_Config_t*> temp_context_config; //TODO:now is empty because I don't know how to use QnnContext_Config_t currently
    _qnn_interface.qnn_context_create(_qnn_backend_handle, _qnn_device_handle, temp_context_config.empty() ? nullptr : temp_context_config.data(), &_qnn_context_handle);
    if (nullptr == _qnn_context_handle) {
        LOGGW("why failed to initialize qnn context\n");
        return 8;
    } else {
        LOGGD("initialize qnn context successfully\n");
    }

    std::vector<const QnnGraph_Config_t*> temp_graph_config; //TODO:now is empty because I don't know how to use QnnGraph_Config_t currently
    //harcode graph name to "qnn_matrix"
    _qnn_interface.qnn_graph_create(_qnn_context_handle, "qnn_matrix", temp_graph_config.empty() ? nullptr : temp_graph_config.data(), &_qnn_graph_handle);
    if (nullptr == _qnn_graph_handle) {
        LOGGW("why failed to initialize qnn graph\n");
        return 9;
    }

    _rpc_lib_handle = dlopen("libcdsprpc.so", RTLD_NOW | RTLD_LOCAL);
    if (nullptr == _rpc_lib_handle) {
        LOGGW("failed to load qualcomm's rpc lib, error:%s\n", dlerror());
        return 10;
    } else {
        LOGGD("load rpcmem lib successfully\n");
        set_rpcmem_initialized(true);
    }
    _pfn_rpc_mem_alloc = reinterpret_cast<pfn_rpc_mem_alloc>(dlsym(_rpc_lib_handle, "rpcmem_alloc"));
    _pfn_rpc_mem_free = reinterpret_cast<pfn_rpc_mem_free>(dlsym(_rpc_lib_handle, "rpcmem_free"));
    _pfn_rpc_mem_to_fd = reinterpret_cast<pfn_rpc_mem_to_fd>(dlsym(_rpc_lib_handle, "rpcmem_to_fd"));
    if (nullptr == _pfn_rpc_mem_alloc || nullptr == _pfn_rpc_mem_free || nullptr == _pfn_rpc_mem_to_fd) {
        LOGGW("unable to access symbols in shared buffer. dlerror(): %s", dlerror());
        dlclose(_rpc_lib_handle);
        return 11;
    }

    LOGGD("leave qni_init\n");
    return 0;
}


int qnn_implementation::qnn_finalize() {
    int ret_status = 0;
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    LOGGD("enter qni_finalize\n");
    if (dlclose(_rpc_lib_handle) != 0) {
        LOGGW("failed to unload qualcomm's rpc lib, error:%s\n", dlerror());
    }

    if (nullptr != _qnn_graph_handle) {
        error = _qnn_interface.qnn_graph_finalize(_qnn_graph_handle, _qnn_profile_handle, nullptr);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN graph_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));

        }
        _qnn_graph_handle = nullptr;
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

    unload_system();

    _loaded_backend.clear();

    for (auto &it : _loaded_lib_handle) {
        int dlclose_error = dlclose(it.second);
        if (dlclose_error != 0) {
            LOGGW("failed to close QNN backend %d, error %s\n", it.first, dlerror());
            ret_status = 1;
        }
    }
    _loaded_lib_handle.clear();
    _lib_path_to_backend_id.clear();
    LOGGD("leave qni_finalize\n");

    return ret_status;
}


const qnn_interface &qnn_implementation::get_qnn_interface() const {
    if (!_qnn_interface.is_loaded()) {
        LOGGW("pls check why _qnn_interface is not loaded\n");
    }
    return _qnn_interface;
}


int qnn_implementation::add_tensor(const char * node_name, Qnn_Tensor_t * p_tensor, bool b_save_tensor) {
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
            {QNN_DATATYPE_INT_8, 1},
            {QNN_DATATYPE_INT_16, 2},
            {QNN_DATATYPE_INT_32, 4},
            {QNN_DATATYPE_INT_64, 8},
            {QNN_DATATYPE_UINT_8, 1},
            {QNN_DATATYPE_UINT_16, 2},
            {QNN_DATATYPE_UINT_32, 4},
            {QNN_DATATYPE_UINT_64, 8},
            {QNN_DATATYPE_FLOAT_16, 2},
            {QNN_DATATYPE_FLOAT_32, 4},
            {QNN_DATATYPE_FLOAT_64, 8},
            {QNN_DATATYPE_BOOL_8, 1},
            {QNN_DATATYPE_SFIXED_POINT_4, 0.5},
            {QNN_DATATYPE_SFIXED_POINT_8, 1},
            {QNN_DATATYPE_SFIXED_POINT_16, 2},
            {QNN_DATATYPE_SFIXED_POINT_32, 4},
            {QNN_DATATYPE_UFIXED_POINT_4, 0.5},
            {QNN_DATATYPE_UFIXED_POINT_8, 1},
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
                                (uint32_t*)((QNN_VER_PTR(*p_tensor))->dimensions) + (QNN_VER_PTR(*p_tensor))->rank,
                        data_type_to_size.find((QNN_VER_PTR(*p_tensor)->dataType))->second,
                                            std::multiplies<float>()));
        LOGGD("qnn tensor size %d\n", qnn_tensor_size);
        qnn_tensor_size =
                std::lround(std::accumulate(QNN_TENSOR_GET_DIMENSIONS(p_tensor),
                                            QNN_TENSOR_GET_DIMENSIONS(p_tensor) + QNN_TENSOR_GET_RANK(p_tensor),
                                            data_type_to_size.find(QNN_TENSOR_GET_DATA_TYPE(p_tensor))->second,
                                            std::multiplies<float>()));

        LOGGD("qnn tensor size %d\n", qnn_tensor_size);
        if (qnn_tensor_size != ((QNN_VER_PTR(*p_tensor)->clientBuf.dataSize))) {
            LOGGW("this is a static tensor %s but length mismatch\n", QNN_VER_PTR(*p_tensor)->name);
            LOGGW("adding STATIC tensor, length mismatch between clientBuf"
                    "size and tensor Dims(dim * rank * sizeof(datatype) for, nodeName: %s, tensorName: %s."
                    "Got tensorSize: %d, tensor.clientBuf.dataSize: %d.\n",
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


int qnn_implementation::add_node(Qnn_OpConfigVersion_t version,
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
                nodeError = add_tensor(name, &tensor, true);
                if (nodeError != 0) {
                    if (nodeError != 2) {
                        LOGGW("failed to add tensor %s on node %s\n",
                              QNN_TENSOR_GET_NAME(tensor), name);
                        FREE_MEMORY(nodeParams, inputs, outputs);
                        return nodeError;
                    }
                }
                LOGGI("succeed to add tensor %s on node %s\n", QNN_TENSOR_GET_NAME(tensor), name);
                nodeParams[nodeParamsCounter].paramType     = QNN_PARAMTYPE_TENSOR;
                nodeParams[nodeParamsCounter].name          = params[i].name;
                nodeParams[nodeParamsCounter++].tensorParam = tensor;
                break;
            }
            case QNN_PARAMTYPE_SCALAR: {
                nodeParams[nodeParamsCounter].paramType     = QNN_PARAMTYPE_SCALAR;
                nodeParams[nodeParamsCounter].name          = params[i].name;
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

    size_t outputsCounter        = 0;
    _output_tensor_map[name] = {};
    for (size_t k = 0; k < numOfOutputs; k++) {
        // create node output tensors
        nodeError = add_tensor(name, outputTensors[k]);
        if (nodeError != 0) {
            LOGGW("add_tensor() failed for output tensor %s on node %s\n", QNN_TENSOR_GET_NAME(outputTensors[k]), name);
            FREE_MEMORY(nodeParams, inputs, outputs);
            return nodeError;
        } else {
            LOGGI("add_tensor() ok for output tensor %s on node %s\n", QNN_TENSOR_GET_NAME(outputTensors[k]), name);
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
        auto validationStatus = _qnn_interface.qnn_backend_validate_op_config(_qnn_backend_handle, opDefinition); //TODO: not work
        if (validationStatus == QNN_BACKEND_ERROR_NOT_SUPPORTED) {
            LOGGD("validation API not supported\n");
        } else if (validationStatus != QNN_SUCCESS) {
            LOGGW("validating op config %s failed\n", name); //TODO:[ ERROR ] OpConfig validation failed for ElementWiseAdd
            //FREE_MEMORY(nodeParams, inputs, outputs);
            //return 3;
        }
    }

    if (_qnn_interface.qnn_graph_add_node(_qnn_graph_handle, opDefinition) != QNN_GRAPH_NO_ERROR) {
        LOGGW("adding node %s failed\n", name);
        FREE_MEMORY(nodeParams, inputs, outputs);
        return 4;
    }

    FREE_MEMORY(nodeParams, inputs, outputs);

    return 0;
}



// =================================================================================================
//
// JNI helper function for PoC#121:Add Qualcomm mobile SoC native backend for GGML(https://github.com/zhouwg/kantv/issues/121)
// should move into ggml-jni-impl.cpp in the future
//
// =================================================================================================
//TODO:
// https://github.com/zhouwg/kantv/issues/121
// PoC-S26: offload a simple f32 2x2 matrix addition operation to QNN CPU backend
int qnn_matrix(int n_backend_type, int n_op_type) {
    int result = 0;
    uint32_t i  = 0;
    uint32_t j  = 0;
    LOGGD("enter qnn_matrix\n");
    LOGGV("[%s], op type:%d\n", __func__, n_op_type);
    GGML_JNI_NOTIFY("[%s], backend_type:%d, op type:%d\n", __func__, n_backend_type, n_op_type);
    uint32_t num_graphs = 0;
    uint32_t num_graph_inputs = 0;
    uint32_t num_graph_outputs = 0;
    QnnSystemContext_GraphInfo_t * graph = nullptr;
    //const QnnSystemContext_BinaryInfo_t * binaryinfo{nullptr};
    //Qnn_ContextBinarySize_t binaryinfo_size = 0;
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    std::string graph_name  = "qnn_matrix";
    std::vector<Qnn_Tensor_t> input_tensor_structs;
    std::vector<Qnn_Tensor_t> output_tensor_structs;

    const QnnProfile_EventId_t* events_ptr = nullptr;
    const QnnProfile_EventId_t* sub_events_ptr = nullptr;
    std::uint32_t num_events = 0;
    std::uint32_t num_sub_events = 0;
    QnnProfile_EventData_t event_data;
    QnnProfile_EventData_t sub_event_data;
    Qnn_OpConfig_t qnn_opconfig;
    typedef struct {
        void * buffer;
        uint64_t num_bytes;
    } buffer_blob_t;

    uint32_t matrix_input_0[] = {1, 2, 3, 4};
    uint32_t matrix_input_1[] = {1, 2, 3, 4};

    auto is_io_tensor = [](Qnn_TensorType_t type) {
        return type < QNN_TENSOR_TYPE_STATIC;
    };
    Qnn_Tensor_t tensor_0 = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_1 = QNN_TENSOR_INIT;
    Qnn_QuantizeParams_t quantize_param = QNN_QUANTIZE_PARAMS_INIT;

    buffer_blob_t buffer_blob;

    uint8_t * qnn_buffer = nullptr;

    struct ggml_context * ctx;
    const int sizey = 2;
    const int sizex = 2;
    const ggml_type qtype = GGML_TYPE_F32;
    size_t ctx_size = 0;
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size += ggml_row_size(qtype,         sizex*sizey);
    ctx_size += ggml_row_size(qtype,         sizex*sizey);
    ctx_size += 1024 * 1024 * 16;
    struct ggml_init_params params = {
            /*.mem_size   =*/ ctx_size,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };
    ctx = ggml_init(params);
    if (!ctx) {
        LOGGW("ggml_init failure\n");
        return 1;
    }
    struct ggml_tensor * m11 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m11, 1.0f);
    struct ggml_tensor * m12 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m12, 2.5f);
    struct ggml_tensor * m2 = ggml_add(ctx, m11, m12);
    struct ggml_cgraph * gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, m2);
    std::vector<uint8_t> work_buffer;
    ggml_graph_compute_helper(work_buffer, gf, 4);
    TENSOR_DUMP(m11);
    TENSOR_DUMP(m12);
    TENSOR_DUMP(m2);
    LOGGD("finish ggml matrix manipulation\n");

    //buffer_blob.buffer = new (std::nothrow)char[1024];
    //assert(nullptr != buffer_blob.buffer);
    //buffer_blob.num_bytes = 1024;
    qnn_implementation _qnn_backend = qnn_implementation("/data/data/com.cdeos.kantv/libQnnCpu.so"); //TODO: hardcode
    int qnn_error = _qnn_backend.qnn_init(nullptr);
    if (0 != qnn_error) {
        LOGGW("init qnn subsystem failed, pls check why\n");
        result = 1;
        return 1;
    }

    qnn_interface _qnn_interface = _qnn_backend.get_qnn_interface();
    if (!_qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        result = 2;
        goto failure;
    }

    qnn_buffer = static_cast<uint8_t *>(_qnn_backend.alloc_rpcmem(8192, 4));
    if (nullptr == qnn_buffer) {
        LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
        goto failure;
    } else {
        LOGGD("alloc rpcmem successfully\n");
    }

    QNN_VER_PTR(tensor_0)->id = 0;
    QNN_VER_PTR(tensor_0)->name = "matrix_input_0";
    //QNN_VER_PTR(tensor_0)->type = QNN_TENSOR_TYPE_APP_WRITE;
    QNN_VER_PTR(tensor_0)->type = QNN_TENSOR_TYPE_STATIC;
    QNN_VER_PTR(tensor_0)->dataFormat = QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER;
    QNN_VER_PTR(tensor_0)->dataType = QNN_DATATYPE_FLOAT_32;
    //QNN_VER_PTR(tensor_0)->memType  = QNN_TENSORMEMTYPE_MEMHANDLE;
    QNN_VER_PTR(tensor_0)->memType  = QNN_TENSORMEMTYPE_RAW;

    quantize_param.encodingDefinition = QNN_DEFINITION_UNDEFINED;
    quantize_param.quantizationEncoding = QNN_QUANTIZATION_ENCODING_UNDEFINED;
    switch (quantize_param.quantizationEncoding) {
        case QNN_QUANTIZATION_ENCODING_SCALE_OFFSET: {
            quantize_param.scaleOffsetEncoding.scale = 0.0f;
            quantize_param.scaleOffsetEncoding.offset = 0;
        }
        break;
        case QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET: {
            quantize_param.axisScaleOffsetEncoding.axis = 0;
            quantize_param.axisScaleOffsetEncoding.numScaleOffsets = 0;
            quantize_param.axisScaleOffsetEncoding.scaleOffset = nullptr;
        }
        break;
        default:
            quantize_param.scaleOffsetEncoding.scale = 0.0f;
            quantize_param.scaleOffsetEncoding.offset = 0;
            break;
    }
    QNN_VER_PTR(tensor_0)->quantizeParams = quantize_param;
    QNN_VER_PTR(tensor_0)->rank = 2;

    //QNN_VER_PTR(tensor_0)->dimensions = reinterpret_cast<uint32_t*>(m11->data);
    QNN_VER_PTR(tensor_0)->dimensions = reinterpret_cast<uint32_t *>(qnn_buffer);
    //QNN_VER_PTR(tensor_0)->clientBuf.dataSize = ggml_element_size(m11);
    QNN_VER_PTR(tensor_0)->clientBuf.dataSize = 0;
    LOGGD("client buf size %d\n", ggml_nbytes(m11));
    LOGGD("is_io_tensor(QNN_VER_PTR(tensor)->type:%d\n", is_io_tensor(QNN_VER_PTR(tensor_0)->type));
    QNN_VER_PTR(tensor_0)->clientBuf.data = is_io_tensor(QNN_VER_PTR(tensor_0)->type) ? nullptr : static_cast<void*>(m11->data);
    QNN_VER_PTR(tensor_0)->clientBuf.dataSize = 8; //TODO:?
    //QNN_VER_PTR(tensor_0)->clientBuf.data = nullptr;
    //QNN_VER_PTR(tensor_0)->clientBuf.dataSize = 0;
    //QNN_VER_PTR(tensor_0)->clientBuf.data = nullptr;
    memcpy(QNN_VER_PTR(tensor_0)->dimensions, matrix_input_0, 16);
    //result = _qnn_backend.add_tensor(QNN_VER_PTR(tensor_0)->name, &tensor_0);
    if (0 != result) {
        LOGGW("add input tensor 0 failure\n");
        //goto failure;
    }

    tensor_1 =  {
            .version= QNN_TENSOR_VERSION_1,
            {.v1= {
                    .id=1,
                    .name= "matrix_input_1",
                    .type= QNN_TENSOR_TYPE_STATIC,
                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                    .dataType= QNN_DATATYPE_FLOAT_32,
                    .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                       QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                       {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                    .rank= 2,
                    .dimensions= (uint32_t*)qnn_buffer + 32,
                    .memType= QNN_TENSORMEMTYPE_RAW,
                    {.clientBuf= { .data=qnn_buffer + 32,
                            .dataSize=8}}}}};
    memcpy(QNN_VER_PTR(tensor_1)->dimensions, matrix_input_1, 16);
    //result =_qnn_backend.add_tensor(QNN_VER_PTR(tensor_1)->name, &tensor_1);
    if (0 != result) {
        LOGGW("add input tensor 1 failure\n");
        //goto failure;
    }

    //ref:/opt/qcom/aistack/qnn/2.20.0.240223/examples/Models/InceptionV3/model/Inception_v3.cpp
    if (1) {
        qnn_opconfig = QNN_OPCONFIG_INIT;
        Qnn_Param_t qnn_params[] = {
                {
                        .paramType=QNN_PARAMTYPE_TENSOR,
                        .name="qnn_matrix_0",
                        .tensorParam=(Qnn_Tensor_t) tensor_0
                },
                {
                        .paramType=QNN_PARAMTYPE_TENSOR,
                        .name="qnn_matrix_1",
                        .tensorParam=(Qnn_Tensor_t) tensor_1
                }
        };
        const char * inputs[] = {
                "matrix_input_0",
                "matrix_input_1"
        };
        uint32_t matrix_output[] = {1, 2, 3, 4};
        Qnn_Tensor_t outputs[] = {
                (Qnn_Tensor_t) {
                        .version= QNN_TENSOR_VERSION_1,
                        {.v1= {
                                .id=0,
                                .name= "output_tensor",
                                .type= QNN_TENSOR_TYPE_STATIC,
                                .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                .dataType= QNN_DATATYPE_FLOAT_32,
                                .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                                   QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                                   {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                                .rank= 2,
                                .dimensions= (uint32_t*)qnn_buffer + 128,
                                .memType= QNN_TENSORMEMTYPE_RAW,
                                {.clientBuf= { .data=qnn_buffer + 128,
                                        .dataSize=0}}}}}
        };


        LOGGD("register tensor rpcmem\n");
        //_qnn_backend.add_node(qnn_opconfig); //TODO: not work
        result = _qnn_backend.register_rpcmem((QNN_VER_PTR(tensor_0))->dimensions, &(tensor_0)); //TODO: not work
        result = _qnn_backend.register_rpcmem((QNN_VER_PTR(tensor_1))->dimensions, &(tensor_0)); //TODO: not work
        result = _qnn_backend.register_rpcmem((QNN_VER_PTR(outputs[0]))->clientBuf.data, &(outputs[0])); //TODO: not work
        if (0 != result) {
            LOGGW("register rpcmem failure\n");
            //goto failure;
        } else {
            LOGGD("register rpcmem successfully\n");
        }

        //ref:https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-50/MasterOpDef.html#elementwiseadd
        _qnn_backend.add_node(QNN_OPCONFIG_VERSION_1, // Op_Config_t Version
                              "qnn_matrix_addition", // Node Name
                              "qti.aisw", // Package Name,  must be "qti.aisw" otherwise error occurs
                              "ElementWiseAdd", // Qnn Node Type,
                              qnn_params, // Node Params
                              2, // Num Node Params
                              inputs, // Input Tensor Names
                              2, // Num Input Tensor Names
                              outputs, // Output Tensors
                              1// Num Output Tensors
        );
    }

    result = _qnn_backend.execute_graph();//TODO:not work currently
    if (0 != result) {
        LOGGW("qnn graph execution failure\n");
        goto failure;
    } else {
        LOGGD("qnn graph execution successfully\n");
    }
    /*
    error = _qnn_interface.qnn_system_context_get_binary_info(
            _qnn_backend.get_qnn_system_handle(),
            buffer_blob.buffer, buffer_blob.num_bytes,
            &binaryinfo, &binaryinfo_size);
    if (error != QNN_SUCCESS) {
        LOGGW("failed to interpret QNN Context binary, error code %d", QNN_GET_ERROR_CODE(error));
        result = 3;
        goto failure;
    }

    if (binaryinfo->version == QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_1) {
        num_graphs = binaryinfo->contextBinaryInfoV1.numGraphs;
        graph = binaryinfo->contextBinaryInfoV1.graphs;
    } else if (binaryinfo->version == QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_2) {
        num_graphs = binaryinfo->contextBinaryInfoV2.numGraphs;
        graph = binaryinfo->contextBinaryInfoV2.graphs;
    } else {
        result = 4;
        LOGGW("unknown QNN BinaryInfo version %d\n", binaryinfo->version);
        goto failure;
    }

    if (num_graphs > 1) {
        result = 5;
        LOGGW("the context binary contains %lu graphs, but now assume that one context binary contains one graph\n", num_graphs);
        goto failure;
    }

    if (graph[0].version != QNN_SYSTEM_CONTEXT_GRAPH_INFO_VERSION_1) {
        result = 6;
        LOGGW("unknown QNN GraphInfo version %d\n", graph[0].version);
        goto failure;
    }

    graph_name = graph->graphInfoV1.graphName;
    LOGGD("graph name:%s\n", graph_name.c_str());

    num_graph_inputs = graph->graphInfoV1.numGraphInputs;
    input_tensor_structs.reserve(num_graph_inputs);
    for (std::uint32_t i = 0; i < num_graph_inputs; ++i) {
        input_tensor_structs.emplace_back(graph->graphInfoV1.graphInputs[i]);
    }

    num_graph_outputs = graph->graphInfoV1.numGraphOutputs;
    output_tensor_structs.reserve(num_graph_outputs);
    for (std::uint32_t i = 0; i < num_graph_outputs; ++i) {
        output_tensor_structs.emplace_back(graph->graphInfoV1.graphOutputs[i]);
    }

    error = _qnn_interface.qnn_profile_get_events(_qnn_backend.get_qnn_profile_handle(), &events_ptr, &num_events);
    if (error != QNN_SUCCESS) {
        LOGGW("failed to get profile events: %d\n", QNN_GET_ERROR_CODE(error));
        goto failure;
    }

    for (i = 0; i < num_events; ++i) {
        error = _qnn_interface.qnn_profile_get_event_data(events_ptr[i], &event_data);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to get event data for event %d: %d\n", i, QNN_GET_ERROR_CODE(error));
            goto failure;
        }

        error = _qnn_interface.qnn_profile_get_sub_events(events_ptr[i], &sub_events_ptr, &num_sub_events);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to get sub events for event %d: %d\n", i, QNN_GET_ERROR_CODE(error));
            goto failure;
        }

        for (j = 0; j < num_sub_events; ++j) {
            error = _qnn_interface.qnn_profile_get_event_data(sub_events_ptr[j], &sub_event_data);
            if (error != QNN_SUCCESS) {
                LOGGW("failed to get sub event data for sub event %d of event %d: %d\n",
                      j, i, QNN_GET_ERROR_CODE(error));
                goto failure;
            }
            if (sub_event_data.type == QNN_PROFILE_EVENTTYPE_NODE &&
                (sub_event_data.unit == QNN_PROFILE_EVENTUNIT_MICROSEC ||
                 sub_event_data.unit == QNN_PROFILE_EVENTUNIT_CYCLES)) {
                //TODO: handle profile data here
                //  sub_event_data.identifier
                //  sub_event_data.value
            }
        }
    }
    */

failure:
    _qnn_backend.unregister_rpcmem();
    _qnn_backend.free_rpcmem(qnn_buffer);

    _qnn_backend.qnn_finalize();
    LOGGD("leave qnn_matrix\n");

    return result;
}


//TODO:
// https://github.com/zhouwg/kantv/issues/121
// PoC-S27:  mapping ggml_tensor to QNN_tensor and offload a simple 2x2 matrix addition operation to QNN CPU backend
int qnn_ggml(int n_backend_type, int n_ggml_op_type) {
    LOGGD("enter qnn_ggml\n");
    LOGGV("op type:%d\n", n_ggml_op_type);
    GGML_JNI_NOTIFY("[%s], backend_type:%d, ggml op type:%d\n", __func__, n_backend_type,
                    n_ggml_op_type);
    LOGGD("leave qnn_ggml\n");

    return 0;
}
