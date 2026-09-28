# 3dmaster-seer 🚀

> **面向 Windows Seer 的工业级高性能 3D/CAD 模型空格极速预览插件**  
> 支持 STEP、IGES、glTF、3MF、STL、OBJ、PLY 以及西门子 UG/NX 原生零件！

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Seer Plugin](https://img.shields.io/badge/Seer-Plugin-blue.svg)](https://1218.io)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20(x64)-brightgreen.svg)]()

---

## 🌟 核心特性 (Key Features)

- ⚡ **无缝嵌入 Seer 宿主**：基于官方标准 Seer SDK 与 Qt6 C++ 原生动态库架构，纯内存级秒级调度，无任何 IPC 中转开销。
- 🛠️ **全格式工业支持**：
  - **STEP / STP** (`.step`, `.stp`)：完整装配树结构、材质固有色渲染与拓扑实体解析。
  - **IGES / IGS** (`.iges`, `.igs`)：工业曲线曲面高保真缝合与几何特征呈现。
  - **现代 3D 网格**：**glTF / GLB** (`.gltf`, `.glb`)、**3MF** (`.3mf`，支持多色与原型实例化)、**STL** (`.stl`)、**OBJ** (`.obj`)、**PLY** (`.ply`)、**OFF** (`.off`)。
  - **UG/NX PRT** (`.prt`)：内置智能二进制嗅探。若本机装有 UG/NX，自动静默后台转码并启用 SHA-256 磁盘二级缓存（再次打开秒级加载）；若未安装则提供专业 CAD 导出导向卡片。
- 📐 **专业 CAD 交互视口**：
  - 黑色 CAD 特征棱线（CAD Edges）清晰勾勒。
  - 动态三向截面剖切（Dynamic Section View），支持滑块平滑控制内部构造。
  - 预设手感切换：**西门子 UG/NX**（中键旋转、左键平移）、**SolidWorks**（中键旋转、Ctrl+中键平移）、**通用模式**（右键旋转、中键平移）。
  - 标准六向工程视图（前/后/左/右/俯/仰）一键切换与窗口自适应居中（Ctrl+F）。
  - 右侧多层级零件装配树交互，支持单个零件独立显隐控制。
  - 正交/透视投影无缝切换。

---

## 📥 安装指南 (Installation)

1. 从 Release 页面下载编译好的 `3dmaster.dll`、`plugin.json` 以及关联运行时库。
2. 将插件文件夹放入 Seer 的插件目录中：
   ```text
   Seer安装目录\plugins\3dmaster\
   ├── 3dmaster.dll
   ├── plugin.json
   └── 3dmaster.ini
   ```
3. 打开 Seer 设置 -> 插件列表，确保已启用 `3dmaster` 插件。
4. 在文件资源管理器中选中任一支持的 3D 模型，按 **空格键** 即可立刻极速预览！

---

## 💻 编译构建 (Build from Source)

### 环境依赖
- Windows 10 / 11 (x64)
- Visual Studio 2022 (MSVC v143, 支持 C++17)
- Qt 6.8.2 (MSVC 2022 64-bit)
- OpenCASCADE Technology (OCCT) 8.0.0
- Seer SDK (位于 `Seer-sdk/` 目录或自动配置)

### 编译步骤
```powershell
cd 3dmaster
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
编译产物 `3dmaster.dll` 将输出至 `build/Release/` 目录。

---

## 🤝 姐妹项目 (Sister Projects)

- 🔍 **[3dmaster-quicklook](https://github.com/luxp1990/3dmaster-quicklook)**：面向 Windows **QuickLook** 用户的 100% 绿色便携 3D/CAD 预览插件。

---

## 📄 开源许可证 (License)

本项目采用 [MIT License](LICENSE) 许可证开源。
