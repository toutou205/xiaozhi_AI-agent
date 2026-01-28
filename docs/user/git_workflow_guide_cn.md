# 项目版本管理与协作指南

本文档详细说明了如何对 `xiaozhi-esp32` 项目进行版本固化、分支开发以及多人协作的标准流程。遵循此规范可以确保主线版本的稳定性，防止代码冲突和混乱。

## 1. 初始化与上传 (Re-initialization)

如果你需要清除旧的 Git 历史并建立一个新的远程仓库连接，请执行以下步骤。

> **注意**: 此操作会删除所有旧的提交记录，请谨慎操作。

### 操作步骤 (Windows PowerShell)

1.  **清除旧 Git 信息**:
    ```powershell
    rm -r -force .git
    ```
    *(如果提示找不到文件，说明已经是干净的文件夹)*

2.  **初始化仓库**:
    ```powershell
    git init
    git add .
    git commit -m "Initial commit: Stable version 2.1.1 (Audio Fixes Applied)"
    git branch -M main
    ```

3.  **关联远程仓库并推送**:
    *   请先在 GitHub 上创建一个空仓库。
    *   将下面的 URL 替换为你自己的仓库地址。
    ```powershell
    git remote add origin https://github.com/YOUR_NAME/YOUR_REPO.git
    git push -u origin main
    ```

---

## 2. 版本固化 (Tagging)

当项目达到一个稳定状态（例如：修复了关键 Bug 或发布了新功能），需要打一个**标签 (Tag)**。这相当于给此时的代码拍了一张“快照”，以后随时可以回退到这个完美状态。

### 操作步骤

```powershell
# 1. 打本地标签 (建议版本号递增，如 v2.1.1, v2.1.2)
git tag -a v2.1.1 -m "Release v2.1.1: Fixed Mute Button logic and Enabled AEC"

# 2. 推送标签到服务器
git push origin v2.1.1
```

**使用场景**:
*   当现有功能验证通过，准备进行大改动之前。
*   发布给用户使用的固件版本对应源码。

---

## 3. 分支开发 (Branching)

为了保护 `main` (主分支) 的稳定性，**严禁直接在 main 分支上开发新功能**。所有新功能或实验性修改必须在独立的开发分支上进行。

### 操作步骤

假设你要开发一个“传感器触发”功能 (Sensor Trigger)：

```powershell
# 1. 创建并切换到新分支 (dev-xxx)
git checkout -b dev-sensor-trigger

# 2. 将新分支推送到服务器
git push -u origin dev-sensor-trigger
```

**现在，你和团队成员应该切换到 `dev-sensor-trigger` 分支进行日常代码提交。** `main` 分支依旧保持着 v2.1.1 的纯净状态。

---

## 4. 多人协作与合并 (Collaboration & Merge)

这是防止“别人把工程弄乱”的核心机制。通过 **Pull Request (PR)** 流程来审查代码。

### 标准工作流

1.  **开发阶段**:
    *   所有成员在 `dev-xxx` 分支上 `git add`, `git commit`, `git push`。
    *   确保 `dev` 分支上的代码在本地编译通过，功能验证无误。

2.  **发起合并请求 (PR)**:
    *   开发完成后，不要使用命令行合并。
    *   打开 GitHub 仓库页面，点击 **"Pull requests"** -> **"New pull request"**。
    *   **Base (目标)** 选择 `main`，**Compare (来源)** 选择 `dev-sensor-trigger`。
    *   填写修改内容的描述，点击 "Create pull request"。

3.  **代码审查 (Code Review)**:
    *   项目负责人（你）会收到邮件通知。
    *   在 GitHub 上查看 Files changed，检查是否有误删或糟糕的代码。
    *   如果有问题，在行间留言要求修改 (Request changes)。

4.  **最终合并**:
    *   审查通过后，点击 GitHub 页面上的 **"Merge pull request"** 按钮。
    *   此时，新功能才会正式进入 `main` 主分支。

---

## 总结速查表

| 需求 | Git 命令/操作 |
| :--- | :--- |
| **开始新项目/清洗历史** | `rm .git` -> `git init` -> `git push` |
| **标记稳定版** | `git tag -a vX.X.X` -> `git push --tags` |
| **开发新功能** | `git checkout -b dev-feature-name` |
| **合并代码** | **不要用命令**，请在 GitHub 网页发起 **Pull Request** |
