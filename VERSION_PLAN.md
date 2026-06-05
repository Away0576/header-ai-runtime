# header-ai-runtime 版本规划与功能拆分

## 0. 版本号规则

本工程统一使用版本号格式：

```text
vA.B.C
```

版本号含义：

| 字段 | 含义 | 修改规则 |
|---|---|---|
| `A` | 大版本号 | 只有发生大规模架构升级、工程方向变化、接口合同重大不兼容时才修改 |
| `B` | 功能版本号 | 每添加一个明确的新功能或完成一个阶段性功能模块时递增 |
| `C` | 修复版本号 | debug、缺陷修复、兼容性修复、小范围功能修正时递增 |

当前阶段统一固定在：

```text
v0.B.C
```

也就是说，在训练工程和 Linux runtime 工程整体跑通之前，统一保持在 `v0` 阶段，不进入 `v1`。当前所有新增功能都通过递增 `B` 表示；同一功能阶段内的问题修复通过递增 `C` 表示。

示例：

```text
v0.5.0  新增 ONNX Runtime C++ 推理功能
v0.5.1  修复 ONNX 输入名不匹配时的错误提示
v0.5.2  修复 ARM Linux 上动态库加载路径问题
v1.0.0  只有在整体工程跑通并进入正式大版本，或发生大规模架构升级时才允许出现
```

## 1. 工程定位

`header-ai-runtime` 是 Linux 嵌入式部署工程，运行环境为目标 Linux 平台。

本工程只负责：

1. 加载 `model.onnx`。
2. 加载 `meta.json`。
3. 接收实时数据输入。
4. 维护滑动窗口。
5. 使用训练阶段保存的参数做归一化。
6. 调用 ONNX Runtime C++ 执行推理。
7. 计算重构误差。
8. 根据阈值判断异常。
9. 执行连续异常报警和恢复逻辑。

本工程不负责：

1. PyTorch 训练。
2. 模型调参。
3. ONNX 导出。
4. 阈值训练计算。
5. Python 训练脚本。

职责边界规定：

1. `header-ai-runtime` 的职责起点是加载 `model.onnx` 和 `meta.json`。
2. `header-ai-runtime` 不允许包含 PyTorch 训练、ONNX 导出、阈值训练计算逻辑。
3. `header-ai-runtime` 可以包含离线 CSV 回放工具，但该工具只能用于模拟实时输入和验证 C++ 推理链路。
4. `header-ai-runtime` 必须严格按 `meta.json` 合同执行滑窗、归一化、推理和阈值判断。

## 2. 与 train 工程的交付边界

`header-ai-runtime` 只消费 `header-ai-train` 产出的两个文件：

```text
model.onnx
meta.json
```

runtime 不允许依赖：

1. `model.pt`。
2. pickle 文件。
3. Python 代码。
4. 训练数据。
5. PyTorch。

## 3. 部署包约定

最终 Linux 设备部署包建议为：

```text
deploy/
├── header_ai_detector       # C++ 可执行程序
├── model.onnx               # 训练工程导出的 ONNX 模型
├── meta.json                # 训练工程导出的元数据
└── config.json              # runtime 现场配置，可选
```

其中必须存在：

```text
header_ai_detector
model.onnx
meta.json
```

## 4. meta.json 读取要求

