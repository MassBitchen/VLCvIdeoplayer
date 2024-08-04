#include <windows.h>
#include <vlc/vlc.h>
#include <commdlg.h>  // �����ļ��Ի���
#include <string>     // ����std::string
#include <tchar.h>    // ����TCHAR
#include <locale>
#include <codecvt>
#include <commctrl.h> // ���ڻ���ؼ�
#include "resource.h" // ����ID_FILE_OPEN

#pragma comment(lib,"libvlc.lib") //���libvlc.lib��
#pragma comment(lib,"libvlccore.lib") //���libvlccore.lib��

// ȫ�ֱ���
libvlc_media_player_t* mediaPlayer = NULL;
bool isPaused = false; // ��¼��ǰ����״̬

// ���ļ��Ի��򲢷���ѡ�е��ļ�·��
std::wstring OpenFileDialog(HWND hwnd) {
    OPENFILENAME ofn;
    wchar_t szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"��Ƶ�ļ�\0*.MP4;*.AVI;*.MKV;*.MOV\0�����ļ�\0*.*\0";
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

// ��std::wstringת��ΪUTF-8�����std::string
std::string WStringToUtf8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
    return convert.to_bytes(wstr);
}

// ��ʼ��VLCý�岥����
bool InitializeVLC(HWND hwnd, const std::wstring& filePath) {
    libvlc_instance_t* inst = libvlc_new(0, NULL);
    if (!inst) {
        MessageBox(hwnd, L"�޷���ʼ��VLCʵ��", L"����", MB_ICONERROR);
        return false;
    }

    // ʹ��WStringToUtf8��wstringת��ΪUTF-8�����string
    std::string utf8FilePath = WStringToUtf8(filePath);
    libvlc_media_t* media = libvlc_media_new_path(inst, utf8FilePath.c_str());
    if (!media) {
        MessageBox(hwnd, L"�޷�����ý��", L"����", MB_ICONERROR);
        libvlc_release(inst);
        return false;
    }

    mediaPlayer = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);

    if (!mediaPlayer) {
        MessageBox(hwnd, L"�޷�����ý�岥����", L"����", MB_ICONERROR);
        libvlc_release(inst);
        return false;
    }

    libvlc_media_player_set_hwnd(mediaPlayer, hwnd);
    return true;
}

// ����ý��
void PlayMedia() {
    if (mediaPlayer) {
        libvlc_media_player_play(mediaPlayer);
    }
}

// ֹͣý��
void StopMedia() {
    if (mediaPlayer) {
        libvlc_media_player_stop(mediaPlayer);
    }
}

// ��ͣ���������ý��
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

// ���ڹ���
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

// Ӧ�ó�����ڵ�
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
        L"VLC������",
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

    // ��Ӳ˵�
    HMENU hMenu = CreateMenu();
    AppendMenu(hMenu, MF_STRING, ID_FILE_OPEN, L"��(&O)");
    AppendMenu(hMenu, MF_STRING, ID_PLAY_PAUSE, L"��ͣ/��������(&P)");
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