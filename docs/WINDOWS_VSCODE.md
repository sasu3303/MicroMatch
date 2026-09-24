# Run MicroMatch in VS Code on Windows

This setup uses WSL so you also practice Linux tooling. The C++ compiler is separate from Python; installing Python alone cannot build this project.

## 1. Install WSL if needed

In Windows PowerShell, check `wsl --status`. If WSL/Ubuntu is already configured, skip installation. Otherwise open PowerShell as Administrator and run:

```powershell
wsl --install
```

Restart when requested. Open Ubuntu from the Start menu and create its Linux username/password. This changes your Windows environment; follow your device administrator's policy if it is a managed computer. [Microsoft WSL installation guide](https://learn.microsoft.com/en-us/windows/wsl/install).

## 2. Install the Linux tools

Run in the Ubuntu terminal, not Windows PowerShell:

```bash
sudo apt update
sudo apt install -y build-essential cmake gdb python3 unzip
```

Use a current Ubuntu distribution with a C++20-capable GCC (tested GCC 13). Confirm:

```bash
g++ --version
cmake --version
python3 --version
```

## 3. Open the extracted project in VS Code's WSL environment

Download and extract the ZIP in Windows. In VS Code, install Microsoft's **WSL**, **C/C++**, and **CMake Tools** extensions. Open the `MicroMatch` folder, press Ctrl+Shift+P, then choose **WSL: Reopen Folder in WSL**. Confirm the lower-left remote indicator says WSL/Ubuntu. Install the C/C++ and CMake Tools extensions in WSL if prompted. [Official VS Code C++/WSL guide](https://code.visualstudio.com/docs/cpp/config-wsl).

For larger builds you can copy the project into your Linux home directory, then use `code .` from that directory. You can start from the extracted Windows folder for this small project.

## 4. Build and see the output

Open VS Code's terminal. It should now be a Linux shell in the project folder containing `CMakeLists.txt`. Run one line at a time:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 scripts/demo.py
```

The last command shows the order matching walkthrough. It does not start a website. Compare it with `evidence/demo-output.txt`.

## 5. Debug a fill

Open `src/book.cpp`. Set a breakpoint on `const Qty executed = std::min(left, resting.quantity);`. Press F5 and select **Debug matching demo (Linux/WSL)**. The provided tasks build Debug mode first. Inspect `input`, `left`, and `resting`; step over the quantity updates. Notice that the maker order may be erased after the fill, so an old reference must not be used afterward.

If GDB is not found, confirm it was installed inside Ubuntu. The GDB configuration is provided for you; it was not executed in the authoring environment.

## Troubleshooting

- `cmake: command not found`: install CMake inside Ubuntu, not only Windows.
- `jthread` or `contains` missing: your compiler/standard library is too old or C++20 is not enabled. Use the supplied CMake build and a current toolchain.
- CMake reports a different generator/source path: create a new build directory such as `build-wsl`; don't reuse a build directory made with another compiler/OS.
- TCP port already used: start the server on `9001`, then run `python3 scripts/client.py --port 9001`.
- Client disconnected after you paused: reconnect; the server uses a 60-second socket timeout.
- Sanitizer startup limitations under a VM/tracer: save the exact error. Do not describe an unavailable sanitizer run as passing.
