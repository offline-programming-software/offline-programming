# olp.exe 在统信 UOS 上跑不通：排查与解决路线

本文回答「怎么解决」，并给出可直接在 UOS 上执行的排查/修复脚本。
**结论先讲**：这不是编译参数问题，而是「Windows 专用内核 PQKit + COM/MFC 体系 vs Linux 系统」的架构问题。
能否解决、代价多大，取决于下面三件事。

---

## 0. 先确诊：三种症状指向三条不同的路

| 现象 | 大概率原因 | 该走哪条路 |
| --- | --- | --- |
| 完全起不来 / 闪退，退出码 `0xC0000135`、`0xC000007B` | 缺少 VC 运行库、UCRT 或位数不符 | 路线 A（装运行库） |
| 界面能出来，弹 **「PQKit 初始化失败」** | `CoCreateInstance` 失败 `0x80040154 (REGDB_E_CLASSNOTREG)`：wine 对清单里「免注册 COM」支持不完整 | 路线 A（脚本 03 显式注册 COM） |
| 界面能用，一打开模型 / 生成轨迹就崩，日志里是 `RPC.dll`、`5axcore.dll`、`Hoops3d`、`python` 相关 | PQKit 内部（MFC + Hoops3d 渲染 + 内置 Python/pywin32/.NET）在 wine 下不完整 | 路线 A 继续试；不行就只能 B / C / D |
| 界面花屏 / 全黑 / 三维视图空白 | Wine 的 OpenGL-D3D 转换 | 路线 A（`QT_OPENGL=software` 等） |

**第一步请先跑诊断脚本并回传结果**（不改动任何东西，只采集信息）：

```bash
cd <部署目录>            # 与 olp.exe 同级
bash uos/01-diagnose-on-uos.sh .
# 产出：olp-uos-report.txt （环境 + 目录自检 + 关键错误行）
#       olp-wine-run.log   （WINEDEBUG=+actctx,+ole,+loaddll,+seh 原始日志）
```

把这两个文件回传，基本就能定位到具体原因和具体缺失项。

---

## 1. 路线 A：把 Windows 版在 UOS 兼容层里调通（成本 1～3 人日，成功率中）

```bash
sudo apt install winetricks                       # 若未安装
export WINEPREFIX=$HOME/.wine-olp
bash uos/02-fix-wineprefix.sh .                   # 建前缀 + 装运行库/字体/组件
bash uos/03-register-pqkit.sh .                   # 显式注册 PQKit 的 COM 组件（关键）
cd <部署目录> && QT_OPENGL=software deepin-wine6-stable ./olp.exe
```

要点说明：

1. **COM 必须显式注册**。程序靠内嵌 `PQKit.manifest` 走免注册 COM，而 wine 的 SxS/actctx 实现对 `comClass` 支持不完整。
   经检查，`UPApiBase.dll`、`UPApiRawData.dll`、`RobotArt/bin/RPC.dll`、`RobotArt/bin/RobotArtPQ.dll`
   四个文件都导出了 `DllRegisterServer`/`DllGetClassObject`，所以用 `wine regsvr32` 注册即可绕过。
   `03-register-pqkit.sh` 会先试 `regsvr32`，失败时回退为导入由 `PQKit.manifest` 生成的
   `PQKit-com-reg.template.reg`（94 个 COM 类 + 2 个类型库）。
2. **运行库版本必须是 14.4x**。PQKit 的 DLL 由 VS2022 编译；放 VS2017 的 14.16 运行库会在初始化时 `0xC0000005` 崩溃（本仓库发布包已用 14.44）。
3. **图形栈**：先试 `QT_OPENGL=software`；仍异常再 `winetricks renderer=gl`、`LIBGL_ALWAYS_SOFTWARE=1`，
   必要时 `WINEDLLOVERRIDES="d3d11,dxgi=b"` 之类逐个试。
4. **中文字体**：`winetricks cjkfonts`（或系统里装 Noto CJK），否则界面字体回退、可能乱码。
5. **换更新的 wine**：UOS 自带的 deepin-wine 版本较老，装 wine-staging 8/9 往往能直接解决一批 COM/图形问题；
   用不同 `WINEPREFIX` 与新 wine 组合对比测试，成本很低、收益不低。
6. **看日志**：`WINEDEBUG=+actctx,+ole,+loaddll,+seh` 抓到的第一处 `err:` 通常就是根因。

**这一路的边界**：PQKit 依赖 MFC、Hoops3d 三维渲染、免注册 COM、内置 Python/Ruby（pywin32、pythonnet 需要 .NET）。
wine 对这套组合支持不完整 —— 界面、文件相关功能希望较大，**核心的模型/轨迹功能不保证能跑通**。
另外，**信创/国产化验收通常明确要求原生，Wine 方案一般过不了验收**，只适合内部试用或过渡。

---

## 2. 路线 B：拆进程 —— UOS 原生界面 + Windows 侧 PQKit 服务（成本 1～3 周）

