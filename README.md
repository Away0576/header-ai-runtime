# header-ai-runtime

## 工程用途

`header-ai-runtime` 是时间序列异常检测的 Linux 嵌入式部署工程，用 C++ 加载训练工程导出的 `model.onnx` 和 `meta.json`，完成实时推理、重构误差计算、阈值判断和报警逻辑。

本工程最终部署形态：

```text
header_ai_detector + model.onnx + meta.json
```

## 工程边界

本工程负责：

1. 加载 `meta.json`。
2. 加载 `model.onnx`。
3. 接收实时数据输入。
4. 维护滑动窗口。
5. 执行归一化。
6. 调用 ONNX Runtime C++ 推理。
7. 计算 MSE 重构误差。
8. 阈值判断。
9. 连续异常报警和恢复。

本工程不负责：

1. PyTorch 训练。
2. ONNX 导出。
3. 阈值训练计算。
4. 模型调参。
5. Python 训练脚本。

职责边界：

1. 本工程的职责起点是加载 `model.onnx` 和 `meta.json`。
2. 本工程不包含 PyTorch 训练、ONNX 导出和阈值训练计算逻辑。
3. 本工程必须严格按照 `meta.json` 合同执行滑窗、归一化、推理和阈值判断。

## 与 train 工程的接口

`header-ai-runtime` 只消费 `header-ai-train` 产出的：

```text
model.onnx
meta.json
```

runtime 不应依赖：

1. `model.pt`。
2. Python。
3. PyTorch。
4. pickle 文件。
5. 训练数据。

## 运行时主流程

```text
实时数据输入
  -> SlidingWindow
  -> Normalizer
  -> ONNX Runtime
  -> MSE 重构误差
  -> 阈值判断
  -> 连续异常报警
```

## 当前实现状态

当前工程已完成 `v0.1.0` 到 `v0.5.0` 的基础 runtime 能力：

1. CMake C++ 工程骨架。
2. `header_ai_detector` 可执行程序。
3. `meta.json` 读取与合同校验。
4. 单变量/多变量预留的 `SlidingWindow`。
5. `time_major` 展平。
6. StandardScaler 归一化。
7. CTest 自测。
8. ONNX Runtime C++ 模型加载和单次推理探测。

ONNX Runtime SDK 默认放置路径：

```text
third_party/onnxruntime-linux-x64-1.18.1
```

也可以通过 CMake 参数指定：

```bash
cmake -S . -B build -DONNXRUNTIME_ROOT=/path/to/onnxruntime-linux-x64-1.18.1
```

## Ubuntu/WSL 构建与验收

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

查看程序版本：

```bash
./build/header_ai_detector --version
```

加载训练工程生成的 `meta.json`：

```bash
./build/header_ai_detector --meta /path/to/meta.json
```

加载 `model.onnx` 并执行一次零输入推理探测：

```bash
./build/header_ai_detector --model /path/to/model.onnx --meta /path/to/meta.json --probe-onnx
```

## 版本推进顺序

详细版本拆分见：

```text
VERSION_PLAN.md
```

版本号统一采用：

```text
vA.B.C
```

规则：

| 字段 | 含义 |
|---|---|
| `A` | 大版本号，只有大规模更新、正式大版本发布或重大不兼容变更时修改 |
| `B` | 功能版本号，每添加一个新功能递增 |
| `C` | 修复版本号，用于 debug、缺陷修复和小范围功能修正 |

当前阶段固定在 `v0.B.C`，先把训练工程和 runtime 工程整体跑通，不进入 `v1`。

推荐按以下顺序推进：

```text
v0.1.0 -> v0.2.0 -> v0.3.0 -> v0.4.0 -> v0.5.0 -> v0.6.0 -> v0.7.0 -> v0.8.0 -> v0.9.0 -> v0.10.0
```

第一阶段目标是完成 `v0.10.0`：Linux C++ 单变量时间序列离线回放和实时推理可跑通版。

## v0.10.0 最小交付目标

`v0.10.0` 完成时，本工程应稳定支持：

1. Linux C++ 编译。
2. `meta.json` 解析和校验。
3. `model.onnx` 加载。
4. 单变量滑动窗口。
5. StandardScaler 归一化。
6. ONNX Runtime CPU 推理。
7. MSE 重构误差计算。
8. 阈值判断。
9. 连续异常报警。
10. 文件回放模拟实时输入。

## 后续扩展

`v0.10.0` 稳定后，再扩展：

1. 多变量输入。
2. 串口、Socket、CAN 等实时数据源。
3. GPIO、MQTT、日志等报警输出。
4. 性能统计。
5. 长时间稳定性测试。
