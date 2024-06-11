<h4>Prerequisites</h4>

```

cd ${PROJECT_ROOT_PATH}

. build/envsetup.sh

lunch 1
```


<h4>Build Android command line program of QNN backend UT and run on Android phone/device</h4>

ensure an Android phone/device has been connected to the dev machine and adb connect works fine firstly.

```
cd core/ggml/llamacpp/tests/ggml-qnn-ut-in-kantv

./build-ggml-qnn.sh

./run-ggml-qnn.sh
```


the following is an example log of the QNN UT Android command line program:

```
:$ ./run-ggml-qnn.sh 0 GGML_OP_ADD 0

/data/local/tmp//libQnnCpu.so
QNN libs already exist on Android phone
ggml-qnn-test: 1 file pushed. 21.9 MB/s (4564560 bytes in 0.199s)
[main, 320]: enter qnn_ggml_op

[main, 321]: ggml op:2(ADD)
[main, 335]: Allocating Memory of size 33554432 bytes, 32 MB

[ggml_backend_qnn_init, 3590]: device 0
[ggml_backend_qnn_init, 3591]: qnn_lib_path /data/local/tmp/
[qnn_init, 1812]: enter qni_init

[load_system, 1673]: system_lib_path:/data/local/tmp/libQnnSystem.so

[load_system, 1722]: find a valid qnn system interface

[load_system, 1732]: initialize qnn system successfully

[qnn_init, 1820]: load QNN system lib successfully

[load_backend, 1551]: lib_path:/data/local/tmp/libQnnCpu.so

[load_backend, 1575]: num_providers=1

[load_backend, 1600]: find a valid qnn interface

[load_backend, 1645]: saver_initialize is null

[qnn_init, 1853]: initialize qnn log successfully

[qnn_init, 1864]: initialize qnn backend successfully

[qnn_init, 1870]: device property is not supported

[qnn_init, 1881]: create device successfully

[qnn_init, 1885]: profiling turned on; level = 2
[qnn_init, 1896]: detailed profiling requested. Creating Qnn Profile object

[qnn_init, 1902]: initialize qnn profile successfully

[qnn_init, 1912]: load rpcmem lib successfully

[qnn_init, 1939]: initialize qnn context successfully

[qnn_init, 1942]: leave qni_init

[ggml_backend_qnn_init, 3645]: qnn device name QNN-CPU
[init_qnn_graph, 2046]: succeed to create graph QNN-CPU, 0xd4a52a9c46bcdc2f

[main, 361]: creating new tensors

[main, 362]: ggml_blck_size(f32) 1
[main, 363]: ggml_type_size(f32) 4
[main, 402]: creating backend buffer

[main, 413]: creating compute graph

[ggml_qnn_can_handle_op, 2098]: op name:ADD, tensor type:f32
[ggml_qnn_can_handle_op, 2100]: src0 type:f32
[ggml_qnn_can_handle_op, 2103]: src1 type:f32
[ggml_qnn_add, 2214]: call ggml_qnn_add

[ggml_qnn_add, 2218]:        tensor_0: type = 0 (  f32) ne =     4 x     4 x     1, nb = (    4,    16,    64)

[ggml_qnn_add, 2222]:        tensor_1: type = 0 (  f32) ne =     4 x     4 x     1, nb = (    4,    16,    64)

[ggml_qnn_add, 2226]:        tensor_2: type = 0 (  f32) ne =     4 x     4 x     1, nb = (    4,    16,    64)

[ggml_qnn_add, 2227]: 4, 4, 1, 1
[ggml_qnn_add, 2228]: tensor0 name tensor_0
[ggml_qnn_add, 2229]: tensor1 name tensor_1
[ggml_qnn_add, 2230]: tensor2 name tensor_2
[ggml_qnn_add, 2257]: graph name ggml_op_qnn_add_4tensor_0_tensor_1
[ggml_qnn_logcallback, 1805]:     15.7ms [ DEBUG ] getNode OpPackage-Name : qti.aisw Node-Type : ElementWiseAdd
[ggml_qnn_logcallback, 1805]:     15.8ms [VERBOSE] validate	Node-Type : ElementWiseAdd	Node-Name : ggml_op_add
[ggml_qnn_logcallback, 1805]:     16.0ms [  INFO ] CpuGraph::finalize
[ggml_qnn_logcallback, 1805]:     16.1ms [ DEBUG ] Setting data pointer for tensor ID: 1
[ggml_qnn_logcallback, 1805]:     16.1ms [ DEBUG ] Setting data pointer for tensor ID: 2
[ggml_qnn_logcallback, 1805]:     16.1ms [ DEBUG ] Setting data pointer for tensor ID: 3
[ggml_qnn_logcallback, 1805]:     16.1ms [  INFO ] CpuGraph::execute
[main, 429]: dump:

[tensor_dump, 167]: dump ggml tensor src0(tensor_0)
[tensor_dump, 171]:            src0: type = 0 (  f32) ne =     4 x     4 x     1, nb = (    4,    16,    64)
[tensor_sum_elements, 151]:     0.27    -0.50     0.01     0.55
[tensor_sum_elements, 155]:

[tensor_sum_elements, 151]:     0.54    -0.91     0.16    -0.24
[tensor_sum_elements, 155]:

[tensor_sum_elements, 151]:    -0.97     0.32    -0.04    -0.47
[tensor_sum_elements, 155]:

[tensor_sum_elements, 151]:    -0.94    -0.17     0.97     0.20
[tensor_sum_elements, 155]:

[tensor_sum_elements, 161]:

[tensor_dump, 174]:

[tensor_dump, 167]: dump ggml tensor src1(tensor_1)
[tensor_dump, 171]:            src1: type = 0 (  f32) ne =     4 x     4 x     1, nb = (    4,    16,    64)
[tensor_sum_elements, 151]:     0.30    -0.73    -0.43     0.36
[tensor_sum_elements, 155]:

[tensor_sum_elements, 151]:     0.86     0.70     0.34     0.67
[tensor_sum_elements, 155]:

[tensor_sum_elements, 151]:     0.60    -0.92     0.32     0.91
[tensor_sum_elements, 155]:

[tensor_sum_elements, 151]:    -0.08     0.99     0.33    -0.36
[tensor_sum_elements, 155]:

[tensor_sum_elements, 161]:

[tensor_dump, 174]:

[tensor_dump, 167]: dump ggml tensor dst(tensor_2)
[tensor_dump, 171]:             dst: type = 0 (  f32) ne =     4 x     4 x     1, nb = (    4,    16,    64)
[tensor_sum_elements, 151]:     0.57    -1.23    -0.42     0.91
[tensor_sum_elements, 155]:

[tensor_sum_elements, 151]:     1.40    -0.21     0.50     0.43
[tensor_sum_elements, 155]:

[tensor_sum_elements, 151]:    -0.37    -0.59     0.27     0.45
[tensor_sum_elements, 155]:

[tensor_sum_elements, 151]:    -1.02     0.82     1.31    -0.16
[tensor_sum_elements, 155]:

[tensor_sum_elements, 161]:

[tensor_dump, 174]:

[ggml_backend_qnn_free, 3394]: enter ggml_backend_qnn_free
[ggml_backend_qnn_free, 3396]: idx 0, name:qnn-cpu
[ggml_backend_qnn_free, 3405]: graph type:ADD
[qnn_finalize, 1958]: succeed to close rpcmem lib

[ggml_backend_qnn_free, 3419]: leave ggml_backend_qnn_free
[main, 454]: duration of ut ADD : 20 milliseconds
```
