# AlgoRepo

AlgoRepo 算法题管理系统
一个基于纯 C 语言开发的个人算法题代码仓库管理系统

## 如何编译

项目已内置一键构建脚本。
确保你已安装 `cmake` 和对应的编译器（`gcc` / `clang` / `MSVC`）
直接在项目根目录下运行对应平台的脚本即可：

### 🐧 Linux / macOS
```bash
# 首次运行确保赋予执行权限
chmod +x build.sh
./build.sh

🪟 Windows
build.bat
💡 说明：脚本会自动创建 build 目录、配置 CMake 并执行增量编译。
         编译完成后，生成的可执行文件将存放在 build/ 目录下。
```
