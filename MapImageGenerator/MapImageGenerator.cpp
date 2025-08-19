// MapImageGenerator.cpp : Defines the entry point for the application.
//
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "framework.h"
#include "MapImageGenerator.h"
#include <regex>
#include <random>
#include "shlwapi.h"
#include <shellapi.h>
#include <shobjidl.h>
#include <atlcomcli.h>

#define MAX_LOADSTRING 100
#define MAX_RECENT_FILES 5

#pragma pack(push, 1)
struct projectValues {
    wchar_t name[64];
    unsigned int width;
    unsigned int height;
    unsigned int waterLvl;
    unsigned int mountainLvl;
    unsigned int coldLvl;
    unsigned int warmLvl;
    unsigned int humidLvl;
    unsigned int dryLvl;
    bool genBeach;
    bool genLava;
    wchar_t seed[25];
};
#pragma pack(pop)

struct palette {
    wchar_t name[64];
    int r;
    int g;
    int b;
};

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND hWnd;
HWND hwndTab;
HWND hwndDisplay, hwndControls;
HWND hUpDown1, hEdit1, hTrack1,
hUpDown2, hEdit2, hTrack2,
hUpDownHei, hEditHei, hTrackHei,
hUpDownMnt, hEditMnt, hTrackMnt,
hUpDownTempL, hEditTempL, hTrackTempL,
hUpDownTempH, hEditTempH, hTrackTempH,
hUpDownHumL, hEditHumL, hTrackHumL,
hUpDownHumH, hEditHumH, hTrackHumH,
hControlsText1, hControlsText2, 
hControlsText3, hControlsText4, 
hControlsText5, hControlsText6,
hControlsText7, hControlsText8,
hControlsText9, hControlsText10,
hGenButton, hRndGenButton, hExportButton,
hSeedText, hSeedWnd, hProgressBar,
hBeachCheckbox, hLavaCheckbox;
HWND ControlsWnd, FinControlsWnd, hPaletteWindow;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
generateImage* ImgGen = new generateImage(400, 400);
projectValues project;
HBITMAP height, temp, humid, finalMap;
HDC hdcMemHei, hdcMemTemp, hdcMemHumid, hdcMemFin;
wchar_t szFileName[MAX_PATH] = L"";
//wchar_t szExportFileName[MAX_PATH] = L"";
bool beachChecked, lavaChecked = false;
static int yScrollPos = 0;
static int xScrollPos = 0;
bool scrolled = false;
wchar_t recentFiles[MAX_RECENT_FILES][MAX_PATH];
int recentFileCount = 0;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    TabWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ControlsWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    FinControlsWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DisplayWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PaletteWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    SeedWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    CreateNewProject(HWND, UINT, WPARAM, LPARAM);
HWND DoCreateTabControl(HWND);
HWND DoCreateDisplayWindow(HWND);
HRESULT OnSize(HWND, LPARAM);
BOOL OnNotify(HWND, HWND, LPARAM);
HANDLE hGenerateMapThread = 0;
DWORD WINAPI GenerateMapThread(LPVOID);
DWORD WINAPI GenerateOpenMapThread(LPVOID);
void CreateControls(HWND);
void CreateControlsFin(HWND);
//void DrawMap();
void switchWindow(HWND, HWND);
void adjustControls(const wchar_t[], const wchar_t[], int, int);
void EnableControls(bool);
bool SaveProject(const wchar_t*);
bool LoadProject(const wchar_t*);
void writeRecentFile(wchar_t*);
void removeRecentFile(int);
bool readRecentFiles();
void UpdateRecentFilesMenu();
static void CustomHandleMouseWheel(HWND, int, BOOL);
void setLanguage(LCID);
void OnProjectSave();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //get preffered language from ini file or 
    //set the system default if ini file doesn't exist 
    //or doesn't have a language entry

    wchar_t langList[2][6] = {
        L"en-US",
        L"pl-PL"
    };
    wchar_t iniPath[MAX_PATH];
    GetModuleFileNameW(NULL, iniPath, MAX_PATH);
    PathRemoveFileSpecW(iniPath);
    PathAppendW(iniPath, L"MapImageGenerator.ini");
    wchar_t lang[64] = L"";
    bool langSupported = false;
    GetPrivateProfileStringW(L"Settings", L"Language", L"", lang, sizeof(lang), iniPath);
    if (wcslen(lang) == 0) {
        LANGID sysLang = GetUserDefaultUILanguage();
        if (LCIDToLocaleName(MAKELCID(sysLang, SORT_DEFAULT), lang, 64, 0) != 0) {
            for (wchar_t* l : langList) {
                if (l == lang) {
                    WritePrivateProfileStringW(L"Settings", L"Language", lang, iniPath);
                    langSupported = true;
                    break;
                }
            }
            if (!langSupported) {
                WritePrivateProfileStringW(L"Settings", L"Language", L"en-US", iniPath);
            }
        }
    }
    else {
        SetThreadUILanguage(LocaleNameToLCID(lang, 0));
    }

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MAPIMAGEGENERATOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAPIMAGEGENERATOR));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAPIMAGEGENERATOR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MAPIMAGEGENERATOR);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDC_MAPIMAGEGENERATOR));
   SetMenu(hWnd, hMenu);

   BITMAPINFO bmi;
   VOID* pvBitsHei;
   VOID* pvBitsTemp;
   VOID* pvBitsHum;

   // setup bitmap info   
   bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bmi.bmiHeader.biWidth = 1000;
   bmi.bmiHeader.biHeight = 1000;
   bmi.bmiHeader.biPlanes = 1;
   bmi.bmiHeader.biBitCount = 32;
   bmi.bmiHeader.biCompression = BI_RGB;
   bmi.bmiHeader.biSizeImage = 1000 * 1000 * 4;

   hwndTab = DoCreateTabControl(hWnd);
   hwndDisplay = DoCreateDisplayWindow(hwndTab);
   HDC hdc = GetDC(hwndDisplay);
   hdcMemHei = CreateCompatibleDC(hdc);
   height = CreateDIBSection(hdcMemHei, &bmi, DIB_RGB_COLORS, &pvBitsHei, NULL, 0x0);
   SelectObject(hdcMemHei, height);
   hdcMemTemp = CreateCompatibleDC(hdc);
   temp = CreateDIBSection(hdcMemTemp, &bmi, DIB_RGB_COLORS, &pvBitsTemp, NULL, 0x0);
   SelectObject(hdcMemTemp, temp);
   hdcMemHumid = CreateCompatibleDC(hdc);
   humid = CreateDIBSection(hdcMemHumid, &bmi, DIB_RGB_COLORS, &pvBitsHum, NULL, 0x0);
   SelectObject(hdcMemHumid, humid);
   hdcMemFin = CreateCompatibleDC(hdc);
   finalMap = CreateDIBSection(hdcMemFin, &bmi, DIB_RGB_COLORS, &pvBitsHei, NULL, 0x0);
   SelectObject(hdcMemFin, finalMap);
   //CreateControls(hWnd);


   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   if (readRecentFiles()) {
       if (LoadProject(recentFiles[0])) {
           EnableControls(false);
           hGenerateMapThread = CreateThread(NULL, 0, GenerateOpenMapThread, NULL, 0, NULL);
           if (hGenerateMapThread == NULL) {
               ExitProcess(3);
           }
           wcscpy(szFileName, recentFiles[0]);
       }
       else {
           removeRecentFile(0);
           DialogBox(hInst, MAKEINTRESOURCE(IDD_CREATENEWPROJECT), hWnd, CreateNewProject);
       }
   }
   else
       DialogBox(hInst, MAKEINTRESOURCE(IDD_CREATENEWPROJECT), hWnd, CreateNewProject);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iPage = TabCtrl_GetCurSel(hwndTab);

    static HBITMAP hBitmapPalette;
    HDC hdcMem;
    TCHAR achTemp[256];

    switch (message)
    {
    case WM_CREATE:
    {
        
        LoadString(hInst, IDS_HRNDGENBTN, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        hRndGenButton = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            achTemp,      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            10,         // x position 
            560,         // y position 
            200,        // Button width
            30,        // Button height
            hWnd,       // Parent window
            (HMENU)IDRANDSEED,
            hInst,
            NULL);

        

        hSeedText = CreateWindow(L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
            55, 600, 200, 20,
            hWnd, (HMENU)ID_SEED, NULL, NULL);

        LoadString(hInst, IDS_GENBTN, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        hGenButton = CreateWindowW(L"BUTTON", achTemp,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            260, 595, 100, 30,
            hWnd, (HMENU)CREATE_IMAGE, NULL, NULL);

        LoadString(hInst, IDS_EXPORTBTN, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        hExportButton = CreateWindowW(L"BUTTON", achTemp,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 630, 100, 30,
            hWnd, (HMENU)ID_EXPORT, NULL, NULL);

        WNDCLASS wc1 = { 0 };
        wc1.lpfnWndProc = ControlsWndProc;
        wc1.hInstance = hInst;
        wc1.lpszClassName = L"ControlsWnd";
        wc1.hbrBackground = (HBRUSH)(COLOR_WINDOW);
        RegisterClass(&wc1);

        WNDCLASS wc2 = { 0 };
        wc2.lpfnWndProc = FinControlsWndProc;
        wc2.hInstance = hInst;
        wc2.lpszClassName = L"FinControlsWnd";
        wc2.hbrBackground = (HBRUSH)(COLOR_WINDOW);
        RegisterClass(&wc2);

        WNDCLASS wc4 = { 0 };
        wc4.lpfnWndProc = PaletteWndProc;
        wc4.hInstance = hInst;
        wc4.lpszClassName = L"PaletteWindow";
        wc4.hbrBackground = (HBRUSH)(COLOR_WINDOW);
        RegisterClass(&wc4);

        WNDCLASS wc5 = { 0 };
        wc5.lpfnWndProc = SeedWndProc;
        wc5.hInstance = hInst;
        wc5.lpszClassName = L"SeedWnd";
        wc5.hbrBackground = (HBRUSH)(COLOR_WINDOW);
        RegisterClass(&wc5);

        ControlsWnd = CreateWindow(L"ControlsWnd", L"Child",
            WS_CHILD | WS_BORDER,
            510, 30, 450, 1000,
            hWnd, NULL, hInst, NULL);
        FinControlsWnd = CreateWindow(L"FinControlsWnd", L"Child",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            510, 30, 450, 1000,
            hWnd, NULL, hInst, NULL);

        hPaletteWindow = CreateWindow(L"PaletteWindow", L"Child",
            WS_CHILD | WS_VISIBLE | WS_BORDER | SS_BITMAP,
            965, 30, 200, 370,
            hWnd, NULL, hInst, NULL);
        hSeedWnd = CreateWindowW(L"SeedWnd", L"Seed:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 600, 50, 20,
            hWnd, NULL, hInst, NULL);

        break;
    }

    case WM_APP + 1:
        {
        //handle map generation completion
        DestroyWindow(hProgressBar);
        InvalidateRect(hwndDisplay, NULL, true);
        UpdateWindow(hwndDisplay);

        EnableControls(true);
        break;
        }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_NEWP:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_CREATENEWPROJECT), hWnd, CreateNewProject);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case CREATE_IMAGE:
            {
                if (wcslen(project.name) == 0) {
                    LoadString(hInst, IDS_NOPROJECTERR, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Project error", MB_OK | MB_ICONWARNING);
                    break;
                }

                wchar_t seed[64];

                SendMessage(hSeedText, WM_GETTEXT, 64, (LPARAM)seed);

                if (seed == NULL || wcslen(seed) == 0 || seed[0] == L'\0' || isspace(seed[0])) {
                    LoadString(hInst, IDS_VALEMPTYSEED, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Validation_Error", MB_OK | MB_ICONWARNING);
                    break;
                }
                if (wcslen(seed) != 24) {
                    LoadString(hInst, IDS_VALSEEDSMALL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Validation_Error", MB_OK | MB_ICONWARNING);
                    break;
                }
                std::wregex number3digit(L"^[0-9]+$");

                if (!std::regex_match(seed, number3digit)) {
                    LoadString(hInst, IDS_VALSEEDNUM, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Validation_Error", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }


                EnableControls(false);

                wchar_t* data = new wchar_t[wcslen(seed) + 1];
                wcscpy(data, seed);
                hGenerateMapThread = CreateThread(NULL, 0, GenerateMapThread, data, 0, NULL);
                if (hGenerateMapThread == NULL) {
                    ExitProcess(3);
                }

                break;
            }
            case IDRANDSEED:
            {
                if (wcslen(project.name) == 0) {
                    LoadString(hInst, IDS_NOPROJECTERR, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Project error", MB_OK | MB_ICONWARNING);
                    break;
                }

                EnableControls(false);

                wchar_t randSeed[64] = {};
                wchar_t randNumber;

                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> distrib(0, 9);

                for (int i = 0; i < 24; i++) {
                    randNumber = distrib(gen) + L'0';
                    randSeed[i] = randNumber;
                }

                SendMessage(hSeedText, WM_SETTEXT, NULL, (LPARAM)randSeed);

                wchar_t* data = new wchar_t[wcslen(randSeed) + 1];
                wcscpy(data, randSeed);
                hGenerateMapThread = CreateThread(NULL, 0, GenerateMapThread, data, 0, NULL);
                if (hGenerateMapThread == NULL) {
                    ExitProcess(3);
                }

                break;
            }
            case IDM_SAVE:
            {
                if (wcslen(project.name) == 0) {
                    LoadString(hInst, IDS_NOPROJECTERR, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Project error", MB_OK | MB_ICONWARNING);
                    break;
                }

                wchar_t seedText[25];
                SendMessage(hSeedText, WM_GETTEXT, 25, (LPARAM)seedText);

                project.width = ImgGen->getWidht();
                project.height = ImgGen->getHeight();
                project.waterLvl = ImgGen->waterLvl;
                project.mountainLvl = ImgGen->mountainLvl;
                project.coldLvl = ImgGen->coldLvl;
                project.warmLvl = ImgGen->warmLvl;
                project.dryLvl = ImgGen->dryLvl;
                project.humidLvl = ImgGen->humidLvl;
                project.genBeach = beachChecked;
                project.genLava = lavaChecked;
                wcscpy(project.seed, seedText);

                if (wcslen(szFileName) == 0) {
                    OnProjectSave();
                    break;
                }
                if (!SaveProject(szFileName)) {
                    LoadString(hInst, IDS_SAVEFAIL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Saving error", MB_OK | MB_ICONERROR);
                    break;
                }
                writeRecentFile(szFileName);

                break;
            }
            case IDM_SAVEAS:
            {
                if (wcslen(project.name) == 0) {
                    LoadString(hInst, IDS_NOPROJECTERR, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Project error", MB_OK | MB_ICONWARNING);
                    break;
                }

                wchar_t seedText[25];
                SendMessage(hSeedText, WM_GETTEXT, 25, (LPARAM)seedText);

                project.width = ImgGen->getWidht();
                project.height = ImgGen->getHeight();
                project.waterLvl = ImgGen->waterLvl;
                project.mountainLvl = ImgGen->mountainLvl;
                project.coldLvl = ImgGen->coldLvl;
                project.warmLvl = ImgGen->warmLvl;
                project.dryLvl = ImgGen->dryLvl;
                project.humidLvl = ImgGen->humidLvl;
                project.genBeach = beachChecked;
                project.genLava = lavaChecked;
                wcscpy(project.seed, seedText);

                OnProjectSave();

                break;
            }
            case IDM_OPEN:
            {
                HRESULT hr;
                CComPtr<IFileOpenDialog> pDlg;
                COMDLG_FILTERSPEC aFileTypes[] = {
                    { L"Dat files", L"*.dat" },
                    { L"All files", L"*.*" }
                };

                hr = pDlg.CoCreateInstance(__uuidof(FileOpenDialog));
                if (FAILED(hr))
                    break;

                pDlg->SetFileTypes(_countof(aFileTypes), aFileTypes);
                pDlg->SetTitle(L"Open project");

                hr = pDlg->Show(hWnd);

                if (SUCCEEDED(hr))
                {
                    CComPtr<IShellItem> pItem;

                    hr = pDlg->GetResult(&pItem);

                    if (SUCCEEDED(hr))
                    {
                        LPOLESTR pwsz = NULL;

                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);

                        if (SUCCEEDED(hr))
                        {
                            if (!LoadProject(pwsz)) {
                                LoadString(hInst, IDS_OPENFAIL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                                MessageBox(hWnd, achTemp, L"Opening error", MB_OK | MB_ICONERROR);
                                break;
                            }
                            wcscpy(szFileName, pwsz);
                            writeRecentFile(szFileName);

                            EnableControls(false);

                            hGenerateMapThread = CreateThread(NULL, 0, GenerateOpenMapThread, NULL, 0, NULL);
                            if (hGenerateMapThread == NULL) {
                                ExitProcess(3);
                            }
                            CoTaskMemFree(pwsz);
                        }
                    }
                }
                break;
            }
            case IDM_RECENT_FILE0 + 1:
            case IDM_RECENT_FILE0 + 2:
            case IDM_RECENT_FILE0 + 3:
            case IDM_RECENT_FILE0 + 4:
            case IDM_RECENT_FILE0 + 5:
            {
                int index = LOWORD(wParam) - IDM_RECENT_FILE0;
                if (index <= recentFileCount) {
                    if (!LoadProject(recentFiles[index - 1])) {
                        LoadString(hInst, IDS_OPENFAIL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                        MessageBox(hWnd, achTemp, L"Opening error", MB_OK | MB_ICONERROR);
                        removeRecentFile(index - 1);
                        break;
                    }
                    else {
                        EnableControls(false);

                        hGenerateMapThread = CreateThread(NULL, 0, GenerateOpenMapThread, NULL, 0, NULL);
                        if (hGenerateMapThread == NULL) {
                            ExitProcess(3);
                        }

                        wcscpy(szFileName, recentFiles[index - 1]);
                        writeRecentFile(szFileName);
                    }
                }
                else {
                    LoadString(hInst, IDS_FILENOTEXIST, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Opening error", MB_OK | MB_ICONERROR);
                }
                    
                break;
            }
            case ID_EXPORT:
            {
                if (wcslen(project.name) == 0) {
                    LoadString(hInst, IDS_NOPROJECTERR, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                    MessageBox(hWnd, achTemp, L"Project error", MB_OK | MB_ICONWARNING);
                    break;
                }

                size_t i;
                char* f = new char[MAX_PATH];
                HRESULT hr;
                CComPtr<IFileSaveDialog> pDlg;
                COMDLG_FILTERSPEC aFileTypes[] = {
                    { L"Bmp files", L"*.bmp" },
                    { L"All files", L"*.*" }
                };

                // Create the file-save dialog COM object.
                hr = pDlg.CoCreateInstance(__uuidof(FileSaveDialog));

                if (FAILED(hr))
                    break;

                // Set the dialog's caption text, file types, Save button label,
                // default file name, and default extension.
                // NOTE: Error handling omitted here for clarity.
                pDlg->SetFileTypes(_countof(aFileTypes), aFileTypes);
                pDlg->SetTitle(L"Export as");
                pDlg->SetOkButtonLabel(L"Export");
                pDlg->SetFileName(project.name);
                pDlg->SetDefaultExtension(L"bmp");

                // Show the dialog.
                hr = pDlg->Show(hWnd);

                // If the user chose a file, save the user's data to that file.
                if (SUCCEEDED(hr))
                {
                    CComPtr<IShellItem> pItem;

                    hr = pDlg->GetResult(&pItem);

                    if (SUCCEEDED(hr))
                    {
                        LPOLESTR pwsz = NULL;

                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);

                        if (SUCCEEDED(hr))
                        {
                            //TODO: Save the file here, 'pwsz' has the full path
                            wcstombs_s(&i, f, (size_t)MAX_PATH, pwsz, (size_t)MAX_PATH - 1);
                            ImgGen->createImage(f);
                            CoTaskMemFree(pwsz);
                        }
                    }
                }
                delete[] f;
                break;
            }
            case IDM_PL:
            {
                setLanguage(MAKELCID(MAKELANGID(LANG_POLISH, SUBLANG_POLISH_POLAND), SORT_DEFAULT));
                break;
            }
            case IDM_EN:
            {
                setLanguage(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
                break;
            }
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
    {
        OnSize(hwndTab, lParam);
        InvalidateRect(hwndDisplay, NULL, false);
        break;
    }
    case WM_VSCROLL:
    {
        SCROLLINFO si = { sizeof(SCROLLINFO) };
        si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);

        int oldPos = si.nPos;
        int newPos = oldPos;

        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            si.nPos -= 20;
            break;
        case SB_LINEDOWN:
            si.nPos += 20;
            break;
        case SB_PAGEUP:
            si.nPos -= si.nPage;
            break;
        case SB_PAGEDOWN:
            si.nPos += si.nPage;
            break;
        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;
        default:
            break;
        }

        si.fMask = SIF_POS;

        int delta = yScrollPos - si.nPos;
        yScrollPos = si.nPos;

        ScrollWindow(hWnd, 0, delta, NULL, NULL);
        scrolled = true;

        RECT rc;
        GetWindowRect(hwndTab, &rc);
        POINT ptTL = { rc.left, rc.top };
        POINT ptBR = { rc.right, rc.bottom };

        ScreenToClient(hWnd, &ptTL);
        ScreenToClient(hWnd, &ptBR);

        rc.left = ptTL.x;
        rc.top = ptTL.y;
        rc.right = ptBR.x;
        rc.bottom = ptBR.y;
        
        if (!SetWindowPos(hwndTab, HWND_TOP, rc.left, rc.top, rc.right - rc.left, 680, SWP_SHOWWINDOW))
            return E_FAIL;

        UpdateWindow(hWnd);
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        break;
    }
    case WM_HSCROLL:
    {
        SCROLLINFO si = { sizeof(SCROLLINFO) };
        si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_HORZ, &si);

        int oldPos = si.nPos;
        int newPos = oldPos;

        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            si.nPos -= 20;
            break;
        case SB_LINEDOWN:
            si.nPos += 20;
            break;
        case SB_PAGEUP:
            si.nPos -= si.nPage;
            break;
        case SB_PAGEDOWN:
            si.nPos += si.nPage;
            break;
        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;
        default:
            break;
        }

        si.fMask = SIF_POS;

        int delta = xScrollPos - si.nPos;
        xScrollPos = si.nPos;

        ScrollWindow(hWnd, delta, 0, NULL, NULL);
        scrolled = true;

        RECT rc;
        GetWindowRect(hwndTab, &rc);
        POINT ptTL = { rc.left, rc.top };
        POINT ptBR = { rc.right, rc.bottom };

        ScreenToClient(hWnd, &ptTL);
        ScreenToClient(hWnd, &ptBR);

        rc.left = ptTL.x;
        rc.top = ptTL.y;
        rc.right = ptBR.x;
        rc.bottom = ptBR.y;

        if (!SetWindowPos(hwndTab, HWND_TOP, rc.left, rc.top, 1170, rc.bottom - rc.top, SWP_SHOWWINDOW))
            return E_FAIL;

        UpdateWindow(hWnd);
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
        break;
    }
    case WM_MOUSEWHEEL:
    {
        if (GET_KEYSTATE_WPARAM(wParam) == MK_SHIFT)
            CustomHandleMouseWheel(hWnd, GET_WHEEL_DELTA_WPARAM(wParam), FALSE);
        else
            CustomHandleMouseWheel(hWnd, GET_WHEEL_DELTA_WPARAM(wParam), TRUE);
        UpdateWindow(hWnd);
        break;
    }
    case WM_NOTIFY:
    {
        OnNotify(hwndTab, hwndDisplay, lParam);
        InvalidateRect(hwndDisplay, NULL, false);
        UpdateWindow(hwndDisplay);
        break;
    }
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        ReleaseDC(hwndDisplay, hdcMemHei);
        ReleaseDC(hwndDisplay, hdcMemHumid);
        ReleaseDC(hwndDisplay, hdcMemTemp);
        ReleaseDC(hwndDisplay, hdcMemFin);
        CloseHandle(hGenerateMapThread);  
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK ControlsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int iPage = TabCtrl_GetCurSel(hwndTab);
    switch (message) {
    case WM_CREATE:
    {
        CreateControls(hWnd);
    }
    break;
    case WM_HSCROLL:
    {
        switch (iPage) {
        case 0: {
            LRESULT WLvlpos = SendMessageW(hTrack1, TBM_GETPOS, 0, 0); //get trackpad value
            if (WLvlpos >= ImgGen->mountainLvl)
                SendMessageW(hTrack1, TBM_SETPOS, 0, ImgGen->mountainLvl - 1);
            else
            {
                ImgGen->waterLvl = (int)WLvlpos;
                SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)WLvlpos); //update updown value
            }
            LRESULT MntPos = SendMessageW(hTrack2, TBM_GETPOS, 0, 0);
            if (MntPos <= ImgGen->waterLvl)
                SendMessageW(hTrack2, TBM_SETPOS, 0, ImgGen->waterLvl + 1);
            else
            {
                ImgGen->mountainLvl = (int)MntPos;
                SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)MntPos);
            }

            ImgGen->changeHeightMap(hdcMemHei);
        }
              break;
        case 1: {
            LRESULT WLvlpos = SendMessageW(hTrack1, TBM_GETPOS, 0, 0); //get trackpad value
            if (WLvlpos >= ImgGen->warmLvl)
                SendMessageW(hTrack1, TBM_SETPOS, 0, ImgGen->warmLvl - 1);
            else
            {
                ImgGen->coldLvl = (int)WLvlpos;
                SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)WLvlpos); //update updown value
            }
            LRESULT MntPos = SendMessageW(hTrack2, TBM_GETPOS, 0, 0);
            if (MntPos <= ImgGen->coldLvl)
                SendMessageW(hTrack2, TBM_SETPOS, 0, ImgGen->coldLvl + 1);
            else
            {
                ImgGen->warmLvl = (int)MntPos;
                SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)MntPos);
            }

            ImgGen->changeTempMap(hdcMemTemp);
        }
              break;
        case 2: {
            LRESULT WLvlpos = SendMessageW(hTrack1, TBM_GETPOS, 0, 0); //get trackpad value
            if (WLvlpos >= ImgGen->humidLvl)
                SendMessageW(hTrack1, TBM_SETPOS, 0, ImgGen->humidLvl - 1);
            else
            {
                ImgGen->dryLvl = (int)WLvlpos;
                SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)WLvlpos); //update updown value
            }
            LRESULT MntPos = SendMessageW(hTrack2, TBM_GETPOS, 0, 0);
            if (MntPos <= ImgGen->dryLvl)
                SendMessageW(hTrack2, TBM_SETPOS, 0, ImgGen->dryLvl + 1);
            else
            {
                ImgGen->humidLvl = (int)MntPos;
                SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)MntPos);
            }

            ImgGen->changeHumidMap(hdcMemHumid);
        }
              break;
        default:
            break;
        }
        ImgGen->makeFinalMap(hdcMemFin, beachChecked, lavaChecked);
        InvalidateRect(hwndDisplay, NULL, false);
        UpdateWindow(hwndDisplay);
        //DrawMap();
    }
    break;
    case WM_NOTIFY:
    {
        OnNotify(hwndTab, hwndDisplay, lParam);

        InvalidateRect(hwndDisplay, NULL, false);
        UpdateWindow(hwndDisplay);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK FinControlsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int iPage = TabCtrl_GetCurSel(hwndTab);
    switch (message) {
    case WM_CREATE:
    {
        CreateControlsFin(hWnd);
    }
    break;
    case WM_HSCROLL:
    {
        switch (iPage) {
        case 3: {
            LRESULT WLvlpos = SendMessageW(hTrackHei, TBM_GETPOS, 0, 0); //get trackpad value
            if (WLvlpos >= ImgGen->mountainLvl)
                SendMessageW(hTrackHei, TBM_SETPOS, 0, ImgGen->mountainLvl - 1);
            else
            {
                ImgGen->waterLvl = (int)WLvlpos;
                SendMessageW(hUpDownHei, UDM_SETPOS, 0, (LPARAM)WLvlpos); //update updown value
            }
            LRESULT MntPos = SendMessageW(hTrackMnt, TBM_GETPOS, 0, 0);
            if (MntPos <= ImgGen->waterLvl)
                SendMessageW(hTrackMnt, TBM_SETPOS, 0, ImgGen->waterLvl + 1);
            else
            {
                ImgGen->mountainLvl = (int)MntPos;
                SendMessageW(hUpDownMnt, UDM_SETPOS, 0, (LPARAM)MntPos);
            }

            ImgGen->changeHeightMap(hdcMemHei);

            LRESULT TempLowLvlpos = SendMessageW(hTrackTempL, TBM_GETPOS, 0, 0);
            if (TempLowLvlpos >= ImgGen->warmLvl)
                SendMessageW(hTrackTempL, TBM_SETPOS, 0, ImgGen->warmLvl - 1);
            else
            {
                ImgGen->coldLvl = (int)TempLowLvlpos;
                SendMessageW(hUpDownTempL, UDM_SETPOS, 0, (LPARAM)TempLowLvlpos);
            }
            LRESULT TempHighLvlPos = SendMessageW(hTrackTempH, TBM_GETPOS, 0, 0);
            if (TempHighLvlPos <= ImgGen->coldLvl)
                SendMessageW(hTrackTempH, TBM_SETPOS, 0, ImgGen->coldLvl + 1);
            else
            {
                ImgGen->warmLvl = (int)TempHighLvlPos;
                SendMessageW(hUpDownTempH, UDM_SETPOS, 0, (LPARAM)TempHighLvlPos);
            }

            ImgGen->changeTempMap(hdcMemTemp);

            LRESULT HumidLowLvlpos = SendMessageW(hTrackHumL, TBM_GETPOS, 0, 0);
            if (HumidLowLvlpos >= ImgGen->humidLvl)
                SendMessageW(hTrackHumL, TBM_SETPOS, 0, ImgGen->humidLvl - 1);
            else
            {
                ImgGen->dryLvl = (int)HumidLowLvlpos;
                SendMessageW(hUpDownHumL, UDM_SETPOS, 0, (LPARAM)HumidLowLvlpos);
            }
            LRESULT HumidHighLvlPos = SendMessageW(hTrackHumH, TBM_GETPOS, 0, 0);
            if (HumidHighLvlPos <= ImgGen->dryLvl)
                SendMessageW(hTrackHumH, TBM_SETPOS, 0, ImgGen->dryLvl + 1);
            else
            {
                ImgGen->humidLvl = (int)HumidHighLvlPos;
                SendMessageW(hUpDownHumH, UDM_SETPOS, 0, (LPARAM)HumidHighLvlPos);
            }

            ImgGen->changeHumidMap(hdcMemHumid);
        }
              break;
        default:
            break;
        }
        ImgGen->makeFinalMap(hdcMemFin, beachChecked, lavaChecked);
        InvalidateRect(hwndDisplay, NULL, false);
        UpdateWindow(hwndDisplay);
    }
    break;
    case WM_COMMAND:
    {
        switch (LOWORD(wParam)) {
        case ID_BEACHCHECKBOX:
        {
            beachChecked = IsDlgButtonChecked(FinControlsWnd, ID_BEACHCHECKBOX);
            if (beachChecked) {
                CheckDlgButton(FinControlsWnd, ID_BEACHCHECKBOX, BST_UNCHECKED);
                beachChecked = false;
                ImgGen->makeFinalMap(hdcMemFin, false, lavaChecked);
                InvalidateRect(hwndDisplay, NULL, false);
                UpdateWindow(hwndDisplay);
            }
            else {
                CheckDlgButton(FinControlsWnd, ID_BEACHCHECKBOX, BST_CHECKED);
                beachChecked = true;
                ImgGen->makeFinalMap(hdcMemFin, true, lavaChecked);
                InvalidateRect(hwndDisplay, NULL, false);
                UpdateWindow(hwndDisplay);
            }
            break;
        }
        case ID_LAVACHECKBOX:
        {
            lavaChecked = IsDlgButtonChecked(FinControlsWnd, ID_LAVACHECKBOX);
            if (lavaChecked) {
                CheckDlgButton(FinControlsWnd, ID_LAVACHECKBOX, BST_UNCHECKED);
                lavaChecked = false;
                ImgGen->makeFinalMap(hdcMemFin, beachChecked, false);
                InvalidateRect(hwndDisplay, NULL, false);
                UpdateWindow(hwndDisplay);
            }
            else {
                CheckDlgButton(FinControlsWnd, ID_LAVACHECKBOX, BST_CHECKED);
                lavaChecked = true;
                ImgGen->makeFinalMap(hdcMemFin, beachChecked, true);
                InvalidateRect(hwndDisplay, NULL, false);
                UpdateWindow(hwndDisplay);
            }
            break;
            break;
        }
        default:
            break;
        }
        break;
    }
    case WM_NOTIFY:
    {
        OnNotify(hwndTab, hwndDisplay, lParam);
        InvalidateRect(hwndDisplay, NULL, false);
        UpdateWindow(hwndDisplay);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK DisplayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int iPage = TabCtrl_GetCurSel(hwndTab);
    switch (message) {
    case WM_CREATE:
    {

    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwndDisplay, &ps);

        int iPage = TabCtrl_GetCurSel(hwndTab);
        int w = ImgGen->getWidht();
        int h = ImgGen->getHeight();
        int left = (500 - w) / 2;
        int top = (500 - h) / 2;
        if (w > 500) {
            w = 500;
            left = 0;
        }
        if (h > 500) { 
            h = 500;
            top = 0;
        }
        switch (iPage) {
        case 0:
            SelectObject(hdc, height);
            BitBlt(hdc, left, top, w, h, hdcMemHei, 0, 0, SRCCOPY);
            break;
        case 1:
            SelectObject(hdc, temp);
            BitBlt(hdc, left, top, w, h, hdcMemTemp, 0, 0, SRCCOPY);
            break;
        case 2:
            SelectObject(hdc, humid);
            BitBlt(hdc, left, top, w, h, hdcMemHumid, 0, 0, SRCCOPY);
            break;
        case 3:
            SelectObject(hdc, finalMap);
            BitBlt(hdc, left, top, w, h, hdcMemFin, 0, 0, SRCCOPY);
            break;
        default:
            break;
        }

        EndPaint(hwndDisplay, &ps);
    }
    break;
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        //had to implement my own erasebkgnd because when scrolling by holding the scroll bar 
        //the window would be redrawn with erasebkng too quickly and cause flicker so i just 
        //draw the background manually around the image using polygon shape
        if (scrolled) {
            scrolled = false;
            //HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
            HBRUSH hBrush = GetSysColorBrush(COLOR_BTNFACE);
            SelectObject(hdc, hBrush);
            SelectObject(hdc, GetStockObject(NULL_PEN));

            int top = (500 - ImgGen->getHeight()) / 2;
            int bottom = top + ImgGen->getHeight();
            int left = (500 - ImgGen->getWidht()) / 2;
            int right = left + ImgGen->getWidht();

            const POINT polygon[20] = { 
                0, 0, 
                500, 0, 
                500, 500, 
                0, 500, 
                0, top,
                left, top,
                left, bottom,
                right, bottom,
                right, top,
                0, top
            };

            Polygon(hdc, polygon, 10);
            return 1;
        }

        //if erase background was not called by scrolling then just do an ordinary redraw
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW));
        return 1;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK PaletteWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int iPage = TabCtrl_GetCurSel(hwndTab);
    palette colors[27] = {
            {L"", 0, 0, 255},
            {L"", 0, 255, 0},
            {L"", 33, 33, 33},
            {L"", 0, 255, 0},
            {L"", 255,255, 0},
            {L"", 255, 0, 0},
            {L"", 255, 255, 0},
            {L"", 0, 255, 0},
            {L"", 0, 0, 255},
            {L"", 0, 98, 115},
            {L"", 0, 150, 148},
            {L"", 207, 198, 45},
            {L"", 92, 70, 29},
            {L"", 23, 145, 25},
            {L"", 4, 105, 4},
            {L"", 5, 130, 5},
            {L"", 52, 52, 37},
            {L"", 140, 137, 69},
            {L"", 66, 66, 66},
            {L"", 255, 255, 255},
            {L"", 109, 115, 0},
            {L"", 110, 84, 0},
            {L"", 171, 171, 62},
            {L"", 135, 94, 11},
            {L"", 255, 255, 0},
            {L"", 237, 232, 209},
            {L"", 140, 35, 17},
    };

    switch (message) {
    case WM_CREATE:
    {
        
    }
    break;
    case WM_PAINT:
    {
        for (int i = 0; i < 27; i++) {
            LoadString(hInst, IDS_PALETTETERRAIN0 + i, colors[i].name, sizeof(colors[i].name) / sizeof(colors[i].name[0]));
        }
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hPaletteWindow, &ps);

        HBRUSH hBrushBkg = GetSysColorBrush(COLOR_BTNFACE);
        SelectObject(hdc, hBrushBkg);
        SelectObject(hdc, GetStockObject(NULL_PEN));
        Rectangle(hdc, 0, 0, 260, 330);

        HFONT hFont = CreateFontW(15, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0,
            0, 0, 0, 0, L"");
        SelectObject(hdc, hFont);
        SelectObject(hdc, GetStockObject(BLACK_PEN));

        SetBkMode(hdc, TRANSPARENT);

        switch (iPage) {
        case 0:
        case 1:
        case 2:
        {
            for (int i = iPage * 3; i < iPage * 3 + 3; i++) {
                HBRUSH hBrush = CreateSolidBrush(RGB(colors[i].r, colors[i].g, colors[i].b));
                SelectObject(hdc, hBrush);
                Rectangle(hdc, 5, 5 + (i - iPage * 3) * 20, 50, 20 + (i - iPage * 3) * 20);
                TextOutW(hdc, 55, 5 + (i - iPage * 3) * 20, colors[i].name, wcslen(colors[i].name));
                DeleteObject(hBrush);
            }
            break;
        }
        case 3:
            for (int i = 9; i < 27; i++) {
                HBRUSH hBrush = CreateSolidBrush(RGB(colors[i].r, colors[i].g, colors[i].b));
                SelectObject(hdc, hBrush);
                Rectangle(hdc, 5, 5 + (i - iPage * 3) * 20, 50, 20 + (i - iPage * 3) * 20);
                TextOutW(hdc, 55, 5 + (i - iPage * 3) * 20, colors[i].name, wcslen(colors[i].name));
                DeleteObject(hBrush);
            }
            break;
        default:
            break;
        }
        DeleteObject(hFont);
        DeleteObject(hBrushBkg);
        EndPaint(hPaletteWindow, &ps);
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK SeedWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    //i had to write this beacuse i wanted a transparent background for this text
    switch (message) {
    case WM_CREATE:
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hSeedWnd, &ps);
        TextOutW(hdc, 0, 2, L"Seed:", 5);
        EndPaint(hSeedWnd, &ps);
        break;
    }
    case WM_ERASEBKGND:
        return 1;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    case WM_NOTIFY:
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->idFrom == IDC_LINK1 && pnmh->code == NM_CLICK) {
            PNMLINK pNMLink = (PNMLINK)lParam;
            ShellExecuteW(NULL, L"open", pNMLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
        }
        break;
    }
    

    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK CreateNewProject(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR achTemp[256];
    HANDLE hGenerateMapThread = 0;
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t name[64];
            wchar_t height[64];
            wchar_t width[64];

            GetDlgItemText(hDlg, IDC_PROJECT_NAME, name, 64);
            GetDlgItemText(hDlg, IDC_WIDTH, width, 64);
            GetDlgItemText(hDlg, IDC_HEIGHT, height, 64);

            if (name == NULL || wcslen(name) == 0 || name[0] == L'\0' || isspace(name[0])) {
                LoadString(hInst, IDS_VALNAMEEMPTY, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                MessageBox(hDlg, achTemp, L"Validation_Error", MB_OK | MB_ICONWARNING);
                return (INT_PTR)TRUE;
            }

            std::wregex projectName(L"[<>:\"/\\\\|?*]");

            if (std::regex_search(name, projectName)) {
                LoadString(hInst, IDS_VALNAMECHAR, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                MessageBox(hDlg, achTemp, L"Validation_Error", MB_OK | MB_ICONWARNING);
                return (INT_PTR)TRUE;
            }
            
            int widthNumber = _wtoi(width);
            int heightNumber = _wtoi(height);

            std::wregex number4digit(L"^[0-9]{1,4}$");

            if (!std::regex_match(width, number4digit) || widthNumber > 1000) {
                LoadString(hInst, IDS_VALWIDTHSIZE, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                MessageBox(hDlg, achTemp, L"Validation_Error", MB_OK | MB_ICONWARNING);
                return (INT_PTR)TRUE;
            }
            if (!std::regex_match(height, number4digit) || heightNumber > 1000) {
                LoadString(hInst, IDS_VALHEIGHTSIZE, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
                MessageBox(hDlg, achTemp, L"Validation_Error", MB_OK | MB_ICONWARNING);
                return (INT_PTR)TRUE;
            }

            InvalidateRect(hwndDisplay, NULL, false);
            
            if (ImgGen)
                delete ImgGen;
            ImgGen = new generateImage(heightNumber, widthNumber);

            EnableControls(false);

            wchar_t randSeed[25] = {};
            wchar_t randNumber;

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(0, 9);

            for (int i = 0; i < 24; i++) {
                randNumber = distrib(gen) + L'0';
                randSeed[i] = randNumber;
            }

            wcscpy(project.name, name);
            project.width = ImgGen->getWidht();
            project.height = ImgGen->getHeight();
            project.waterLvl = ImgGen->waterLvl;
            project.mountainLvl = ImgGen->mountainLvl;
            project.coldLvl = ImgGen->coldLvl;
            project.warmLvl = ImgGen->warmLvl;
            project.dryLvl = ImgGen->dryLvl;
            project.humidLvl = ImgGen->humidLvl;
            project.genBeach = false;
            project.genLava = false;
            wcscpy(project.seed, randSeed);

            SendMessage(hSeedText, WM_SETTEXT, NULL, (LPARAM)randSeed);
            int iPage = TabCtrl_GetCurSel(hwndTab);
            switch (iPage) {
            case 0:
            {
                SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)ImgGen->waterLvl);
                SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)ImgGen->mountainLvl);
                SendMessageW(hTrack1, TBM_SETPOS, 0, ImgGen->waterLvl);
                SendMessageW(hTrack2, TBM_SETPOS, 0, ImgGen->mountainLvl);
                break;
            }
            case 1:
            {
                SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)ImgGen->coldLvl);
                SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)ImgGen->warmLvl);
                SendMessageW(hTrack1, TBM_SETPOS, 0, ImgGen->coldLvl);
                SendMessageW(hTrack2, TBM_SETPOS, 0, ImgGen->warmLvl);
                break;
            }
            case 2:
            {
                SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)ImgGen->dryLvl);
                SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)ImgGen->humidLvl);
                SendMessageW(hTrack1, TBM_SETPOS, 0, ImgGen->dryLvl);
                SendMessageW(hTrack2, TBM_SETPOS, 0, ImgGen->humidLvl);
                break;
            }
            case 3:
            {
                SendMessageW(hUpDownHei, UDM_SETPOS, 0, (LPARAM)ImgGen->waterLvl);
                SendMessageW(hUpDownMnt, UDM_SETPOS, 0, (LPARAM)ImgGen->mountainLvl);
                SendMessageW(hTrackHei, TBM_SETPOS, 0, ImgGen->waterLvl);
                SendMessageW(hTrackMnt, TBM_SETPOS, 0, ImgGen->mountainLvl);
                SendMessageW(hUpDownTempL, UDM_SETPOS, 0, (LPARAM)ImgGen->coldLvl);
                SendMessageW(hUpDownTempH, UDM_SETPOS, 0, (LPARAM)ImgGen->warmLvl);
                SendMessageW(hTrackTempL, TBM_SETPOS, 0, ImgGen->coldLvl);
                SendMessageW(hTrackTempH, TBM_SETPOS, 0, ImgGen->warmLvl);
                SendMessageW(hUpDownHumL, UDM_SETPOS, 0, (LPARAM)ImgGen->dryLvl);
                SendMessageW(hUpDownHumH, UDM_SETPOS, 0, (LPARAM)ImgGen->humidLvl);
                SendMessageW(hTrackHumL, TBM_SETPOS, 0, ImgGen->dryLvl);
                SendMessageW(hTrackHumH, TBM_SETPOS, 0, ImgGen->humidLvl);
                break;
            }
            default:
                break;
            }
            CheckDlgButton(FinControlsWnd, ID_BEACHCHECKBOX, false);
            CheckDlgButton(FinControlsWnd, ID_LAVACHECKBOX, false);
            

            wchar_t* data = new wchar_t[wcslen(randSeed) + 1];
            wcscpy(data, randSeed);
            hGenerateMapThread = CreateThread(NULL, 0, GenerateMapThread, data, 0, NULL);
            if (hGenerateMapThread == NULL) {
                ExitProcess(3);
            }

            wcscpy(szFileName, L"");
            //wcscpy(szExportFileName, L"");

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

