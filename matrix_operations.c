
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX 10

#define ID_AR 101
#define ID_AC 102
#define ID_BR 103
#define ID_BC 104
#define ID_A 105
#define ID_B 106
#define ID_ADD 201
#define ID_MULTIPLY 202
#define ID_TRANS_A 203
#define ID_TRANS_B 204
#define ID_CLEAR 205
#define ID_OUTPUT 301

HWND mainWindow;
HWND editAR, editAC, editBR, editBC;
HWND editA, editB, editOutput;

HFONT appFont;
HBRUSH backgroundBrush, panelBrush, inputBrush;

COLORREF backgroundColor = RGB(23, 20, 42);
COLORREF panelColor = RGB(39, 32, 64);
COLORREF inputColor = RGB(27, 23, 46);
COLORREF accentColor = RGB(139, 111, 230);
COLORREF textColor = RGB(245, 242, 255);
COLORREF mutedColor = RGB(181, 170, 210);

HWND makeControl(const char *className, const char *text,
                 DWORD style, int x, int y, int w, int h, int id)
{
    HWND control = CreateWindowExA(
        0, className, text,
        WS_CHILD | WS_VISIBLE | style,
        x, y, w, h, mainWindow,
        (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);

    if (control && appFont)
        SendMessageA(control, WM_SETFONT, (WPARAM)appFont, TRUE);

    return control;
}

void makeLabel(const char *text, int x, int y, int w, int h)
{
    makeControl("STATIC", text, SS_LEFT, x, y, w, h, 0);
}

void makeButton(const char *text, int id, int x, int y, int w, int h)
{
    makeControl("BUTTON", text, BS_OWNERDRAW, x, y, w, h, id);
}

void showOutput(const char *text)
{
    SetWindowTextA(editOutput, text);
}

int getDimension(HWND control, int *value)
{
    char text[32], *end;
    long number;

    GetWindowTextA(control, text, sizeof(text));

    if (text[0] == '\0')
        return 0;

    number = strtol(text, &end, 10);

    while (isspace((unsigned char)*end))
        end++;

    if (*end != '\0' || number < 1 || number > MAX)
        return 0;

    *value = (int)number;
    return 1;
}

int readMatrix(HWND control, int matrix[MAX][MAX],
               int rows, int cols)
{
    char text[4096], *p, *end;
    long value;
    int i, j;

    GetWindowTextA(control, text, sizeof(text));
    p = text;

    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < cols; j++)
        {
            while (isspace((unsigned char)*p))
                p++;

            if (*p == '\0')
                return 0;

            value = strtol(p, &end, 10);

            if (end == p || value < INT_MIN || value > INT_MAX)
                return 0;

            matrix[i][j] = (int)value;
            p = end;
        }
    }

    while (isspace((unsigned char)*p))
        p++;

    return *p == '\0';
}

int loadA(int a[MAX][MAX], int *rows, int *cols)
{
    if (!getDimension(editAR, rows) ||
        !getDimension(editAC, cols))
    {
        showOutput("Invalid dimensions for Matrix A.\r\n"
                   "Enter rows and columns from 1 to 10.");
        return 0;
    }

    if (!readMatrix(editA, a, *rows, *cols))
    {
        showOutput("Invalid Matrix A values.\r\n"
                   "Enter exactly the required number of integers.");
        return 0;
    }

    return 1;
}

int loadB(int b[MAX][MAX], int *rows, int *cols)
{
    if (!getDimension(editBR, rows) ||
        !getDimension(editBC, cols))
    {
        showOutput("Invalid dimensions for Matrix B.\r\n"
                   "Enter rows and columns from 1 to 10.");
        return 0;
    }

    if (!readMatrix(editB, b, *rows, *cols))
    {
        showOutput("Invalid Matrix B values.\r\n"
                   "Enter exactly the required number of integers.");
        return 0;
    }

    return 1;
}

void addMatrices(int a[MAX][MAX], int b[MAX][MAX],
                 int result[MAX][MAX], int rows, int cols)
{
    int i, j;

    for (i = 0; i < rows; i++)
        for (j = 0; j < cols; j++)
            result[i][j] = a[i][j] + b[i][j];
}

void multiplyMatrices(int a[MAX][MAX], int b[MAX][MAX],
                      int result[MAX][MAX],
                      int rowsA, int colsA, int colsB)
{
    int i, j, k;

    for (i = 0; i < rowsA; i++)
    {
        for (j = 0; j < colsB; j++)
        {
            result[i][j] = 0;

            for (k = 0; k < colsA; k++)
                result[i][j] += a[i][k] * b[k][j];
        }
    }
}

