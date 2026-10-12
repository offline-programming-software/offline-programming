# olp Release（Windows x64）构建与统信 UOS 运行说明

本分支 `feature/release-win64` 用于产出 **Release|x64 的 `olp.exe`**，并说明它在统信 UOS 上如何运行。

- 构建配置：`Release|x64`，MSVC v141（Visual Studio 2017，14.16.27023），Qt 5.12.8 msvc2017_64
- 第三方库：vcpkg `x64-windows`（libzmq 4.3.5、libarchive、OpenSSL 3、zlib、lzma、lz4、zstd）
- 产物：`olp\x64\Release\olp.exe`（约 3.0 MB，已内嵌 `PQKit.manifest`）
- 子系统：`/SUBSYSTEM:CONSOLE`（工程原有设定，运行时会附带一个控制台窗口）

---

## 一、结论先说：能不能在统信 UOS 上跑

**`olp.exe` 是 Windows PE 程序，统信 UOS 是 Linux 系统，二者不能直接兼容运行。** 只有两条路：

| 目标机器 | 可行性 | 做法 |
| --- | --- | --- |
| **x86_64 UOS**（兆芯 / 海光 / Intel / AMD） | 可以，需实测 | 用 UOS 自带的 Windows 兼容层（deepin-wine）运行本文的部署包 |
| **ARM64 UOS**（飞腾 / 鲲鹏 / 龙芯） | **不可以** | x86 的 Wine 不能执行 x64 PE；只能做原生 Linux 移植 |

原生移植不是换编译参数就能完成的：机器人内核 `PQKit` 是 Windows 专用 SDK（免注册 COM `RPC.dll`/`UPApiBase.dll`、MFC、Hoops3d、内置 Python/Ruby），
且 `MainWindow::InitPQKit()`、`PQKitCallback`、各业务类全部依赖 `#import "RPC.tlb"` 生成的 COM 包装。
要原生支持 UOS，需要把 Qt 工程改为 Linux 版 qmake/CMake 工程，并用 Linux 版内核替换 PQKit 及全部 COM 调用。

---

## 二、编译

### 环境要求

| 组件 | 本机实际使用 |
| --- | --- |
| Visual Studio 2017（v141 工具集） | `C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional` |
| Qt 5.12.8 msvc2017_64 | `D:\Qt\Qt5.12.8\5.12.8\msvc2017_64` |
| Qt VS Tools 的 QtMsBuild | `%LOCALAPPDATA%\QtMsBuild` |
| vcpkg（x64-windows） | `D:\vcpkg`（含 eigen3、nlohmann、openssl、lzma 等） |

### 一键构建

```powershell
powershell -ExecutionPolicy Bypass -File tools\build-release-msvc2017.ps1
```

等价的手工命令：

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\MSBuild.exe' `
  'olp\olp.sln' /p:Configuration=Release /p:Platform=x64 `
  /p:QtMsBuild="$env:LOCALAPPDATA\QtMsBuild" /m
```

### 本次为让 Release 能编过所做的工程修改（`olp\olp.vcxproj`）

原工程 Release 配置**无法编译**（首次尝试报 55 个错误），共修 5 处：

1. **新增 PreBuildEvent 预置 `rpc.tlh`**：`include\pqkitcallback.h` 直接 `#include "rpc.tlh"`，而该文件是 `RPC.tlb` 经 `#import` 生成的中间头文件。
   干净构建时先由 `olp\prebuild\rpc.tlh` 拷入中间目录，否则报 `fatal error C1083: 无法打开包括文件: "rpc.tlh"`。
   （`olp\prebuild\rpc.tlh` 已随本分支提交，86.9 KB。）
2. **修正头文件目录**：原为 `D:\offline-programming\...`（该路径已不存在），改为 `E:\offline-programming\...`；
   并补齐 vcpkg 的 `eigen3`、`Eigen`、`Eigen\src` 与 vcpkg include 根目录。Debug 配置同步修正。
3. **Release 关闭 `/MP`**：`#import "RPC.tlb"` 与多进程编译互斥，报 `error C2813`；改为与 Debug 一致的单进程编译。
4. **Release 嵌入 `PQKit.manifest`**：原先只有 Debug 配置有。该清单是 PQKit 的免注册 COM（SxS）声明，
   缺失时 `InitPQKit()` 里的 `CoCreateInstance(__uuidof(PQPlatformComponent))` 会失败。
