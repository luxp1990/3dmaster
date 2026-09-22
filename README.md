# 3dmaster — 工业级 3D/CAD 模型预览插件

<p align="center">
  <strong>高性能 3D 模型预览插件，同时支持 <a href="https://github.com/ccseer/Seer">Seer</a> 文件预览器和 <a href="https://github.com/QL-Win/QuickLook">QuickLook</a> 快速预览</strong>
</p>

## ✨ 功能特性

- **广泛格式支持**：STEP (.stp/.step), IGES (.igs/.iges), glTF/GLB, 3MF, STL, OBJ, PLY, OFF
- **真实材质色彩渲染**：自动提取 CAD 零件颜色，视线自适应双面光照
- **装配体支持**：零件树状列表、独立显隐控制、隔离模式
- **交互操作手感预设**：西门子 UG NX / SolidWorks / 通用 三种操控模式
- **动态剖视**：X/Y/Z 三轴截面实时剖切
- **辅助显示**：地面网格、坐标轴、线框、特征棱线、包围盒
- **多种着色模式**：固有材质色、顶点色、工业白模、法线可视化
- **异步加载**：多线程后台解析，不卡主界面
- **暗色主题 UI**：紧凑型折叠侧边栏，可滚动自适应

## 🏗 项目架构

本项目采用 **Monorepo** 结构，两个插件共享核心 3D 渲染和模型加载代码：

```
3dmaster/
├── 3dmaster/                    # Seer 插件 (C++ DLL)
│   ├── src/                     # 核心源码
│   │   ├── master_widget.cpp    # ★ OpenGL 3D 视口 (共享)
│   │   ├── master_sidebar.cpp   # ★ 侧边控制面板 (共享)
│   │   ├── model_loader_worker.cpp # ★ CAD 模型加载 (共享)
│   │   ├── master_viewer.cpp    # Seer 查看器容器
│   │   └── plugin_entry.cpp     # Seer 插件入口
│   ├── include/                 # 头文件
│   └── test/                    # 测试程序
│
├── 3dmaster-quicklook/          # QuickLook 插件
│   ├── 3dmaster-preview/        # C++ 守护进程 (嵌入 Win32 窗口)
│   │   ├── src/
│   │   │   ├── preview_main.cpp
│   │   │   ├── preview_window.cpp
│   │   │   └── ipc_server.cpp
│   │   └── CMakeLists.txt
│   └── QuickLook.Plugin.3DMaster/  # C# WPF 插件壳
│       ├── Plugin.cs
│       ├── Model3DViewerHost.cs    # HwndHost 嵌入
│       └── DaemonClient.cs         # 命名管道 IPC
│
└── test_models/                 # 示例模型文件
```

**核心设计**：QuickLook 守护进程直接编译 `3dmaster/src/` 中的共享源码（`master_widget.cpp`, `master_sidebar.cpp`, `model_loader_worker.cpp`），**零代码复制**，确保两个插件行为完全一致。

## 📋 前置依赖

| 依赖 | 版本 | 说明 |
|------|------|------|
| [Open CASCADE Technology (OCCT)](https://dev.opencascade.org/) | 8.0+ | CAD 内核，提供 STEP/IGES 解析和 BRep 离散化 |
| [Qt 6](https://www.qt.io/) | 6.8+ | GUI 框架，提供 OpenGL Widget |
| [Seer SDK](https://github.com/ccseer/Seer-sdk) | latest | Seer 插件接口（自动通过 CMake FetchContent 获取） |
| [.NET SDK](https://dotnet.microsoft.com/) | 6.0+ | 编译 QuickLook C# 插件壳 |
| [Visual Studio 2022](https://visualstudio.microsoft.com/) | 17.x | MSVC 编译器 + CMake |
| [QuickLook](https://github.com/QL-Win/QuickLook) | latest | (仅 QuickLook 插件需要) |

### 依赖目录结构

将 OCCT 和 Qt6 放在与本仓库同级或仓库根目录下：

```
3dmaster/              # 本仓库根目录
├── occt/              # OCCT 安装目录
│   ├── inc/           # 头文件
│   └── win64/vc14/lib/  # 链接库
├── qt6/6.8.x/msvc2022_64/  # Qt6 安装目录
└── ...
```

## 🔨 构建步骤

### 1. Seer 插件 (`3dmaster.dll`)

```powershell
# 配置 (首次)
cd 3dmaster
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="D:/path/to/qt6/6.8.2/msvc2022_64"

# 编译
cmake --build build --config Release --target 3dmaster

# 产出: build/Release/3dmaster.dll
```

### 2. QuickLook 插件

#### 2a. 构建 C++ 守护进程 (`3dmaster-preview.exe`)

```powershell
cd 3dmaster-quicklook/3dmaster-preview
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="D:/path/to/qt6/6.8.2/msvc2022_64"

cmake --build build --config Release --target 3dmaster-preview

# 产出: build/Release/3dmaster-preview.exe
```

#### 2b. 构建 C# 插件壳 (`QuickLook.Plugin.ThreeDMaster.dll`)

```powershell
cd 3dmaster-quicklook/QuickLook.Plugin.3DMaster
dotnet build QuickLook.Plugin.3DMaster.csproj -c Release

# 产出: bin/Release/QuickLook.Plugin.ThreeDMaster.dll
```

#### 2c. 打包 `.qlplugin` (可选)

```powershell
cd 3dmaster-quicklook/QuickLook.Plugin.3DMaster
.\pack-plugin.ps1

# 产出: ../QuickLook.Plugin.ThreeDMaster.qlplugin
```

## 📦 安装部署

### Seer 插件

1. 将 `3dmaster.dll` + OCCT 运行时 DLL (`TK*.dll`) + Qt6 运行时 DLL + `plugin.json` 放入 Seer 插件目录
2. 重启 Seer

### QuickLook 插件

1. 将 `3dmaster-preview.exe` + OCCT 运行时 DLL + Qt6 运行时 DLL 放入插件目录
2. 将 `QuickLook.Plugin.ThreeDMaster.dll` + `QuickLook.Plugin.Metadata.config` 放入：
   ```
   %LocalAppData%\Packages\21090PaddyXu.QuickLook_egxr34yet59cg\LocalCache\Roaming\pooi.moe\QuickLook\QuickLook.Plugin\QuickLook.Plugin.ThreeDMaster\
   ```
3. 重启 QuickLook

## 🗂 支持的文件格式

| 格式 | 扩展名 | 引擎 |
|------|--------|------|
| STEP | `.stp`, `.step` | OCCT TKDESTEP |
| IGES | `.igs`, `.iges` | OCCT TKDEIGES |
| glTF | `.gltf`, `.glb` | OCCT TKDEGLTF |
| 3MF | `.3mf` | Qt XML 解析 |
| STL | `.stl` | 内置解析器 |
| OBJ | `.obj` | 内置解析器 |
| PLY | `.ply` | 内置解析器 |
| OFF | `.off` | 内置解析器 |

## 📄 许可证

本项目基于 [MIT License](LICENSE) 开源。

本项目使用 [Open CASCADE Technology (OCCT)](https://dev.opencascade.org/)，其基于 LGPL-2.1 许可证发布。