runtime 必须读取并校验以下字段：

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
  "threshold": 0.0123,
  "threshold_percentile": 99.0,
  "normalization": {
    "type": "standard",
    "mean": [0.0],
    "std": [1.0]
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

启动时必须校验：

1. `schema_version` 是否支持。
2. `model_type` 是否为 `autoencoder`。
3. `input_dim == window_size * feature_dim`。
4. `normalization.mean` 长度是否等于 `feature_dim`。
5. `normalization.std` 长度是否等于 `feature_dim`。
6. `normalization.std` 中不能有 0。
7. `threshold` 必须大于等于 0。
8. `consecutive_count` 和 `clear_count` 必须大于 0。

校验失败时程序必须明确报错并退出。

## 5. 实时推理主流程

```text
读取实时采样
  ↓
写入 SlidingWindow
  ↓
窗口未满则继续等待
  ↓
窗口满后按 time_major 展平
  ↓
Normalizer 执行标准化
  ↓
ONNX Runtime 推理
  ↓
计算 MSE
  ↓
AnomalyDetector 更新状态
  ↓
输出 NORMAL / WARNING / ALARM
```

重构误差计算：

```text
mse = mean((reconstruction - normalized_input) ^ 2)
```

注意：MSE 必须在归一化空间计算，和训练工程保持一致。

## 6. 版本路线图

### v0.1.0 - C++ 工程基础初始化

目标：建立 Linux C++ runtime 工程骨架。

需要实现：

1. CMake 工程。
2. `src/` 和 `include/` 目录。
3. 可执行程序 `header_ai_detector`。
4. 基础 README。
5. 编译说明。

建议目录：

```text
header-ai-runtime/
├── README.md
├── CMakeLists.txt
├── include/
│   ├── MetaConfig.h
│   ├── SlidingWindow.h
│   ├── Normalizer.h
│   ├── OnnxAutoEncoder.h
│   └── AnomalyDetector.h
└── src/
    ├── main.cpp
    ├── MetaConfig.cpp
    ├── SlidingWindow.cpp
    ├── Normalizer.cpp
    ├── OnnxAutoEncoder.cpp
    └── AnomalyDetector.cpp
```

验收标准：

1. Linux 上可以通过 CMake 编译空程序。
2. 程序可以启动并打印版本信息。
3. 暂不要求加载 ONNX。

### v0.2.0 - meta.json 解析与校验

目标：runtime 能读取训练工程交付的 `meta.json`。

需要实现：

1. 使用 JSON 库读取 `meta.json`。
2. 定义 `MetaConfig` 数据结构。
3. 校验所有必需字段。
4. 校验字段类型和取值范围。
5. 校验 `input_dim == window_size * feature_dim`。

验收标准：

1. 合法 `meta.json` 能成功加载。
2. 缺字段时启动失败并打印明确错误。
3. `std=0` 时启动失败。
4. `input_dim` 不匹配时启动失败。

### v0.3.0 - 滑动窗口模块

目标：实现实时数据窗口缓存。

需要实现：

1. `SlidingWindow` 类。
2. 支持单变量输入。
3. 支持多变量输入的数据结构预留。
4. 支持 `ready()` 判断窗口是否已满。
5. 支持按 `time_major` 展平。
6. 窗口满后每来一个新点，自动丢弃最旧点。

单变量窗口：

```text
[x1, x2, ..., x60]
```

多变量窗口展平：

```text
[t1_f1, t1_f2, ..., t1_fn, t2_f1, ..., t60_fn]
```

验收标准：

1. 输入 100 个点，`window_size=60` 时，从第 60 个点开始输出窗口。
2. 展平长度等于 `input_dim`。
3. 展平顺序与训练工程一致。

### v0.4.0 - 归一化模块

目标：实现与训练工程一致的 StandardScaler。

需要实现：

1. `Normalizer` 类。
2. 从 `meta.json` 获取 `mean/std`。
3. 对每个时间点的每个特征执行归一化。
4. 支持单变量。
5. 支持多变量预留。

公式：

```text
x_norm = (x - mean[feature_index]) / std[feature_index]
```

验收标准：

1. 单变量归一化结果与 Python 计算一致。
2. 多变量展平数据能按 feature index 使用对应 `mean/std`。
3. 归一化输出长度不变。

### v0.5.0 - ONNX Runtime C++ 推理

目标：加载 `model.onnx` 并完成一次推理。

需要实现：

1. 封装 `OnnxAutoEncoder` 类。
2. 初始化 ONNX Runtime session。
3. 使用 CPUExecutionProvider。
4. 根据 `meta.json` 中的 input/output name 绑定输入输出。
5. 输入 shape 为 `[1, input_dim]`。
6. 输出 shape 为 `[1, input_dim]`。

验收标准：

1. 能加载 `model.onnx`。
2. 能对一个窗口完成推理。
3. 输出长度等于 `input_dim`。
4. ONNX 输入输出名称不匹配时明确报错。

### v0.6.0 - 重构误差与阈值判断

目标：实现异常检测核心逻辑。

需要实现：

1. 计算 normalized input 与 reconstruction 的 MSE。
2. 读取 `threshold`。
3. 判断 `mse > threshold`。
4. 输出当前窗口的 error score。
5. 输出当前窗口是否异常。

验收标准：

1. C++ MSE 与 Python ONNX Runtime 结果基本一致。
2. 阈值判断逻辑正确。
3. 每个窗口都能输出 score 和 anomaly flag。

### v0.7.0 - 连续异常报警状态机

目标：避免单次尖峰造成误报。

需要实现：

1. `AnomalyDetector` 类。
2. 连续异常计数。
3. 连续正常计数。
4. 达到 `consecutive_count` 后进入 ALARM。
5. 达到 `clear_count` 后退出 ALARM。
6. 输出状态：`NORMAL`、`ALARM`。

默认逻辑：

```text
if mse > threshold:
    anomaly_count += 1
    normal_count = 0
else:
    normal_count += 1
    anomaly_count = 0

if anomaly_count >= consecutive_count:
    alarm = true

if normal_count >= clear_count:
    alarm = false
```

验收标准：

1. 连续 3 个异常窗口后报警。
2. 连续 5 个正常窗口后解除报警。
3. 状态切换有明确日志。

### v0.8.0 - 离线 CSV 回放验证

目标：在接真实设备前，用文件模拟实时输入。

需要实现：

1. 从 CSV 或 TXT 读取测试时间序列。
2. 按行模拟实时采样输入。
3. 逐窗口执行完整推理流程。
4. 输出每个窗口的 timestamp、mse、threshold、state。
5. 可选输出结果 CSV。

验收标准：

1. 能用训练工程提供的样例数据跑完整流程。
2. C++ 输出的窗口数量与 Python 一致。
3. C++ 输出的 MSE 与 Python 对齐。

### v0.9.0 - 实时输入接口抽象

目标：为现场数据接入做准备。

需要实现：

1. 定义 `IDataSource` 接口。
2. 实现 `FileDataSource`。
3. 预留 `SerialDataSource`。
4. 预留 `SocketDataSource`。
5. 主程序不直接依赖具体输入方式。

接口示例：

```cpp
class IDataSource {
public:
    virtual bool readSample(std::vector<float>& sample) = 0;
    virtual ~IDataSource() = default;
};
```

验收标准：

1. 文件输入可以跑通。
2. 后续接串口或 socket 不需要改推理核心逻辑。

### v0.10.0 - 单变量 Linux 部署可跑通版

目标：形成第一个可部署版本。

功能范围：

1. Linux C++ 编译。
2. 加载 `model.onnx`。
3. 加载 `meta.json`。
4. 单变量实时输入。
5. 滑动窗口。
6. StandardScaler 归一化。
7. ONNX Runtime CPU 推理。
8. MSE 重构误差。
9. 阈值判断。
10. 连续异常报警。
11. 文件回放验证。

验收标准：

1. 在 Linux 目标平台能启动。
2. 能加载训练工程 v0.10.0 产物。
3. 能处理单变量时间序列。
4. 能稳定输出 `NORMAL` / `ALARM`。
5. 不依赖 Python。

### v0.11.0 - 多变量输入支持

目标：支持多个传感器或多个特征。

需要实现：

1. 多列 CSV 输入。
2. 多变量 `SlidingWindow`。
3. 多变量归一化。
4. 多变量 flatten。
5. 与 train 工程的 feature 顺序对齐。

验收标准：

1. `feature_dim > 1` 时能正常运行。
2. `input_dim == window_size * feature_dim`。
3. 多变量 C++ 推理结果与 Python 对齐。

### v0.12.0 - 现场报警输出

目标：将 ALARM 状态对接真实系统。

可选实现：

1. 日志文件。
2. 标准输出。
3. MQTT。
4. TCP/UDP。
5. 串口。
6. GPIO。

验收标准：

1. 报警输出与推理核心解耦。
2. 输出失败不能导致推理线程静默退出。
3. 每次报警和恢复都有可追溯记录。

### v0.13.0 - 性能与稳定性优化

目标：满足嵌入式长期运行要求。

需要实现：

1. 长时间运行测试。
2. 内存占用监控。
3. 推理耗时统计。
4. 异常输入处理。
5. 日志滚动。
6. 进程退出清理。

验收标准：

1. 连续运行 24 小时无崩溃。
2. 内存无明显泄漏。
3. 单窗口推理延迟满足现场实时性要求。

## 7. 推荐实施顺序

严格按以下顺序推进：

```text
v0.1.0 -> v0.2.0 -> v0.3.0 -> v0.4.0 -> v0.5.0 -> v0.6.0 -> v0.7.0 -> v0.8.0 -> v0.9.0 -> v0.10.0
```

`v0.10.0` 完成前，不建议提前接 GPIO、MQTT、串口等现场功能。

## 8. 第一阶段最小可行目标

最小可行版本到 `v0.10.0` 为止，必须实现：

```text
model.onnx + meta.json
  ↓
Linux C++ 加载
  ↓
文件模拟实时输入
  ↓
滑动窗口
  ↓
归一化
  ↓
ONNX Runtime 推理
  ↓
MSE
  ↓
阈值判断
  ↓
连续异常报警
```

该版本完成后，再推进多变量、真实数据接口、现场报警输出和性能优化。
