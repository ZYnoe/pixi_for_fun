# pixi-hello

这是一个用 **Pixi 管理开发环境**、用 **setuptools 构建 Python + C 扩展** 的最小示例项目。

运行后会调用 `c_hello` 这个本地扩展模块，并打印一条来自 C 扩展的问候语。

## 这个仓库里有什么

- `pixi.toml`：管理开发环境、任务、平台和本地可编辑依赖
- `pixi.lock`：锁定环境版本，方便别人复现
- `pyproject.toml`：描述 Python 包和构建方式
- `src/c_hello/_c_hello.c`：C 扩展源码
- `src/c_hello/__init__.py`：Python 包入口，把 C 扩展里的函数暴露出来
- `scripts/demo.py`：最简单的演示脚本

## 快速开始

### 1. 安装 Pixi

macOS / Linux:

```bash
curl -fsSL https://pixi.sh/install.sh | sh
```

Windows PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -c "irm -useb https://pixi.sh/install.ps1 | iex"
```

安装完成后，重新打开终端，确认：

```bash
pixi --version
```

### 2. 克隆并进入项目

```bash
git clone https://github.com/ZYnoe/pixi_for_fun.git
cd pixi_for_fun
```

### 3. 直接运行示例

```bash
pixi run hello
```

这个命令会在 Pixi 环境里执行 `python demo.py`。如果环境还没装好，Pixi 会先根据当前清单和锁文件把环境准备好，再执行任务。

### 4. 如果你想进入环境自己调试

```bash
pixi shell
python scripts/demo.py
```

## 这个项目为什么这样组织

这个仓库把两件事拆开了：

- `pixi.toml` 负责“开发环境管理”
- `pyproject.toml` 负责“Python 包构建和发布元数据”

这样做的好处是：

- 开发环境可复现：别人拉代码后可以直接 `pixi run` 或 `pixi shell`
- Python 打包方式标准：编辑器、构建工具、打包工具都更容易识别
- 本地源码可以 editable 安装：改完 Python 或 C 扩展后，工作流更顺

## `pixi.toml` 里现在做了什么

当前配置里主要有三类内容：

### 1. 工作区信息

```toml
[workspace]
name = "pixi-hello"
channels = ["conda-forge"]
platforms = ["osx-arm64", "osx-64", "linux-64", "win-64"]
```

这里告诉 Pixi：

- 项目名是什么
- 依赖从哪个 channel 拉
- 锁文件要为哪些平台生成（这里覆盖了主流的 macOS/Linux/Windows 桌面平台，方便别人在不同机器上直接复现）

### 2. Conda 依赖

```toml
[dependencies]
python = ">=3.12,<3.13"
pip = "*"
setuptools = "*"
c-compiler = "*"
```

这里放的是“环境层”的依赖，比如：

- `python`：解释器版本
- `c-compiler`：编译 C 扩展时需要
- `setuptools`：构建 Python 包时需要
- `pip`：Pixi 自己装 PyPI 包用的是内置的 uv，不需要 pip；但下面的 `rebuild` 任务里会手动调 `pip install` 触发 C 扩展重编译，所以这里把 pip 留着。`wheel` 不再显式列——现代 setuptools 不需要。

### 3. PyPI / 本地源码依赖

```toml
[pypi-dependencies]
pixi-hello = { path = ".", editable = true }
```

这表示把当前项目本身以 **editable** 模式装进环境里。这样你在仓库里改源码时，环境里就能直接看到更新。

### 4. 任务

```toml
[tasks]
hello = "python scripts/demo.py"
rebuild = "pip install -e . --no-deps --force-reinstall --no-build-isolation"
```

以后只需要：

```bash
pixi run hello
```

不用每次都先手动激活环境再找命令。

> 关于 `rebuild`：setuptools 的 editable 安装**不会自动重编译 C 扩展**。改完 `.c` 文件后，记得先 `pixi run rebuild` 触发重新构建，否则 `pixi run hello` 还在跑旧的 `.so`。

## 简单教程：怎么用 Pixi 管这种项目

下面这套流程很适合“一个 Python 项目，可能还会带一点本地编译逻辑”的场景。

### 第一步：初始化项目

```bash
pixi init
```

这会创建 Pixi 工作区清单。你可以使用 `pixi.toml`，也可以把配置写进 `pyproject.toml` 里的 `tool.pixi`。

### 第二步：先把环境层依赖交给 Pixi

比如这个项目，我们会先加：

```bash
pixi add python=3.12 pip setuptools c-compiler
```

这类依赖适合放进 Pixi，因为它们更像“开发和构建环境”的组成部分，而不只是运行时代码库。

### 第三步：把你的包构建信息写进 `pyproject.toml`

比如本项目用了 `setuptools`：

```toml
[build-system]
requires = ["setuptools>=68", "wheel"]
build-backend = "setuptools.build_meta"
```

如果你有扩展模块，也在这里声明。

### 第四步：把当前项目本身装进 Pixi 环境

这种项目最顺手的方式，是在 `pixi.toml` 里写本地可编辑依赖：

```toml
[pypi-dependencies]
your-project = { path = ".", editable = true }
```

这样做以后，环境会把当前仓库当成一个本地 Python 包来安装，而且是可编辑模式。

### 第五步：把常用命令写成任务

例如：

```toml
[tasks]
dev = "python scripts/demo.py"
test = "pytest"
lint = "ruff check ."
fmt = "ruff format ."
```

然后你就可以这样用：

```bash
pixi run dev
pixi run test
pixi run lint
pixi run fmt
```

这比“先激活环境，再手打一串命令”更稳定，也更方便团队统一。

### 第六步：后续新增依赖时怎么选

可以按这个简单规则：

- 跟解释器、编译器、系统库、构建工具有关：优先放 `pixi add ...`
- 纯 Python 库：通常用 `pixi add --pypi ...`
- 当前仓库自身源码：放到 `[pypi-dependencies]` 里，使用 `path = "."` 和 `editable = true`

示例：

```bash
pixi add numpy
pixi add --pypi rich pytest
```

### 第七步：安装、运行、进入环境

最常用的三个命令：

```bash
pixi install
pixi run hello
pixi shell
```

说明：

- `pixi install`：显式安装环境
- `pixi run ...`：在环境中运行任务或命令
- `pixi shell`：进入交互式环境

很多时候你甚至不用先手动执行 `pixi install`，因为 `pixi run` 和 `pixi shell` 会在需要时自动安装环境。

## 推荐工作流

如果你以后继续维护类似项目，可以按这个顺序：

1. 改 `pixi.toml` 管环境和任务
2. 改 `pyproject.toml` 管打包和构建
3. 改完 C 代码后跑一次 `pixi run rebuild`，让 `.so` 重新编译
4. 跑 `pixi run ...` 验证功能
5. 提交 `pixi.toml`、`pixi.lock`、`pyproject.toml` 和源码
6. 不提交 `.pixi/`、`.venv/`、`__pycache__/`、`*.so`、`*.egg-info/`、`uv.lock`

## 常见命令备忘

```bash
# 安装环境
pixi install

# 运行任务
pixi run hello

# 直接在环境里执行命令
pixi run python scripts/demo.py

# 进入 shell
pixi shell

# 添加 conda 依赖
pixi add python=3.12 numpy

# 添加 PyPI 依赖
pixi add --pypi rich pytest
```

## 一句话理解

如果你把这个仓库当模板来看，可以这样记：

- `pixi.toml` 解决“怎么得到一套能工作的开发环境”
- `pyproject.toml` 解决“这个 Python 项目本身怎么被构建”
- `pixi run <task>` 解决“团队怎么用统一命令跑项目”