本工程已经有 ZeroMQ 基础设施（`src/OnlineProcess/Client.cpp`、`Device.cpp`、`worker/zmqWorker.cpp`），
天然适合把「Windows 专用内核」隔离成一个服务：

- Windows 侧（真机或 UOS 上的 KVM 虚拟机）跑一个 PQKit 服务进程，把现有
  `InitPQKit()` / `CPQKitCallback` 的能力暴露成 zmq 请求-应答 + 回调；
- UOS 侧把 Qt 界面用 Linux 版 Qt 原生编译，删掉 `ATL/COM`、`#import "RPC.tlb"`、`CComPtr`，
  改成对服务的调用；
- 好处：**不用替换几何/路径内核**，界面原生、计算完全复用现有 Windows 代码；
- 代价：部署形态变成两台机器/一个虚拟机；验收是否认可「原生界面 + Windows 后端」需要提前确认。

---

## 3. 路线 C：原生移植（成本数人月，唯一能过信创验收的形态）

**先确认一件事：PQKit 供应商有没有 Linux（或国产化平台）版本？**

- **有**：工作变成「改接口 + 换工程」——
  `.vcxproj/.sln` → CMake；Qt 5.12 MSVC → Qt 5.15/6（UOS 源或自编）；
  `#import "RPC.tlb"` + `CComPtr<IPQPlatformComponent>` + `PQKitCallback`（IDispatch 回调）
  → 纯 C++ 抽象接口（**方法名可以一一保留**，约 100+ 个调用点机械替换）；
  去掉 `Resource.rc/.aps`、`PQKit.manifest`、`windows.h`、`#pragma execution_character_set`。1～2 人月量级。
- **没有**：必须**替换内核**，这是主要成本：
  | PQKit 提供的能力 | Linux 可用替代 |
  | --- | --- |
  | STEP/IGES 等几何导入、B-Rep | OpenCASCADE (OCCT) |
  | 网格/格式转换 | Assimp |
  | 三维显示（Hoops3d） | OpenSceneGraph / VTK / Qt3D |
  | 路径生成、运动学逆解、碰撞检测 | 需自研或采购，**最难替换** |

  建议拆里程碑：先「先把界面 + 文件 IO 原生化，内核用桩件（stub）」，跑通后再逐个替换内核能力，
  这样每一阶段都可交付、可回退。

---

## 4. 路线 D：不改程序（成本最低）

在 UOS 上跑 Windows 虚拟机（KVM + Windows）或远程桌面 / 云桌面。
可以立刻交付演示，但不属于国产化替代，长期不可行。

---

## 5. 需要你确认的三个问题

1. **PQKit 有没有 Linux/国产化版本？**（决定路线 C 是「改接口」还是「重写内核」）
2. **目标 UOS 是 x86_64 还是 ARM64？**（ARM64 直接排除路线 A；飞腾/鲲鹏/龙芯必须走 C）
3. **验收要求是「必须原生」还是「能用即可」？**（决定能不能用 A/B/D）

---

## 6. 常见退出码 / 报错对照

| 报错 | 含义 | 处理 |
| --- | --- | --- |
| `0xC0000135` STATUS_DLL_NOT_FOUND | 缺 DLL（常见：msvcp140/vcruntime140/ucrtbase） | 跑 `02-fix-wineprefix.sh`；确认运行库是 14.4x |
| `0xC000007B` STATUS_INVALID_IMAGE_FORMAT | 32/64 位不匹配 | 用 win64 前缀；确认是 x64 包 |
| `0x80040154` REGDB_E_CLASSNOTREG | COM 类未注册 | 跑 `03-register-pqkit.sh` |
| `0xC0000005` 访问冲突 | 多为运行库版本混用（14.16 与 PQKit 的 14.38） | 换成 14.4x 运行库 |
| 日志里 `err:module:import_dll Library XXX.dll not found` | PQKit 内部依赖缺失 | 按名字补 winetricks 组件（msxml6/gdiplus/riched20/dotnet48 等） |
| 三维视图空白/花屏 | OpenGL/D3D 转换 | `QT_OPENGL=software`、`wine` 换版本、`winetricks renderer=gl` |

---

## 7. 本目录文件

| 文件 | 作用 |
| --- | --- |
| `01-diagnose-on-uos.sh` | 采集环境 + 启动日志，生成可回传的报告 |
| `02-fix-wineprefix.sh` | 建专用 wine 前缀，装运行库 / 字体 / GUI 组件 |
| `03-register-pqkit.sh` | `regsvr32` 注册 PQKit COM 组件，失败时回退导入注册表 |
| `04-verify-on-uos.sh` | **可运行性验收**：启动一次并逐项 PASS/FAIL，指出下一步 |
| `05-pack-portable.sh` | 把验收通过的环境冻结成**自包含可复现包**（前缀 + 应用 + 启动器） |
| `PQKit-com-reg.template.reg` | 由 `olp/PQKit.manifest` 生成：94 个 COM 类 + 2 个类型库 |
| `.gitattributes` | 强制本目录 `.sh` 以 LF 换行提交（否则拷到 Linux 会报 `\r` 错误） |

