# Starlight GUI

<p>
  <img src="https://img.shields.io/badge/License-Apache%202.0-blue.svg" alt="License">
  <img src="https://img.shields.io/badge/OSI-Certified-brightgreen.svg" alt="OSI Certified">
  <img src="https://img.shields.io/badge/Platform-Windows%2011-0078d7.svg" alt="Platform">
  <img src="https://img.shields.io/badge/Language-C++/WinRT-0078d7.svg" alt="Language">
</p>

## 简介

Starlight GUI 是一个基于 C++/WinRT 的 WinUI3 开源项目，为开发者的兴趣作品，致力于打造一个功能强大、界面美观、易于使用的开源 Windows 11 工具箱。项目采用原生的 WinUI3 设计，完美贴合 Windows 11 系统的操作习惯和视觉风格。拥有很多实用功能和自定义设置，改善日常电脑的使用体验。  
**注意：本软件的大部分功能需要在管理员权限下才能正常运行！**

**开发者**: Stars  
**许可证**: Apache 2.0 License | OSI Certified  
**Copyright © 2025 Starlight. All rights reserved.**

![Alt](https://repobeats.axiom.co/api/embed/33edd92df6ac6348e3eb2c6c1ba38046ca12e037.svg "Repobeats analytics image")
## 主要功能

### 任务管理器
- 完全重构的系统任务管理器
- 集成大部分系统任务管理器核心功能
- 驱动级结束进程能力
- 实时系统资源监控

### Terminator/进程终止器
- 提供**12种**不同的进程终止方法
- 使用**系统API/不稳定方法/漏洞**终止进程

### Elevator/权限提升器
- 通过系统API启动拥有TrustedInstaller权限的进程
- 支持完整SE特权获取

### Driver Loader/驱动加载器
- 通过系统服务加载驱动程序
- 支持绕过签名验证（通过系统漏洞）
*警告：该功能使用RTCore64.sys实现任意内存读写，此驱动已被大部分杀软标记为危险，若要使用请自行添加信任。*

### 设置菜单
- 功能模块配置
- 界面个性化设置

## 开发环境设置

### 环境
- Windows 11 操作系统
- Visual Studio 2022 或更高版本
- C++ 17以上 / WinRT WinUI3 开发环境

### 安装步骤

1. **克隆项目**
   ```bash
   git clone https://github.com/RinoRika/StarlightGUI.git
   cd StarlightGUI
   ```

2. **使用 Visual Studio 打开项目**
   - 打开 Visual Studio
   - 选择 "打开项目或解决方案"
   - 导航到项目目录，选择 `.sln` 解决方案文件

3. **还原 NuGet 包**
   - 在解决方案资源管理器中右键点击解决方案
   - 选择"还原 NuGet 包"

4. **配置构建环境**
   - 确保选择正确的构建配置（Debug/Release）
   - 该项目依赖WinUI3，请注意架构配置

5. **构建和运行**
   - 使用菜单中的"生成"→"生成解决方案"

6. **生成安装包**
   - 在解决方案资源管理器中右键点击"Starlight GUI (Desktop)"
   - 选择"打包和发布"→"创建应用程序包"

## 免责声明

**重要提示**: 本项目仅供学习和研究目的使用。使用者需对使用本软件所产生的任何后果承担全部责任。

- 本软件涉及系统底层操作，不当使用可能导致系统不稳定
- 通过本软件加载或创建的内容可能有潜在安全风险

安装并运行本软件，即代表您已**阅读并同意**以下内容：

- 您正在了解相关风险的情况下使用
- 您因使用本软件造成的任何损失，开发者均不负责
- 您遵守当地法律法规，合法使用本软件，不用于恶意用途
- 您理解本软件开发者仅为兴趣所做，本软件的最终解释权为开发者所有
- 您保证不对本软件及其开发者发表任何侮辱性、污蔑性言论

## 特别感谢

### AI辅助开发
- **Deepseek**
- **ChatGPT**
- **Copilot**

### 项目支持
- **Wormwaker** - 技术思路启发和参考(Terminator,Elevator)
- **Kali_MC** - 驱动开发

### 开发环境
- **Visual Studio** - 强大的集成开发环境
- **Microsoft WinUI3** - 现代化UI框架
- **C++/WinRT** - 高效Windows运行时支持

---

<p align="center">
  <sub>使用Starlight GUI，体验原生的Windows 11万能工具箱</sub>
</p>
