# RustMonitor

Простой внешний монитор процесса Rust для Windows.

## Что делает

- ищет `RustClient.exe`;
- показывает PID;
- фиксирует список загруженных модулей/DLL;
- фиксирует появление и исчезновение модулей;
- сохраняет события в `rust_monitor.log`.

Монитор не внедряется в процесс Rust, не изменяет его память и не пытается обходить античит.

## Сборка

Требуется Visual Studio 2022 с компонентом "Desktop development with C++" и CMake.

```text
cmake -S . -B build
cmake --build build --config Release
```

Исполняемый файл будет в:

```text
build/Release/RustMonitor.exe
```

Запускайте монитор отдельно от Rust.
