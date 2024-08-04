#include <windows.h>
#include <vlc/vlc.h>
#include <commdlg.h>  // 用于文件对话框
#include <string>     // 用于std::string
#include <tchar.h>    // 用于TCHAR
#include <locale>
#include <codecvt>
#include <commctrl.h> // 用于滑块控件
#include "resource.h" // 用于ID_FILE_OPEN

#pragma comment(lib,"libvlc.lib") //添加libvlc.lib库
#pragma comment(lib,"libvlccore.lib") //添加libvlccore.lib库

// 全局变量
libvlc_media_player_t* mediaPlayer = NULL;
bool isPaused = false; // 记录当前播放状态

// 打开文件对话框并返回选中的文件路径
std::wstring OpenFileDialog(HWND hwnd) {
    OPENFILENAME ofn;
    wchar_t szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"视频文件\0*.MP4;*.AVI;*.MKV;*.MOV\0所有文件\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        return std::wstring(ofn.lpstrFile);
    }
    return L"";
}

// 将std::wstring转换为UTF-8编码的std::string
std::string WStringToUtf8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
    return convert.to_bytes(wstr);
}

// 初始化VLC媒体播放器
bool InitializeVLC(HWND hwnd, const std::wstring& filePath) {
    libvlc_instance_t* inst = libvlc_new(0, NULL);
    if (!inst) {
        MessageBox(hwnd, L"无法初始化VLC实例", L"错误", MB_ICONERROR);
        return false;
    }

    // 使用WStringToUtf8将wstring转换为UTF-8编码的string
    std::string utf8FilePath = WStringToUtf8(filePath);
    libvlc_media_t* media = libvlc_media_new_path(inst, utf8FilePath.c_str());
    if (!media) {
        MessageBox(hwnd, L"无法创建媒体", L"错误", MB_ICONERROR);
        libvlc_release(inst);
        return false;
    }

    mediaPlayer = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);

    if (!mediaPlayer) {
        MessageBox(hwnd, L"无法创建媒体播放器", L"错误", MB_ICONERROR);
        libvlc_release(inst);
        return false;
    }

    libvlc_media_player_set_hwnd(mediaPlayer, hwnd);
    return true;
}

// 播放媒体
void PlayMedia() {
    if (mediaPlayer) {
        libvlc_media_player_play(mediaPlayer);
    }
}

// 停止媒体
void StopMedia() {
    if (mediaPlayer) {
        libvlc_media_player_stop(mediaPlayer);
    }
}

// 暂停或继续播放媒体
void PauseOrResumeMedia() {
    if (mediaPlayer) {
        if (isPaused) {
            libvlc_media_player_set_pause(mediaPlayer, 0);
            isPaused = false;
        }
        else {
            libvlc_media_player_set_pause(mediaPlayer, 1);
            isPaused = true;
        }
    }
}

// 窗口过程
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        StopMedia();
        if (mediaPlayer) {
            libvlc_media_player_release(mediaPlayer);
            mediaPlayer = NULL;
        }
        PostQuitMessage(0);
        return 0;
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_FILE_OPEN: {
            std::wstring filePath = OpenFileDialog(hwnd);
            if (!filePath.empty()) {
                StopMedia();
                if (InitializeVLC(hwnd, filePath)) {
                    PlayMedia();
                }
            }
            break;
        }
        case ID_PLAY_PAUSE: {
            PauseOrResumeMedia();
            break;
        }
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// 应用程序入口点
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"VLCPlayerWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"VLC播放器",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // 添加菜单
    HMENU hMenu = CreateMenu();
    AppendMenu(hMenu, MF_STRING, ID_FILE_OPEN, L"打开(&O)");
    AppendMenu(hMenu, MF_STRING, ID_PLAY_PAUSE, L"暂停/继续播放(&P)");
    SetMenu(hwnd, hMenu);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
//-------------------------------------------------