HWND DoCreateTabControl(HWND hwndParent)
{
    RECT rcClient;
    INITCOMMONCONTROLSEX icex;
    HWND hwndTab;
    TCITEM tie;
    int i;
    TCHAR achTemp[256];

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    // Get the dimensions of the parent window's client area, and 
    // create a tab control child window of that size.
    GetClientRect(hwndParent, &rcClient);
    hwndTab = CreateWindow(WC_TABCONTROL, L"",
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, rcClient.right, rcClient.bottom,
        hwndParent, NULL, hInst, NULL);
    if (hwndTab == NULL)
    {
        return NULL;
    }

    tie.mask = TCIF_TEXT | TCIF_IMAGE;
    tie.iImage = -1;
    tie.pszText = achTemp;

    for (i = 0; i < 4; i++)
    {
        LoadString(hInst, IDS_HEI_MAP + i,
            achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        if (TabCtrl_InsertItem(hwndTab, i, &tie) == -1)
        {
            DestroyWindow(hwndTab);
            return NULL;
        }
    }
    SendMessage(hwndTab, TCM_SETCURSEL, 3, 0);
    return hwndTab;
}

HWND DoCreateDisplayWindow(HWND hwndTab)
{
    
    WNDCLASS wc3 = { 0 };
    wc3.lpfnWndProc = DisplayWndProc;
    wc3.hInstance = hInst;
    wc3.lpszClassName = L"DisplayWindow";
    wc3.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    RegisterClass(&wc3);

    HWND hwndStatic = CreateWindow(L"DisplayWindow", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | SS_BITMAP | WS_CLIPCHILDREN,
        5, 30, 502, 502,
        hwndTab, NULL, hInst,
        NULL);
    return hwndStatic;
}

void CreateControls(HWND hwnd) {
    INITCOMMONCONTROLSEX icex;
    TCHAR achTemp[256];
    int pos = 10;

    //create water level controls
    LoadString(hInst, IDS_WATERLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hControlsText1 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, pos, 300, 20,
        hwnd, (HMENU)1, NULL, NULL);

    //create updow control
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);

    hUpDown1 = CreateWindowEx(0, UPDOWN_CLASSW, NULL, WS_CHILD | WS_VISIBLE
        | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
        0, 0, 0, 0, hwnd, (HMENU)IDM_UPDOWN1, NULL, NULL);

    hEdit1 = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDITW, NULL, WS_CHILD
        | WS_VISIBLE | ES_RIGHT, 30, pos + 30, 70, 25, hwnd,
        (HMENU)IDM_EDIT, NULL, NULL);

    SendMessageW(hUpDown1, UDM_SETBUDDY, (WPARAM)hEdit1, 0);
    SendMessageW(hUpDown1, UDM_SETRANGE, 0, MAKELPARAM(UD_MAX_POS, UD_MIN_POS));
    SendMessageW(hUpDown1, UDM_SETPOS32, 0, 0);
    SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)100);  //updown starting value

    //create trackpad control
    HWND hLeftLabel1 = CreateWindowW(L"Static", L"0",        
        WS_CHILD | WS_VISIBLE, 0, 0, 10, 30, hwnd, (HMENU)2, NULL, NULL);

    HWND hRightLabel1 = CreateWindowW(L"Static", L"255",
        WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, (HMENU)3, NULL, NULL);

    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    hTrack1 = CreateWindowEx(0, TRACKBAR_CLASSW, L"Trackbar Control",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        130, pos + 30, 255, 30, hwnd, (HMENU)4, NULL, NULL);

    SendMessageW(hTrack1, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
    SendMessageW(hTrack1, TBM_SETPAGESIZE, 0, 10);
    SendMessageW(hTrack1, TBM_SETTICFREQ, 10, 0);
    SendMessageW(hTrack1, TBM_SETPOS, FALSE, 100);
    SendMessageW(hTrack1, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabel1);
    SendMessageW(hTrack1, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabel1);

    pos += 60;

    //create mountain height controls
    LoadString(hInst, IDS_MOUNTAINLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hControlsText2 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, pos, 300, 20,
        hwnd, (HMENU)5, NULL, NULL);

    //create updow control
    hUpDown2 = CreateWindowEx(0, UPDOWN_CLASSW, NULL, WS_CHILD | WS_VISIBLE
        | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
        0, 0, 0, 0, hwnd, (HMENU)IDM_UPDOWN2, NULL, NULL);

    hEdit2 = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, NULL, WS_CHILD
        | WS_VISIBLE | ES_RIGHT, 30, pos + 30, 70, 25, hwnd,
        (HMENU)IDM_EDIT_MOUNT_LVL, NULL, NULL);

    SendMessageW(hUpDown2, UDM_SETBUDDY, (WPARAM)hEdit2, 0);
    SendMessageW(hUpDown2, UDM_SETRANGE, 0, MAKELPARAM(UD_MAX_POS, UD_MIN_POS));
    SendMessageW(hUpDown2, UDM_SETPOS32, 0, 0);
    SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)200);

    //create trackpad control
    HWND hLeftLabel2 = CreateWindowW(L"Static", L"0",
        WS_CHILD | WS_VISIBLE, 0, 0, 10, 30, hwnd, (HMENU)6, NULL, NULL);

    HWND hRightLabel2 = CreateWindowW(L"Static", L"255",
        WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, (HMENU)7, NULL, NULL);

    hTrack2 = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        130, pos + 30, 255, 30, hwnd, (HMENU)8, NULL, NULL);

    SendMessageW(hTrack2, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
    SendMessageW(hTrack2, TBM_SETPAGESIZE, 0, 10);
    SendMessageW(hTrack2, TBM_SETTICFREQ, 10, 0);
    SendMessageW(hTrack2, TBM_SETPOS, FALSE, 200);
    SendMessageW(hTrack2, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabel2);
    SendMessageW(hTrack2, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabel2);
}