void transposeMatrix(int matrix[MAX][MAX],
                     int result[MAX][MAX],
                     int rows, int cols)
{
    int i, j;

    for (i = 0; i < rows; i++)
        for (j = 0; j < cols; j++)
            result[j][i] = matrix[i][j];
}

void displayMatrix(int matrix[MAX][MAX],
                   int rows, int cols, const char *title)
{
    char output[8192] = "";
    char line[128];
    int i, j;

    snprintf(output, sizeof(output), "%s\r\n\r\n", title);

    for (i = 0; i < rows; i++)
    {
        line[0] = '\0';

        for (j = 0; j < cols; j++)
        {
            char value[32];
            snprintf(value, sizeof(value), "%8d", matrix[i][j]);

            if (strlen(line) + strlen(value) < sizeof(line) - 1)
                strcat(line, value);
        }

        if (strlen(output) + strlen(line) + 3 < sizeof(output))
        {
            strcat(output, line);
            strcat(output, "\r\n");
        }
    }

    snprintf(output + strlen(output),
             sizeof(output) - strlen(output),
             "\r\nMatrix size: %d x %d", rows, cols);

    showOutput(output);
}

void createInterface(void)
{
    makeLabel("PREMIUM GLASS", 35, 18, 250, 25);
    makeLabel("Matrix Operations", 35, 44, 500, 38);
    makeLabel("Addition  |  Multiplication  |  Transpose",
              37, 78, 600, 22);

    makeLabel("MATRIX A", 48, 119, 200, 25);
    makeLabel("ROWS", 53, 151, 65, 20);
    makeLabel("COLUMNS", 157, 151, 80, 20);

    editAR = makeControl("EDIT", "2",
        WS_BORDER | ES_AUTOHSCROLL, 52, 174, 80, 30, ID_AR);
    editAC = makeControl("EDIT", "2",
        WS_BORDER | ES_AUTOHSCROLL, 157, 174, 80, 30, ID_AC);

    makeLabel("Enter values separated by spaces or new lines",
              52, 211, 405, 22);

    editA = makeControl("EDIT", "1 2\r\n3 4",
        WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
        WS_VSCROLL | ES_WANTRETURN,
        52, 237, 405, 128, ID_A);

    makeLabel("MATRIX B", 513, 119, 200, 25);
    makeLabel("ROWS", 518, 151, 65, 20);
    makeLabel("COLUMNS", 622, 151, 80, 20);

    editBR = makeControl("EDIT", "2",
        WS_BORDER | ES_AUTOHSCROLL, 517, 174, 80, 30, ID_BR);
    editBC = makeControl("EDIT", "2",
        WS_BORDER | ES_AUTOHSCROLL, 622, 174, 80, 30, ID_BC);

    makeLabel("Enter values separated by spaces or new lines",
              517, 211, 405, 22);

    editB = makeControl("EDIT", "5 6\r\n7 8",
        WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
        WS_VSCROLL | ES_WANTRETURN,
        517, 237, 405, 128, ID_B);

    makeLabel("CHOOSE AN OPERATION", 45, 400, 400, 24);

    makeButton("ADD", ID_ADD, 45, 429, 155, 47);
    makeButton("MULTIPLY", ID_MULTIPLY, 211, 429, 155, 47);
    makeButton("TRANSPOSE A", ID_TRANS_A, 377, 429, 155, 47);
    makeButton("TRANSPOSE B", ID_TRANS_B, 543, 429, 155, 47);
    makeButton("CLEAR", ID_CLEAR, 709, 429, 205, 47);

    makeLabel("RESULT", 45, 497, 250, 25);

    editOutput = makeControl("EDIT",
        "Your result will appear here.",
        WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
        WS_VSCROLL | ES_READONLY,
        45, 526, 869, 145, ID_OUTPUT);
}

