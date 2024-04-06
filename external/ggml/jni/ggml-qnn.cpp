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
#include "HTP/QnnHtpDevice.h"

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


enum class OutputDataType {
    FLOAT_ONLY, NATIVE_ONLY, FLOAT_AND_NATIVE, INVALID
};


enum class InputDataType {
    FLOAT, NATIVE, INVALID
};


typedef struct GraphInfo {
    Qnn_GraphHandle_t graph;
    char * graphName;
    Qnn_Tensor_t * inputTensors;
    uint32_t numInputTensors;
    Qnn_Tensor_t * outputTensors;
    uint32_t numOutputTensors;
} GraphInfo_t;
typedef GraphInfo_t * GraphInfoPtr_t;


typedef struct GraphConfigInfo {
    char * graphName;
    const QnnGraph_Config_t ** graphConfigs;
} GraphConfigInfo_t;


typedef int ( * ComposeGraphsHandleType_t)(
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


typedef int ( * FreeGraphsHandleType_t)(GraphInfo_t ***, uint32_t);


using pfn_rpc_mem_alloc                     = void * (*)(int, uint32_t, int);
using pfn_rpc_mem_free                      = void (*)(void *);
using pfn_rpc_mem_to_fd                     = int (*)(void *);

using _pfn_QnnSaver_initialize              = decltype(QnnSaver_initialize);
using _pfn_QnnInterface_getProviders        = decltype(QnnInterface_getProviders);
using _pfn_QnnSystemInterface_getProviders  = decltype(QnnSystemInterface_getProviders);

using PopulateInputTensorsRetType_t         = std::tuple<int, size_t, size_t>;
using ReadBatchDataRetType_t                = std::tuple<int, size_t, size_t>;
using ReadInputListRetType_t                = std::tuple<std::vector<std::vector<std::string>>, std::unordered_map<std::string, uint32_t>, bool>;
using ReadInputListsRetType_t               = std::tuple<std::vector<std::vector<std::vector<std::string>>>, std::vector<std::unordered_map<std::string, uint32_t>>, bool>;


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


static int freeGraphsInfo(GraphInfoPtr_t ** graphsInfo, uint32_t numGraphs) {
    if (graphsInfo == nullptr || * graphsInfo == nullptr) {
        return 1;
    }

    for (uint32_t i = 0; i < numGraphs; i++) {
        free((*graphsInfo)[i]->graphName);
        freeQnnTensors((*graphsInfo)[i]->inputTensors, (*graphsInfo)[i]->numInputTensors);
        freeQnnTensors((*graphsInfo)[i]->outputTensors, (*graphsInfo)[i]->numOutputTensors);
    }

    free(**graphsInfo);
    free(*graphsInfo);
    *graphsInfo = nullptr;

    return 0;
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
    int  returnStatus = 0;
    size_t length{0};
    std::tie(returnStatus, length) = getDataTypeSizeInBytes(dataType);
    if (0 != returnStatus) {
        return std::make_tuple(returnStatus, 0);
    }
    length *= calculateElementCount(dims);
    return std::make_tuple(0, length);
}


static ReadBatchDataRetType_t readBatchData(const std::vector<std::string> &filePaths,
                                                         const size_t filePathsIndexOffset,
                                                         const bool loopBackToStart,
                                                         const std::vector<size_t> &dims,
                                                         const Qnn_DataType_t dataType,
                                                         uint8_t *buffer) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return std::make_tuple(1, 0, 0);
    }
    int err = 0;
    size_t tensorLength{0};
    std::tie(err, tensorLength) = calculateLength(dims, dataType);
    if (0 != err) {
        return std::make_tuple(err, 0, 0);
    }
    size_t numInputsCopied = 0;
    size_t numBatchSize = 0;
    size_t totalLength = 0;
    size_t fileIndex = filePathsIndexOffset;
    while (true) {
        if (fileIndex >= filePaths.size()) {
            if (loopBackToStart) {
                fileIndex = fileIndex % filePaths.size();
            } else {
                numBatchSize += (tensorLength - totalLength) / (totalLength / numBatchSize);
                // pad the vector with zeros
                memset(buffer + totalLength, 0, (tensorLength - totalLength) * sizeof(char));
                break;
            }
        }
        std::ifstream in(filePaths[fileIndex], std::ifstream::binary);
        if (!in) {
            LOGGW("failed to open input file: %s", (filePaths[fileIndex]).c_str());
            return std::make_tuple(1, numInputsCopied, numBatchSize);
        }
        in.seekg(0, in.end);
        const size_t fileSize = in.tellg();
        in.seekg(0, in.beg);
        if ((tensorLength % fileSize) != 0 || fileSize > tensorLength || fileSize == 0) {
            LOGGW(
                    "Given input file %s with file size in bytes %d. If the model expects a batch size of "
                    "one, the file size should match the tensor extent: %d bytes. If the model expects a "
                    "batch size > 1, the file size should evenly divide the tensor extent: %d bytes.",
                    filePaths[fileIndex].c_str(),
                    fileSize,
                    tensorLength,
                    tensorLength);
            return std::make_tuple(1, numInputsCopied, numBatchSize);
        }
        if (!in.read(reinterpret_cast<char *>(buffer + (numInputsCopied * fileSize)), fileSize)) {
            LOGGW("Failed to read the contents of: %s", filePaths.front().c_str());
            return std::make_tuple(1, numInputsCopied, numBatchSize);
        }
        totalLength += fileSize;
        numInputsCopied += 1;
        numBatchSize += 1;
        fileIndex += 1;
        if (totalLength >= tensorLength) {
            break;
        }
    }
    return std::make_tuple(0, numInputsCopied, numBatchSize);
}


static ReadBatchDataRetType_t readBatchDataAndUpdateQueue(
        std::queue<std::string> & filePaths,
        std::vector<size_t> dims,
        Qnn_DataType_t dataType,
        uint8_t * buffer) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return std::make_tuple(1, 0, 0);
    }

    int err = 0;
    size_t len = 0;
    std::tie(err, len) = calculateLength(dims, dataType);
    if (0 != err) {
        return std::make_tuple(err, 0, 0);
    }

    size_t numInputsCopied = 0;
    size_t numBatchSize    = 0;
    size_t totalLength     = 0;
    do {
        if (filePaths.empty()) {
            numBatchSize += (len - totalLength) / (totalLength / numBatchSize);
            // pad the vector with zeros
            memset(buffer + totalLength, 0, (len - totalLength) * sizeof(char));
            totalLength = len;
        } else {
            std::ifstream in(filePaths.front(), std::ifstream::binary);
            if (!in) {
                LOGGW("failed to open input file: %s", filePaths.front().c_str());
                return std::make_tuple(1, numInputsCopied, numBatchSize);
            }
            in.seekg(0, in.end);
            const size_t length = in.tellg();
            in.seekg(0, in.beg);
            if ((len % length) != 0 || length > len || length == 0) {
                LOGGW("input file %s: file size in bytes (%d), should be multiples of: %d",
                          filePaths.front().c_str(),
                          length,
                          len);
                return std::make_tuple(1, numInputsCopied, numBatchSize);
            }
            if (!in.read(reinterpret_cast<char*>(buffer + (numInputsCopied * length)), length)) {
                LOGGW("failed to read the contents of: %s", filePaths.front().c_str());
                return std::make_tuple(1, numInputsCopied, numBatchSize);
            }
            LOGGI("return from readDataFromFile()");
            totalLength += length;
            numInputsCopied += 1;
            numBatchSize += 1;
            filePaths.pop();
        }
    } while (totalLength < len);

    return std::make_tuple(0, numInputsCopied, numBatchSize);
}


static std::tuple<int, size_t> getFileSize(std::string & filePath) {
    std::ifstream in(filePath, std::ifstream::binary);
    if (!in) {
        LOGGW("failed to open input file: %s", filePath.c_str());
        return std::make_tuple(1, 0);
    }
    in.seekg(0, in.end);
    const size_t length = in.tellg();
    in.seekg(0, in.beg);

    return std::make_tuple(0, length);
}


static int readBinaryFromFile(std::string & filePath, uint8_t * buffer, size_t bufferSize) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return 1;
    }
    std::ifstream in(filePath, std::ifstream::binary);
    if (!in) {
        LOGGW("failed to open input file: %s", filePath.c_str());
        return 1;
    }
    if (!in.read(reinterpret_cast<char*>(buffer), bufferSize)) {
        LOGGW("failed to read the contents of: %s", filePath.c_str());
        return 1;
    }

    return 0;
}


static int writeDataToFile(std::string fileDir,
                                               std::string fileName,
                                               std::vector<size_t> dims,
                                               Qnn_DataType_t dataType,
                                               uint8_t * buffer) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return 1;
    }
    if (!mkdir(fileDir.c_str(), 0777)) {
        LOGGW("Failed to create output directory: %s", fileDir.c_str());
        return 2;
    }
    const std::string outputPath(fileDir + "/" + fileName);
    std::ofstream os(outputPath, std::ofstream::binary);
    if (!os) {
        LOGGW("Failed to open output file for writing: %s", outputPath.c_str());
        return 3;
    }
    int err = 0;
    size_t length = 0;
    std::tie(err, length) = calculateLength(dims, dataType);
    if (0 != err) {
        LOGGW("failed to get length of data");
        return err;
    }
    for (size_t l = 0; l < length; l++) {
        os.write(reinterpret_cast<char*>(&(*(buffer + l))), 1);
    }

    return 0;
}


static int writeBatchDataToFile(std::vector<std::string> fileDirs,
                                                    std::string fileName,
                                                    std::vector<size_t> dims,
                                                    Qnn_DataType_t dataType,
                                                    uint8_t * buffer,
                                                    const size_t batchSize) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return 1;
    }
    int err = 0;
    size_t length{0};
    std::tie(err, length) = calculateLength(dims, dataType);
    if (0 != err) {
        return err;
    }
    auto outputSize = (length / batchSize);
    for (size_t batchIndex = 0; batchIndex < fileDirs.size(); batchIndex++) {
        std::string fileDir = fileDirs[batchIndex];
        if (0 == mkdir(fileDir.c_str(), 0777)) {
            LOGGW("Failed to create output directory: %s", fileDir.c_str());
            return 1;
        }
        const std::string outputPath(fileDir + "/" + fileName);
        std::ofstream os(outputPath, std::ofstream::binary);
        if (!os) {
            LOGGW("Failed to open output file for writing: %s", outputPath.c_str());
            return 1;
        }
        for (size_t l = 0; l < outputSize; l++) {
            size_t bufferIndex = l + (batchIndex * outputSize);
            os.write(reinterpret_cast<char *>(&(*(buffer + bufferIndex))), 1);
        }
    }
    return 0;
}