void CreateControlsFin(HWND hwnd) {
    INITCOMMONCONTROLSEX icex;
    TCHAR achTemp[256];

    int pos = 10;

    //height controls
    LoadString(hInst, IDS_WATERLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hControlsText3 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, pos, 300, 20,
        hwnd, (HMENU)1, NULL, NULL);

    //create updow control
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);

    hUpDownHei = CreateWindowEx(0, UPDOWN_CLASSW, NULL, WS_CHILD | WS_VISIBLE
        | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
        0, 0, 0, 0, hwnd, (HMENU)IDM_UPDOWNHEI, NULL, NULL);

    hEditHei = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDITW, NULL, WS_CHILD
        | WS_VISIBLE | ES_RIGHT, 30, pos + 30, 70, 25, hwnd,
        (HMENU)IDM_EDIT, NULL, NULL);

    SendMessageW(hUpDownHei, UDM_SETBUDDY, (WPARAM)hEditHei, 0);
    SendMessageW(hUpDownHei, UDM_SETRANGE, 0, MAKELPARAM(UD_MAX_POS, UD_MIN_POS));
    SendMessageW(hUpDownHei, UDM_SETPOS32, 0, 0);
    SendMessageW(hUpDownHei, UDM_SETPOS, 0, (LPARAM)100);  //updown starting value

    //create trackpad control
    HWND hLeftLabelWLvl = CreateWindowW(L"Static", L"0",
        WS_CHILD | WS_VISIBLE, 0, 0, 10, 30, hwnd, (HMENU)2, NULL, NULL);

    HWND hRightLabelWLvl = CreateWindowW(L"Static", L"255",
        WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, (HMENU)3, NULL, NULL);

    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    hTrackHei = CreateWindowEx(0, TRACKBAR_CLASSW, L"Trackbar Control",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        130, pos + 30, 255, 30, hwnd, (HMENU)4, NULL, NULL);

    SendMessageW(hTrackHei, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
    SendMessageW(hTrackHei, TBM_SETPAGESIZE, 0, 10);
    SendMessageW(hTrackHei, TBM_SETTICFREQ, 10, 0);
    SendMessageW(hTrackHei, TBM_SETPOS, FALSE, 100);
    SendMessageW(hTrackHei, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabelWLvl);
    SendMessageW(hTrackHei, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabelWLvl);

    pos += 60;

    //create mountain height controls
    LoadString(hInst, IDS_MOUNTAINLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hControlsText4 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, pos, 300, 20,
        hwnd, (HMENU)5, NULL, NULL);

    hUpDownMnt = CreateWindowEx(0, UPDOWN_CLASSW, NULL, WS_CHILD | WS_VISIBLE
        | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
        0, 0, 0, 0, hwnd, (HMENU)IDM_UPDOWNMNT, NULL, NULL);

    hEditMnt = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, NULL, WS_CHILD
        | WS_VISIBLE | ES_RIGHT, 30, pos + 30, 70, 25, hwnd,
        (HMENU)IDM_EDIT_MOUNT_LVL, NULL, NULL);

    SendMessageW(hUpDownMnt, UDM_SETBUDDY, (WPARAM)hEditMnt, 0);
    SendMessageW(hUpDownMnt, UDM_SETRANGE, 0, MAKELPARAM(UD_MAX_POS, UD_MIN_POS));
    SendMessageW(hUpDownMnt, UDM_SETPOS32, 0, 0);
    SendMessageW(hUpDownMnt, UDM_SETPOS, 0, (LPARAM)200);

    HWND hLeftLabelMnt = CreateWindowW(L"Static", L"0",
        WS_CHILD | WS_VISIBLE, 0, 0, 10, 30, hwnd, (HMENU)6, NULL, NULL);

    HWND hRightLabelMnt = CreateWindowW(L"Static", L"255",
        WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, (HMENU)7, NULL, NULL);

    hTrackMnt = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        130, pos + 30, 255, 30, hwnd, (HMENU)8, NULL, NULL);

    SendMessageW(hTrackMnt, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
    SendMessageW(hTrackMnt, TBM_SETPAGESIZE, 0, 10);
    SendMessageW(hTrackMnt, TBM_SETTICFREQ, 10, 0);
    SendMessageW(hTrackMnt, TBM_SETPOS, FALSE, 200);
    SendMessageW(hTrackMnt, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabelMnt);
    SendMessageW(hTrackMnt, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabelMnt);

    //temperature low controls
    pos += 60;
    LoadString(hInst, IDS_LOWTEMPLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hControlsText5 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, pos, 300, 20,
        hwnd, (HMENU)9, NULL, NULL);

    hUpDownTempL = CreateWindowEx(0, UPDOWN_CLASSW, NULL, WS_CHILD | WS_VISIBLE
        | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
        0, 0, 0, 0, hwnd, (HMENU)IDM_UPDOWNTEMPL, NULL, NULL);

    hEditTempL = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDITW, NULL, WS_CHILD
        | WS_VISIBLE | ES_RIGHT, 30, pos + 30, 70, 25, hwnd,
        (HMENU)11, NULL, NULL);

    SendMessageW(hUpDownTempL, UDM_SETBUDDY, (WPARAM)hEditTempL, 0);
    SendMessageW(hUpDownTempL, UDM_SETRANGE, 0, MAKELPARAM(UD_MAX_POS, UD_MIN_POS));
    SendMessageW(hUpDownTempL, UDM_SETPOS32, 0, 0);
    SendMessageW(hUpDownTempL, UDM_SETPOS, 0, (LPARAM)100);

    HWND hLeftLabelTempL = CreateWindowW(L"Static", L"0",
        WS_CHILD | WS_VISIBLE, 0, 0, 10, 30, hwnd, (HMENU)12, NULL, NULL);

    HWND hRightLabelTempL = CreateWindowW(L"Static", L"255",
        WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, (HMENU)13, NULL, NULL);

    hTrackTempL = CreateWindowEx(0, TRACKBAR_CLASSW, L"Trackbar Control",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        130, pos + 30, 255, 30, hwnd, (HMENU)14, NULL, NULL);

    SendMessageW(hTrackTempL, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
    SendMessageW(hTrackTempL, TBM_SETPAGESIZE, 0, 10);
    SendMessageW(hTrackTempL, TBM_SETTICFREQ, 10, 0);
    SendMessageW(hTrackTempL, TBM_SETPOS, FALSE, 100);
    SendMessageW(hTrackTempL, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabelTempL);
    SendMessageW(hTrackTempL, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabelTempL);

    //temp high controls
    pos += 60;
    LoadString(hInst, IDS_HIGHTEMPLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hControlsText6 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, pos, 300, 20,
        hwnd, (HMENU)15, NULL, NULL);

    hUpDownTempH = CreateWindowEx(0, UPDOWN_CLASSW, NULL, WS_CHILD | WS_VISIBLE
        | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
        0, 0, 0, 0, hwnd, (HMENU)IDM_UPDOWNTEMPH, NULL, NULL);

    hEditTempH = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, NULL, WS_CHILD
        | WS_VISIBLE | ES_RIGHT, 30, pos + 30, 70, 25, hwnd,
        (HMENU)17, NULL, NULL);

    SendMessageW(hUpDownTempH, UDM_SETBUDDY, (WPARAM)hEditTempH, 0);
    SendMessageW(hUpDownTempH, UDM_SETRANGE, 0, MAKELPARAM(UD_MAX_POS, UD_MIN_POS));
    SendMessageW(hUpDownTempH, UDM_SETPOS32, 0, 0);
    SendMessageW(hUpDownTempH, UDM_SETPOS, 0, (LPARAM)200);

    HWND hLeftLabelTempH = CreateWindowW(L"Static", L"0",
        WS_CHILD | WS_VISIBLE, 0, 0, 10, 30, hwnd, (HMENU)18, NULL, NULL);

    HWND hRightLabelTempH = CreateWindowW(L"Static", L"255",
        WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, (HMENU)19, NULL, NULL);

    hTrackTempH = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        130, pos + 30, 255, 30, hwnd, (HMENU)20, NULL, NULL);

    SendMessageW(hTrackTempH, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
    SendMessageW(hTrackTempH, TBM_SETPAGESIZE, 0, 10);
    SendMessageW(hTrackTempH, TBM_SETTICFREQ, 10, 0);
    SendMessageW(hTrackTempH, TBM_SETPOS, FALSE, 200);
    SendMessageW(hTrackTempH, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabelTempH);
    SendMessageW(hTrackTempH, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabelTempH);

    //humidity low controls
    pos += 60;
    LoadString(hInst, IDS_LOWHUMLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hControlsText7 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, pos, 300, 20,
        hwnd, (HMENU)21, NULL, NULL);

    hUpDownHumL = CreateWindowEx(0, UPDOWN_CLASSW, NULL, WS_CHILD | WS_VISIBLE
        | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
        0, 0, 0, 0, hwnd, (HMENU)IDM_UPDOWNHUMIDL, NULL, NULL);

    hEditHumL = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDITW, NULL, WS_CHILD
        | WS_VISIBLE | ES_RIGHT, 30, pos + 30, 70, 25, hwnd,
        (HMENU)22, NULL, NULL);

    SendMessageW(hUpDownHumL, UDM_SETBUDDY, (WPARAM)hEditHumL, 0);
    SendMessageW(hUpDownHumL, UDM_SETRANGE, 0, MAKELPARAM(UD_MAX_POS, UD_MIN_POS));
    SendMessageW(hUpDownHumL, UDM_SETPOS32, 0, 0);
    SendMessageW(hUpDownHumL, UDM_SETPOS, 0, (LPARAM)100);

    HWND hLeftLabelHumL = CreateWindowW(L"Static", L"0",
        WS_CHILD | WS_VISIBLE, 0, 0, 10, 30, hwnd, (HMENU)23, NULL, NULL);

    HWND hRightLabelHumL = CreateWindowW(L"Static", L"255",
        WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, (HMENU)24, NULL, NULL);

    hTrackHumL = CreateWindowEx(0, TRACKBAR_CLASSW, L"Trackbar Control",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        130, pos + 30, 255, 30, hwnd, (HMENU)25, NULL, NULL);

    SendMessageW(hTrackHumL, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
    SendMessageW(hTrackHumL, TBM_SETPAGESIZE, 0, 10);
    SendMessageW(hTrackHumL, TBM_SETTICFREQ, 10, 0);
    SendMessageW(hTrackHumL, TBM_SETPOS, FALSE, 100);
    SendMessageW(hTrackHumL, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabelHumL);
    SendMessageW(hTrackHumL, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabelHumL);

    //humidity high controls
    pos += 60;
    LoadString(hInst, IDS_HIGHHUMLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hControlsText8 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, pos, 300, 20,
        hwnd, (HMENU)26, NULL, NULL);

    hUpDownHumH = CreateWindowEx(0, UPDOWN_CLASSW, NULL, WS_CHILD | WS_VISIBLE
        | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
        0, 0, 0, 0, hwnd, (HMENU)IDM_UPDOWNHUMIDH, NULL, NULL);

    hEditHumH = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, NULL, WS_CHILD
        | WS_VISIBLE | ES_RIGHT, 30, pos + 30, 70, 25, hwnd,
        (HMENU)27, NULL, NULL);

    SendMessageW(hUpDownHumH, UDM_SETBUDDY, (WPARAM)hEditHumH, 0);
    SendMessageW(hUpDownHumH, UDM_SETRANGE, 0, MAKELPARAM(UD_MAX_POS, UD_MIN_POS));
    SendMessageW(hUpDownHumH, UDM_SETPOS32, 0, 0);
    SendMessageW(hUpDownHumH, UDM_SETPOS, 0, (LPARAM)200);

    HWND hLeftLabelHumH = CreateWindowW(L"Static", L"0",
        WS_CHILD | WS_VISIBLE, 0, 0, 10, 30, hwnd, (HMENU)28, NULL, NULL);

    HWND hRightLabelHumH = CreateWindowW(L"Static", L"255",
        WS_CHILD | WS_VISIBLE, 0, 0, 30, 30, hwnd, (HMENU)29, NULL, NULL);

    hTrackHumH = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        130, pos + 30, 255, 30, hwnd, (HMENU)30, NULL, NULL);

    SendMessageW(hTrackHumH, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
    SendMessageW(hTrackHumH, TBM_SETPAGESIZE, 0, 10);
    SendMessageW(hTrackHumH, TBM_SETTICFREQ, 10, 0);
    SendMessageW(hTrackHumH, TBM_SETPOS, FALSE, 200);
    SendMessageW(hTrackHumH, TBM_SETBUDDY, TRUE, (LPARAM)hLeftLabelHumH);
    SendMessageW(hTrackHumH, TBM_SETBUDDY, FALSE, (LPARAM)hRightLabelHumH);

    pos += 70;
    LoadString(hInst, IDS_GENBEACH, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hBeachCheckbox = CreateWindowW(L"Button", L"",
        WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 
        30, pos, 20, 20, hwnd, HMENU(ID_BEACHCHECKBOX), NULL, NULL);
    CheckDlgButton(hwnd, ID_BEACHCHECKBOX, BST_UNCHECKED);
    hControlsText9 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        50, pos, 300, 20,
        hwnd, (HMENU)32, NULL, NULL);

    pos += 20;
    LoadString(hInst, IDS_GENLAVA, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
    hLavaCheckbox = CreateWindowW(L"Button", L"",
        WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
        30, pos, 20, 20, hwnd, HMENU(ID_LAVACHECKBOX), NULL, NULL);
    CheckDlgButton(hwnd, ID_LAVACHECKBOX, BST_UNCHECKED);
    hControlsText10 = CreateWindowW(L"Static", achTemp,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        50, pos, 300, 20,
        hwnd, (HMENU)33, NULL, NULL);
}

// Handles the WM_SIZE message for the main window by resizing the 
//   tab control. 
// hwndTab - handle of the tab control.
// lParam - the lParam parameter of the WM_SIZE message.
//
HRESULT OnSize(HWND hwndTab, LPARAM lParam)
{
    if (hwndTab == NULL)
        return E_INVALIDARG;

    // Resize the tab control to fit the client are of main window.
    if (!SetWindowPos(hwndTab, HWND_TOP, 0, 0, GET_X_LPARAM(lParam) > 1168 ? GET_X_LPARAM(lParam) : 1170, GET_Y_LPARAM(lParam) > 670 ? GET_Y_LPARAM(lParam) : 700, SWP_SHOWWINDOW))
        return E_FAIL;

    int iPage = TabCtrl_GetCurSel(hwndTab);
    switch (iPage) {
    case 0:
    case 1:
    case 2:
    {
        if (!SetWindowPos(ControlsWnd, HWND_TOP, 510, 30, 450, GET_Y_LPARAM(lParam) > 670 ? GET_Y_LPARAM(lParam) - 35 : 700, SWP_SHOWWINDOW))
            return E_FAIL;
        if (!SetWindowPos(hPaletteWindow, HWND_TOP, 965, 30, 200, 70, SWP_SHOWWINDOW))
            return E_FAIL;
        
        break;
    }
    case 3:
    {
        if (!SetWindowPos(FinControlsWnd, HWND_TOP, 510, 30, 450, GET_Y_LPARAM(lParam) > 670 ? GET_Y_LPARAM(lParam) - 35 : 700, SWP_SHOWWINDOW))
            return E_FAIL;
        if (!SetWindowPos(hPaletteWindow, HWND_TOP, 965, 30, 200, 370, SWP_SHOWWINDOW))
            return E_FAIL;
        break;
    }
    default:
        break;
    }
    
    int clientHeight = HIWORD(lParam);
    SCROLLINFO si = { sizeof(SCROLLINFO) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = 670;
    si.nPage = clientHeight;
    si.nPos = 0;
    yScrollPos = 0;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

    int clientWidth = LOWORD(lParam);
    si.nMax = 1165;
    si.nPage = clientWidth;
    xScrollPos = 0;
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

    if (!SetWindowPos(hRndGenButton, HWND_TOP, 10, 560, 200, 30, SWP_SHOWWINDOW))
        return E_FAIL;
    if (!SetWindowPos(hSeedText, HWND_TOP, 55, 600, 200, 20, SWP_SHOWWINDOW))
        return E_FAIL;
    if (!SetWindowPos(hSeedWnd, HWND_TOP, 10, 600, 50, 20, SWP_SHOWWINDOW))
        return E_FAIL;
    if (!SetWindowPos(hGenButton, HWND_TOP, 260, 595, 100, 30, SWP_SHOWWINDOW))
        return E_FAIL;
    if (!SetWindowPos(hExportButton, HWND_TOP, 10, 630, 100, 30, SWP_SHOWWINDOW))
        return E_FAIL;

    return S_OK;
}

// Handles notifications from the tab control, as follows: 
//   TCN_SELCHANGING - always returns FALSE to allow the user to select a 
//     different tab.  
//   TCN_SELCHANGE - 
// hwndTab - handle of the tab control.
// hwndDisplay - handle of the static control. 
// lParam - the lParam parameter of the WM_NOTIFY message.
//
BOOL OnNotify(HWND hwndTab, HWND hwndDisplay, LPARAM lParam)
{
    int iPage = TabCtrl_GetCurSel(hwndTab);
    LPNMUPDOWN lpnmud;
    TCHAR achTemp[256];
    TCHAR achTemp1[256];

    switch (((LPNMHDR)lParam)->code)
    {
    case TCN_SELCHANGING:
    {
        // Return FALSE to allow the selection to change.
        return FALSE;
    }

    case TCN_SELCHANGE:
    {
        LRESULT result;

        RECT rc;
        GetWindowRect(hwndTab, &rc);
        ScreenToClient(hWnd, (LPPOINT)&rc.left);
        ScreenToClient(hWnd, (LPPOINT)&rc.right);

        switch (iPage) {
        case 0: {
            LoadString(hInst, IDS_WATERLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            LoadString(hInst, IDS_MOUNTAINLVL, achTemp1, sizeof(achTemp1) / sizeof(achTemp1[0]));
            switchWindow(FinControlsWnd, ControlsWnd);
            adjustControls(achTemp, achTemp1, ImgGen->waterLvl, ImgGen->mountainLvl);
            SetWindowPos(hPaletteWindow, HWND_TOP, 965, 30, 200, 70, SWP_SHOWWINDOW);
            SetWindowPos(ControlsWnd, HWND_TOP, 510, 30, 450, rc.bottom > 670 ? rc.bottom - 35 : 700, SWP_SHOWWINDOW);
        }
              break;
        case 1: {
            LoadString(hInst, IDS_LOWTEMPLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            LoadString(hInst, IDS_HIGHTEMPLVL, achTemp1, sizeof(achTemp1) / sizeof(achTemp1[0]));
            switchWindow(FinControlsWnd, ControlsWnd);
            adjustControls(achTemp, achTemp1, ImgGen->coldLvl, ImgGen->warmLvl);
            SetWindowPos(hPaletteWindow, HWND_TOP, 965, 30, 200, 70, SWP_SHOWWINDOW);
            SetWindowPos(ControlsWnd, HWND_TOP, 510, 30, 450, rc.bottom > 670 ? rc.bottom - 35 : 700, SWP_SHOWWINDOW);
        }
              break;
        case 2: {
            LoadString(hInst, IDS_LOWHUMLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            LoadString(hInst, IDS_HIGHHUMLVL, achTemp1, sizeof(achTemp1) / sizeof(achTemp1[0]));
            switchWindow(FinControlsWnd, ControlsWnd);
            adjustControls(achTemp, achTemp1, ImgGen->dryLvl, ImgGen->humidLvl);
            SetWindowPos(hPaletteWindow, HWND_TOP, 965, 30, 200, 70, SWP_SHOWWINDOW);
            SetWindowPos(ControlsWnd, HWND_TOP, 510, 30, 450, rc.bottom > 670 ? rc.bottom - 35 : 700, SWP_SHOWWINDOW);
        }
              break;
        case 3: {
            switchWindow(ControlsWnd, FinControlsWnd);

            SendMessageW(hUpDownHei, UDM_SETPOS, 0, (LPARAM)ImgGen->waterLvl);
            SendMessageW(hUpDownMnt, UDM_SETPOS, 0, (LPARAM)ImgGen->mountainLvl);
            SendMessageW(hTrackHei, TBM_SETPOS, TRUE, (LPARAM)ImgGen->waterLvl);
            SendMessageW(hTrackMnt, TBM_SETPOS, TRUE, (LPARAM)ImgGen->mountainLvl);

            SendMessageW(hUpDownTempL, UDM_SETPOS, 0, (LPARAM)ImgGen->coldLvl);
            SendMessageW(hUpDownTempH, UDM_SETPOS, 0, (LPARAM)ImgGen->warmLvl);
            SendMessageW(hTrackTempL, TBM_SETPOS, TRUE, (LPARAM)ImgGen->coldLvl);
            SendMessageW(hTrackTempH, TBM_SETPOS, TRUE, (LPARAM)ImgGen->warmLvl);

            SendMessageW(hUpDownHumL, UDM_SETPOS, 0, (LPARAM)ImgGen->dryLvl);
            SendMessageW(hUpDownHumH, UDM_SETPOS, 0, (LPARAM)ImgGen->humidLvl);
            SendMessageW(hTrackHumL, TBM_SETPOS, TRUE, (LPARAM)ImgGen->dryLvl);
            SendMessageW(hTrackHumH, TBM_SETPOS, TRUE, (LPARAM)ImgGen->humidLvl);

            SetWindowPos(hPaletteWindow, HWND_TOP, 965, 30, 200, 370, SWP_SHOWWINDOW);
            SetWindowPos(FinControlsWnd, HWND_TOP, 510, 30, 450, rc.bottom > 670 ? rc.bottom - 35 : 700, SWP_SHOWWINDOW);
            break;
        }
        default: break;
        }
        InvalidateRect(hPaletteWindow, NULL, false);
        break;
    }
    case UDN_DELTAPOS:
    {
        lpnmud = (NMUPDOWN*)lParam;

        unsigned int value = lpnmud->iPos + lpnmud->iDelta;

        if (value < UD_MIN_POS) {
            value = UD_MIN_POS;
        }

        if (value > UD_MAX_POS) {
            value = UD_MAX_POS;
        }

        switch (iPage) {
        case 0: {
            if (value >= ImgGen->mountainLvl && lpnmud->hdr.idFrom == IDM_UPDOWN1) {
                value = ImgGen->mountainLvl - 1;
                SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)ImgGen->mountainLvl - 2);
            }
            if (value <= ImgGen->waterLvl && lpnmud->hdr.idFrom == IDM_UPDOWN2) {
                value = ImgGen->waterLvl + 1;
                SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)ImgGen->waterLvl + 2);
            }

            switch (lpnmud->hdr.idFrom) {
            case IDM_UPDOWN1:
                SendMessageW(hTrack1, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->waterLvl = value;
                ImgGen->changeHeightMap(hdcMemHei);
                break;
            case IDM_UPDOWN2:
                SendMessageW(hTrack2, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->mountainLvl = value;
                ImgGen->changeHeightMap(hdcMemHei);
                break;
            default:
                break;
            }
        }
              break;
        case 1: {
            if (value >= ImgGen->warmLvl && lpnmud->hdr.idFrom == IDM_UPDOWN1) {
                value = ImgGen->warmLvl - 1;
                SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)ImgGen->warmLvl - 2);
            }
            if (value <= ImgGen->coldLvl && lpnmud->hdr.idFrom == IDM_UPDOWN2) {
                value = ImgGen->coldLvl + 1;
                SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)ImgGen->coldLvl + 2);
            }

            switch (lpnmud->hdr.idFrom) {
            case IDM_UPDOWN1:
                SendMessageW(hTrack1, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->coldLvl = value;
                ImgGen->changeTempMap(hdcMemTemp);
                break;
            case IDM_UPDOWN2:
                SendMessageW(hTrack2, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->warmLvl = value;
                ImgGen->changeTempMap(hdcMemTemp);
                break;
            default:
                break;
            }
        }
              break;
        case 2: {
            if (value >= ImgGen->humidLvl && lpnmud->hdr.idFrom == IDM_UPDOWN1) {
                value = ImgGen->humidLvl - 1;
                SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)ImgGen->humidLvl - 2);
            }
            if (value <= ImgGen->dryLvl && lpnmud->hdr.idFrom == IDM_UPDOWN2) {
                value = ImgGen->dryLvl + 1;
                SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)ImgGen->dryLvl + 2);
            }

            switch (lpnmud->hdr.idFrom) {
            case IDM_UPDOWN1:
                SendMessageW(hTrack1, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->dryLvl = value;
                ImgGen->changeHumidMap(hdcMemHumid);

                break;
            case IDM_UPDOWN2:
                SendMessageW(hTrack2, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->humidLvl = value;
                ImgGen->changeHumidMap(hdcMemHumid);
                break;
            default:
                break;
            }
        }
              break;
        case 3: {
            if (value >= ImgGen->mountainLvl && lpnmud->hdr.idFrom == IDM_UPDOWNHEI) {
                value = ImgGen->mountainLvl - 1;
                SendMessageW(hUpDownHei, UDM_SETPOS, 0, (LPARAM)ImgGen->mountainLvl - 2);
            }
            if (value <= ImgGen->waterLvl && lpnmud->hdr.idFrom == IDM_UPDOWNMNT) {
                value = ImgGen->waterLvl + 1;
                SendMessageW(hUpDownMnt, UDM_SETPOS, 0, (LPARAM)ImgGen->waterLvl + 2);
            }

            switch (lpnmud->hdr.idFrom) {
            case IDM_UPDOWNHEI:
                SendMessageW(hTrackHei, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->waterLvl = value;
                ImgGen->changeHeightMap(hdcMemHei);
                break;
            case IDM_UPDOWNMNT:
                SendMessageW(hTrackMnt, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->mountainLvl = value;
                ImgGen->changeHeightMap(hdcMemHei);
                break;
            default:
                break;
            }

            if (value >= ImgGen->warmLvl && lpnmud->hdr.idFrom == IDM_UPDOWNTEMPL) {
                value = ImgGen->warmLvl - 1;
                SendMessageW(hUpDownTempL, UDM_SETPOS, 0, (LPARAM)ImgGen->warmLvl - 2);
            }
            if (value <= ImgGen->coldLvl && lpnmud->hdr.idFrom == IDM_UPDOWNTEMPH) {
                value = ImgGen->coldLvl + 1;
                SendMessageW(hUpDownTempH, UDM_SETPOS, 0, (LPARAM)ImgGen->coldLvl + 2);
            }

            switch (lpnmud->hdr.idFrom) {
            case IDM_UPDOWNTEMPL:
                SendMessageW(hTrackTempL, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->coldLvl = value;
                ImgGen->changeTempMap(hdcMemTemp);
                break;
            case IDM_UPDOWNTEMPH:
                SendMessageW(hTrackTempH, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->warmLvl = value;
                ImgGen->changeTempMap(hdcMemTemp);
                break;
            default:
                break;
            }
            if (value >= ImgGen->humidLvl && lpnmud->hdr.idFrom == IDM_UPDOWNHUMIDL) {
                value = ImgGen->humidLvl - 1;
                SendMessageW(hUpDownHumL, UDM_SETPOS, 0, (LPARAM)ImgGen->humidLvl - 2);
            }
            if (value <= ImgGen->dryLvl && lpnmud->hdr.idFrom == IDM_UPDOWNHUMIDH) {
                value = ImgGen->dryLvl + 1;
                SendMessageW(hUpDownHumH, UDM_SETPOS, 0, (LPARAM)ImgGen->dryLvl + 2);
            }

            switch (lpnmud->hdr.idFrom) {
            case IDM_UPDOWNHUMIDL:
                SendMessageW(hTrackHumL, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->dryLvl = value;
                ImgGen->changeHumidMap(hdcMemHumid);
                break;
            case IDM_UPDOWNHUMIDH:
                SendMessageW(hTrackHumH, TBM_SETPOS, TRUE, (LPARAM)value);
                ImgGen->humidLvl = value;
                ImgGen->changeHumidMap(hdcMemHumid);
                break;
            default:
                break;
            }
        }
              break;
        default: break;
        }
        ImgGen->makeFinalMap(hdcMemFin, beachChecked, lavaChecked);
        InvalidateRect(hwndDisplay, NULL, false);
        UpdateWindow(hwndDisplay);
        break;
    }

    return TRUE;
    }
}

DWORD WINAPI GenerateMapThread(LPVOID lpParam) {

    wchar_t* seed = (wchar_t*)lpParam;

    wchar_t seedHei[7] = {};
    wchar_t seedTemp[7] = {};
    wchar_t seedHum[7] = {};
    wchar_t seedDetail[7] = {};

    for (int i = 0; i < 6; i++) {
        seedHei[i] = seed[i];
        seedTemp[i] = seed[i + 6];
        seedHum[i] = seed[i + 12];
        seedDetail[i] = seed[i + 18];
    }

    hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL,
        WS_CHILD | WS_VISIBLE, 125,
        225,
        250, 50,
        hwndDisplay, (HMENU)0, hInst, NULL);
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 5));
    SendMessage(hProgressBar, PBM_SETSTEP, (WPARAM)1, 0);

    ImgGen->createHeightMap(hdcMemHei, _wtoi(seedHei));
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);
    ImgGen->createTempMap(0, 0, hdcMemTemp, _wtoi(seedTemp));
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);
    ImgGen->createHumidMap(0, 0, hdcMemHumid, _wtoi(seedHum));
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);
    ImgGen->createDetailMap(_wtoi(seedDetail), 5);
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);
    ImgGen->makeFinalMap(hdcMemFin, beachChecked, lavaChecked);
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);

    PostMessage(hWnd, WM_APP + 1, 0, 0);

    delete[] seed;

    return 0;
}