void drawRoundedPanel(HDC hdc, int x, int y, int w, int h,
                      COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);

    RoundRect(hdc, x, y, x + w, y + h, 22, 22);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void drawButton(const DRAWITEMSTRUCT *dis)
{
    RECT rect = dis->rcItem;
    COLORREF fill;
    HBRUSH brush;
    HPEN pen;
    HBRUSH oldBrush;
    HPEN oldPen;
    char text[80];

    GetWindowTextA(dis->hwndItem, text, sizeof(text));

    if (dis->itemState & ODS_SELECTED)
        fill = RGB(101, 79, 175);
    else if ((int)dis->CtlID == ID_ADD ||
             (int)dis->CtlID == ID_MULTIPLY)
        fill = accentColor;
    else
        fill = RGB(65, 53, 99);

    brush = CreateSolidBrush(fill);
    pen = CreatePen(PS_SOLID, 1, fill);

    oldBrush = (HBRUSH)SelectObject(dis->hDC, brush);
    oldPen = (HPEN)SelectObject(dis->hDC, pen);

    RoundRect(dis->hDC, rect.left, rect.top,
              rect.right, rect.bottom, 14, 14);

    SelectObject(dis->hDC, oldBrush);
    SelectObject(dis->hDC, oldPen);

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, textColor);
    SelectObject(dis->hDC, appFont);

    DrawTextA(dis->hDC, text, -1, &rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DeleteObject(brush);
    DeleteObject(pen);
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            mainWindow = hwnd;
            createInterface();
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;

            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, backgroundBrush);

            drawRoundedPanel(hdc, 25, 105, 450, 275, panelColor);
            drawRoundedPanel(hdc, 495, 105, 450, 275, panelColor);
            drawRoundedPanel(hdc, 25, 390, 920, 100, panelColor);
            drawRoundedPanel(hdc, 25, 487, 920, 195, panelColor);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DRAWITEM:
            drawButton((DRAWITEMSTRUCT *)lParam);
            return TRUE;

        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, mutedColor);
            SetBkColor(hdc, panelColor);
            return (LRESULT)panelBrush;
        }

        case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, textColor);
            SetBkColor(hdc, inputColor);
            return (LRESULT)inputBrush;
        }

        case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            int a[MAX][MAX] = {{0}};
            int b[MAX][MAX] = {{0}};
            int result[MAX][MAX] = {{0}};
            int rowsA, colsA, rowsB, colsB;

            switch (id)
            {
                case ID_ADD:
                    if (!loadA(a, &rowsA, &colsA) ||
                        !loadB(b, &rowsB, &colsB))
                        return 0;

                    if (rowsA != rowsB || colsA != colsB)
                    {
                        showOutput("Addition is not possible.\r\n"
                                   "Both matrices must have equal dimensions.");
                        return 0;
                    }

                    addMatrices(a, b, result, rowsA, colsA);
                    displayMatrix(result, rowsA, colsA, "MATRIX ADDITION");
                    return 0;

                case ID_MULTIPLY:
                    if (!loadA(a, &rowsA, &colsA) ||
                        !loadB(b, &rowsB, &colsB))
                        return 0;

                    if (colsA != rowsB)
                    {
                        showOutput("Multiplication is not possible.\r\n"
                                   "Columns of A must equal rows of B.");
                        return 0;
                    }

                    multiplyMatrices(a, b, result,
                                     rowsA, colsA, colsB);
                    displayMatrix(result, rowsA, colsB,
                                  "MATRIX MULTIPLICATION");
                    return 0;

                case ID_TRANS_A:
                    if (!loadA(a, &rowsA, &colsA))
                        return 0;

                    transposeMatrix(a, result, rowsA, colsA);
                    displayMatrix(result, colsA, rowsA,
                                  "TRANSPOSE OF MATRIX A");
                    return 0;

                case ID_TRANS_B:
                    if (!loadB(b, &rowsB, &colsB))
                        return 0;

                    transposeMatrix(b, result, rowsB, colsB);
                    displayMatrix(result, colsB, rowsB,
                                  "TRANSPOSE OF MATRIX B");
                    return 0;

                case ID_CLEAR:
                    SetWindowTextA(editAR, "2");
                    SetWindowTextA(editAC, "2");
                    SetWindowTextA(editBR, "2");
                    SetWindowTextA(editBC, "2");
                    SetWindowTextA(editA, "");
                    SetWindowTextA(editB, "");
                    showOutput("Enter matrix values and choose an operation.");
                    return 0;
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSA wc = {0};
    MSG msg;

    backgroundBrush = CreateSolidBrush(backgroundColor);
    panelBrush = CreateSolidBrush(panelColor);
    inputBrush = CreateSolidBrush(inputColor);

    appFont = CreateFontA(
        16, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");

    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = hInstance;
    wc.lpszClassName = "PremiumGlassMatrixWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = backgroundBrush;

    if (!RegisterClassA(&wc))
    {
        MessageBoxA(NULL, "Could not register the window.",
                    "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    mainWindow = CreateWindowExA(
        0, "PremiumGlassMatrixWindow",
        "Premium Glass - Matrix Operations",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        990, 770, NULL, NULL, hInstance, NULL);

    if (!mainWindow)
    {
        MessageBoxA(NULL, "Could not create the application window.",
                    "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(mainWindow, nCmdShow);
    UpdateWindow(mainWindow);

    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    DeleteObject(appFont);
    DeleteObject(backgroundBrush);
    DeleteObject(panelBrush);
    DeleteObject(inputBrush);

    return (int)msg.wParam;
}
