# header-ai-runtime 开发记录

本文记录当前阶段在 WSL 中推进 `header-ai-runtime` 的主要操作、验证结果和遇到的问题，便于后续回溯。

## 1. 仓库与分支准备

1. 本地 Windows 侧已有仓库 `C:\Users\SESA855007\header-ai-runtime`。
2. 企业 GitHub 仓库地址为：

   ```text
   git@github.schneider-electric.com:SESA855007/header-ai-runtime.git
   ```

3. 初始发现企业远程默认分支曾指向功能分支，随后手动改回 `main`。
4. WSL 仓库路径固定为：

   ```text
   /home/xin/header-ai-runtime
   ```

5. 后续开发约定：
   - `main` 只与远程 `origin/main` 对齐。
   - 每个阶段新建功能分支。
   - 功能完成后推送分支并提交 PR，由人工合并。
   - 不直接修改或推送 `main`。

## 2. WSL 网络与 Git 访问处理

### 2.1 公司 GitHub Enterprise

WSL 最初无法稳定解析或访问：

```text
github.schneider-electric.com
```

处理过程：

1. 检查 Windows 侧 DNS，确认企业 GitHub 解析到：

   ```text
   10.236.34.75 github.schneider-electric.com
   ```

2. WSL 中 `/etc/resolv.conf` 使用公司 DNS：

   ```text
   nameserver 10.177.1.172
   nameserver 10.177.40.177
   ```

3. 在 WSL 中测试后，企业 GitHub HTTPS/SSH 均恢复可用。
4. 后续 WSL 的 `origin` 改为 SSH：

   ```text
   git@github.schneider-electric.com:SESA855007/header-ai-runtime.git
   ```

5. SSH 验证成功：

   ```text
   Hi SESA855007! You've successfully authenticated, but GitHub does not provide shell access.
   ```

### 2.2 公网 GitHub / PyPI / NuGet

WSL 到公网依赖源仍不稳定或不可用，包括：

```text
github.com
release-assets.githubusercontent.com
pypi.org
www.nuget.org
```

主要错误：

```text
OpenSSL SSL_connect: Connection reset by peer
Connection timeout
```

结论：

- WSL 与公司 GitHub Enterprise 是通的。
- WSL 与公网依赖源不通或被公司网络拦截。
- ONNX Runtime SDK 不能直接通过 WSL 下载。

最终处理方式：用户手动用浏览器下载 ONNX Runtime Linux x64 SDK，然后由 WSL 使用本地文件。

## 3. 训练产物检查

用户提供训练产物目录：

```text
C:\Users\SESA855007\Downloads\data\data
```

发现关键文件：

```text
meta.json
model.onnx
metrics.json
validation_report.json
evaluation_windows_normal.csv
evaluation_windows_jumpsup.csv
```

其中 `meta.json` 关键字段包括：

```json
{
  "schema_version": "1.0",
  "model_type": "autoencoder",
  "input_name": "input",
  "output_name": "reconstruction",
  "window_size": 60,
  "feature_dim": 1,
  "input_dim": 60,
  "flatten_order": "time_major",
  "threshold": 0.009797872975468636,
  "normalization": {
    "type": "standard",
    "mean": [43.097919775672935],
    "std": [28.07703083103901]
  },
  "alarm": {
    "consecutive_count": 3,
    "clear_count": 5
  },
  "onnx": {
    "opset": 17
  }
}
```

该文件作为 v0.2-v0.5 的真实验收输入。

## 4. v0.1-v0.4 开发内容

功能分支：

```text
feature/v0.1-v0.4-runtime-core
```

提交：

```text
4140855 Implement runtime core v0.1-v0.4
```

实现内容：

1. 新增 CMake C++ 工程骨架。
2. 新增可执行程序：

   ```text
   header_ai_detector
   ```

3. 新增核心模块：

   ```text
   include/MetaConfig.h
   include/SlidingWindow.h
   include/Normalizer.h
   src/MetaConfig.cpp
   src/SlidingWindow.cpp
   src/Normalizer.cpp
   src/main.cpp
   ```

4. 新增测试：

   ```text
   tests/runtime_core_tests.cpp
   ```

5. 新增 `.gitignore`，避免提交 CMake 构建产物。
6. 更新 README，加入 Ubuntu/WSL 构建与验收说明。

### 4.1 MetaConfig

实现 `meta.json` 读取与校验。

校验项包括：