DWORD WINAPI GenerateOpenMapThread(LPVOID lpParam) {
    SendMessage(hSeedText, WM_SETTEXT, NULL, (LPARAM)project.seed);

    wchar_t seedHei[7] = {};
    wchar_t seedTemp[7] = {};
    wchar_t seedHum[7] = {};
    wchar_t seedDetail[7] = {};

    for (int i = 0; i < 6; i++) {
        seedHei[i] = project.seed[i];
        seedTemp[i] = project.seed[i + 6];
        seedHum[i] = project.seed[i + 12];
        seedDetail[i] = project.seed[i + 18];
    }

    hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL,
        WS_CHILD | WS_VISIBLE, 125,
        225,
        250, 50,
        hwndDisplay, (HMENU)0, hInst, NULL);
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 6));
    SendMessage(hProgressBar, PBM_SETSTEP, (WPARAM)1, 0);


    if (ImgGen)
        delete ImgGen;
    ImgGen = new generateImage(project.height, project.width);

    ImgGen->createHeightMap(hdcMemHei, _wtoi(seedHei));
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);
    ImgGen->createTempMap(0, 0, hdcMemTemp, _wtoi(seedTemp));
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);
    ImgGen->createHumidMap(0, 0, hdcMemHumid, _wtoi(seedHum));
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);
    ImgGen->createDetailMap(_wtoi(seedDetail), 5);
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);
    ImgGen->makeFinalMap(hdcMemFin, beachChecked, lavaChecked);
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);
    ImgGen->waterLvl = project.waterLvl;
    ImgGen->mountainLvl = project.mountainLvl;
    ImgGen->changeHeightMap(hdcMemHei);
    ImgGen->coldLvl = project.coldLvl;
    ImgGen->warmLvl = project.warmLvl;
    ImgGen->changeTempMap(hdcMemTemp);
    ImgGen->dryLvl = project.dryLvl;
    ImgGen->humidLvl = project.humidLvl;
    ImgGen->changeHumidMap(hdcMemHumid);
    beachChecked = project.genBeach;
    lavaChecked = project.genLava;
    ImgGen->makeFinalMap(hdcMemFin, beachChecked, lavaChecked);
    SendMessage(hProgressBar, PBM_STEPIT, 0, 0);

    int iPage = TabCtrl_GetCurSel(hwndTab);
    switch (iPage) {
    case 0:
    {
        SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)ImgGen->waterLvl);
        SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)ImgGen->mountainLvl);
        SendMessageW(hTrack1, TBM_SETPOS, 0, ImgGen->waterLvl);
        SendMessageW(hTrack2, TBM_SETPOS, 0, ImgGen->mountainLvl);
        break;
    }
    case 1:
    {
        SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)ImgGen->coldLvl);
        SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)ImgGen->warmLvl);
        SendMessageW(hTrack1, TBM_SETPOS, 0, ImgGen->coldLvl);
        SendMessageW(hTrack2, TBM_SETPOS, 0, ImgGen->warmLvl);
        break;
    }
    case 2:
    {
        SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)ImgGen->dryLvl);
        SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)ImgGen->humidLvl);
        SendMessageW(hTrack1, TBM_SETPOS, 0, ImgGen->dryLvl);
        SendMessageW(hTrack2, TBM_SETPOS, 0, ImgGen->humidLvl);
        break;
    }
    case 3:
    {
        SendMessageW(hUpDownHei, UDM_SETPOS, 0, (LPARAM)ImgGen->waterLvl);
        SendMessageW(hUpDownMnt, UDM_SETPOS, 0, (LPARAM)ImgGen->mountainLvl);
        SendMessageW(hTrackHei, TBM_SETPOS, 0, ImgGen->waterLvl);
        SendMessageW(hTrackMnt, TBM_SETPOS, 0, ImgGen->mountainLvl);
        SendMessageW(hUpDownTempL, UDM_SETPOS, 0, (LPARAM)ImgGen->coldLvl);
        SendMessageW(hUpDownTempH, UDM_SETPOS, 0, (LPARAM)ImgGen->warmLvl);
        SendMessageW(hTrackTempL, TBM_SETPOS, 0, ImgGen->coldLvl);
        SendMessageW(hTrackTempH, TBM_SETPOS, 0, ImgGen->warmLvl);
        SendMessageW(hUpDownHumL, UDM_SETPOS, 0, (LPARAM)ImgGen->dryLvl);
        SendMessageW(hUpDownHumH, UDM_SETPOS, 0, (LPARAM)ImgGen->humidLvl);
        SendMessageW(hTrackHumL, TBM_SETPOS, 0, ImgGen->dryLvl);
        SendMessageW(hTrackHumH, TBM_SETPOS, 0, ImgGen->humidLvl);
        break;
    }
    default:
        break;
    }
    CheckDlgButton(FinControlsWnd, ID_BEACHCHECKBOX, beachChecked);
    CheckDlgButton(FinControlsWnd, ID_LAVACHECKBOX, lavaChecked);

    PostMessage(hWnd, WM_APP + 1, 0, 0);

    return 0;
}