static int writeBinaryToFile(std::string fileDir,
                                                 std::string fileName,
                                                 uint8_t * buffer,
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

static std::string sanitizeTensorName(std::string name) {
    std::string sanitizedName = std::regex_replace(name, std::regex("\\W+"), "_");
    if (!std::isalpha(sanitizedName[0]) && sanitizedName[0] != '_') {
        sanitizedName = "_" + sanitizedName;
    }
    return sanitizedName;
}

static void split(std::vector<std::string> &splitString,
                       const std::string &tokenizedString,
                       const char separator) {
    splitString.clear();
    std::istringstream tokenizedStringStream(tokenizedString);
    while (!tokenizedStringStream.eof()) {
        std::string value;
        getline(tokenizedStringStream, value, separator);
        if (!value.empty()) {
            splitString.push_back(value);
        }
    }
}
static std::unordered_map<std::string, uint32_t> extractInputNameIndices(
        const std::string &inputLine, const std::string &separator) {
    std::vector<std::string> inputFilePaths;
    std::unordered_map<std::string, uint32_t> inputNameToIndex;
    split(inputFilePaths, inputLine, ' ');
    size_t inputCount = 0;
    for (uint32_t idx = 0; idx < inputFilePaths.size(); idx++) {
        auto position = inputFilePaths[idx].find(separator);
        if (position != std::string::npos) {
            auto unsanitizedTensorName = inputFilePaths[idx].substr(0, position);
            auto sanitizedTensorName = sanitizeTensorName(unsanitizedTensorName);
            if (sanitizedTensorName != unsanitizedTensorName) {
                inputNameToIndex[unsanitizedTensorName] = idx;
            }
            inputNameToIndex[sanitizedTensorName] = idx;
            inputCount = inputCount + 1;
        }
    }
    return inputCount == inputFilePaths.size() ? inputNameToIndex
                                               : std::unordered_map<std::string, uint32_t>{};
}


static void parseInputFilePaths(std::vector<std::string> &inputFilePaths,
                                     std::vector<std::string> &paths,
                                     std::string separator) {
    for (auto &inputInfo : inputFilePaths) {
        auto position = inputInfo.find(separator);
        if (position != std::string::npos) {
            auto path = inputInfo.substr(position + separator.size());
            paths.push_back(path);
        } else {
            paths.push_back(inputInfo);
        }
    }
}


static ReadInputListRetType_t readInputList(const std::string inputFileListPath) {
    std::queue<std::string> lines;
    std::ifstream fileListStream(inputFileListPath);
    if (!fileListStream) {
        LOGGW("failed to open input file: %s", inputFileListPath.c_str());
        return std::make_tuple(std::vector<std::vector<std::string>>{},
                               std::unordered_map<std::string, uint32_t>{},
                               false);
    }

    std::string fileLine;
    while (std::getline(fileListStream, fileLine)) {
        if (fileLine.empty()) continue;
        lines.push(fileLine);
    }

    if (!lines.empty() && lines.front().compare(0, 1, "#") == 0) {
        lines.pop();
    }

    if (!lines.empty() && lines.front().compare(0, 1, "%") == 0) {
        lines.pop();
    }

    std::string separator = ":=";
    std::vector<std::vector<std::string>> filePathsList;
    std::unordered_map<std::string, uint32_t> inputNameToIndex;
    if (!lines.empty()) {
        inputNameToIndex = extractInputNameIndices(lines.front(), separator);
    }
    while (!lines.empty()) {
        std::vector<std::string> paths{};
        std::vector<std::string> inputFilePaths;
        split(inputFilePaths, lines.front(), ' ');
        parseInputFilePaths(inputFilePaths, paths, separator);
        filePathsList.reserve(paths.size());
        for (size_t idx = 0; idx < paths.size(); idx++) {
            if (idx >= filePathsList.size()) {
                filePathsList.push_back(std::vector<std::string>());
            }
            filePathsList[idx].push_back(paths[idx]);
        }
        lines.pop();
    }
    return std::make_tuple(filePathsList, inputNameToIndex, true);
}


static ReadInputListsRetType_t readInputLists(
        std::vector<std::string> inputFileListPaths) {
    std::vector<std::vector<std::vector<std::string>>> filePathsLists;
    std::vector<std::unordered_map<std::string, uint32_t>> inputNameToIndexMaps;
    for (auto const &path : inputFileListPaths) {
        bool readSuccess;
        std::vector<std::vector<std::string>> filePathList;
        std::unordered_map<std::string, uint32_t> inputNameToIndex;
        std::tie(filePathList, inputNameToIndex, readSuccess) = readInputList(path);
        if (!readSuccess) {
            filePathsLists.clear();
            return std::make_tuple(filePathsLists, inputNameToIndexMaps, false);
        }
        filePathsLists.push_back(filePathList);
        inputNameToIndexMaps.push_back(inputNameToIndex);
    }
    return std::make_tuple(filePathsLists, inputNameToIndexMaps, true);
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


class qnn_io_tensor {
public:
    int setupInputAndOutputTensors(Qnn_Tensor_t ** inputs,
                                          Qnn_Tensor_t ** outputs,
                                          GraphInfo_t graphInfo);

    int  writeOutputTensors(uint32_t graphIdx,
                                  size_t startIdx,
                                  char *graphName,
                                  Qnn_Tensor_t *outputs,
                                  uint32_t numOutputs,
                                  OutputDataType outputDatatype,
                                  uint32_t graphsCount,
                                  std::string outputPath,
                                  size_t numInputFilesPopulated,
                                  size_t outputBatchSize);

    PopulateInputTensorsRetType_t populateInputTensors(
            uint32_t graphIdx,
            const std::vector<std::vector<std::string>> &filePathsVector,
            const size_t filePathsIndexOffset,
            const bool loopBackToStart,
            const std::unordered_map<std::string, uint32_t> &inputNameToIndex,
            Qnn_Tensor_t *inputs,
            GraphInfo_t graphInfo,
            InputDataType inputDataType);

    int  tearDownInputAndOutputTensors(Qnn_Tensor_t *inputs,
                                             Qnn_Tensor_t *outputs,
                                             size_t numInputTensors,
                                             size_t numOutputTensors);


    // Helper method to read data from files to a buffer.
    int readDataAndAllocateBuffer(
            std::queue<std::string>& filePaths,
            std::vector<size_t> dims,
            Qnn_DataType_t dataType,
            uint8_t** bufferToCopy) {
        int returnStatus = 0;
        *bufferToCopy           = nullptr;
        returnStatus            = allocateBuffer(bufferToCopy, dims, dataType);
        if (0 == returnStatus) {
            int status = 0;
            std::tie(status, m_numFilesPopulated, m_batchSize) = readBatchDataAndUpdateQueue(
                    filePaths, dims, dataType, reinterpret_cast<uint8_t*>(*bufferToCopy));
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
            std::queue<std::string>& filePaths,
            Qnn_Tensor_t* input,
            InputDataType inputDataType) {
        if (nullptr == input) {
            LOGGW("input is nullptr");
            return 1;
        }

        int  returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(input), QNN_TENSOR_GET_RANK(input));

        if (inputDataType == InputDataType::FLOAT &&
            QNN_TENSOR_GET_DATA_TYPE(input) != QNN_DATATYPE_FLOAT_32) {
            uint8_t* fileToBuffer = nullptr;
            returnStatus = readDataAndAllocateBuffer(filePaths, dims, QNN_DATATYPE_FLOAT_32, &fileToBuffer);
            if (0 == returnStatus) {
                LOGGW("readDataFromFileToBuffer successful");
                returnStatus = copyFromFloatToNative(reinterpret_cast<float*>(fileToBuffer), input);
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
                    static_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(input).data));
            if (0 != status) {
                LOGGW("Failure in datautil::readBatchDataAndUpdateQueue");
                returnStatus = 1;
            }
        }
        return returnStatus;
    }

    // Helper method to populate all input tensors during execution.
    int populateInputTensors(
            uint32_t graphIdx,
            std::vector<std::queue<std::string>>& filePathsQueue,
            Qnn_Tensor_t* inputs,
            GraphInfo_t graphInfo,
            InputDataType inputDataType) {
        LOGGW("populateInputTensors() graphIndx %d", graphIdx);
        if (nullptr == inputs) {
            LOGGW("inputs is nullptr");
            return 1;
        }
        auto inputCount = graphInfo.numInputTensors;
        if (filePathsQueue.size() != inputCount) {
            LOGGW(
                    "Incorrect amount of Input files for graphIdx: %d. Expected: %d, "
                    "received: %d",
                    graphIdx,
                    inputCount,
                    filePathsQueue.size());
            return 1;
        }

        for (size_t inputIdx = 0; inputIdx < inputCount; inputIdx++) {
            if (0 !=
                populateInputTensor(filePathsQueue[inputIdx], &(inputs[inputIdx]), inputDataType)) {
                LOGGW("populateInputTensor() failure for input: %d", inputIdx);
                return 1;
            }
        }
        return 0;
    }

    // Helper method to populate an input tensor in the graph during execution.
    // It relies on reading data from buffer provided during executeGraph() call.
    int populateInputTensor(
            uint8_t* buffer, Qnn_Tensor_t* input, InputDataType inputDataType) {
        if (nullptr == input) {
            LOGGW("input is nullptr");
            return 1;
        }
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(input), QNN_TENSOR_GET_RANK(input));
        if (inputDataType == InputDataType::FLOAT &&
            QNN_TENSOR_GET_DATA_TYPE(input) != QNN_DATATYPE_FLOAT_32) {
            LOGGW("Received FLOAT input, but model needs non-float input");
            if (0 != copyFromFloatToNative(reinterpret_cast<float*>(buffer), input)) {
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
            memscpy(reinterpret_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(input).data), length, buffer, length);
        }
        return 0;
    }

    // Helper method to populate all input tensors.
    int populateInputTensors(
            uint32_t graphIdx,
            std::vector<uint8_t*> inputBuffers,
            Qnn_Tensor_t* inputs,
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
    template <typename T>
    int allocateBuffer(T ** buffer, size_t & elementCount) {
        LOGGD("elementCount: %d, sizeof(T): %d, total size: %d",
                  elementCount,
                  sizeof(T),
                  elementCount * sizeof(T));
        *buffer = (T*)malloc(elementCount * sizeof(T));
        if (nullptr == *buffer) {
            LOGGW("mem alloc failed for *buffer");
            return 1;
        }

        return 0;
    }

    //Helper method convert output to floatbuffer
    int converQnntensortoFloatBuffer(Qnn_Tensor_t* output,float** floatBuffer){
        int result = 0;
        if (nullptr == output) {
            return 1;
        }

        result     = convertToFloat(floatBuffer, output);
        if (0 != result) {
            return 2;
        }
        return result;
    }

    // Helper method to convert Output tensors to float and write them to files.
    int convertAndWriteOutputTensorInFloat(
            Qnn_Tensor_t* output, std::vector<std::string> outputPaths, std::string fileName) {
        if (nullptr == output) {
            return 1;
        }

        auto returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
        float* floatBuffer = nullptr;
        returnStatus       = convertToFloat(&floatBuffer, output);
        if (0 != returnStatus) {
            LOGGD("failure in convertToFloat");
            return 1;
        }
        uint8_t* bufferToWrite = reinterpret_cast<uint8_t*>(floatBuffer);
        if (0 != writeBatchDataToFile(
                    outputPaths, fileName, dims, QNN_DATATYPE_FLOAT_32, bufferToWrite, m_batchSize)) {
            LOGGW("failure in writeBatchDataToFile");
            returnStatus = 1;
        }
        if (nullptr != floatBuffer) {
            LOGGD("freeing floatBuffer");
            free(floatBuffer);
            floatBuffer = nullptr;
        }
        return returnStatus;
    }


    // Helper method to write out output. There is no de-quantization here.
    // Just write output as is to files
    int writeOutputTensor(Qnn_Tensor_t* output,
                          std::vector<std::string> outputPaths,
                          std::string & fileName) {
        if (nullptr == output) {
            LOGGW("output is nullptr");
            return 1;
        }
        int returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
        uint8_t* bufferToWrite = reinterpret_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(output).data);
        if (0 != writeBatchDataToFile(outputPaths,
                                           fileName,
                                           dims,
                                           QNN_TENSOR_GET_DATA_TYPE(output),
                                           bufferToWrite,
                                           m_batchSize)) {
            LOGGW("failure in writeBatchDataToFile");
            returnStatus = 1;
        }
        return returnStatus;
    }



private:
    size_t m_batchSize          = 0;
    size_t m_numFilesPopulated  = 0;

    PopulateInputTensorsRetType_t
    populateInputTensor(const std::vector<std::string> &filePaths,
                        const size_t filePathsIndexOffset,
                        const bool loopBackToStart,
                        Qnn_Tensor_t *input,
                        InputDataType inputDataType);

    PopulateInputTensorsRetType_t
    readDataAndAllocateBuffer(const std::vector<std::string> &filePaths,
                              const size_t filePathsIndexOffset,
                              const bool loopBackToStart,
                              std::vector<size_t> dims,
                              Qnn_DataType_t dataType,
                              uint8_t **bufferToCopy);



    int  convertToFloat(float ** out, Qnn_Tensor_t * output);

    int  convertAndWriteOutputTensorInFloat(Qnn_Tensor_t *output,
                                                  std::vector<std::string> outputPaths,
                                                  std::string fileName,
                                                  size_t outputBatchSize);

    int  writeOutputTensor(Qnn_Tensor_t *output,
                                 std::vector<std::string> outputPaths,
                                 std::string fileName,
                                 size_t outputBatchSize);

    int  allocateAndCopyBuffer(uint8_t **buffer, Qnn_Tensor_t *tensor);

    int  tearDownTensors(Qnn_Tensor_t *tensors, uint32_t tensorCount);

    int  allocateBuffer(uint8_t **buffer, std::vector<size_t> dims, Qnn_DataType_t dataType);

    int  copyFromFloatToNative(float *floatBuffer, Qnn_Tensor_t *tensor);

    int  setupTensors(Qnn_Tensor_t **tensors, uint32_t tensorCount,
                            Qnn_Tensor_t *tensorsInfo);

    int fillDims(std::vector<size_t> &dims, uint32_t *inDimensions, uint32_t rank);
};