1. `schema_version == "1.0"`。
2. `model_type == "autoencoder"`。
3. `input_dim == window_size * feature_dim`。
4. `flatten_order == "time_major"`。
5. `normalization.type == "standard"`。
6. `mean/std` 长度等于 `feature_dim`。
7. `std` 不能包含 0。
8. `threshold >= 0`。
9. `consecutive_count` 和 `clear_count` 大于 0。
10. `input_name` 和 `output_name` 非空。

由于 WSL 当时没有可用 JSON 开发库，且公网下载不通，v0.2 先实现了一个面向当前 `meta.json` 合同的内部 JSON 解析器，避免引入外部依赖。

### 4.2 SlidingWindow

实现滑动窗口：

1. 支持 `window_size`。
2. 支持 `feature_dim`。
3. 窗口未满时 `ready() == false`。
4. 窗口满后新样本进入，最旧样本自动移除。
5. 支持 `time_major` 展平。

测试覆盖：

- 输入 100 个点，`window_size=60` 时，从第 60 个点开始产生窗口。
- 总计产生 41 个 ready 窗口。
- 多变量展平顺序为：

  ```text
  t1_f1, t1_f2, t2_f1, t2_f2, ...
  ```

### 4.3 Normalizer

实现 StandardScaler：

```text
x_norm = (x - mean[feature_index]) / std[feature_index]
```

支持按 `time_major` 展平后的单变量和多变量数据。

### 4.4 v0.1-v0.4 验收