void switchWindow(HWND toHide, HWND toShow) {
    if (IsWindowVisible(toHide)) ShowWindow(toHide, SW_HIDE);
    if (!IsWindowVisible(toShow)) {
        ShowWindow(toShow, SW_SHOW);
        UpdateWindow(GetParent(toShow));
    }
}

void adjustControls(const wchar_t text1[], const wchar_t text2[], int valueLow, int valueHigh) {
    SendMessageW(hControlsText1, WM_SETTEXT, 0, (LPARAM)text1);
    SendMessageW(hControlsText2, WM_SETTEXT, 0, (LPARAM)text2);
    SendMessageW(hUpDown1, UDM_SETPOS, 0, (LPARAM)valueLow);
    SendMessageW(hUpDown2, UDM_SETPOS, 0, (LPARAM)valueHigh);
    SendMessageW(hTrack1, TBM_SETPOS, TRUE, (LPARAM)valueLow);
    SendMessageW(hTrack2, TBM_SETPOS, TRUE, (LPARAM)valueHigh);
}

void EnableControls(bool state) {
    HWND controls[] = { 
        hUpDown1, hEdit1, hTrack1,
        hUpDown2, hEdit2, hTrack2,
        hUpDownHei, hEditHei, hTrackHei,
        hUpDownMnt, hEditMnt, hTrackMnt,
        hUpDownTempL, hEditTempL, hTrackTempL,
        hUpDownTempH, hEditTempH, hTrackTempH,
        hUpDownHumL, hEditHumL, hTrackHumL,
        hUpDownHumH, hEditHumH, hTrackHumH,
        hGenButton, hRndGenButton, hExportButton,
        hSeedText, hBeachCheckbox, hLavaCheckbox };
    for (HWND handle : controls) {
        EnableWindow(handle, state);
    }
}