---

## 8. 「能不能保证在 UOS 上跑起来」——工程上怎么做

**先说结论：没有任何人能在不接触目标机的情况下"保证"它跑起来。** Wine 兼容性取决于
PQKit 内部（MFC + Hoops3d + Python/.NET）的具体调用路径，只能实测。
但可以把「碰运气」变成「可验证 + 可复现」，具体四件事：

### 8.1 把失败分层，逐层拿掉（不要一上来就猜）

| 层 | 典型失败 | 判据 | 对策 |
| --- | --- | --- | --- |
| L1 运行库 | `0xC0000135` / `0xC000007B` | 进程起不来 | 用 14.4x 运行库；`winetricks vcrun2022` |
| L2 COM 注册 | 「PQKit 初始化失败」/`0x80040154` | 日志有 `REGDB_E_CLASSNOTREG` | 脚本 03 显式 `regsvr32`（**已确认四个 DLL 都导出 `DllRegisterServer`**） |
| L3 依赖 DLL | `err:module:import_dll` | 日志指名缺哪个 | 按缺的名字补 `winetricks`（msxml6 / gdiplus / riched20 / dotnet48…） |
| L4 图形 | 花屏 / 三维视图空白 | 界面出来但显示异常 | `QT_OPENGL=software`、`winetricks renderer=gl`、换 wine 版本 |
| L5 内核 | 打开模型 / 生成轨迹时崩 | 日志里 `5axcore`、`Hoops`、`python` 报错 | 这一层最难；见 8.4 兜底 |

脚本 `04-verify-on-uos.sh` 就是把 L1~L5 变成可自动判定的检查项：
进程是否存活、有无 `REGDB_E_CLASSNOTREG`、有无 `import_dll`、
PQKit 三元组（`RPC.dll`/`UPApiBase.dll`/`RobotArtPQ.dll`）是否被加载、内核 DLL 是否被加载、有无崩溃记录。

### 8.2 换掉 UOS 自带的旧 wine（性价比最高的一步）

UOS 自带的 deepin-wine 版本偏老，很多 COM/图形问题是新版 wine 已经修掉的。
装一份 `wine-staging`（8/9/10 任一），用**同一个前缀方案**对比测试，成本很低：
能过就固定用它，过不了再回头查具体报错。脚本都支持 `export WINE=wine` 指定运行器。

### 8.3 冻结环境：同一份包，在任何 UOS x86_64 上结果一致

环境差异（装了哪些原生 DLL、注册表内容、wine 版本）是"这台能跑、那台不能跑"的主因。
`05-pack-portable.sh` 把 **wine 前缀 + 应用 + 启动器 + 注册脚本**打成一个自包含目录：

- 应用放进前缀内的 `drive_c/olp`，注册表里记 `C:\olp\...`，因此**整包放到任何路径都能跑**；
- 前缀里连 `winetricks` 装的原生 DLL 与注册表一起带走；
- 打包脚本还会生成 `olp-portable.tar.gz`，拷到别的机器解压即用。

这样交付物就从「一个 exe + 一堆说明」变成「一份已验收、可复现的运行环境」——
这是在没有 PQKit Linux 版的前提下，最接近"保证运行"的形态。

### 8.4 万一内核层（L5）过不去：退化方案

需要提前和业务确认「哪些功能是必须的」：

1. **文件进/文件出**（导入模型 → 生成轨迹 → 导出结果）是核心。若只有**三维交互视图**出问题，
   可以考虑给程序加一个隐藏的批处理入口（`olp.exe --batch in.robx out.txt`），
   绕开窗口渲染，把核心计算留在 Wine 里跑——这条路径由我们自己改代码，可控性最高。
2. 若连 `CoCreateInstance` 之后的几何内核都起不来，Wine 这条路基本到头，
   此时只剩：换成 Windows 虚拟机/远程桌面（路线 D），或重新评估原生移植（路线 C）。

### 8.5 验收清单（建议作为交付判据）

- [ ] `04-verify-on-uos.sh` 全部 PASS
- [ ] 手动打开一个真实 `.robx` 工程，模型可见
- [ ] 生成一次轨迹并导出文件，与 Windows 版结果逐字节/逐行比对一致
- [ ] 连续运行 30 分钟无崩溃、无内存暴涨
- [ ] 用 `05-pack-portable.sh` 的包在**另一台** UOS 机器上重复以上步骤

> 建议按这个顺序推进：`01` 出报告 → `02/03` 修复 → `04` 验收 →
> 换 wine 版本再验收（对比）→ `05` 冻结 → 第二台机器复现。
> 每一步都有明确的成功判据，能清楚知道"卡在哪一层"，而不是反复试。