// Helper method to read data from files to a buffer.
PopulateInputTensorsRetType_t qnn_io_tensor::readDataAndAllocateBuffer(
        const std::vector<std::string> &filePaths,
        const size_t filePathsIndexOffset,
        const bool loopBackToStart,
        std::vector<size_t> dims,
        Qnn_DataType_t dataType,
        uint8_t **bufferToCopy) {
    int returnStatus = 0;
    *bufferToCopy = nullptr;
    returnStatus = allocateBuffer(bufferToCopy, dims, dataType);
    size_t numFilesPopulated = 0;
    size_t batchSize = 0;
    int status = 0;
    std::tie(status, numFilesPopulated, batchSize) =
            readBatchData(filePaths,
                                    filePathsIndexOffset,
                                    loopBackToStart,
                                    dims,
                                    dataType,
                                    reinterpret_cast<uint8_t *>(*bufferToCopy));
    if (0 != status) {
        LOGGD("readBatchData failure\n");
        returnStatus = 1;
    }
    if (0 != returnStatus) {
        if (nullptr != *bufferToCopy) {
            free(*bufferToCopy);
            *bufferToCopy = nullptr;
        }
    }
    return {returnStatus, numFilesPopulated, batchSize};
}


// helper method to copy a float buffer, quantize it, and copy it to a tensor (Qnn_Tensor_t) buffer.
int qnn_io_tensor::copyFromFloatToNative(float * floatBuffer, Qnn_Tensor_t * tensor) {
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
            if ( 0  !=
                castFromFloat<uint8_t>(
                        static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        floatBuffer,
                        calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<uint8_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_16:
            if ( 0  !=
                castFromFloat<uint16_t>(
                        static_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        floatBuffer,
                        calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<uint16_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_32:
            if ( 0  !=
                castFromFloat<uint32_t>(
                        static_cast<uint32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        floatBuffer,
                        calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<uint32_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_8:
            if ( 0  !=
                castFromFloat<int8_t>(
                        static_cast<int8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        floatBuffer,
                        calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<int8_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_16:
            if ( 0  !=
                castFromFloat<int16_t>(
                        static_cast<int16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        floatBuffer,
                        calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<int16_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_32:
            if ( 0  !=
                castFromFloat<int32_t>(
                        static_cast<int32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        floatBuffer,
                        calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<int32_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_BOOL_8:
            if ( 0  !=
                castFromFloat<uint8_t>(
                        static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        floatBuffer,
                        calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<bool>");
                returnStatus =  1 ;
            }
            break;

        default:
            LOGGW("Datatype not supported yet!");
            returnStatus =  1 ;
            break;
    }
    return returnStatus;
}


// Helper method to populate an input tensor in the graph during execution.
// It relies on reading data from files provided during app creation.
PopulateInputTensorsRetType_t qnn_io_tensor::populateInputTensor(
        const std::vector<std::string> &filePaths,
        const size_t filePathsIndexOffset,
        const bool loopBackToStart,
        Qnn_Tensor_t *input,
        InputDataType inputDataType) {
    if (nullptr == input) {
        LOGGW("input is nullptr");
        return { 1 , 0, 0};
    }

    auto returnStatus =  0 ;
    size_t numFilesPopulated = 0;
    size_t batchSize = 0;
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(input), QNN_TENSOR_GET_RANK(input));

    if (inputDataType == InputDataType::FLOAT &&
        QNN_TENSOR_GET_DATA_TYPE(input) != QNN_DATATYPE_FLOAT_32) {
        uint8_t *fileToBuffer = nullptr;
        std::tie(returnStatus, numFilesPopulated, batchSize) =
                readDataAndAllocateBuffer(filePaths,
                                          filePathsIndexOffset,
                                          loopBackToStart,
                                          dims,
                                          QNN_DATATYPE_FLOAT_32,
                                          &fileToBuffer);
        if ( 0  == returnStatus) {
            LOGGD("readDataFromFileToBuffer successful");
            returnStatus = copyFromFloatToNative(reinterpret_cast<float *>(fileToBuffer), input);
        }
        if (nullptr != fileToBuffer) {
            free(fileToBuffer);
            fileToBuffer = nullptr;
        }
    } else {
        int status = 0;
        std::tie(status, numFilesPopulated, batchSize) =
                readBatchData(filePaths,
                                        filePathsIndexOffset,
                                        loopBackToStart,
                                        dims,
                                        QNN_TENSOR_GET_DATA_TYPE(input),
                                        static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(
                                                input).data));
        if ( 0  != status) {
            LOGGW("failure in readBatchData");
            returnStatus =  1 ;
        }
    }
    return {returnStatus, numFilesPopulated, batchSize};
}


// Helper method to populate all input tensors during execution.
PopulateInputTensorsRetType_t qnn_io_tensor::populateInputTensors(
        uint32_t graphIdx,
        const std::vector<std::vector<std::string>> &filePathsVector,
        const size_t filePathsIndexOffset,
        const bool loopBackToStart,
        const std::unordered_map<std::string, uint32_t> &inputNameToIndex,
        Qnn_Tensor_t *inputs,
        GraphInfo_t graphInfo,
        InputDataType inputDataType) {
    LOGGD("populateInputTensors() graphIndx %d", graphIdx);
    if (nullptr == inputs) {
        LOGGW("inputs is nullptr");
        return { 1 , 0, 0};
    }
    auto inputCount = graphInfo.numInputTensors;
    if (filePathsVector.size() != inputCount) {
        LOGGW(
                "Incorrect amount of Input files for graphIdx: %d. Expected: %d, "
                "received: %d",
                graphIdx,
                inputCount,
                filePathsVector.size());
        return { 1 , 0, 0};
    }
    size_t numFilesPopulated = 0;
    size_t numBatchSize = 0;
    for (size_t inputIdx = 0; inputIdx < inputCount; inputIdx++) {
        size_t inputNameIdx = inputIdx;
        LOGGD("index = %d input column index = %d", inputIdx, inputNameIdx);
        std::string inputNodeName;
        if (QNN_TENSOR_GET_NAME(graphInfo.inputTensors[inputIdx]))
            inputNodeName = QNN_TENSOR_GET_NAME(graphInfo.inputTensors[inputIdx]);
        if (!inputNodeName.empty() &&
            inputNameToIndex.find(inputNodeName) != inputNameToIndex.end()) {
            inputNameIdx = inputNameToIndex.at(inputNodeName);
        }
        int returnStatus = 0;
        size_t currentInputNumFilesPopulated = 0;
        size_t currentInputNumBatchSize = 0;
        std::tie(returnStatus, currentInputNumFilesPopulated, currentInputNumBatchSize) =
                populateInputTensor(filePathsVector[inputNameIdx],
                                    filePathsIndexOffset,
                                    loopBackToStart,
                                    &(inputs[inputIdx]),
                                    inputDataType);
        if ( 0  != returnStatus) {
            LOGGW("populateInputTensorFromFiles failed for input %s with index %d",
                      inputNodeName.c_str(),
                      inputIdx);
            return { 1 , currentInputNumFilesPopulated, currentInputNumBatchSize};
        }
        if (inputIdx == 0) {
            numFilesPopulated = currentInputNumFilesPopulated;
            numBatchSize = currentInputNumBatchSize;
        } else {
            if (numFilesPopulated != currentInputNumFilesPopulated ||
                numBatchSize != currentInputNumBatchSize) {
                LOGGW(
                        "Current input tensor with name: %s with index %d files populated = %d, batch size = %d"
                        " does not match with expected files populated = %d, batch size = %d",
                        inputNodeName.c_str(),
                        inputIdx,
                        currentInputNumFilesPopulated,
                        currentInputNumBatchSize,
                        numFilesPopulated,
                        numBatchSize);
                return { 1 , numFilesPopulated, numBatchSize};
            }
        }
    }
    return { 0 , numFilesPopulated, numBatchSize};
}


// Setup details for Qnn_Tensor_t for execution
// based on information in Qnn_TensorWrapper_t provided by model.so.
int qnn_io_tensor::setupTensors(Qnn_Tensor_t ** tensors, uint32_t tensorCount, Qnn_Tensor_t * tensorWrappers) {
    int result =  0 ;

    if (nullptr == tensorWrappers) {
        LOGGW("tensorWrappers is nullptr");
        return 1 ;
    }
    if (0 == tensorCount) {
        LOGGI("why tensor count is 0, pls check\n");
        return 2 ;
    }

    *tensors = (Qnn_Tensor_t *)calloc(1, tensorCount * sizeof(Qnn_Tensor_t));
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
                     ?  0
                     :  1);
        }

        if (0 == result) {
            LOGGD("deepCopyQnnTensorInfo successful");
            QNN_TENSOR_SET_MEM_TYPE(((*tensors) + tensorIdx), QNN_TENSORMEMTYPE_RAW);
        }

        Qnn_ClientBuffer_t clientBuffer = QNN_CLIENT_BUFFER_INIT;
        result =  allocateBuffer(reinterpret_cast<uint8_t **>(&clientBuffer.data),
                                      dims,
                                      QNN_TENSOR_GET_DATA_TYPE((*tensors) + tensorIdx));
        int     status = 0;
        size_t  length = 0;
        std::tie(status, length) =
                calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE((*tensors) + tensorIdx));
        if (status !=  0 ) {
            result = 1;
        }
        clientBuffer.dataSize = length;
        QNN_TENSOR_SET_CLIENT_BUF(((*tensors) + tensorIdx), clientBuffer);

        if (0  != result) {
            LOGGW("failure in setupTensors, cleaning up resources");
            if (nullptr != (QNN_TENSOR_GET_CLIENT_BUF((*tensors) + tensorIdx)).data) {
                free(QNN_TENSOR_GET_CLIENT_BUF((*tensors) + tensorIdx).data);
            }
            tearDownTensors(*tensors, tensorIdx);
            *tensors        = nullptr;
            result    =  1 ;
            LOGGW("failure in setupTensors, done cleaning up resources");
            return result;
        }
    }

    return result;
}


// Setup details for all input and output tensors for graph execution.
int qnn_io_tensor::setupInputAndOutputTensors(
        Qnn_Tensor_t **inputs, Qnn_Tensor_t **outputs, GraphInfo_t graphInfo) {
    int returnStatus =  0 ;
    if (0 !=
        setupTensors(inputs, graphInfo.numInputTensors, (graphInfo.inputTensors))) {
        LOGGW("failure in setting up input tensors");
        returnStatus =  1 ;
    } else {
        LOGGI("succeed in setting up input tensors");
    }

    if (0 !=
        setupTensors(outputs, graphInfo.numOutputTensors, (graphInfo.outputTensors))) {
        LOGGW("failure in setting up output tensors");
        returnStatus =  1 ;
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


// Clean up all tensors related data after execution.
int qnn_io_tensor::tearDownTensors(Qnn_Tensor_t *tensors, uint32_t tensorCount) {
    for (size_t tensorIdx = 0; tensorIdx < tensorCount; tensorIdx++) {
        //LOGGD("freeing resources for tensor: %d", tensorIdx);
        //GGML_JNI_NOTIFY("freeing resources for tensor: %d", tensorIdx);
        if (nullptr != QNN_TENSOR_GET_DIMENSIONS(tensors[tensorIdx])) {
            //LOGGD("freeing dimensions");
            //GGML_JNI_NOTIFY("freeing dimensions");
            free(QNN_TENSOR_GET_DIMENSIONS(tensors[tensorIdx]));
        }
        if (nullptr != QNN_TENSOR_GET_CLIENT_BUF(tensors[tensorIdx]).data) {
            //LOGGD("freeing clientBuf.data");
            //GGML_JNI_NOTIFY("freeing clientBuf.data");
            free(QNN_TENSOR_GET_CLIENT_BUF(tensors[tensorIdx]).data);
        }
    }
    free(tensors);
    return  0 ;
}


// Clean up all input and output tensors after execution.
int qnn_io_tensor::tearDownInputAndOutputTensors(Qnn_Tensor_t *inputs,
                                                                       Qnn_Tensor_t *outputs,
                                                                       size_t numInputTensors,
                                                                       size_t numOutputTensors) {
    if (nullptr != inputs) {
        //LOGGD("cleaning up resources for input tensors");
        //GGML_JNI_NOTIFY("cleaning up resources for input tensors");
        tearDownTensors(inputs, numInputTensors);
        inputs = nullptr;
    }
    if (nullptr != outputs) {
        //LOGGD("cleaning up resources for output tensors");
        //GGML_JNI_NOTIFY("cleaning up resources for output tensors");
        tearDownTensors(outputs, numOutputTensors);
        outputs = nullptr;
    }
    return  0 ;
}


// Helper method to allocate a buffer.
int qnn_io_tensor::allocateBuffer(uint8_t **buffer, std::vector<size_t> dims, Qnn_DataType_t dataType) {
    size_t elementCount = calculateElementCount(dims);
    auto returnStatus =  0 ;
    switch (dataType) {
        case QNN_DATATYPE_FLOAT_32:
            LOGGD("allocating float buffer");
            returnStatus =
                    allocateBuffer < float > (reinterpret_cast<float **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_UINT_8:
        case QNN_DATATYPE_UFIXED_POINT_8:
            LOGGD("allocating uint8_t buffer");
            returnStatus =
                    allocateBuffer < uint8_t > (reinterpret_cast<uint8_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_UINT_16:
        case QNN_DATATYPE_UFIXED_POINT_16:
            LOGGD("allocating uint16_t buffer");
            returnStatus = allocateBuffer < uint16_t >
                           (reinterpret_cast<uint16_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_UINT_32:
            LOGGD("allocating uint32_t buffer");
            returnStatus = allocateBuffer < uint32_t >
                           (reinterpret_cast<uint32_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_INT_8:
            LOGGD("allocating int8_t buffer");
            returnStatus =
                    allocateBuffer < int8_t > (reinterpret_cast<int8_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_INT_16:
            LOGGD("allocating int16_t buffer");
            returnStatus =
                    allocateBuffer < int16_t > (reinterpret_cast<int16_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_INT_32:
            LOGGD("allocating int32_t buffer");
            returnStatus =
                    allocateBuffer < int32_t > (reinterpret_cast<int32_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_BOOL_8:
            LOGGD("allocating bool buffer");
            returnStatus =
                    allocateBuffer < uint8_t > (reinterpret_cast<uint8_t **>(buffer), elementCount);
            break;

        default:
            LOGGW("Datatype not supported yet!");
            returnStatus =  1 ;
            break;
    }
    return returnStatus;
}


// Convert data to float or de-quantization. This is used when
// user requests for float output and the model produces
// non-float output.
int qnn_io_tensor::convertToFloat(float **out, Qnn_Tensor_t *tensor) {
    if (nullptr == tensor) {
        LOGGW("tensors is nullptr");
        return  1 ;
    }
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(tensor), QNN_TENSOR_GET_RANK(tensor));
    int result =  0 ;
    size_t elementCount = calculateElementCount(dims);
    result = allocateBuffer<float>(out, elementCount);
    if ( 0  != result) {
        LOGGW("failure in allocateBuffer<float>");
        return result;
    }
    Qnn_DataType_t data_type = QNN_TENSOR_GET_DATA_TYPE(tensor);
    LOGGI("tensor data type %d\n", data_type);
    switch (data_type) {
        case QNN_DATATYPE_UFIXED_POINT_8:
            if ( 0  !=
                tfNToFloat<uint8_t>(
                        *out,
                        reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                        QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                        elementCount)) {
                LOGGW("failure in tfNToFloat<uint8_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_UFIXED_POINT_16:
            if ( 0  !=
                tfNToFloat<uint16_t>(
                        *out,
                        reinterpret_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                        QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                        elementCount)) {
                LOGGW("failure in tfNToFloat<uint8_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_8:
            if ( 0  !=
                castToFloat<uint8_t>(
                        *out,
                        reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        elementCount)) {
                LOGGW("failure in castToFloat<uint8_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_16:
            if ( 0  !=
                castToFloat<uint16_t>(
                        *out,
                        reinterpret_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        elementCount)) {
                LOGGW("failure in castToFloat<uint16_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_32:
            if ( 0  !=
                castToFloat<uint32_t>(
                        *out,
                        reinterpret_cast<uint32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        elementCount)) {
                LOGGW("failure in castToFloat<uint32_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_8:
            if ( 0  !=
                castToFloat<int8_t>(
                        *out,
                        reinterpret_cast<int8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        elementCount)) {
                LOGGW("failure in castToFloat<int8_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_16:
            if ( 0  !=
                castToFloat<int16_t>(
                        *out,
                        reinterpret_cast<int16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        elementCount)) {
                LOGGW("failure in castToFloat<int16_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_32:
            if ( 0  !=
                castToFloat<int32_t>(
                        *out,
                        reinterpret_cast<int32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        elementCount)) {
                LOGGW("failure in castToFloat<int32_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_BOOL_8:
            if ( 0  !=
                castToFloat<uint8_t>(
                        *out,
                        reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                        elementCount)) {
                LOGGW("failure in castToFloat<bool>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_FLOAT_32:
            {
                float * src = (float*)(QNN_TENSOR_GET_CLIENT_BUF(tensor).data);
                float * dst = (*out);
                LOGGI("dump output tensor:\n");
                for (size_t i = 0; i < elementCount; i++) {
                    LOGGI("%.2f\t", src[i]);
                    dst[i] = src[i];
                }
            }
            break;

        default:
            LOGGW("datatype not supported yet!");
            result =  1 ;
            break;
    }
    if ( 0  != result) {
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
int qnn_io_tensor::convertAndWriteOutputTensorInFloat(
        Qnn_Tensor_t *output,
        std::vector<std::string> outputPaths,
        std::string fileName,
        size_t outputBatchSize) {
    if (nullptr == output) {
        LOGGW("output is nullptr");
        return  1 ;
    }

    auto returnStatus =  0 ;
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
    float *floatBuffer = nullptr;
    returnStatus = convertToFloat(&floatBuffer, output);
    if ( 0  != returnStatus) {
        LOGGW("failure in convertToFloat");
        return  1 ;
    }
    uint8_t *bufferToWrite = reinterpret_cast<uint8_t *>(floatBuffer);
    if ( 0  !=
        writeBatchDataToFile(
                outputPaths, fileName, dims, QNN_DATATYPE_FLOAT_32, bufferToWrite,
                outputBatchSize)) {
        LOGGW("failure in writeBatchDataToFile");
        returnStatus =  1 ;
    }
    if (nullptr != floatBuffer) {
        LOGGD("freeing floatBuffer");
        free(floatBuffer);
        floatBuffer = nullptr;
    }
    return returnStatus;
}


// Helper method to write out output. There is no de-quantization here.
// Just write output as is to files.
int qnn_io_tensor::writeOutputTensor(Qnn_Tensor_t *output,
                                                           std::vector<std::string> outputPaths,
                                                           std::string fileName,
                                                           size_t outputBatchSize) {
    if (nullptr == output) {
        LOGGW("output is nullptr");
        return  1 ;
    }
    auto returnStatus =  0 ;
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
    uint8_t *bufferToWrite = reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(output).data);
    if ( 0  !=
        writeBatchDataToFile(outputPaths,
                                       fileName,
                                       dims,
                                       QNN_TENSOR_GET_DATA_TYPE(output),
                                       bufferToWrite,
                                       outputBatchSize)) {
        LOGGW("failure in writeBatchDataToFile");
        returnStatus =  1 ;
    }
    return returnStatus;
}


// write out all output tensors to files. If output_data_type is float,
// then all outputs will be raw floats regardless of what the model outputs.
// If the output_data_type is native, then output is written as produced by the model.
// Also, for native option, a json with quantization parameters is written out.
// If output_data_type is float_and_native, both above are done.
// If the output in the graph is float, then output_data_type has no effect.
int qnn_io_tensor::writeOutputTensors(uint32_t graphIdx,
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
        return  1 ;
    }
    if (graphsCount > 1) {
        if (nullptr != graphName && strlen(graphName) > 0) {
            outputPath += ("/" + std::string(graphName));
        } else {
            outputPath += ("/" + std::string("Graph_") +
                           std::to_string(graphIdx));
        }
    }
    auto returnStatus =  0 ;
    std::vector<std::string> outputPaths;
    for (size_t idx = 0; idx < numInputFilesPopulated; idx++) {
        std::string output = outputPath + ("/" + std::string("Result_") +
                                           std::to_string(startIdx + idx));
        outputPaths.push_back(output);
    }
    for (size_t outputIdx = 0; outputIdx < numOutputs; outputIdx++) {
        LOGGD("Writing output for outputIdx: %d", outputIdx);
        GGML_JNI_NOTIFY("Writing output for outputIdx: %d", outputIdx);
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
            if ( 0  == returnStatus) {
                returnStatus = writeOutputTensor(
                        &(outputs[outputIdx]), outputPaths, outputFileNative, outputBatchSize);
            }
        }
    }
    return returnStatus;
}


// Helper method to allocate a buffer and copy data to it.
int qnn_io_tensor::allocateAndCopyBuffer(uint8_t **buffer, Qnn_Tensor_t *tensor) {
    if (nullptr == tensor) {
        return  1 ;
    }
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(tensor), QNN_TENSOR_GET_RANK(tensor));
    int datautilStatus = 0;
    size_t length;
    std::tie(datautilStatus, length) =
            calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE(tensor));
    if (datautilStatus !=  0 ) {
        return  1 ;
    }
    if ( 0  != allocateBuffer(buffer, dims, QNN_TENSOR_GET_DATA_TYPE(tensor))) {
        LOGGW("failure in allocateBuffer");
        return  1 ;
    }
    memscpy(*buffer,
                           length * sizeof(uint8_t),
                           QNN_TENSOR_GET_CLIENT_BUF(tensor).data,
                           length * sizeof(uint8_t));
    return 0;
}


int qnn_io_tensor::fillDims(std::vector<size_t> &dims, uint32_t *inDimensions, uint32_t rank) {
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


class qnn_implementation {
public:
    using BackendIdType = decltype(QnnInterface_t{}.backendId);

    explicit qnn_implementation(const std::string & lib_path, const std::string & backend_name, const std::string & model_name) :
    _lib_path(std::move(lib_path)) ,
    _backend_name(std::move(backend_name)),
    _model_name(std::move(model_name))
    {};

    ~qnn_implementation() {
        //TODO:
        _input_tensors.clear();
        _output_tensors.clear();
    }

    int qnn_init(const QnnSaver_Config_t ** saver_config);

    int qnn_finalize();

    const qnn_interface & get_qnn_interface() {
        if (!_qnn_interface.is_loaded()) {
            LOGGW("pls check why _qnn_interface is not loaded\n");
        }
        return _qnn_interface;
    }


    const QNN_INTERFACE_VER_TYPE & get_qnn_raw_interface () {
        if (!_qnn_interface.is_loaded()) {
            LOGGW("pls check why _qnn_interface is not loaded\n");
        }
        return _qnn_raw_interface;
    }

    const QNN_SYSTEM_INTERFACE_VER_TYPE & get_qnn_raw_system_interface () {
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


    int init_qnn_model(const char* graph_name,
                       bool debug,
                       uint8_t do_node_validation              = 1,
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

        QnnHtpDevice_Infrastructure_t * htp_infra = static_cast<QnnHtpDevice_Infrastructure_t *>(device_infra);
        QnnHtpDevice_PerfInfrastructure_t * htp_perfinfra = &htp_infra->perfInfra;
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
            const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = {&rpc_pollingTime, NULL};
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
        powerConfig.option                              = QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_DCVS_V3;
        powerConfig.dcvsV3Config.dcvsEnable             = 0;
        powerConfig.dcvsV3Config.setDcvsEnable          = 1;
        powerConfig.dcvsV3Config.contextId              = _qnn_power_configid;
        powerConfig.dcvsV3Config.powerMode              = QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_PERFORMANCE_MODE;
        powerConfig.dcvsV3Config.setSleepLatency        = 1; // True to consider Latency parameter otherwise False
        powerConfig.dcvsV3Config.setBusParams           = 1; // True to consider Bus parameter otherwise False
        powerConfig.dcvsV3Config.setCoreParams          = 1; // True to consider Core parameter otherwise False
        powerConfig.dcvsV3Config.sleepDisable           = 0; // True to consider sleep/LPM modes, False to enable
        powerConfig.dcvsV3Config.setSleepDisable        = 0; // True to consider sleep disable/enable parameter otherwise False
        // Set Sleep latency parameter
        uint32_t latencyValue                           = 40;
        powerConfig.dcvsV3Config.sleepLatency           = latencyValue; // range 40-2000 micro sec
        // set Bus Clock Parameters (refer QnnHtpPerfInfrastructure_VoltageCorner_t enum)
        powerConfig.dcvsV3Config.busVoltageCornerMin    = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.busVoltageCornerMax    = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        // set Core Clock Parameters (refer QnnHtpPerfInfrastructure_VoltageCorner_t enum)
        powerConfig.dcvsV3Config.coreVoltageCornerMin   = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.coreVoltageCornerTarget= DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.coreVoltageCornerMax   = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
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
    int add_tensor(const char * node_name, Qnn_Tensor_t * p_tensor, bool b_save_tensor = true);

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
                 const char * name,
                 const char * packageName,
                 const char * type,
                 Qnn_Param_t * params,
                 uint32_t numOfParams,
                 const char ** inputNames,
                 uint32_t numOfInputs,
                 Qnn_Tensor_t * outputTensors,
                 uint32_t numOfOutputs);

    int get_tensor(const char *& node_name, const char *& tensor_name, Qnn_Tensor_t & tensor) {
        std::string map_entry = std::string(tensor_name);
        if (_tensors_map.find(tensor_name) == _tensors_map.end()) {
            LOGGW("tensor %s not found on node %s\n", map_entry.c_str(), node_name);
            return 1;
        }

        tensor = _tensors_map[map_entry];
        return 0;
    }

    int add_node(const Qnn_OpConfig_t & op_config) {
        return _qnn_interface.qnn_graph_add_node(_qnn_graph_handle, op_config);
    };

    int get_graphinfo_from_model(GraphInfoPtr_t ** graphs_info, uint32_t * num_graphsinfo);


    // compose all QNN graphs from prebuilt Qualcomm's dedicated MODEL.so which generated by Qualcomm's dedicated tool
    int compose_special_graph();

    int finalize_graph() {
        LOGGI("enter %s\n", __func__);

        int error = 0;
        LOGGD("qnn graph handle %s, %p\n", get_qnn_graph_name().c_str(), get_qnn_graph_handle());
        for (size_t graph_idx = 0; graph_idx < _graphs_count; graph_idx++) {
            LOGGD("graph handle %p\n", (*_graphs_info)[graph_idx].graph);
            error = _qnn_raw_interface.graphFinalize((*_graphs_info)[graph_idx].graph, _qnn_profile_handle, nullptr);
            if (QNN_GRAPH_NO_ERROR != error) {
                LOGGW("qnn graph finalize failure, error %d\n", error);
                GGML_JNI_NOTIFY("qnn graph finalize failure, error %d\n", error);
                //return 1;
            } else {
                LOGGW("qnn graph finalize successfully\n");
                GGML_JNI_NOTIFY("qnn graph finalize successfully\n");
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

    int execute_special_graph();

    int execute_common_graph(const std::vector<uint8_t *> & input_node_values,
                             std::vector<float *> & output_node_values,
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
    int load_system();
    int unload_system();

    int load_prebuit_qnn_model();
    int unload_prebuilt_qnn_model();

    int load_backend(std::string & lib_path, const QnnSaver_Config_t ** saver_config);
    int unload_backend();

    void set_qnn_raw_interface(QNN_INTERFACE_VER_TYPE & raw_interface) {
        _qnn_raw_interface = raw_interface;
    }

    void set_qnn_raw_system_interface(QNN_SYSTEM_INTERFACE_VER_TYPE & raw_interface) {
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

    bool _debug_tensor                              = false; // flag to indicate if requested graph is to be run in debug mode
    bool _do_node_validations                       = true;  // flag to indicate whether all add_node calls need to be validated
    //QnnLog_Level_t _qnn_log_level                   = QNN_LOG_LEVEL_DEBUG;
    QnnLog_Level_t _qnn_log_level                   = QNN_LOG_LEVEL_VERBOSE;

    ggml_qnn_profile_level _profile_level           = ggml_qnn_profile_level::profile_detail;

    qnn_interface _qnn_interface;

    void * _system_lib_handle                       = nullptr;
    void * _model_lib_handle                        = nullptr;

    Qnn_GraphHandle_t _qnn_graph_handle             = nullptr;

    Qnn_LogHandle_t _qnn_log_handle                 = nullptr;

    Qnn_ProfileHandle_t _qnn_profile_handle         = nullptr;

    Qnn_DeviceHandle_t _qnn_device_handle           = nullptr;

    Qnn_BackendHandle_t _qnn_backend_handle         = nullptr;

    Qnn_ContextHandle_t _qnn_context_handle         = nullptr;

    QnnSystemContext_Handle_t _qnn_system_handle    = nullptr;

    QnnHtpDevice_PerfInfrastructure_t * _qnn_htp_perfinfra  = nullptr;
    uint32_t _qnn_power_configid                            = 1;
    uint32_t _qnn_rpc_pollingtime                           = 9999; // 0-10000 us for high performing

    QNN_INTERFACE_VER_TYPE _qnn_raw_interface;
    QNN_SYSTEM_INTERFACE_VER_TYPE _qnn_raw_system_interface;

    ComposeGraphsHandleType_t _qnn_composegraphs_handle     = nullptr;
    FreeGraphsHandleType_t _qnn_freegraphs_handle           = nullptr;

    std::unordered_set<Qnn_MemHandle_t> _qnn_mem_set;

    GraphInfo_t ** _graphs_info                             = nullptr;
    uint32_t _graphs_count                                  = 0;

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

    qnn_io_tensor _io_tensor;

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
    if (!is_rpcmem_initialized()) {
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
        LOGGW("failed to register shared memory, error %d, %s\n", QNN_GET_ERROR_CODE(error), strerror(error));
        return 6;
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


int qnn_implementation::load_backend(std::string & lib_path, const QnnSaver_Config_t ** saver_config) {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    LOGGD("lib_path:%s\n", lib_path.c_str());

    void * lib_handle = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
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

    return 0;
}


int qnn_implementation::unload_backend() {
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


int qnn_implementation::load_system() {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;

    std::string system_lib_path = _lib_path + "libQnnSystem.so";
    LOGGD("system_lib_path:%s\n", system_lib_path.c_str());

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

    uint32_t num_providers = 0;
    const QnnSystemInterface_t ** provider_list = nullptr;
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


int qnn_implementation::unload_system() {
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


int qnn_implementation::load_prebuit_qnn_model() {

    LOGGD("model name %s\n", _model_name.c_str());
    if (_model_name.empty()) {
        LOGGD("model name is empty\n");
        return 0;
    }

    void * model_handle = dlopen(_model_name.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (nullptr == model_handle) {
        LOGGW("failed to load prebuilt qnn model:%s, error:%s\n", _model_name.c_str(), dlerror());
        return 1;
    }

    _model_lib_handle = model_handle;

    _qnn_composegraphs_handle = (ComposeGraphsHandleType_t)dlsym(_model_lib_handle, "QnnModel_composeGraphs");
    if (nullptr == _qnn_composegraphs_handle) {
        LOGGW("failed to load prebuilt qnn model:%s, error:%s\n", _model_name.c_str(), dlerror());
        dlclose(_model_lib_handle);
        _model_lib_handle = nullptr;
        return 2;
    }

    _qnn_freegraphs_handle = (FreeGraphsHandleType_t)dlsym(_model_lib_handle, "QnnModel_freeGraphsInfo");
    if (nullptr == _qnn_freegraphs_handle) {
        LOGGW("failed to load prebuilt qnn model:%s, error:%s\n", _model_name.c_str(), dlerror());
        dlclose(_model_lib_handle);
        _model_lib_handle = nullptr;
        return 3;
    }

    return 0;
}


int qnn_implementation::unload_prebuilt_qnn_model() {
    if (nullptr != _model_lib_handle) {
        dlclose(_model_lib_handle);
        _model_lib_handle           = nullptr;
        _qnn_composegraphs_handle   = nullptr;
        _qnn_freegraphs_handle      = nullptr;
    }

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
        //snprintf(reinterpret_cast<char *const>(s_qnn_jni_buf + len_content), JNI_BUF_LEN - len_content, "\n");
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
        LOGGW("failed to create device\n");
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

    std::vector<const QnnContext_Config_t*> temp_context_config; //TODO:now is empty because I don't know how to use QnnContext_Config_t currently
    _qnn_interface.qnn_context_create(_qnn_backend_handle, _qnn_device_handle, temp_context_config.empty() ? nullptr : temp_context_config.data(), &_qnn_context_handle);
    if (nullptr == _qnn_context_handle) {
        LOGGW("why failed to initialize qnn context\n");
        return 8;
    } else {
        LOGGD("initialize qnn context successfully\n");
    }

    _rpc_lib_handle = dlopen("libcdsprpc.so", RTLD_NOW | RTLD_LOCAL);
    if (nullptr == _rpc_lib_handle) {
        LOGGW("failed to load qualcomm's rpc lib, error:%s\n", dlerror());
        return 9;
    } else {
        LOGGD("load rpcmem lib successfully\n");
        set_rpcmem_initialized(true);
    }
    _pfn_rpc_mem_alloc  = reinterpret_cast<pfn_rpc_mem_alloc>(dlsym(_rpc_lib_handle, "rpcmem_alloc"));
    _pfn_rpc_mem_free   = reinterpret_cast<pfn_rpc_mem_free>(dlsym(_rpc_lib_handle, "rpcmem_free"));
    _pfn_rpc_mem_to_fd  = reinterpret_cast<pfn_rpc_mem_to_fd>(dlsym(_rpc_lib_handle, "rpcmem_to_fd"));
    if (nullptr == _pfn_rpc_mem_alloc || nullptr == _pfn_rpc_mem_free || nullptr == _pfn_rpc_mem_to_fd) {
        LOGGW("unable to access symbols in shared buffer. dlerror(): %s", dlerror());
        dlclose(_rpc_lib_handle);
        return 10;
    }

    LOGGD("leave qni_init\n");

    return 0;
}


int qnn_implementation::qnn_finalize() {
    int ret_status = 0;
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    ENTER_FUNC();

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
        GGML_JNI_NOTIFY("adding qnn graph node %s failed", name);
        FREE_MEMORY(nodeParams, inputs, outputs);
        return 4;
    } else {
        LOGGW("adding qnn graph node %s succesfully\n", name);
    }

    FREE_MEMORY(nodeParams, inputs, outputs);

    return 0;
}


int qnn_implementation::init_qnn_model(const char * graph_name, bool debug, uint8_t do_node_validation, const QnnGraph_Config_t ** graph_configs) {
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

    result = _qnn_raw_interface.graphCreate(_qnn_context_handle, graph_name, graph_configs, &_qnn_graph_handle);
    if (result != QNN_GRAPH_NO_ERROR || nullptr == _qnn_graph_handle) {
        LOGGW("failed to create graph in qnn context\n");
        return 3;
    } else {
        LOGGI("succeed to create graph %s, %p\n", graph_name, _qnn_graph_handle);
    }

    LEAVE_FUNC();
    return 0;
}


int qnn_implementation::get_graphinfo_from_model(GraphInfoPtr_t ** graphsInfo, uint32_t * numGraphsInfo) {
    int  err = 0;
    int  numModels = 1;

    ENTER_FUNC();

    *graphsInfo = (GraphInfo_t **)malloc(numModels * sizeof(GraphInfo_t *));
    if (nullptr == *graphsInfo) {
        LOGGW("malloc failure\n");
        return 1;
    }

    GraphInfo_t *graphArr = (GraphInfo_t *)malloc(numModels * sizeof(GraphInfo_t));
    if (nullptr == graphArr) {
        LOGGW("malloc failure\n");
        return 2;
    }

    for (uint32_t i = 0; i < numModels; i++) {
        graphArr[i].graph = _qnn_graph_handle;
        graphArr[i].graphName = ::strndup(_graph_name.c_str(), _graph_name.size());
        if (nullptr == graphArr[i].graphName) {
            LOGGW("graph name is null\n");
            return 3;
        }

        // allocate and add graph input/output TensorsWrapper. Note: no need to make deep copies of
        // the tensor's pointer members as they are already allocated on heap in the addTensor
        // function call.
        std::vector<Qnn_Tensor_t> graphInputTensors = get_graph_input_tensors();
        size_t numInputTensors                      = graphInputTensors.size();
        LOGGD("numInputTensors %d\n", numInputTensors);
        size_t inputTensorsSize                     = numInputTensors * sizeof(Qnn_Tensor_t);
        graphArr[i].inputTensors                    = (Qnn_Tensor_t *)malloc(inputTensorsSize);
        memscpy(graphArr[i].inputTensors, inputTensorsSize, graphInputTensors.data(), inputTensorsSize);
        graphArr[i].numInputTensors = (uint32_t)numInputTensors;
        // allocate and add graph outputTensors
        std::vector<Qnn_Tensor_t> graphOutputTensors = get_graph_output_tensors();
        size_t numOutputTensors                      = graphOutputTensors.size();
        LOGGD("numOutputTensors %d\n", numOutputTensors);
        size_t outputTensorsSize                     = numOutputTensors * sizeof(Qnn_Tensor_t);
        graphArr[i].outputTensors                    = (Qnn_Tensor_t *)malloc(outputTensorsSize);
        memscpy(graphArr[i].outputTensors, outputTensorsSize, graphOutputTensors.data(), outputTensorsSize);
        graphArr[i].numOutputTensors = (uint32_t)numOutputTensors;

        // have return object point to the populated graph struct
        (*graphsInfo)[i] = graphArr + i;

        // graph composition is complete by this stage, free if any cached tensors remaining
        free_cached_tensors();
    }

    *numGraphsInfo = numModels;

    LEAVE_FUNC();

    return err;
}


// compose all QNN graphs from prebuilt Qualcomm's dedicated MODEL.so which generated by Qualcomm's dedicated tool
int qnn_implementation::compose_special_graph() {
    ENTER_FUNC();

    int result = 0;

    if (nullptr == _qnn_composegraphs_handle) {
        LOGGW("no prebuilt QNN model\n");
        LEAVE_FUNC();
        return 0;
    }

    result = _qnn_composegraphs_handle(
               _qnn_backend_handle,
               _qnn_raw_interface,
                _qnn_context_handle,
                (const GraphConfigInfo_t **) nullptr,
                0,
                &_graphs_info,
                &_graphs_count,
                _debug_tensor,
                ggml_qnn_logcallback,
                _qnn_log_level);
    if (result != 0) {
        LOGGW("failed to compose graphs via prebuilt QNN mode:%s\n", _model_name.c_str());
        result = 1;
    }

    LEAVE_FUNC();

    return result;
}


// execute_special_graph() is currently used by qnn-sample-app's main.cpp.
// This function runs all the graphs present in model.so by reading
// inputs from input_list based files and writes output to .raw files.
int qnn_implementation::execute_special_graph() {
    ENTER_FUNC();
    int return_value                                = 0;
    bool read_success                               = false;
    std::string input_list_path                     = "/sdcard/kantv/raw_list.txt";
    std::string output_path                         = "/sdcard/kantv/out/";

    std::vector<std::string> input_list_paths;
    std::vector<std::vector<std::vector<std::string>>> input_file_lists;
    std::vector<std::unordered_map<std::string, uint32_t>> input_name_to_index;


    OutputDataType output_datatype                  = OutputDataType::FLOAT_ONLY;
    InputDataType input_datatype                    = InputDataType::FLOAT;

    split(input_list_paths, input_list_path, ',');


    std::tie(input_file_lists, input_name_to_index, read_success) = readInputLists(input_list_paths);
    if (!read_success) {
        LOGGW("can not read data from file\n");
        return 1;
    } else {
        LOGGD("succeed to read data from file\n", input_list_path.c_str());
    }
    LOGGD("input_file_lists.size() %d", input_file_lists.size());

    for (size_t graph_idx = 0; graph_idx < _graphs_count; graph_idx++) {
        LOGGD("Starting execution for graph_idx: %d", graph_idx);
        if (graph_idx >= input_file_lists.size()) {
            LOGGW("No Inputs available for: %d", graph_idx);
            return_value = 1;
            break;
        }

        Qnn_Tensor_t *inputs    = nullptr;
        Qnn_Tensor_t *outputs   = nullptr;
        if (0 != _io_tensor.setupInputAndOutputTensors(&inputs, &outputs, (*_graphs_info)[graph_idx])) {
            LOGGE("Error in setting up Input and output Tensors for graph_idx: %d", graph_idx);
            return_value = 1;
            break;
        }
        auto graphs_info = (*_graphs_info)[graph_idx];
        auto input_file_list = input_file_lists[graph_idx];
        if (!input_file_list.empty()) {
            size_t total_count = input_file_list[0].size();
            size_t input_file_index_offset = 0;
            while (input_file_index_offset < total_count) {
                int iotReturnStatus;
                size_t numInputFilesPopulated;
                size_t batchSize;
                std::tie(iotReturnStatus, numInputFilesPopulated, batchSize) =
                        _io_tensor.populateInputTensors(graph_idx,
                                                        input_file_list,
                                                        input_file_index_offset,
                                                        false,
                                                        input_name_to_index[graph_idx],
                                                        inputs,
                                                        graphs_info,
                                                        input_datatype);
                if (0 != iotReturnStatus) {
                    return_value = 1;
                }
                if (0 == return_value) {
                    LOGGD("successfully populated input tensors for graph_idx: %d", graph_idx);
                    GGML_JNI_NOTIFY("successfully populated input tensors for graph_idx: %d", graph_idx);
                    Qnn_ErrorHandle_t executeStatus = QNN_GRAPH_NO_ERROR;
                    executeStatus = _qnn_raw_interface.graphExecute(graphs_info.graph,
                                                                            inputs,
                                                                            graphs_info.numInputTensors,
                                                                            outputs,
                                                                            graphs_info.numOutputTensors,
                                                                            _qnn_profile_handle,
                                                                            nullptr);
                    if (QNN_GRAPH_NO_ERROR != executeStatus) {
                        return_value = 1;
                    }
                    if (0 == return_value) {
                        LOGGD("successfully executed graph_idx: %d ", graph_idx);
                        GGML_JNI_NOTIFY("successfully executed graph_idx: %d ", graph_idx);

                        if (0 !=
                                _io_tensor.writeOutputTensors(graph_idx,
                                                          input_file_index_offset,
                                                          graphs_info.graphName,
                                                          outputs,
                                                          graphs_info.numOutputTensors,
                                                          output_datatype,
                                                          _graphs_count,
                                                          output_path,
                                                          numInputFilesPopulated,
                                                          batchSize)) {
                            return_value = 1;
                        }
                    }
                    input_file_index_offset += numInputFilesPopulated;
                }
                if (0 != return_value) {
                    LOGGE("execution of graph: %d failure\n", graph_idx);
                    break;
                }
            }
        }
        _io_tensor.tearDownInputAndOutputTensors(inputs, outputs, graphs_info.numInputTensors, graphs_info.numOutputTensors);
        inputs = nullptr;
        outputs = nullptr;
        if (0 != return_value) {
            break;
        }
    }

    freeGraphsInfo(&_graphs_info, _graphs_count);
    _graphs_info = nullptr;

    LEAVE_FUNC();

    return return_value;
}


int qnn_implementation::execute_common_graph(const std::vector<uint8_t *> & input_node_values,
                         std::vector<float *> & output_node_values,
                         std::vector<size_t> output_node_sizes) {

    int result                      = 0;
    float * output_tmp              = nullptr;

    InputDataType input_datatype    = InputDataType::FLOAT;
    OutputDataType output_datatype  = OutputDataType::FLOAT_ONLY;

    LOGGI("enter %s\n", __func__);

    LOGGD("graph count %d\n", _graphs_count);

    if (1 != _graphs_count) {
        LOGGW("only support one graph current\n");
        result = 1;
        return 1;
    }

    for (size_t graph_idx = 0; graph_idx < _graphs_count; graph_idx++) {
        LOGGI("starting setup input and output tensor for graph : %zu", graph_idx);

        Qnn_Tensor_t *inputs    = nullptr;
        Qnn_Tensor_t *outputs   = nullptr;
        if (0 != _io_tensor.setupInputAndOutputTensors(&inputs, &outputs, (*_graphs_info)[graph_idx])) {
            LOGGW("failed to setting up input and output QNN Tensors for graph_idx: %zu", graph_idx);
            result  = 2;
        }

        auto graph_info = (*_graphs_info)[graph_idx];
        std::vector<uint8_t *> input_vec;

        if (0 == result) {
            for (auto input_node_value : input_node_values) {
                input_vec.push_back(input_node_value);
            }

            if (0 != _io_tensor.populateInputTensors(graph_idx, input_vec, inputs, graph_info, input_datatype)) {
                LOGGE("failed to populateInputTensors");
                result  = 3;
            }
        }

        if (0 == result) {
            LOGGD("successfully populated input tensors for graph_idx: %zu", graph_idx);
            result  = _qnn_raw_interface.graphExecute(graph_info.graph,
                                                                    inputs,
                                                                    graph_info.numInputTensors,
                                                                    outputs,
                                                                    graph_info.numOutputTensors,
                                                                    _qnn_profile_handle,
                                                                    nullptr);


            if (0 != result) {
                LOGGE("graph execution failure\n");
                GGML_JNI_NOTIFY("graph execution failure");
                result = 4;
            } else {
                LOGGI("got it, qnn graph execution successfully\n"); //04-05-2024(April,5,2024),13:00, but the compute result is not correct
                GGML_JNI_NOTIFY("qnn graph execution successfully");
            }

            for (int i = 0; i < graph_info.numOutputTensors; ++i) {
                if (nullptr == output_node_values.at(i)) {
                    LOGGW("output node is null\n");
                    continue;
                }

                output_tmp              = nullptr;

                if (0 != result)
                    continue;

                if (0 !=  _io_tensor.converQnntensortoFloatBuffer(&(outputs[i]), &output_tmp)) {
                    LOGGE("error in convert output at idx %d", i);
                    result = 5;
                }

                if ((0 == result) && (nullptr != output_tmp)) {
                    memcpy(output_node_values.at(i), output_tmp, output_node_sizes[i] * sizeof(float));
                    free(output_tmp);
                    output_tmp = nullptr;
                }

            }
        }

        _io_tensor.tearDownInputAndOutputTensors(inputs, outputs, graph_info.numInputTensors, graph_info.numOutputTensors);

        inputs  = nullptr;
        outputs = nullptr;
    }

    if (0 != result) {
        LOGGE("execute common graph failure\n");
        GGML_JNI_NOTIFY("execute common graph failure");
    }
    LOGGI("leave %s\n", __func__);


    return result;
}


int qnn_implementation::finalize_qnn_model() {
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


extern "C" int ggml_qqn_debug_composegraphs(GraphInfoPtr_t ** graphsInfo,uint32_t * numGraphsInfo);
// this function is used to troubleshooting issue in
// PoC-S26: offload a simple f32 2x2 matrix addition operation to QNN CPU backend
// https://github.com/zhouwg/kantv/issues/121
int qnn_implementation::run_qnn_matrix() {
    LOGGD("enter run_qnn_matrix\n");

    int result = 0;

    //attention here
    int32_t input_matrix[2][4]  = { {1, 1, 1, 1}, {2, 2, 2, 2}};
    float   output_matrix[1][4] = { {1.0,1.0,1.0,1.0} };

    Qnn_Tensor_t tensor_0               = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_1               = QNN_TENSOR_INIT;

    input_node_values.push_back((uint8_t*)input_matrix[0]);
    input_node_values.push_back((uint8_t*)input_matrix[1]);

    output_node_values.push_back(output_matrix[0]);

    output_node_sizes.push_back(4); // 2 * 2 = 4

    init_qnn_model("qnn_matrix_addition", true);

    //step-1
    //compose_special_graph();
    //step-2
    //ggml_qqn_debug_composegraphs(&_graphs_info, &_graphs_count);
    //get_graphinfo_from_model(&_graphs_info, &_graphs_count);
    //finalize_graph();
    //execute_special_graph();

    //step-3
    //attention here
    uint32_t dimensions_input_0[] = {2, 2};
    uint32_t dimensions_input_1[] = {2, 2};
    uint32_t dimensions_output[]  = {2, 2};
    uint32_t temp_buf[1024]       = { 0 };

    tensor_0 =  {
                   .version= QNN_TENSOR_VERSION_1,
                   {.v1= {
                     .id=0,
                     .name= "tensor_0",
                     .type= QNN_TENSOR_TYPE_APP_WRITE,
                     .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                     .dataType= QNN_DATATYPE_FLOAT_32,
                     .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                        QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                        {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                     .rank= 2,
                     .dimensions=dimensions_input_0,
                     .memType= QNN_TENSORMEMTYPE_RAW,
                     {.clientBuf= { .data=nullptr,
                                    .dataSize=0}}}}
    };

    tensor_1 =  {
                   .version= QNN_TENSOR_VERSION_1,
                   {.v1= {
                     .id=0,
                     .name= "tensor_1",
                     .type= QNN_TENSOR_TYPE_APP_WRITE,
                     .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                     .dataType= QNN_DATATYPE_FLOAT_32,
                     .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                        QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                        {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                     .rank= 2,
                     .dimensions=dimensions_input_1,
                     .memType= QNN_TENSORMEMTYPE_RAW,
                     {.clientBuf= { .data=nullptr,
                                    .dataSize=0}}}}
    };

    Qnn_Param_t qnn_params[] = {
            {.paramType=QNN_PARAMTYPE_TENSOR,
                    .name="node_param_0",
                    {.tensorParam=(Qnn_Tensor_t) {
                            .version= QNN_TENSOR_VERSION_1,
                            {.v1= {
                                    .id=0,
                                    .name= "param_0",
                                    .type= QNN_TENSOR_TYPE_STATIC,
                                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                    .dataType= QNN_DATATYPE_UINT_32,
                                    .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                                       QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                                       {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                                    .rank= 2,
                                    .dimensions=dimensions_input_0,
                                    .memType= QNN_TENSORMEMTYPE_RAW,
                                    {.clientBuf= { .data=(uint8_t*)temp_buf,
                                            .dataSize=16}}}}}}},

            {.paramType=QNN_PARAMTYPE_TENSOR,
                    .name="node_param_1",
                    {.tensorParam=(Qnn_Tensor_t) {
                            .version= QNN_TENSOR_VERSION_1,
                            {.v1= {
                                    .id=0,
                                    .name= "param_1",
                                    .type= QNN_TENSOR_TYPE_STATIC,
                                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                    .dataType= QNN_DATATYPE_UINT_32,
                                    .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                                       QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                                       {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                                    .rank= 2,
                                    .dimensions=dimensions_input_1,
                                    .memType= QNN_TENSORMEMTYPE_RAW,
                                    {.clientBuf= { .data=(uint8_t*)temp_buf + 32,
                                            .dataSize=16}}}}}}},

    };

    const char * inputs[] = {
        "tensor_0",
        "tensor_1"
    };

    Qnn_Tensor_t outputs[] = {
    (Qnn_Tensor_t) {
          .version= QNN_TENSOR_VERSION_1,
          {.v1= {
            .id=0,
            .name= "tensor_2",
            .type= QNN_TENSOR_TYPE_APP_READ,
            .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
            .dataType= QNN_DATATYPE_FLOAT_32,
            .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                               QNN_QUANTIZATION_ENCODING_UNDEFINED,
                               {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
            .rank= 2,
            .dimensions= dimensions_output,
            .memType= QNN_TENSORMEMTYPE_RAW,
            {.clientBuf= { .data=nullptr,
                           .dataSize=0}}}}}
    };

    Qnn_Param_t context_params[] = {}; // 04-05-2024(April-5,2024), to make QNN SDK happy
    add_tensor("qnn_matrix_addtition", &tensor_0);
    add_tensor("qnn_matrix_addtition", &tensor_1);
    add_node(QNN_OPCONFIG_VERSION_1, // Op_Config_t Version
                         get_qnn_graph_name().c_str(), // Node Name
                         QNN_OP_PACKAGE_NAME_QTI_AISW, // Package Name,  must be "qti.aisw" otherwise error occurs
                         QNN_OP_ELEMENT_WISE_ADD, // Qnn Node Type,
                         context_params, // Node Params
                         0, // Num Node Params
                         inputs, // Input Tensor Names
                         2, // Num Input Tensor Names
                         outputs, // Output Tensors
                         1// Num Output Tensors
    );
    get_graphinfo_from_model(&_graphs_info, &_graphs_count);

    LOGGD("graphs count %d\n", _graphs_count);
    finalize_graph();

    //TODO:hardcode in PoC stage, shape of input matrix is 2x2
    for (size_t i = 0; i < input_node_values.size(); i++) {
        if (0 == i) {
            LOGGD("input matrix A:\n");
            GGML_JNI_NOTIFY("input matrix A:");
        } else if (1 == i) {
            LOGGD("input matrix B:\n");
            GGML_JNI_NOTIFY("input matrix B:");
        }
        uint32_t *temp = (uint32_t*)input_node_values.at(i);
        LOGGD("%d \t %d\n", temp[0], temp[1]);
        LOGGD("%d \t %d\n", temp[2], temp[3]);
        GGML_JNI_NOTIFY("%d \t %d\n", temp[0], temp[1]);
        GGML_JNI_NOTIFY("%d \t %d\n", temp[2], temp[3]);
    }

    result = execute_common_graph(input_node_values, output_node_values, output_node_sizes);
    if (0 == result) {
        //TODO:hardcode in PoC stage, shape of output matrix is 2x2
        LOGGD("output matrix:\n");
        GGML_JNI_NOTIFY("output matrix:");
        for (size_t i = 0; i < output_node_values.size(); i++) {
            float *temp = output_node_values.at(i);
            LOGGD("%.2f \t %.2f\n", temp[0], temp[1]);
            LOGGD("%.2f \t %.2f\n", temp[2], temp[3]);
            GGML_JNI_NOTIFY("%.2f \t %.2f\n", temp[0], temp[1]);
            GGML_JNI_NOTIFY("%.2f \t %.2f\n", temp[2], temp[3]);
        }
        LOGGD("\n");
    }

    input_node_values.clear();
    output_node_values.clear();

    finalize_qnn_model();

    LOGGD("leave run_qnn_matrix\n");

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


//qnn_implementation qnn_backend = qnn_implementation("/data/data/com.cdeos.kantv/", "libQnnCpu.so", "");
int qnn_matrix(int n_backend_type, int n_op_type) {
    int result                      = 0;
    std::string graph_name          = "qnn_matrix";
    uint32_t matrix_input_0[]       = {1, 2, 3, 4};
    uint32_t matrix_input_1[]       = {1, 2, 3, 4};


    auto is_io_tensor = [](Qnn_TensorType_t type) {
        return type < QNN_TENSOR_TYPE_STATIC;
    };
    uint8_t * qnn_buffer                = nullptr;
    Qnn_Tensor_t tensor_0               = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_1               = QNN_TENSOR_INIT;
    Qnn_QuantizeParams_t quantize_param = QNN_QUANTIZE_PARAMS_INIT;
    Qnn_OpConfig_t qnn_opconfig         = QNN_OPCONFIG_INIT;

    LOGGD("enter qnn_matrix\n");
    LOGGI("qnn matrix addition operation, backend type:%d, op type:%d\n", n_backend_type, n_op_type);
    GGML_JNI_NOTIFY("qnn matrix addition operation, backend_type:%d, op type:%d", n_backend_type, n_op_type);

    qnn_implementation qnn_backend = qnn_implementation("/data/data/com.cdeos.kantv/", "libQnnCpu.so", "");
    result = qnn_backend.qnn_init(nullptr);
    if (0 != result) {
        LOGGW("init qnn subsystem failed, pls check why\n");
        result = 1;
        return 1;
    }
    QNN_INTERFACE_VER_TYPE qnn_raw_interface                = qnn_backend.get_qnn_raw_interface();
    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface  = qnn_backend.get_qnn_raw_system_interface();
    qnn_interface qnn_interface                             = qnn_backend.get_qnn_interface();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        result = 2;
        goto failure;
    }
    qnn_buffer = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(8192, 4));
    if (nullptr == qnn_buffer) {
        LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
        goto failure;
    } else {
        LOGGD("alloc rpcmem successfully\n");
    }

    qnn_backend.run_qnn_matrix();

failure:
    qnn_backend.unregister_rpcmem();
    qnn_backend.free_rpcmem(qnn_buffer);
    qnn_backend.qnn_finalize();

    LOGGD("leave qnn_matrix\n");

    return result;
}


//TODO:
// https://github.com/zhouwg/kantv/issues/121
// PoC-S27:  mapping ggml_tensor to QNN_tensor and offload a simple 2x2 matrix addition operation to QNN CPU backend
int qnn_ggml(int n_backend_type, int n_ggml_op_type) {
    uint32_t i                                  = 0;
    uint32_t j                                  = 0;
    int error                                   = 0;
    int result                                  = 0;
    std::string graph_name                      = "qnn_matrix";

    const int sizey                             = 2;
    const int sizex                             = 2;
    size_t ctx_size                             = 0;
    struct ggml_context * ctx                   = nullptr;
    struct ggml_tensor  * m11                   = nullptr;
    struct ggml_tensor  * m12                   = nullptr;
    struct ggml_tensor  * m2                    = nullptr;
    struct ggml_cgraph  * gf                    = nullptr;
    std::vector<uint8_t> work_buffer;
    const ggml_type qtype           = GGML_TYPE_F32;
    struct ggml_init_params params = {
            /*.mem_size   =*/ 0,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };

    uint8_t * qnn_buffer                        = nullptr;

    LOGGD("enter qnn_ggml\n");
    LOGGI("[%s], backend type:%d, op type:%d\n", __func__, n_backend_type, n_ggml_op_type);
    GGML_JNI_NOTIFY("[%s], backend_type:%d, ggml op type:%d", __func__, n_backend_type, n_ggml_op_type);

    qnn_implementation qnn_backend = qnn_implementation("/data/data/com.cdeos.kantv/", "libQnnCpu.so", "");
    error  = qnn_backend.qnn_init(nullptr);
    if (0 != error) {
        LOGGW("init qnn subsystem failed, pls check why\n");
        result = 1;
        return 1;
    }

    QNN_INTERFACE_VER_TYPE qnn_raw_interface                = qnn_backend.get_qnn_raw_interface();
    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface  = qnn_backend.get_qnn_raw_system_interface();
    qnn_interface qnn_interface                             = qnn_backend.get_qnn_interface();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        result = 2;
    }

    qnn_buffer = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(8192, 4));
    if (nullptr == qnn_buffer) {
        LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
        goto failure;
    } else {
        LOGGD("alloc rpcmem successfully\n");
    }

    // GGML matrix addition operation
    ctx_size        += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size        += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size        += ggml_row_size(qtype,         sizex*sizey);
    ctx_size        += ggml_row_size(qtype,         sizex*sizey);
    ctx_size        += 1024 * 1024 * 16;
    params.mem_size =  ctx_size;
    ctx             =  ggml_init(params);
    if (!ctx) {
        LOGGW("ggml_init failure\n");
        return 1;
    }
    m11             = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m11, 1.0f);
    m12             = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m12, 2.5f);
    m2              = ggml_add(ctx, m11, m12);
    gf              = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, m2);
    ggml_graph_compute_helper(work_buffer, gf, 4);
    TENSOR_DUMP(m11);
    TENSOR_DUMP(m12);
    TENSOR_DUMP(m2);
    LOGGD("finish ggml matrix addition operation\n");
    // end GGML matrix addition operation


failure:
    qnn_backend.unregister_rpcmem();
    qnn_backend.free_rpcmem(qnn_buffer);
    qnn_backend.qnn_finalize();

    LOGGD("leave qnn_ggml\n");

    return result;
}


//#include "Inception_v3.cpp"