bool SaveProject(const wchar_t* filename) {
    TCHAR achTemp[256];
    HANDLE hFile = CreateFileW(
        filename, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        LoadString(hInst, IDS_FILECREATEERR, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        MessageBox(hWnd, achTemp, L"Error", MB_ICONERROR);
        return false;
    }

    wchar_t* pn = project.name;

    DWORD bytesWritten = 0;
    BOOL result = WriteFile(
        hFile, &project, sizeof(project),
        &bytesWritten, NULL
    );
    
    CloseHandle(hFile);

    return result && bytesWritten == sizeof(project);
}

bool LoadProject(const wchar_t* filename) {
    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD read = 0;
    BOOL result = ReadFile(hFile, &project, sizeof(projectValues), &read, NULL);
    CloseHandle(hFile);

    return result && read == sizeof(projectValues);
}

void writeRecentFile(wchar_t* newfile) {
    //if file is already most recent do nothing
    if (!StrCmpW(newfile, recentFiles[0])) return;

    wchar_t iniPath[MAX_PATH];
    GetModuleFileNameW(NULL, iniPath, MAX_PATH);
    PathRemoveFileSpecW(iniPath);                   // Remove exe name
    PathAppendW(iniPath, L"MapImageGenerator.ini");  // Add ini name

    //if file is in the list move it to the top
    int index;
    for (int i = 0; i < recentFileCount; i++) {
        if (!StrCmpW(newfile, recentFiles[i])) {
            for (int j = i; j > 0; j--) {
                wcscpy(recentFiles[j], recentFiles[j - 1]);
            }
            wcscpy(recentFiles[0], newfile);
            for (int i = 0; i < recentFileCount; i++) {
                wchar_t key[16] = L"File";
                key[4] = L'0' + i;

                WritePrivateProfileStringW(L"RecentFiles", key, recentFiles[i], iniPath);
            }
            UpdateRecentFilesMenu();
            return;
        }
    }
    
    //otherwise move old files down and put newfile on top
    for (int i = recentFileCount; i > 0; i--) {
        wcscpy(recentFiles[i], recentFiles[i - 1]);
    }
    wcscpy(recentFiles[0], newfile);

    if (recentFileCount < 5) recentFileCount++;

    for (int i = 0; i < recentFileCount; i++) {
        wchar_t key[16] = L"File";
        key[4] = L'0' + i;

        WritePrivateProfileStringW(L"RecentFiles", key, recentFiles[i], iniPath);
    }

    wchar_t countStr[2] = L"";
    _itow(recentFileCount, countStr, 10);
    WritePrivateProfileStringW(L"RecentFiles", L"Count", countStr, iniPath);

    UpdateRecentFilesMenu();
}

bool readRecentFiles() {
    wchar_t buffer[MAX_PATH];
    wchar_t iniPath[MAX_PATH];
    GetModuleFileNameW(NULL, iniPath, MAX_PATH);
    PathRemoveFileSpecW(iniPath);
    PathAppendW(iniPath, L"MapImageGenerator.ini");
    wchar_t countStr[2] = L"";

    GetPrivateProfileStringW(L"RecentFiles", L"Count", L"0", countStr, sizeof(countStr), iniPath);
    recentFileCount = _wtoi(countStr);

    if (recentFileCount > MAX_RECENT_FILES) recentFileCount = MAX_RECENT_FILES;

    for (int i = 0; i < recentFileCount; i++) {
        wchar_t key[16] = L"File";
        key[4] = L'0' + i;

        GetPrivateProfileStringW(L"RecentFiles", key, L"", buffer, sizeof(buffer), iniPath);
        wcscpy(recentFiles[i], buffer);
    }

    UpdateRecentFilesMenu();

    return recentFileCount > 0 ? true : false;
}

void removeRecentFile(int index) {
    wchar_t iniPath[MAX_PATH];
    GetModuleFileNameW(NULL, iniPath, MAX_PATH);
    PathRemoveFileSpecW(iniPath);
    PathAppendW(iniPath, L"MapImageGenerator.ini");

    for (int i = index; i < recentFileCount; i++) {
        wcscpy(recentFiles[i], recentFiles[i + 1]);
    }
    
    for (int i = 0; i < recentFileCount; i++) {
        wchar_t key[16] = L"File";
        key[4] = L'0' + i;

        WritePrivateProfileStringW(L"RecentFiles", key, i == recentFileCount - 1 ? NULL : recentFiles[i], iniPath);
    }

    recentFileCount--;
    wchar_t countStr[2] = L"";
    _itow(recentFileCount, countStr, 10);
    WritePrivateProfileStringW(L"RecentFiles", L"Count", countStr, iniPath);

    UpdateRecentFilesMenu();
}

void UpdateRecentFilesMenu()
{
    HMENU hMenu = GetMenu(hWnd);
    HMENU hFileMenu = GetSubMenu(hMenu, 0);
    MENUITEMINFOW mii = { 0 };

    DeleteMenu(hFileMenu, IDM_RECENT_FILE0, MF_BYCOMMAND);

    int itemcount = GetMenuItemCount(hFileMenu);
    if (itemcount > 9) {
        int i = 7;
        while (i < itemcount - 2) {
            DeleteMenu(hFileMenu, 7, MF_BYPOSITION);
            i++;
        }
    }

    if (recentFileCount == 0) {
        WCHAR label[MAX_PATH + 10] = L"";
        LoadString(hInst, IDS_NORECENTFILES, label, sizeof(label) / sizeof(label[0]));

        mii.cbSize = sizeof(MENUITEMINFOW);
        mii.fMask = MIIM_STRING | MIIM_STATE;
        mii.fType = MFT_STRING;
        mii.fState = MFS_DISABLED;
        mii.dwTypeData = label;
        InsertMenuItemW(hFileMenu, 7, TRUE, &mii);
        return;
    }

    for (int i = recentFileCount; i > 0; i--) {
        WCHAR label[MAX_PATH + 10];

        StringCchPrintfW(label, ARRAYSIZE(label), L"&%d %s", i, recentFiles[i - 1]);

        mii.cbSize = sizeof(MENUITEMINFOW);
        mii.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
        mii.fType = MFT_STRING;
        mii.fState = MFS_ENABLED;
        mii.wID = IDM_RECENT_FILE0 + i;
        mii.dwTypeData = label;

        InsertMenuItemW(hFileMenu, 7, TRUE, &mii);
    }
}

static void CustomHandleMouseWheel(HWND hwnd, int iDelta, BOOL isVertical)
{
    SCROLLINFO si;
    int nOldPos;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd, (isVertical ? SB_VERT : SB_HORZ), &si);

    nOldPos = si.nPos;
    si.nPos = isVertical ? yScrollPos - iDelta : xScrollPos - iDelta;

    SetScrollInfo(hwnd, (isVertical ? SB_VERT : SB_HORZ), &si, TRUE);
    GetScrollInfo(hwnd, (isVertical ? SB_VERT : SB_HORZ), &si);
    isVertical ? yScrollPos = si.nPos : xScrollPos = si.nPos;
    ScrollWindow(hwnd, (isVertical ? 0 : nOldPos - si.nPos), (isVertical ? nOldPos - si.nPos : 0), NULL, NULL);
    UpdateWindow(hwnd);
}

