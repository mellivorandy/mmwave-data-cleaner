# mmwave-data-cleaner

<br>

[<img alt="ci" src="https://github.com/mellivorandy/mmwave-data-cleaner/actions/workflows/c-cpp.yml/badge.svg" height="20">](https://github.com/mellivorandy/mmwave-data-cleaner/actions)
[<img alt="license" src="https://img.shields.io/github/license/mellivorandy/mmwave-data-cleaner?style=for-the-badge&logo=GITHUB&color=light%20green" height="20">](https://github.com/mellivorandy/mmwave-data-cleaner?tab=MIT-1-ov-file)

<br>

A CSV data cleaning pipeline for mmWave gesture datasets.

<br>

## 功能
- 以 `keepColumns` 選擇並重新排序欄位

- 串接式列過濾（**gesturePresence = 0**、**frameNum** 空值等）

- 完整輸出被丟棄的列，方便後續稽核與比對

<br>

## 系統需求
- 支援 `C++17` 的編譯器（GCC、Clang、MSVC）

- Make

<br>

## 建置
使用 Makefile：
```bash
make
```

不使用 Makefile：
```bash
g++ -O2 -std=c++17 main.cpp DataCleaner.cpp -o data_cleaner
```

<br>

## Makefile 指令與用法

本專案提供多個 Make 指令：

- debug
  - 功能：以除錯模式編譯。
  
  - 用法：
    
    ```bash
    make debug
    
    ./data_cleaner   # or ./data_cleaner.exe
    ```

- release
  - 功能：以發佈模式編譯。
  
  - 用法：

    ```bash
    make release
    ./data_cleaner
    ```

- run
  - 功能：先執行編譯 `all`（若尚未建置），再執行程式。
  
  - 用法：

    ```bash
    make run
    ```

- clean
  - 功能：移除可執行檔。
  
  - 用法：

    ```bash
    make clean
    ```

- rebuild
  - 功能：強制重新建置，等同 `make clean all`。
  
  - 用法：

    ```bash
    make rebuild
    ```

- clean-output
  - 功能：清除 `data/` 目錄下由程式產生的 CSV 檔案，但保留指定的輸入檔（避免誤刪原始資料）。

  - 指令：

    ```bash
    make clean-output
    ```

  - 預設保留清單（可依需求調整）：
    - `left-to-right.csv`，
    - `right-to-left.csv`，
    - `up-to-down.csv`，
    - `down-to-up.csv`，
    - `push.csv`，
    - `pull.csv`。
  
  - 注意：
    - 該目標需要類 Unix 環境的 `find`（Linux/macOS、或 Windows 的 MSYS2/WSL/Git Bash）。

    - 若在純 Windows PowerShell 環境，建議改在 MSYS2/WSL 下執行，或自行以等效指令取代。

<br>

> [!TIP]
> 其他提示：
>
> - 使用平行編譯加速建置：
>  ```bash
>  make -j
>  ```
>
> - 若要臨時覆寫變數（例如自訂目標檔名）：
>  ```bash
>  make TARGET=my_cleaner
>  ```

<br>

## Quickstart

1) 編輯 `main.cpp` 設定 CSV 路徑與參數

2) cd 至專案根目錄

```bash
cd mmwave-data-cleaner
```

3) 建置並執行：
```bash
make
```

```bash
./data_cleaner
```

<br>

## Configuration

`PipelineConfig` 欄位說明：

- `inputPath`：來源 CSV (待清洗)。

- `outputCleanPath`：清洗後 CSV。

- `outputDroppedPath`：被丟棄列的 CSV（保留所有 keepColumns，方便稽核）。

- `keepColumns`：要保留的欄位（順序即為輸出順序）。

- `gesturePresenceCol`：用於過濾的欄位（值為 **0** 則丟棄）。

- `frameNumCol`：用於過濾的欄位（空字串或缺失則丟棄）。

- `excludeFromClean`：從 clean 輸出中排除的欄位（但 dropped 仍保留完整欄位）。

- `printDroppedToStderr`：是否在 stderr 顯示被丟棄列與原因。

- `readerBufferBytes`：讀檔緩衝區大小（預設 `64 KB`）。

<br>

## License

The source code is licensed under <a href="LICENSE">MIT license</a>.
