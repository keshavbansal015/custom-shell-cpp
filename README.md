# ⚡ Custom POSIX C++ Shell (Rush / CppShell)

A lightweight, robust, Unix-compliant interactive shell written from scratch in modern **C++17**.

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg?style=for-the-badge&logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-black.svg?style=for-the-badge&logo=linux)](https://en.wikipedia.org/wiki/POSIX)
[![Build](https://img.shields.io/badge/Build-CMake-orange.svg?style=for-the-badge&logo=cmake)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)](LICENSE)

---

## 📸 Preview & Demo

```text
  ____            _       ____  _          _ _ 
 |  _ \ _   _ ___| |_    / ___|| |__   ___| | |
 | |_) | | | / __| __|   \___ \| '_ \ / _ \ | |
 |  _ <| |_| \__ \ |_     ___) | | | |  __/ | |
 |_| \_\\__,_|___/\__|___|____/|_| |_|\___|_|_|
                     |_____|                   
================================================
$ echo "Welcome to Custom C++ Shell"
Welcome to Custom C++ Shell

$ ls -la | grep "\.cpp" | wc -l
       5

$ cat << EOF > sample.txt
$ echo "Standard and Error stream redirection" 1> out.log 2> err.log
$ history
1  echo "Welcome to Custom C++ Shell"
2  ls -la | grep "\.cpp" | wc -l
...
```

---

## ✨ Features

- 🛠️ **Custom Builtin Commands**:
  - `cd` — Directory navigation supporting `~`, `-`, and relative/absolute paths.
  - `pwd` — Print current working directory.
  - `echo` — Formatted printing with quote stripping, escape sequences, and environment variable expansion.
  - `type` — Identify whether a target is a shell builtin or an external executable in `$PATH`.
  - `history` — Persistent command history across sessions with file synchronization.
  - `declare` — Environment and shell variable management.
  - `complete` — Programmable tab-completion registration.
  - `jobs` — Background job tracking, status monitoring, and auto-reaping.
  - `exit` — Clean process termination and state saving.

- 🔀 **Pipelining & Multi-Stage IPC**:
  - Supports arbitrary multi-stage pipelines (`cmd1 | cmd2 | cmd3 | ...`).
  - Implemented using low-level POSIX `pipe()`, `fork()`, and `dup2()` system calls.

- 📤 **I/O Redirection**:
  - Standard output redirection (`>` and `1>`) & append (`>>` and `1>>`).
  - Standard error redirection (`2>` and `2>>`).
  - Automatic directory tree creation for target output paths.

- ⌨️ **Interactive Terminal & Raw Mode**:
  - Custom Raw Mode parser utilizing `termios`.
  - Up / Down arrow keys for dynamic command history traversal.
  - Smart backspace, character-by-character echoing, and line buffering.

- 💡 **Tab Autocompletion**:
  - Builtin command completion.
  - Binary executable discovery across `$PATH`.
  - Filesystem path and directory autocompletion with Longest Common Prefix (LCP) matching.
  - Programmable completion hooks (`complete` command support).

- ⚙️ **Process Management & Background Jobs**:
  - Async job launching with `&`.
  - Zombie process cleanup via asynchronous `waitpid(WNOHANG)`.

---

## 🏛️ Architecture & System Design

```mermaid
flowchart TD
    A([User Input / Keyboard]) --> B[Terminal Engine<br/>termios / Raw Mode]
    B -->|Autocompletion Tab| C[Autocomplete Engine<br/>LCP / Path / $PATH Search]
    B -->|Return Key| D[Command Parser]
    
    D --> E{Pipeline Check<br/>contains '|' ?}
    E -->|Yes| F[Pipeline Executor<br/>fork / pipe / dup2]
    E -->|No| G[Redirection Parser<br/>stdout / stderr / append]
    
    G --> H{Builtin vs External}
    H -->|Builtin| I[Builtin Handlers<br/>cd, pwd, echo, type, declare, etc.]
    H -->|External| J[Process Spawner<br/>fork / execv / PATH resolution]
    
    I --> K[Output & Job Manager]
    J --> K
    F --> K
    K --> L[Terminal Display / Files]
```

---

## 📁 Project Structure

```text
.
├── CMakeLists.txt         # CMake build configuration
├── your_program.sh        # Quick build & run wrapper
└── src/
    ├── main.cpp           # REPL loop & input event dispatching
    ├── parser.h/.cpp      # Command line tokenization, quoting & redirection parser
    ├── commands.h/.cpp    # Builtin command implementations & process execution
    ├── autocomplete.h/.cpp# Tab completion, PATH lookup & LCP engine
    └── terminal.h/.cpp    # POSIX termios raw mode management
```

---

## 🚀 Getting Started

### Prerequisites

- **C++ Compiler**: `g++` or `clang++` (supporting C++17 or newer)
- **Build System**: `CMake` (>= 3.10)
- **OS**: POSIX-compliant system (macOS / Linux / WSL)

### 🔨 Build & Compilation

Clone the repository and build using CMake:

```bash
# Clone the repository
git clone https://github.com/keshavbansal015/custom-shell-cpp.git
cd custom-shell-cpp

# Configure and compile
cmake -B build -S .
cmake --build build
```

### 🏃 Running the Shell

Direct execution:
```bash
./build/shell
```

Or using the helper launcher:
```bash
./your_program.sh
```

---

## 🧪 Usage Examples

#### 1. Piping commands
```bash
$ ps aux | grep cpp | awk '{print $2}'
```

#### 2. Redirecting output & errors
```bash
$ ls -la non_existing_dir 2> error.log
$ echo "Hello world" >> output.txt
```

#### 3. Inspecting commands
```bash
$ type cd
cd is a shell builtin

$ type grep
grep is /usr/bin/grep
```

#### 4. Variable declaration and substitution
```bash
$ declare MY_VAR="Custom Shell"
$ echo $MY_VAR
Custom Shell
```

---

## 📜 License

This project is licensed under the [MIT License](LICENSE).
