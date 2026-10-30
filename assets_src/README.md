# assets_src —— 原始素材包（不进 git）

这里放从 Kenney 下载的**原始压缩包解压目录**。真正的游戏用图会从里面挑出来，
复制到 `graphics/` 下，所以这个目录只是"仓库"，不参与构建，也不进版本库。

## 要下的包

| 包 | 用途 | 地址 |
|---|---|---|
| **Fish Pack** | 主包：玩家、目标物、障碍、背景 | https://kenney.nl/assets/fish-pack |
| **New Platformer Pack** | 借零件：地形、道具、装饰 | https://kenney.nl/assets/new-platformer-pack |
| **UI Pack** | 按钮、面板、进度条 | https://kenney.nl/assets/ui-pack |
| **Input Prompts** | 键盘按键图标（←→↑↓、空格） | https://kenney.nl/assets/input-prompts |
| **Audio** | 打击音、环境音 | https://kenney.nl/assets/category:Audio |

全部 **CC0**：可商用、可改、不用署名。归入 `CREDITS.md` 时仍建议记一笔来源，方便追溯。

## 怎么下

1. 打开上面的链接
2. 点页面里的 **Download**
3. 弹出"考虑捐赠"的对话框 → 点 **Continue without donating...**
4. 得到 `kenney_xxx.zip`

## 放哪

解压后按这个结构放（目录名不要求完全一致，我会自己找）：

```
assets_src/
├── fish-pack/                 ← 解压后的内容（里面通常有 PNG/、Vector/、License.txt）
├── new-platformer-pack/
├── ui-pack/
├── input-prompts/
└── audio/
```

**注意**：Kenney 的包里常有两套资源——`PNG/`（位图）和 `Vector/`（SVG）。我们只用 **PNG**。

## 下完之后

跟我说一声。我会：
1. 列出每个包里实际有哪些图（文件名就是内容说明）
2. 按"玩家 / 目标物 / 障碍 / 背景 / UI / 按键"分类，挑出候选
3. 抽出 `Theme` 层，把资源路径、坐标、锚点集中到一个文件
4. 替换现有四类精灵，玩法逻辑一行不动

## 为什么这个目录不进 git

单个 Kenney 包解压后常有 50–200 MB。仓库里只保留**最终选中的那几十张图**（几 MB），
原始包留本地当素材库即可。