在 WSL 中执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ctest --output-on-failure
./header_ai_detector --meta /mnt/c/Users/SESA855007/Downloads/data/data/meta.json
```

结果：

```text
100% tests passed
Loaded meta.json
schema_version=1.0
model_type=autoencoder
window_size=60
feature_dim=1
input_dim=60
threshold=0.00979787
```

v0.1-v0.4 验收通过后推送功能分支并创建 PR，随后用户手动合并到 `main`。

## 5. v0.5 开发内容

功能分支：

```text
feature/v0.5-onnx-runtime
```

提交：

```text
60f6e48 Implement ONNX Runtime model probe v0.5
```

实现目标：

1. 接入 ONNX Runtime C++ SDK。
2. 加载训练工程导出的 `model.onnx`。
3. 校验模型输入输出名称与 `meta.json` 一致。
4. 校验输入输出 shape 为 `[1, input_dim]`。
5. 执行一次零输入推理探测。
6. 确认输出长度等于 `input_dim`。

### 5.1 ONNX Runtime SDK 获取

由于 WSL 无法从公网 GitHub Release 下载 SDK，用户手动下载：

```text
onnxruntime-linux-x64-1.18.1.tgz
```

下载位置：

```text
C:\Users\SESA855007\Downloads\onnxruntime-linux-x64-1.18.1.tgz
```

在 WSL 中解压到：

```text
/home/xin/header-ai-runtime/third_party/onnxruntime-linux-x64-1.18.1
```

确认包含：

```text
include/onnxruntime_cxx_api.h
include/onnxruntime_c_api.h
lib/libonnxruntime.so
```

`.gitignore` 已忽略：

```text
third_party/onnxruntime*/
```

因此 SDK 不提交进仓库。

### 5.2 CMake 接入

新增 CMake 配置：

```cmake
set(DEFAULT_ONNXRUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime-linux-x64-1.18.1")
set(ONNXRUNTIME_ROOT "${DEFAULT_ONNXRUNTIME_ROOT}" CACHE PATH "ONNX Runtime SDK root directory")
option(HEADER_AI_ENABLE_ONNX "Build ONNX Runtime inference support" OFF)
```

如果检测到：

```text
${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h
${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so
```

则自动启用：

```text
HEADER_AI_ENABLE_ONNX=ON
```

并把 `libonnxruntime.so` 链接到 `header_ai_runtime_core`。

### 5.3 OnnxAutoEncoder

新增文件：

```text
include/OnnxAutoEncoder.h
src/OnnxAutoEncoder.cpp
```

主要功能：

1. 创建 ONNX Runtime `Env`。
2. 创建 CPU session。
3. 加载 `model.onnx`。
4. 获取模型输入名和输出名。
5. 对比 `meta.json` 中的 `input_name` 和 `output_name`。
6. 校验输入输出 shape。
7. 执行一次推理。
8. 将输出统一转换为 `std::vector<double>`。

### 5.4 main 命令扩展

新增命令：

```bash
./build/header_ai_detector --model /path/to/model.onnx --meta /path/to/meta.json --probe-onnx
```

输出示例：

```text
ONNX probe completed
input_dim=60
output_dim=60
```

## 6. v0.5 遇到的问题与处理

### 6.1 WSL 无法下载 ONNX Runtime SDK

尝试过：

1. GitHub Release：

   ```text
   https://github.com/microsoft/onnxruntime/releases/...
   ```

2. PyPI：

   ```bash
   python3 -m pip install --user onnxruntime
   ```

3. NuGet：

   ```text
   https://www.nuget.org/api/v2/package/Microsoft.ML.OnnxRuntime/...
   ```

均失败，主要错误为：

```text
Connection reset by peer
Connection timeout
```

处理：改为浏览器手动下载 SDK，再在 WSL 中解压使用。

### 6.2 ONNX 输入类型不是最初假设的 float32

首次运行 ONNX 探测时报错：

```text
ONNX input tensor type must be float32
```

原因：训练导出的模型输入类型不是最初硬编码假设的 float32。

处理：`OnnxAutoEncoder` 改为支持：

```text
float32
float64
```

并根据模型实际输入类型构造对应 tensor。

### 6.3 ONNX Runtime TypeInfo 生命周期问题

增强错误信息后出现异常类型编号，例如：

```text
actual type=-580476458
```

原因：直接链式调用：

```cpp
session_.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo()
```

临时 `TypeInfo` 生命周期结束后，后续读取到无效数据。

处理：显式保存 `TypeInfo` 对象：

```cpp
const auto input_type_info = session_.GetInputTypeInfo(0);
const auto input_info = input_type_info.GetTensorTypeAndShapeInfo();
```

输出端同理处理。

### 6.4 WSL 推送 HTTPS 需要凭据

v0.5 提交完成后，使用 HTTPS remote 推送时卡在：

```text
Username for 'https://github.schneider-electric.com':
```

处理：确认 WSL SSH 到企业 GitHub 可用后，将 remote 改为：

```text
git@github.schneider-electric.com:SESA855007/header-ai-runtime.git
```

随后通过 SSH 推送成功。

## 7. v0.5 验收

执行命令：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ctest --output-on-failure
cd ..
./build/header_ai_detector --version
./build/header_ai_detector --meta /mnt/c/Users/SESA855007/Downloads/data/data/meta.json
./build/header_ai_detector --model /mnt/c/Users/SESA855007/Downloads/data/data/model.onnx --meta /mnt/c/Users/SESA855007/Downloads/data/data/meta.json --probe-onnx
```

结果：

```text
100% tests passed
header_ai_detector v0.5.0
Loaded meta.json
ONNX probe completed
input_dim=60
output_dim=60
```

v0.5 验收通过后，推送分支：

```text
feature/v0.5-onnx-runtime
```

PR 链接：

```text
https://github.schneider-electric.com/SESA855007/header-ai-runtime/pull/new/feature/v0.5-onnx-runtime
```

## 8. 当前状态

当前 WSL 仓库状态：

```text
branch: feature/v0.5-onnx-runtime
latest commit: 60f6e48 Implement ONNX Runtime model probe v0.5
working tree: clean
```

当前已完成：

1. v0.1.0 C++ 工程基础初始化。
2. v0.2.0 `meta.json` 解析与校验。
3. v0.3.0 滑动窗口模块。
4. v0.4.0 StandardScaler 归一化模块。
5. v0.5.0 ONNX Runtime C++ 模型加载和单次推理探测。

下一步建议：

1. 合并 v0.5 PR 到 `main`。
2. 回到 `main` 并对齐远程。
3. 新建 v0.6 分支，实现 MSE 重构误差和阈值判断。

## 9. v0.6 开发补充记录

功能分支：

```text
feature/v0.6-error-threshold
```

提交：

```text
1798e3a Implement reconstruction error threshold v0.6
```

实现内容：

1. 新增 `ReconstructionError` 模块。
2. 实现 `calculateMeanSquaredError(expected, actual)`。
3. 实现 `isAnomaly(mse, threshold)`。
4. 阈值判断采用严格大于：`mse > threshold`。
5. ONNX 探测输出新增 `mse`、`threshold` 和 `is_anomaly`。
6. 单元测试覆盖 MSE、长度不匹配、空输入和负阈值。

真实训练产物验收结果：

```text
ONNX probe completed
input_dim=60
output_dim=60
mse=0.00780021
threshold=0.00979787
is_anomaly=false
```

v0.6 PR 已合并到 `main`。

## 10. v0.7 开发记录

功能分支：

```text
feature/v0.7-alarm-state-machine
```

目标：实现连续异常报警状态机，避免单次尖峰造成误报。

计划实现内容：

1. 新增 `AnomalyDetector` 类。
2. 读取 `meta.json` 中的 `alarm.consecutive_count` 和 `alarm.clear_count`。
3. 连续异常达到 `consecutive_count` 后进入 `ALARM`。
4. 连续正常达到 `clear_count` 后恢复 `NORMAL`。
5. ONNX 探测输出当前状态 `state`。
6. 单元测试覆盖报警进入、报警保持、报警恢复和非法配置。

## 11. v0.8-v0.10 开发记录

功能分支：

```text
feature/v0.8-v0.10-file-replay
```

目标：一次性推进到第一阶段最小可行版本 `v0.10.0`，形成 Linux C++ 单变量离线回放和实时推理可跑通版。

### 11.1 v0.8 文件回放

新增文件数据源能力：

```text
include/IDataSource.h
include/FileDataSource.h
src/FileDataSource.cpp
```

`FileDataSource` 支持：

1. CSV 输入。
2. 空白分隔 TXT 输入。
3. 首行 header 自动跳过。
4. 空行和 `#` 注释行跳过。
5. `timestamp,value` 形式输入，取最后 `feature_dim` 个数值作为特征。
6. 单变量和多变量预留。
7. 数据行出现非数值时明确报错。

### 11.2 v0.9 输入接口抽象

新增 `IDataSource` 接口：

```cpp
class IDataSource {
public:
    virtual ~IDataSource() = default;
    virtual bool readSample(std::vector<double>& sample) = 0;
};
```

主流程依赖 `IDataSource` 抽象读取样本，后续可扩展：

1. `SerialDataSource`。
2. `SocketDataSource`。
3. CAN/GPIO 等现场输入。

当前实现的具体数据源为：

```text
FileDataSource
```

### 11.3 v0.10 单变量完整链路

`header_ai_detector` 新增完整回放命令：

```bash
./build/header_ai_detector --model /path/to/model.onnx --meta /path/to/meta.json --replay /path/to/samples.csv
```

完整链路：

```text
FileDataSource
  -> SlidingWindow
  -> Normalizer
  -> OnnxAutoEncoder
  -> calculateMeanSquaredError
  -> isAnomaly
  -> AnomalyDetector
  -> stdout CSV result
```

输出字段：

```text
window_index,end_sample_index,mse,threshold,is_anomaly,state
```

末尾输出汇总：

```text
total_samples=<N>
total_windows=<M>
```

### 11.4 v0.8-v0.10 验收

构建与单元测试：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ctest --output-on-failure
```

结果：

```text
100% tests passed
```

真实模型探测：

```bash
./build/header_ai_detector \
  --model /mnt/c/Users/SESA855007/Downloads/data/data/model.onnx \
  --meta /mnt/c/Users/SESA855007/Downloads/data/data/meta.json \
  --probe-onnx
```

临时生成 80 行单变量 CSV：

```text
timestamp,value
0,43.097919775672935
1,43.097919775672935
...
```

完整回放验收：

```bash
./build/header_ai_detector \
  --model /mnt/c/Users/SESA855007/Downloads/data/data/model.onnx \
  --meta /mnt/c/Users/SESA855007/Downloads/data/data/meta.json \
  --replay /tmp/header_ai_runtime_replay.csv
```

结果：

```text
window_index,end_sample_index,mse,threshold,is_anomaly,state
0,59,0.00780020916862,0.00979787297547,false,NORMAL
...
20,79,0.00780020916862,0.00979787297547,false,NORMAL
total_samples=80
total_windows=21
```

说明：`window_size=60`，输入 80 个样本，因此产生 `80 - 60 + 1 = 21` 个窗口，符合预期。

### 11.5 当前部署形态

当前最小运行依赖：

```text
header_ai_detector
model.onnx
meta.json
libonnxruntime.so
```

当前阶段仍使用动态链接 ONNX Runtime。后续部署到目标 Linux 设备时，需要根据目标架构准备对应的 `libonnxruntime.so`。