void setLanguage(LCID lang) {
    TCHAR achTemp[256];
    int iPage = TabCtrl_GetCurSel(hwndTab);
    wchar_t iniPath[MAX_PATH];
    GetModuleFileNameW(NULL, iniPath, MAX_PATH);
    PathRemoveFileSpecW(iniPath);
    PathAppendW(iniPath, L"MapImageGenerator.ini");
    wchar_t langTxt[64] = L"";
    LCIDToLocaleName(lang, langTxt, 64, 0);
    
    if (LCIDToLocaleName(MAKELCID(lang, SORT_DEFAULT), langTxt, 64, 0) != 0) {
        WritePrivateProfileStringW(L"Settings", L"Language", langTxt, iniPath);
        SetThreadUILanguage(lang);

        HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDC_MAPIMAGEGENERATOR));
        SetMenu(hWnd, hMenu);
        UpdateRecentFilesMenu();

        LoadString(hInst, IDS_GENBTN, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SetWindowText(hGenButton, achTemp);
        LoadString(hInst, IDS_HRNDGENBTN, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SetWindowText(hRndGenButton, achTemp);
        LoadString(hInst, IDS_EXPORTBTN, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SetWindowText(hExportButton, achTemp);

        switch (iPage) {
        case 0:
        {
            LoadString(hInst, IDS_WATERLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            SendMessageW(hControlsText1, WM_SETTEXT, 0, (LPARAM)achTemp);
            LoadString(hInst, IDS_MOUNTAINLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            SendMessageW(hControlsText2, WM_SETTEXT, 0, (LPARAM)achTemp);
            break;
        }
        case 1:
        {
            LoadString(hInst, IDS_LOWTEMPLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            SendMessageW(hControlsText1, WM_SETTEXT, 0, (LPARAM)achTemp);
            LoadString(hInst, IDS_HIGHTEMPLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            SendMessageW(hControlsText2, WM_SETTEXT, 0, (LPARAM)achTemp);
            break;
        }
        case 2:
        {
            LoadString(hInst, IDS_LOWHUMLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            SendMessageW(hControlsText1, WM_SETTEXT, 0, (LPARAM)achTemp);
            LoadString(hInst, IDS_HIGHHUMLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            SendMessageW(hControlsText2, WM_SETTEXT, 0, (LPARAM)achTemp);
            break;
        }
        default:
            break;
        }

        LoadString(hInst, IDS_WATERLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SendMessageW(hControlsText3, WM_SETTEXT, 0, (LPARAM)achTemp);
        LoadString(hInst, IDS_MOUNTAINLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SendMessageW(hControlsText4, WM_SETTEXT, 0, (LPARAM)achTemp);
        LoadString(hInst, IDS_LOWTEMPLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SendMessageW(hControlsText5, WM_SETTEXT, 0, (LPARAM)achTemp);
        LoadString(hInst, IDS_HIGHTEMPLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SendMessageW(hControlsText6, WM_SETTEXT, 0, (LPARAM)achTemp);
        LoadString(hInst, IDS_LOWHUMLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SendMessageW(hControlsText7, WM_SETTEXT, 0, (LPARAM)achTemp);
        LoadString(hInst, IDS_HIGHHUMLVL, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SendMessageW(hControlsText8, WM_SETTEXT, 0, (LPARAM)achTemp);
        LoadString(hInst, IDS_GENBEACH, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SendMessageW(hControlsText9, WM_SETTEXT, 0, (LPARAM)achTemp);
        LoadString(hInst, IDS_GENLAVA, achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
        SendMessageW(hControlsText10, WM_SETTEXT, 0, (LPARAM)achTemp);

        TCITEM tie;
        tie.mask = TCIF_TEXT | TCIF_IMAGE;
        tie.iImage = -1;
        tie.pszText = achTemp;
        for (int i = 0; i < 4; i++)
        {
            LoadString(hInst, IDS_HEI_MAP + i,
                achTemp, sizeof(achTemp) / sizeof(achTemp[0]));
            TabCtrl_SetItem(hwndTab, i, &tie);
        }

        InvalidateRect(hPaletteWindow, NULL, true);
    }
}

void OnProjectSave()
{
    HRESULT hr;
    CComPtr<IFileSaveDialog> pDlg;
    COMDLG_FILTERSPEC aFileTypes[] = {
        { L"Dat files", L"*.dat" },
        { L"All files", L"*.*" }
    };

    hr = pDlg.CoCreateInstance(__uuidof(FileSaveDialog));

    if (FAILED(hr))
        return;

    pDlg->SetFileTypes(_countof(aFileTypes), aFileTypes);
    pDlg->SetTitle(L"Save as");
    pDlg->SetOkButtonLabel(L"Save");
    pDlg->SetFileName(project.name);
    pDlg->SetDefaultExtension(L"dat");

    hr = pDlg->Show(hWnd);

    if (SUCCEEDED(hr))
    {
        CComPtr<IShellItem> pItem;

        hr = pDlg->GetResult(&pItem);

        if (SUCCEEDED(hr))
        {
            LPOLESTR pwsz = NULL;

            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);

            if (SUCCEEDED(hr))
            {
                SaveProject(pwsz);
                wcscpy(szFileName, pwsz);
                writeRecentFile(szFileName);
                CoTaskMemFree(pwsz);
            }
        }
    }
}