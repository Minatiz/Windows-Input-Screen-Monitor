# Windows Input & Screen Monitor

## Educational Use Only

This project is intended strictly for learning purposes, such as understanding Windows internals, low-level hooks and system APIs.
Do not use this software on systems you do not own or have explicit permission to test.

## Features

- **Mouse Activity Logger**

- **Keyboard Logging**

- **Screen Capture**

- **Multithreaded**

## Requirements

- **MinGW-w64**

## Build Instructions

The project includes a simple Makefile.

### Build

```bash
# To build it:
make
```

### Clean

```bash
# This removes the executable and output directory:
make clean
```

## Limitations

- **Windows-only (Win32 API)**

- **Screenshots are uncompressed BMP files**

## Resources & Documentation

This project utilizes various Windows APIs and C Runtime libraries. Below are the primary resources used for development:

- **Official API Reference:** [Windows App Development API Reference](https://learn.microsoft.com/en-us/windows/apps/api-reference/)
- **Console Management:** [Windows Console Documentation](https://learn.microsoft.com/en-us/windows/console/)
- **Input Hooking:** [Windows Hooks (WH_KEYBOARD_LL / WH_MOUSE_LL)](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-hooks)
- **Graphics & GDI:** [BitBlt & Bitmap Management](https://learn.microsoft.com/en-us/windows/win32/gdi/bitmaps)
- **Synchronization:** [Critical Sections](https://learn.microsoft.com/en-us/windows/win32/sync/critical-section-objects) and [Event Objects](https://learn.microsoft.com/en-us/windows/win32/sync/event-objects)
- **Thread Management:** [CRT Threading (\_beginthread)](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex)
- **Character Mapping:** [Unicode Translation (ToUnicode)](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-tounicode)
- **System Metrics:** [GetSystemMetrics (Screen Resolution)](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsystemmetrics)
