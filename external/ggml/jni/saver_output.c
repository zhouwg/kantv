//==============================================================================
//
//  Copyright (c) 2020-2024 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.

//  saver_output.c is generated automatically by Qualcomm's dedicated tool
//
//  this customized saver_output.c is used to troubleshooting issue in
//  PoC-S26: offload a simple f32 2x2 matrix addition operation to QNN CPU backend
//  https://github.com/zhouwg/kantv/issues/121
//
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "QnnInterface.h"

#include "ggml-jni.h"

#define VALIDATE(res, value) \
   do { \
      if (res == 0 || res == QNN_COMMON_ERROR_NOT_SUPPORTED) \
      { \
         res = value; \
         if (res != 0) \
         { \
            if (res == QNN_COMMON_ERROR_NOT_SUPPORTED) \
            { \
               LOGGD("WARNING! Line %d QNN feature/API not supported\n", __LINE__); \
               GGML_JNI_NOTIFY("WARNING! Line %d QNN feature/API not supported\n", __LINE__); \
            } else { \
               LOGGD("ERROR! Line %d with error value: %d\n", __LINE__, (unsigned int)error); \
            } \
         } \
      } \
   } \
   while(0)


static void qnn_saver_logcallback(const char* fmt,
                                 QnnLog_Level_t level,
                                 uint64_t timestamp,
                                 va_list argp) {

    static unsigned char s_qnn_saver_buf[JNI_BUF_LEN];

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
        int len_content = 0;
        memset(s_qnn_saver_buf, 0, JNI_BUF_LEN);
        len_content = vsnprintf(s_qnn_saver_buf, JNI_BUF_LEN, fmt, argp);
        snprintf((s_qnn_saver_buf + len_content), JNI_BUF_LEN - len_content, "\n");
        LOGGD("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_saver_buf);
        //if (level <= QNN_LOG_LEVEL_INFO)
        {
            GGML_JNI_NOTIFY("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_saver_buf);
        }
    }
}


int qnn_saver_main(int argc, char **argv) {
    LOGGI("enter %s", __func__);
    GGML_JNI_NOTIFY("enter %s", __func__);
    Qnn_ErrorHandle_t error = 0;
    QnnLog_Level_t logLevel = QNN_LOG_LEVEL_VERBOSE;
    int logging = 1;
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (!strcmp("--logging", arg) || !strcmp("-l", arg)) {
            logging = 1;
            if (i + 1 == argc) {
                printf("No log level provided, defaulting to QNN_LOG_LEVEL_ERROR\n");
                break;
            }
            char *value = argv[++i];
            if (!strcmp("error", value)) {
                logLevel = QNN_LOG_LEVEL_ERROR;
            } else if (!strcmp("warn", value)) {
                logLevel = QNN_LOG_LEVEL_WARN;
            } else if (!strcmp("info", value)) {
                logLevel = QNN_LOG_LEVEL_INFO;
            } else if (!strcmp("debug", value)) {
                logLevel = QNN_LOG_LEVEL_DEBUG;
            } else if (!strcmp("verbose", value)) {
                logLevel = QNN_LOG_LEVEL_VERBOSE;
            } else {
                printf("WARNING: Unknown log level provided: %s, defaulting to QNN_LOG_LEVEL_ERROR\n",
                       value);
            }
        } else {
            printf("Usage: %s [options]\n\n"
                   "-l <level>, --logging <level>      Enable logging, acceptable levels are: error,warn,info,debug,verbose\n",
                   argv[0]);
            return -1;
        }
    }

    LOGGD("log level %d\n", logLevel);
    FILE *fp = fopen("/sdcard/kantv/params.bin", "rb");
    if (!fp) {
        error = -1;
        LOGGI("ERROR! Could not open params.bin, ensure this file is in the current working directory when executing this program\n");
        GGML_JNI_NOTIFY("ERROR! Could not open params.bin, ensure this file is in the current working directory when executing this program\n");
        return error;
    }

    const QnnInterface_t **providerList = NULL;
    uint32_t numProviders;
    VALIDATE(error, QnnInterface_getProviders(&providerList, &numProviders));
    LOGGD("numProviders %d\n", numProviders);
    GGML_JNI_NOTIFY("numProviders %d\n", numProviders);
    for (int idx = 0; idx < numProviders; idx++) {
        LOGGD("backend name %s\n", providerList[idx]->providerName);
        GGML_JNI_NOTIFY("backend name %s\n", providerList[idx]->providerName);
    }
    QNN_INTERFACE_VER_TYPE interface = providerList[0]->QNN_INTERFACE_VER_NAME;

    Qnn_LogHandle_t loghandle = NULL;
    if (logging) {
        VALIDATE(error, interface.logCreate(qnn_saver_logcallback, logLevel, &loghandle));
    }
    //VALIDATE(error, interface.propertyHasCapability((QnnProperty_Key_t) 304)); //QNN_PROPERTY_GRAPH_SUPPORT_NULL_INPUTS
    VALIDATE(error, interface.propertyHasCapability((QnnProperty_Key_t) QNN_PROPERTY_GRAPH_SUPPORT_NULL_INPUTS));

    const QnnBackend_Config_t *backend_0_config_0[] = {NULL};
    Qnn_BackendHandle_t backend_0;
    VALIDATE(error, interface.backendCreate(loghandle, backend_0_config_0, &backend_0));

    const QnnDevice_Config_t *device_0_config_0[] = {NULL};
    Qnn_DeviceHandle_t device_0;
    VALIDATE(error, interface.deviceCreate(loghandle, device_0_config_0, &device_0));

    const QnnContext_Config_t *context_0_config_0[] = {NULL};
    Qnn_ContextHandle_t context_0;
    VALIDATE(error, interface.contextCreate(backend_0, device_0, context_0_config_0, &context_0));

    const QnnGraph_Config_t *context_0_convReluModel_config_0[] = {NULL};
    Qnn_GraphHandle_t context_0_convReluModel;
    VALIDATE(error,
             interface.graphCreate(context_0, "convReluModel", context_0_convReluModel_config_0,
                                   &context_0_convReluModel));

    //how to compose qnn graph

    //step-1:
    uint32_t context_0_convReluModel_tensor_0_dims[] = {1, 299, 299, 3};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647/*QNN_DEFINITION_UNDEFINED*/,
            (Qnn_QuantizationEncoding_t) 2147483647/*QNN_QUANTIZATION_ENCODING_UNDEFINED*/, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_0_clientBuf = {NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_tensor_0_v1 = {0, "input_0",
                                                          (Qnn_TensorType_t) 0/*QNN_TENSOR_TYPE_APP_WRITE*/,
                                                          0/*QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER*/,
                                                          (Qnn_DataType_t) 562/*QNN_DATATYPE_FLOAT_32*/,
                                                          context_0_convReluModel_tensor_0_quantizeParams,
                                                          4, context_0_convReluModel_tensor_0_dims,
                                                          (Qnn_TensorMemType_t) 0/*QNN_TENSORMEMTYPE_RAW*/,
                                                          context_0_convReluModel_tensor_0_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_tensor_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_0_v1};
    VALIDATE(error, interface.tensorCreateGraphTensor(context_0_convReluModel,
                                                      &context_0_convReluModel_tensor_0));




    //step-2:
    uint32_t context_0_convReluModel_tensor_1_dims[] = {3, 3, 3, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_1_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static float context_0_convReluModel_tensor_1_data[864];
    fread(context_0_convReluModel_tensor_1_data, 4, 864, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_1_clientBuf = {
            (void *) context_0_convReluModel_tensor_1_data, 3456};
    Qnn_TensorV1_t context_0_convReluModel_tensor_1_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_weight",
                                                          (Qnn_TensorType_t) 4/*QNN_TENSOR_TYPE_STATIC*/,
                                                          0/*QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER*/,
                                                          (Qnn_DataType_t) 562/*QNN_DATATYPE_FLOAT_32*/,
                                                          context_0_convReluModel_tensor_1_quantizeParams,
                                                          4, context_0_convReluModel_tensor_1_dims,
                                                          (Qnn_TensorMemType_t) 0,
                                                          context_0_convReluModel_tensor_1_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_tensor_1 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_1_v1};
    VALIDATE(error, interface.tensorCreateGraphTensor(context_0_convReluModel,
                                                      &context_0_convReluModel_tensor_1));



    //step-3:
    uint32_t context_0_convReluModel_tensor_2_dims[] = {32};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_2_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static float context_0_convReluModel_tensor_2_data[32];
    fread(context_0_convReluModel_tensor_2_data, 4, 32, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_2_clientBuf = {
            (void *) context_0_convReluModel_tensor_2_data, 128};
    Qnn_TensorV1_t context_0_convReluModel_tensor_2_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_bias",
                                                          (Qnn_TensorType_t) 4/*QNN_TENSOR_TYPE_STATIC*/,
                                                          0,
                                                          (Qnn_DataType_t) 562/*QNN_DATATYPE_FLOAT_32*/,
                                                          context_0_convReluModel_tensor_2_quantizeParams,
                                                          1, context_0_convReluModel_tensor_2_dims,
                                                          (Qnn_TensorMemType_t) 0,
                                                          context_0_convReluModel_tensor_2_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_tensor_2 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_2_v1};
    VALIDATE(error, interface.tensorCreateGraphTensor(context_0_convReluModel,
                                                      &context_0_convReluModel_tensor_2));



    //step-4:
    uint32_t context_0_convReluModel_tensor_3_dims[] = {2};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_3_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static uint32_t context_0_convReluModel_tensor_3_data[2];
    fread(context_0_convReluModel_tensor_3_data, 4, 2, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_3_clientBuf = {
            (void *) context_0_convReluModel_tensor_3_data, 8};
    Qnn_TensorV1_t context_0_convReluModel_tensor_3_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_dilation",
                                                          (Qnn_TensorType_t) 4/*QNN_TENSOR_TYPE_STATIC*/, 0,
                                                          (Qnn_DataType_t) 306/*QNN_DATATYPE_UINT_32*/,
                                                          context_0_convReluModel_tensor_3_quantizeParams,
                                                          1, context_0_convReluModel_tensor_3_dims,
                                                          (Qnn_TensorMemType_t) 0,
                                                          context_0_convReluModel_tensor_3_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_tensor_3 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_3_v1};
    VALIDATE(error, interface.tensorCreateGraphTensor(context_0_convReluModel,
                                                      &context_0_convReluModel_tensor_3));




    //step-5:
    uint32_t context_0_convReluModel_tensor_4_dims[] = {2, 2};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_4_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static uint32_t context_0_convReluModel_tensor_4_data[4];
    fread(context_0_convReluModel_tensor_4_data, 4, 4, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_4_clientBuf = {
            (void *) context_0_convReluModel_tensor_4_data, 16};
    Qnn_TensorV1_t context_0_convReluModel_tensor_4_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_pad_amount",
                                                          (Qnn_TensorType_t) 4/*QNN_TENSOR_TYPE_STATIC*/, 0,
                                                          (Qnn_DataType_t) 306/*QNN_DATATYPE_UINT_32*/,
                                                          context_0_convReluModel_tensor_4_quantizeParams,
                                                          2, context_0_convReluModel_tensor_4_dims,
                                                          (Qnn_TensorMemType_t) 0,
                                                          context_0_convReluModel_tensor_4_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_tensor_4 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_4_v1};
    VALIDATE(error, interface.tensorCreateGraphTensor(context_0_convReluModel,
                                                      &context_0_convReluModel_tensor_4));




    //step-6:
    uint32_t context_0_convReluModel_tensor_5_dims[] = {2};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_5_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static uint32_t context_0_convReluModel_tensor_5_data[2];
    fread(context_0_convReluModel_tensor_5_data, 4, 2, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_5_clientBuf = {
            (void *) context_0_convReluModel_tensor_5_data, 8};
    Qnn_TensorV1_t context_0_convReluModel_tensor_5_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_stride",
                                                          (Qnn_TensorType_t) 4/*QNN_TENSOR_TYPE_STATIC*/, 0,
                                                          (Qnn_DataType_t) 306/*QNN_DATATYPE_UINT_32*/,
                                                          context_0_convReluModel_tensor_5_quantizeParams,
                                                          1, context_0_convReluModel_tensor_5_dims,
                                                          (Qnn_TensorMemType_t) 0,
                                                          context_0_convReluModel_tensor_5_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_tensor_5 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_5_v1};
    VALIDATE(error, interface.tensorCreateGraphTensor(context_0_convReluModel,
                                                      &context_0_convReluModel_tensor_5));




    //step-7:
    uint32_t context_0_convReluModel_tensor_6_dims[] = {1, 149, 149, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_6_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_6_clientBuf = {NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_tensor_6_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_BatchNorm_FusedBatchNorm_0",
                                                          (Qnn_TensorType_t) 3/*QNN_TENSOR_TYPE_NATIVE*/, 0,
                                                          (Qnn_DataType_t) 562/*QNN_DATATYPE_FLOAT_32*/,
                                                          context_0_convReluModel_tensor_6_quantizeParams,
                                                          4, context_0_convReluModel_tensor_6_dims,
                                                          (Qnn_TensorMemType_t) 0,
                                                          context_0_convReluModel_tensor_6_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_tensor_6 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_6_v1};
    VALIDATE(error, interface.tensorCreateGraphTensor(context_0_convReluModel,
                                                      &context_0_convReluModel_tensor_6));




    //step-8:
    uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_dims[] = {2};
    Qnn_QuantizeParams_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_data[2];
    fread(InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_data, 4, 2, fp);
    Qnn_ClientBuffer_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_clientBuf = {
            (void *) InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_data, 8};
    Qnn_TensorV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_v1 = {4,
                                                                                       "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_dilation",
                                                                                       (Qnn_TensorType_t) 4/*QNN_TENSOR_TYPE_STATIC*/,
                                                                                       0,
                                                                                       (Qnn_DataType_t) 306,
                                                                                       InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_quantizeParams,
                                                                                       1,
                                                                                       InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_dims,
                                                                                       (Qnn_TensorMemType_t) 0,
                                                                                       InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_clientBuf};
    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor = {
            (Qnn_TensorVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_v1};
    Qnn_Param_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0 = {(Qnn_ParamType_t) 1,
                                                                          "dilation", .tensorParam = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor};

    uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_dims[] = {2, 2};
    Qnn_QuantizeParams_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_data[4];
    fread(InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_data, 4, 4, fp);
    Qnn_ClientBuffer_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_clientBuf = {
            (void *) InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_data, 16};
    Qnn_TensorV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_v1 = {5,
                                                                                       "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_pad_amount",
                                                                                       (Qnn_TensorType_t) 4,
                                                                                       0,
                                                                                       (Qnn_DataType_t) 306,
                                                                                       InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_quantizeParams,
                                                                                       2,
                                                                                       InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_dims,
                                                                                       (Qnn_TensorMemType_t) 0,
                                                                                       InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_clientBuf};
    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor = {
            (Qnn_TensorVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_v1};
    Qnn_Param_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1 = {(Qnn_ParamType_t) 1,
                                                                          "pad_amount", .tensorParam = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor};

    uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_dims[] = {2};
    Qnn_QuantizeParams_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_data[2];
    fread(InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_data, 4, 2, fp);
    Qnn_ClientBuffer_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_clientBuf = {
            (void *) InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_data, 8};
    Qnn_TensorV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_v1 = {6,
                                                                                       "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_stride",
                                                                                       (Qnn_TensorType_t) 4,
                                                                                       0,
                                                                                       (Qnn_DataType_t) 306,
                                                                                       InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_quantizeParams,
                                                                                       1,
                                                                                       InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_dims,
                                                                                       (Qnn_TensorMemType_t) 0,
                                                                                       InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_clientBuf};
    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor = {
            (Qnn_TensorVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_v1};
    Qnn_Param_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2 = {(Qnn_ParamType_t) 1,
                                                                          "stride", .tensorParam = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor};

    Qnn_Scalar_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3_scalar = {
            (Qnn_DataType_t) 306, .uint32Value = 1};
    Qnn_Param_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3 = {(Qnn_ParamType_t) 0,
                                                                          "group", .scalarParam = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3_scalar};

    Qnn_Param_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_params[] = {
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3};
    uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_dims[] = {1, 299, 299, 3};
    Qnn_QuantizeParams_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_clientBuf = {NULL, 0};
    Qnn_TensorV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_v1 = {1, "input_0",
                                                                                (Qnn_TensorType_t) 0,
                                                                                0,
                                                                                (Qnn_DataType_t) 562,
                                                                                InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_quantizeParams,
                                                                                4,
                                                                                InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_dims,
                                                                                (Qnn_TensorMemType_t) 0,
                                                                                InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_clientBuf};
    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_v1};

    uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_dims[] = {3, 3, 3, 32};
    Qnn_QuantizeParams_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_clientBuf = {NULL, 0};
    Qnn_TensorV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_v1 = {2,
                                                                                "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_weight",
                                                                                (Qnn_TensorType_t) 4,
                                                                                0,
                                                                                (Qnn_DataType_t) 562,
                                                                                InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_quantizeParams,
                                                                                4,
                                                                                InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_dims,
                                                                                (Qnn_TensorMemType_t) 0,
                                                                                InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_clientBuf};
    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1 = {
            (Qnn_TensorVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_v1};

    uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_dims[] = {32};
    Qnn_QuantizeParams_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_clientBuf = {NULL, 0};
    Qnn_TensorV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_v1 = {3,
                                                                                "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_bias",
                                                                                (Qnn_TensorType_t) 4,
                                                                                0,
                                                                                (Qnn_DataType_t) 562,
                                                                                InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_quantizeParams,
                                                                                1,
                                                                                InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_dims,
                                                                                (Qnn_TensorMemType_t) 0,
                                                                                InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_clientBuf};
    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2 = {
            (Qnn_TensorVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_v1};

    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_inputs[] = {
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2};
    uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_dims[] = {1, 149, 149, 32};
    Qnn_QuantizeParams_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_clientBuf = {NULL,
                                                                                            0};
    Qnn_TensorV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_v1 = {7,
                                                                                 "InceptionV3_InceptionV3_Conv2d_1a_3x3_BatchNorm_FusedBatchNorm_0",
                                                                                 (Qnn_TensorType_t) 3,
                                                                                 0,
                                                                                 (Qnn_DataType_t) 562,
                                                                                 InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_quantizeParams,
                                                                                 4,
                                                                                 InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_dims,
                                                                                 (Qnn_TensorMemType_t) 0,
                                                                                 InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_clientBuf};
    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_v1};

    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_outputs[] = {
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0};

    Qnn_OpConfigV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_v1 = {
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D", "qti.aisw", "Conv2d", 4,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_params, 3,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_inputs, 1,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_outputs};
    Qnn_OpConfig_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0 = {
            (Qnn_OpConfigVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_v1};
    VALIDATE(error, interface.backendValidateOpConfig(backend_0,
                                                      InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0));


    uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_dims[] = {
            2};
    Qnn_QuantizeParams_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_data[2];
    fread(context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_data,
          4, 2, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_clientBuf = {
            (void *) context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_data,
            8};
    Qnn_TensorV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_v1 = {
            context_0_convReluModel_tensor_3.v1.id,
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_dilation", (Qnn_TensorType_t) 4, 0,
            (Qnn_DataType_t) 306,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_quantizeParams,
            1,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_dims,
            (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor_v1};
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0 = {
            (Qnn_ParamType_t) 1,
            "dilation", .tensorParam = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0_tensor};

    uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_dims[] = {
            2, 2};
    Qnn_QuantizeParams_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_data[4];
    fread(context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_data,
          4, 4, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_clientBuf = {
            (void *) context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_data,
            16};
    Qnn_TensorV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_v1 = {
            context_0_convReluModel_tensor_4.v1.id,
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_pad_amount", (Qnn_TensorType_t) 4, 0,
            (Qnn_DataType_t) 306,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_quantizeParams,
            2,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_dims,
            (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor_v1};
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1 = {
            (Qnn_ParamType_t) 1,
            "pad_amount", .tensorParam = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1_tensor};

    uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_dims[] = {
            2};
    Qnn_QuantizeParams_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_data[2];
    fread(context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_data,
          4, 2, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_clientBuf = {
            (void *) context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_data,
            8};
    Qnn_TensorV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_v1 = {
            context_0_convReluModel_tensor_5.v1.id,
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_stride", (Qnn_TensorType_t) 4, 0,
            (Qnn_DataType_t) 306,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_quantizeParams,
            1,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_dims,
            (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor_v1};
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2 = {
            (Qnn_ParamType_t) 1,
            "stride", .tensorParam = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2_tensor};

    Qnn_Scalar_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3_scalar = {
            (Qnn_DataType_t) 306, .uint32Value = 1};
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3 = {
            (Qnn_ParamType_t) 0,
            "group", .scalarParam = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3_scalar};

    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_params[] = {
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3};
    uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_dims[] = {
            1, 299, 299, 3};
    Qnn_QuantizeParams_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_clientBuf = {
            NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_v1 = {
            context_0_convReluModel_tensor_0.v1.id, "input_0", (Qnn_TensorType_t) 0, 0,
            (Qnn_DataType_t) 562,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_quantizeParams,
            4, context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_dims,
            (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0_v1};

    uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_dims[] = {
            3, 3, 3, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_clientBuf = {
            NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_v1 = {
            context_0_convReluModel_tensor_1.v1.id,
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_weight", (Qnn_TensorType_t) 4, 0,
            (Qnn_DataType_t) 562,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_quantizeParams,
            4, context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_dims,
            (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1_v1};

    uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_dims[] = {
            32};
    Qnn_QuantizeParams_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_clientBuf = {
            NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_v1 = {
            context_0_convReluModel_tensor_2.v1.id,
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_bias", (Qnn_TensorType_t) 4, 0,
            (Qnn_DataType_t) 562,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_quantizeParams,
            1, context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_dims,
            (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2_v1};

    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_inputs[] = {
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_1,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_input_2};
    uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_dims[] = {
            1, 149, 149, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_clientBuf = {
            NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_v1 = {
            context_0_convReluModel_tensor_6.v1.id,
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_BatchNorm_FusedBatchNorm_0",
            (Qnn_TensorType_t) 3, 0, (Qnn_DataType_t) 562,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_quantizeParams,
            4, context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_dims,
            (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0_v1};

    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_outputs[] = {
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_output_0};
    Qnn_OpConfigV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_v1 = {
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D", "qti.aisw", "Conv2d", 4,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_params, 3,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_inputs, 1,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_outputs};
    Qnn_OpConfig_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0 = {
            (Qnn_OpConfigVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_v1};
    VALIDATE(error, interface.graphAddNode(context_0_convReluModel,
                                           context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0));


    //step-9:
    uint32_t context_0_convReluModel_tensor_7_dims[] = {1, 149, 149, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_7_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_7_clientBuf = {NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_tensor_7_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0",
                                                          (Qnn_TensorType_t) 1, 0,
                                                          (Qnn_DataType_t) 562,
                                                          context_0_convReluModel_tensor_7_quantizeParams,
                                                          4, context_0_convReluModel_tensor_7_dims,
                                                          (Qnn_TensorMemType_t) 0,
                                                          context_0_convReluModel_tensor_7_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_tensor_7 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_7_v1};
    VALIDATE(error, interface.tensorCreateGraphTensor(context_0_convReluModel,
                                                      &context_0_convReluModel_tensor_7));





    //step-10:
    Qnn_Param_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_params[] = {};
    uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_dims[] = {1, 149, 149, 32};
    Qnn_QuantizeParams_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_clientBuf = {NULL, 0};
    Qnn_TensorV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_v1 = {7,
                                                                              "InceptionV3_InceptionV3_Conv2d_1a_3x3_BatchNorm_FusedBatchNorm_0",
                                                                              (Qnn_TensorType_t) 3/*QNN_TENSOR_TYPE_NATIVE*/,
                                                                              0,
                                                                              (Qnn_DataType_t) 562,
                                                                              InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_quantizeParams,
                                                                              4,
                                                                              InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_dims,
                                                                              (Qnn_TensorMemType_t) 0,
                                                                              InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_clientBuf};
    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_v1};

    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_inputs[] = {
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0};
    uint32_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_dims[] = {1, 149, 149, 32};
    Qnn_QuantizeParams_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_clientBuf = {NULL, 0};
    Qnn_TensorV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_v1 = {8,
                                                                               "InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0",
                                                                               (Qnn_TensorType_t) 1/*QNN_TENSOR_TYPE_APP_READ*/,
                                                                               0,
                                                                               (Qnn_DataType_t) 562,
                                                                               InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_quantizeParams,
                                                                               4,
                                                                               InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_dims,
                                                                               (Qnn_TensorMemType_t) 0,
                                                                               InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_clientBuf};
    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_v1};

    Qnn_Tensor_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_outputs[] = {
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0};
    Qnn_OpConfigV1_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_v1 = {
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu", "qti.aisw", "Relu", 0,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_params, 1,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_inputs, 1,
            InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_outputs};
    Qnn_OpConfig_t InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0 = {
            (Qnn_OpConfigVersion_t) 1, .v1 = InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_v1};
    VALIDATE(error, interface.backendValidateOpConfig(backend_0,
                                                      InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0));

    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_params[] = {};
    uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_dims[] = {
            1, 149, 149, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_clientBuf = {
            NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_v1 = {
            context_0_convReluModel_tensor_6.v1.id,
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_BatchNorm_FusedBatchNorm_0",
            (Qnn_TensorType_t) 3, 0, (Qnn_DataType_t) 562,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_quantizeParams,
            4, context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_dims,
            (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0_v1};

    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_inputs[] = {
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_input_0};
    uint32_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_dims[] = {
            1, 149, 149, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_clientBuf = {
            NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_v1 = {
            context_0_convReluModel_tensor_7.v1.id, "InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0",
            (Qnn_TensorType_t) 1, 0, (Qnn_DataType_t) 562,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_quantizeParams,
            4, context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_dims,
            (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0_v1};

    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_outputs[] = {
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_output_0};
    Qnn_OpConfigV1_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_v1 = {
            "InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu", "qti.aisw", "Relu", 0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_params, 1,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_inputs, 1,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_outputs};
    Qnn_OpConfig_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0 = {
            (Qnn_OpConfigVersion_t) 1, .v1 = context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_v1};
    VALIDATE(error, interface.graphAddNode(context_0_convReluModel,
                                           context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0));



    VALIDATE(error, interface.graphFinalize(context_0_convReluModel, NULL, NULL));


    uint32_t context_0_convReluModel_inputTensor_0_0_dims[] = {1, 299, 299, 3};
    Qnn_QuantizeParams_t context_0_convReluModel_inputTensor_0_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static float context_0_convReluModel_inputTensor_0_0_data[268203];
    fread(context_0_convReluModel_inputTensor_0_0_data, 4, 268203, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_inputTensor_0_0_clientBuf = {
            (void *) context_0_convReluModel_inputTensor_0_0_data, 1072812};
    Qnn_TensorV1_t context_0_convReluModel_inputTensor_0_0_v1 = {
            context_0_convReluModel_tensor_0.v1.id, NULL, (Qnn_TensorType_t) 0/*QNN_TENSOR_TYPE_APP_WRITE*/, 0,
            (Qnn_DataType_t) 562/*QNN_DATATYPE_FLOAT_32*/, context_0_convReluModel_inputTensor_0_0_quantizeParams, 4,
            context_0_convReluModel_inputTensor_0_0_dims, (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_inputTensor_0_0_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_inputTensor_0_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_inputTensor_0_0_v1};

    uint32_t context_0_convReluModel_outputTensor_0_0_dims[] = {1, 149, 149, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_outputTensor_0_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static float context_0_convReluModel_outputTensor_0_0_data[710432];
    fread(context_0_convReluModel_outputTensor_0_0_data, 4, 710432, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_outputTensor_0_0_clientBuf = {
            (void *) context_0_convReluModel_outputTensor_0_0_data, 2841728};
    Qnn_TensorV1_t context_0_convReluModel_outputTensor_0_0_v1 = {
            context_0_convReluModel_tensor_7.v1.id, NULL, (Qnn_TensorType_t) 1/*QNN_TENSOR_TYPE_APP_READ*/, 0,
            (Qnn_DataType_t) 562/*QNN_DATATYPE_FLOAT_32*/, context_0_convReluModel_outputTensor_0_0_quantizeParams, 4,
            context_0_convReluModel_outputTensor_0_0_dims, (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_outputTensor_0_0_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_outputTensor_0_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_outputTensor_0_0_v1};

    Qnn_Tensor_t context_0_convReluModel_inputTensors_0[] = {
            context_0_convReluModel_inputTensor_0_0};
    Qnn_Tensor_t context_0_convReluModel_outputTensors_0[] = {
            context_0_convReluModel_outputTensor_0_0};
    VALIDATE(error,
             interface.graphExecute(context_0_convReluModel, context_0_convReluModel_inputTensors_0,
                                    1, context_0_convReluModel_outputTensors_0, 1, NULL, NULL));






    uint32_t context_0_convReluModel_inputTensor_1_0_dims[] = {1, 299, 299, 3};
    Qnn_QuantizeParams_t context_0_convReluModel_inputTensor_1_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static float context_0_convReluModel_inputTensor_1_0_data[268203];
    fread(context_0_convReluModel_inputTensor_1_0_data, 4, 268203, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_inputTensor_1_0_clientBuf = {
            (void *) context_0_convReluModel_inputTensor_1_0_data, 1072812};
    Qnn_TensorV1_t context_0_convReluModel_inputTensor_1_0_v1 = {
            context_0_convReluModel_tensor_0.v1.id, NULL, (Qnn_TensorType_t) 0, 0,
            (Qnn_DataType_t) 562, context_0_convReluModel_inputTensor_1_0_quantizeParams, 4,
            context_0_convReluModel_inputTensor_1_0_dims, (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_inputTensor_1_0_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_inputTensor_1_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_inputTensor_1_0_v1};

    uint32_t context_0_convReluModel_outputTensor_1_0_dims[] = {1, 149, 149, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_outputTensor_1_0_quantizeParams = {
            (Qnn_Definition_t) 2147483647,
            (Qnn_QuantizationEncoding_t) 2147483647, .scaleOffsetEncoding = {0.0, 0}};
    static float context_0_convReluModel_outputTensor_1_0_data[710432];
    fread(context_0_convReluModel_outputTensor_1_0_data, 4, 710432, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_outputTensor_1_0_clientBuf = {
            (void *) context_0_convReluModel_outputTensor_1_0_data, 2841728};
    Qnn_TensorV1_t context_0_convReluModel_outputTensor_1_0_v1 = {
            context_0_convReluModel_tensor_7.v1.id, NULL, (Qnn_TensorType_t) 1, 0,
            (Qnn_DataType_t) 562, context_0_convReluModel_outputTensor_1_0_quantizeParams, 4,
            context_0_convReluModel_outputTensor_1_0_dims, (Qnn_TensorMemType_t) 0,
            context_0_convReluModel_outputTensor_1_0_clientBuf};
    Qnn_Tensor_t context_0_convReluModel_outputTensor_1_0 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_outputTensor_1_0_v1};

    Qnn_Tensor_t context_0_convReluModel_inputTensors_1[] = {
            context_0_convReluModel_inputTensor_1_0};
    Qnn_Tensor_t context_0_convReluModel_outputTensors_1[] = {
            context_0_convReluModel_outputTensor_1_0};
    VALIDATE(error,
             interface.graphExecute(context_0_convReluModel, context_0_convReluModel_inputTensors_1,
                                    1, context_0_convReluModel_outputTensors_1, 1, NULL, NULL));






    VALIDATE(error, interface.contextFree(context_0, NULL));

    VALIDATE(error, interface.deviceFree(device_0));

    VALIDATE(error, interface.backendFree(backend_0));

    if (logging) {
        VALIDATE(error, interface.logFree(loghandle));
    }

    if (fclose(fp)) error = -1;

    LOGGI("leave %s", __func__);
    GGML_JNI_NOTIFY("leave %s", __func__);
    return error == 0 || error == QNN_COMMON_ERROR_NOT_SUPPORTED ? 0 : error;
}
