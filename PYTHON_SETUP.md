# Python 環境設置說明

## 為什麼需要特別處理 Python？

你遇到的問題是一個經典的 **Nix 與系統工具混用的衝突**。

### 問題根源

1. **Nix 使用自己的工具鏈**：
   - Nix gcc (14.3.0) + Nix glibc (2.40)
   - Nix Python (3.13.18) + 完整的開發頭文件

2. **系統也有自己的工具**：
   - Ubuntu Python (3.13.13) + Ubuntu glibc (2.35)
   - 系統頭文件在 `/usr/include`

3. **CMake/pybind11 預設找到系統的 Python**：
   - 導致使用 `/usr/include/python3.13`
   - 但編譯器是 Nix 的 gcc
   - **混合使用會導致 glibc 版本不匹配！**

### 錯誤表現

```
/usr/include/stdio.h:205:27: error: 'L_tmpnam' was not declared
/nix/store/.../bits/stdio2.h:111:10: error: '__fprintf_chk' was not declared
```

這些錯誤是因為 Nix 的 glibc 2.40 和系統的 glibc 2.35 頭文件不兼容。

## 解決方案

### ✅ 方案 1: 完全使用 Nix（推薦）

這就是我們實現的方案：

1. **shell.nix** 提供 Python 和開發工具
2. **CMake 時明確指定 Nix Python**：
   ```bash
   cmake -DPYTHON_EXECUTABLE=/nix/store/.../python3 ..
   ```
3. **uv 管理 Python 依賴**（開發、測試工具）

**使用方法**：
```bash
# 方法 1: 使用提供的腳本
./build.sh

# 方法 2: 手動編譯
rm -rf build && mkdir build && cd build
NIX_PYTHON=$(find /nix/store -name "python3-3.13*" -type d | head -1)/bin/python3
cmake -DPYTHON_EXECUTABLE="$NIX_PYTHON" ..
make -j8
```

### 🔧 方案 2: 完全使用系統工具（不推薦）

如果不使用 Nix：
```bash
# 不進入 nix-shell
apt-get install python3-dev libboost-dev cmake
rm -rf build && mkdir build && cd build
cmake ..
make -j8
```

## UV 的作用

`uv` 在這個專案中的作用是：

1. **管理 Python 開發依賴**（pytest, pytest-benchmark 等）
2. **不負責 C++ 編譯**（這是 CMake 的工作）
3. **提供一致的 Python 環境**

```bash
# 使用 uv 管理測試依賴
uv sync  # 安裝 pyproject.toml 中的依賴

# 運行測試
uv run pytest python/tests/
```

## 專案結構

```
.
├── shell.nix          # Nix 環境定義（C++ 工具 + Python）
├── pyproject.toml     # Python 專案配置（uv 管理）
├── .python-version    # Python 版本聲明
├── CMakeLists.txt     # C++ 編譯配置
├── build.sh           # 便捷編譯腳本
└── src/
    ├── orderbook/     # C++ header-only 庫
    └── orderbook_py/  # Python bindings (pybind11)
```

## 常見問題

### Q: 為什麼不能直接用 `make`？

A: 因為 CMake 需要知道使用哪個 Python。如果不指定，它會找到系統的 Python，導致 glibc 衝突。

### Q: uv 和 Nix 衝突嗎？

A: 不衝突！它們各司其職：
- **Nix**: 提供 C++ 工具鏈和 Python 解釋器
- **uv**: 管理 Python 包依賴（測試、開發工具）

### Q: 能否只用 uv，不用 Nix？

A: 可以，但你需要系統上安裝所有 C++ 依賴（cmake, boost, gcc 等）。Nix 的優勢是提供可重現的環境。

### Q: 為什麼需要 PYTHON_EXECUTABLE？

A: pybind11 使用舊的 CMake Python 查找模組，需要明確指定 Python 路徑來避免找到錯誤的 Python。

## 最佳實踐

1. **開發時**：進入 `nix-shell`，使用 `./build.sh` 編譯
2. **測試時**：使用 `uv run pytest`
3. **部署時**：只需 `.so` 模組和對應版本的 Python

## 未來改進

可以考慮：
- 使用 `flake.nix` 替代 `shell.nix`（更現代的 Nix）
- 在 CMakeLists.txt 中自動檢測 Nix Python
- 使用 Nix 的 buildPythonPackage 來構建整個專案