5. **`.gitignore` 增加 `dist/`**：防止 2 GB+ 的运行包被误提交。

> 注意：`.gitignore` 中 `olp/` 被整体忽略（历史提交「移除olp目录版本追踪」），
> 本次是用 `git add -f` 单独把 `olp\olp.vcxproj` 与 `olp\prebuild\rpc.tlh` 纳入版本的，其余 `olp/` 内容仍未跟踪。

---

## 三、部署运行包

在构建产物目录旁准备一个干净目录，依次：

```powershell
$dist = 'E:\offline-programming\dist\olp-Release-win64'
New-Item -ItemType Directory -Force $dist | Out-Null

# 1) 主程序 + vcpkg 运行库（AppLocal 已自动拷到 x64\Release）
Copy-Item 'olp\x64\Release\olp.exe' $dist
Copy-Item 'olp\x64\Release\*.dll'  $dist

# 2) Qt 运行库与插件
& 'D:\Qt\Qt5.12.8\5.12.8\msvc2017_64\bin\windeployqt.exe' --release --no-translations --dir $dist "$dist\olp.exe"

# 3) VC++ 运行库：必须用 14.4x，不能用 VS2017 自带的 14.16（见下方「已知坑」）
Copy-Item 'D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\x64\Microsoft.VC143.CRT\*.dll' $dist

# 4) PQKit 内核运行时（约 2.1 GB，必须与 olp.exe 同目录，不能改名）
robocopy 'olp\x64\Debug\PQKit' "$dist\PQKit" /E
```

### 已知坑：VC++ 运行库版本

PQKit 的 DLL 由 VS2022（14.38 运行库）编译。若在程序目录放入 VS2017 的旧运行库（14.16），
进程会在 PQKit 初始化时以 `0xC0000005` 崩溃，事件日志显示 `MSVCP140.dll` 访问冲突。
**必须放 14.4x（本分支使用 14.44.35211）。**

### 已验证结果（Windows 11 x64）

- `dumpbin`：machine `8664`（x64），Release 依赖（`Qt5Charts/Widgets/Gui/Core.dll`，无调试 CRT）
- `mt.exe` 提取：`PQKit` 免注册 COM 清单已嵌入
- 启动运行：主窗口标题「自动喷涂离线编程与仿真软件」，加载 307 个模块、常驻约 255 MB，
  PQKit 的 `RPC.dll`、`UPApiBase.dll`、`RobotArtPQ.dll` 均加载成功

---

## 四、在统信 UOS（x86_64）上运行

```bash
# 1) 安装兼容引擎（若系统未预装）
sudo apt install deepin-wine6-stable deepin-wine32 deepin-wine-helper

# 2) 进入部署包目录（工作目录必须是这里，PQKit 按相对路径加载）
cd /path/to/olp-Release-win64

# 3) 运行
export WINEPREFIX="$HOME/.wine-olp"     # 独立前缀，避免污染其它应用
export QT_OPENGL=software               # 三维视图/界面异常时强制软件 OpenGL
export LANG=zh_CN.UTF-8
deepin-wine6-stable ./olp.exe
```

用图形化「兼容引擎」添加应用时，工作目录要指向本目录，可执行文件选 `olp.exe`。

**仍需在目标机实测的风险**：PQKit 依赖免注册 COM、MFC、Hoops3d 三维渲染与内置 Python/Ruby 运行时，
Wine 对其支持并不完整 —— 界面能否起来、模型与轨迹功能是否可用，必须实机验证，本仓库无法预先保证。

---

## 五、交付物

| 位置 | 内容 | 是否入库 |
| --- | --- | --- |
| `olp\x64\Release\olp.exe` | Release 主程序（3.02 MB） | 否（构建产物） |
| `dist\olp-Release-win64\` | 完整可运行目录（2.17 GB，6465 个文件） | 否（`.gitignore` 已忽略 `dist/`） |
| `dist\olp-Release-win64.zip` | 可直接拷到 UOS 的压缩包（675.8 MB） | 否 |
| `olp\olp.vcxproj`、`olp\prebuild\rpc.tlh` | 本次构建修复与中间头文件 | **是** |
| `tools\build-release-msvc2017.ps1` | 一键 Release 构建脚本 | **是** |
