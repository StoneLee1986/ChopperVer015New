# CHOPPER · AI Dev 分支说明

> 本文件只存在于 `ai-dev` 分支，用来记录「这个分支和原版有什么不同」，以及人和 AI 的协作约定。

## 这是什么

- 原版工程：`E:\BaiduSyncdisk\...\ChopperVer015New`（主工程，**不要动**）
- AI 开发版：`ChopperVer015New_AIDev`（本项目，独立分支 `ai-dev`）
- 两者是**同一个仓库的两个分支**，不是一个仓库两套代码。改动互不影响，随时可以互相 cherry-pick。

名字带 `_AIDev` 是刻意的：VS 的「最近项目」列表里能一眼分清，不会手滑打开错的那个。
运行起来的窗口标题也是 `CHOPPER - AI Dev`。

## 与原版（main）的差异

| 项目 | 原版 | 本分支 |
|---|---|---|
| SFML 路径 | 写死 `E:\BaiduSyncdisk\...\SFML2` | `$(ProjectDir)SFML2`，随项目走 |
| Release 链接库 | 误用 Debug 库 `sfml-*-d.lib` | 正确使用 `sfml-*.lib` |
| 平台 | Win32 + x64（x64 必失败） | 只保留 Win32 |
| sfml-main | 链接了 | 已移除（Console 子系统直接用 `main()`） |
| 工程名 | ChopperVer015New | ChopperVer015New_AIDev |
| 窗口标题 | Timber!!! | CHOPPER - AI Dev |

## 怎么构建 / 运行

1. 双击 `ChopperVer015New_AIDev.slnx`（VS 2026）
2. 平台选 **x86**，配置 Debug 或 Release
3. **F5** 运行 —— 工作目录已设为项目根，DLL 和 `graphics/` 都能找到

命令行构建（不用开 VS）：

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
  .\ChopperVer015New_AIDev.vcxproj /p:Configuration=Debug /p:Platform=Win32 /m
```

产物在 `Debug\` / `Release\`，exe 名 `ChopperVer015New_AIDev.exe`。

## 已知限制

- **只有 32 位**。随附的 SFML 2.6.0 是 x86 静态库，x64 构建会 `LNK4272` + 75 个未解析符号。
  想上 x64 必须换一份 64 位 SFML（`SFML2\lib` 和根目录 DLL 一起换）。
- **不要双击 `Debug\` 里的 exe**：DLL 在项目根、资源是相对路径，双击会找不到 `graphics/`。要么 F5，要么把 exe 拷到项目根再运行。

## 协作约定（省钱的关键）

1. **AI 直接读文件，不要往对话里粘代码。** 读 5000 行约 0.06 元；粘一遍既贵又会过期。
2. **每轮改完必须编译**（Debug + Release 各一次）。编译不花 token，是这里最便宜的正确性验证。
3. **只提交到 `ai-dev`，永不碰 `main`。** 想合并回主线由人决定。
4. 提交信息写清楚「改了什么 / 为什么」，方便回退。
5. 提需求时给目标 + 现象，不用给实现细节；报错直接贴完整编译输出。

## 分支工作流

```powershell
# 本分支已经建好并提交，推送到 GitHub（只需第一次带 -u）
git push -u origin ai-dev

# 以后同步 main 的新提交到本分支
git fetch origin
git merge origin/main        # 或 git rebase origin/main

# 查看差异
git log --oneline ai-dev ^main
git diff main...ai-dev
```

## 当前状态

- [x] 工程改名、路径相对化、Release 库修正、x64 移除
- [x] Debug|Win32 / Release|Win32 均 0 错误 0 警告
- [ ] 待补：把 `snapshot.ps1` 的用途写清楚
- [ ] 待补：音频延迟手动校准（原版只有自动同步）
