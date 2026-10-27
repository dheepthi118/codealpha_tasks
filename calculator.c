
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define HISTORY_SIZE 5

HWND mainWindow;

char current[64] = "0";
char expression[120] = "";
char history[HISTORY_SIZE][160];

int historyCount = 0;
int startNewNumber = 1;
int hasError = 0;
int scientificMode = 0;
int degreesMode = 1;

double firstNumber = 0;
char pendingOperator = 0;

COLORREF bgColor = RGB(23, 20, 42);
COLORREF panelColor = RGB(37, 31, 63);
COLORREF buttonColor = RGB(51, 43, 80);
COLORREF accentColor = RGB(139, 111, 230);
COLORREF textColor = RGB(245, 242, 255);
COLORREF mutedColor = RGB(175, 164, 207);

const char *standardButtons[20] = {
    "7", "8", "9", "/",
    "4", "5", "6", "*",
    "1", "2", "3", "-",
    "+/-", "0", ".", "+",
    "C", "DEL", "%", "="
};

const char *scientificButtons[15] = {
    "sin", "cos", "tan", "ln", "log",
    "sqrt", "x^2", "x^y", "1/x", "pi",
    "e", "DEG/RAD", "abs", "exp", "C"
};

void formatNumber(double value, char *out)
{
    if (fabs(value) < 1e-12)
        value = 0;

    sprintf(out, "%.10g", value);
}

void addHistory(const char *entry)
{
    int i;

    if (historyCount < HISTORY_SIZE)
        historyCount++;

    for (i = historyCount - 1; i > 0; i--)
        strcpy(history[i], history[i - 1]);

    strcpy(history[0], entry);
}

void setError(const char *message)
{
    strncpy(current, message, sizeof(current) - 1);
    current[sizeof(current) - 1] = '\0';

    hasError = 1;
    startNewNumber = 1;
    pendingOperator = 0;
}

void refresh(void)
{
    InvalidateRect(mainWindow, NULL, TRUE);
    UpdateWindow(mainWindow);
}

void calculate(void)
{
    double secondNumber = atof(current);
    double result = 0;
    char entry[160];

    if (pendingOperator == 0)
        return;

    switch (pendingOperator)
    {
        case '+':
            result = firstNumber + secondNumber;
            break;

        case '-':
            result = firstNumber - secondNumber;
            break;

        case '*':
            result = firstNumber * secondNumber;
            break;

        case '/':
            if (secondNumber == 0)
            {
                setError("Cannot divide by zero");
                return;
            }
            result = firstNumber / secondNumber;
            break;

        case '^':
            result = pow(firstNumber, secondNumber);
            break;

        default:
            return;
    }

    if (!isfinite(result))
    {
        setError("Math error");
        return;
    }

    sprintf(entry, "%.10g %c %.10g = %.10g",
            firstNumber, pendingOperator,
            secondNumber, result);

    addHistory(entry);
    strcpy(expression, entry);
    formatNumber(result, current);

    pendingOperator = 0;
    startNewNumber = 1;
}

void selectOperator(char op)
{
    if (hasError)
        return;

    if (pendingOperator != 0 && !startNewNumber)
        calculate();

    if (hasError)
        return;

    firstNumber = atof(current);
    pendingOperator = op;

    sprintf(expression, "%.10g %c", firstNumber, op);
    startNewNumber = 1;
}

void applyUnary(const char *name, double result)
{
    char entry[160];
    double original = atof(current);

    if (!isfinite(result))
    {
        setError("Math error");
        return;
    }

    sprintf(entry, "%s(%.10g) = %.10g",
            name, original, result);

    addHistory(entry);
    strcpy(expression, entry);
    formatNumber(result, current);

    startNewNumber = 1;
}

void scientificAction(int index)
{
    double x = atof(current);
    double result = 0;
    double angle = x;

    if (hasError)
    {
        strcpy(current, "0");
        hasError = 0;
    }

    switch (index)
    {
        case 0:  /* sine */
            if (degreesMode)
                angle = x * 3.14159265358979323846 / 180.0;
            applyUnary("sin", sin(angle));
            break;

        case 1:  /* cosine */
            if (degreesMode)
                angle = x * 3.14159265358979323846 / 180.0;
            applyUnary("cos", cos(angle));
            break;

        case 2:  /* tangent */
            if (degreesMode)
                angle = x * 3.14159265358979323846 / 180.0;

            if (fabs(cos(angle)) < 1e-12)
                setError("Undefined");
            else
                applyUnary("tan", tan(angle));
            break;

        case 3:  /* natural logarithm */
            if (x <= 0)
                setError("Invalid input");
            else
                applyUnary("ln", log(x));
            break;

        case 4:  /* base-10 logarithm */
            if (x <= 0)
                setError("Invalid input");
            else
                applyUnary("log", log10(x));
            break;

        case 5:  /* square root */
            if (x < 0)
                setError("Invalid input");
            else
                applyUnary("sqrt", sqrt(x));
            break;

        case 6:  /* square */
            applyUnary("square", x * x);
            break;

        case 7:  /* power operator */
            selectOperator('^');
            break;

        case 8:  /* reciprocal */
            if (x == 0)
                setError("Cannot divide by zero");
            else
                applyUnary("1/x", 1.0 / x);
            break;

        case 9:  /* pi */
            formatNumber(3.14159265358979323846, current);
            startNewNumber = 1;
            strcpy(expression, "Constant: pi");
            break;

        case 10: /* e */
            formatNumber(2.71828182845904523536, current);
            startNewNumber = 1;
            strcpy(expression, "Constant: e");
            break;

        case 11: /* degrees/radians */
            degreesMode = !degreesMode;
            break;

        case 12: /* absolute value */
            result = fabs(x);
            applyUnary("abs", result);
            break;

        case 13: /* exponential */
            result = exp(x);
            applyUnary("exp", result);
            break;

        case 14: /* clear */
            strcpy(current, "0");
            expression[0] = '\0';
            pendingOperator = 0;
            firstNumber = 0;
            startNewNumber = 1;
            hasError = 0;
            break;
    }
}

void standardAction(int index)
{
    const char *label = standardButtons[index];
    char key = label[0];
    int length;

    if (hasError)
    {
        strcpy(current, "0");
        expression[0] = '\0';
        hasError = 0;
        pendingOperator = 0;
        startNewNumber = 1;
    }

    if (index >= 0 && index <= 10 &&
        strlen(label) == 1 &&
        key >= '0' && key <= '9')
    {
        if (startNewNumber)
        {
            strcpy(current, label);
            startNewNumber = 0;
        }
        else
        {
            length = (int)strlen(current);

            if (length < 30)
            {
                if (strcmp(current, "0") == 0)
                    strcpy(current, label);
                else
                    strcat(current, label);
            }
        }

        expression[0] = '\0';
        return;
    }

    switch (index)
    {
        case 11:
            selectOperator('-');
            break;

        case 15:
            selectOperator('+');
            break;

        case 3:
            selectOperator('/');
            break;

        case 7:
            selectOperator('*');
            break;

        case 12: /* plus/minus */
            if (atof(current) != 0)
            {
                if (current[0] == '-')
                    memmove(current, current + 1, strlen(current));
                else
                {
                    length = (int)strlen(current);
                    memmove(current + 1, current, length + 1);
                    current[0] = '-';
                }
            }
            expression[0] = '\0';
            break;

        case 14: /* decimal */
            if (startNewNumber)
            {
                strcpy(current, "0.");
                startNewNumber = 0;
            }
            else if (strchr(current, '.') == NULL)
            {
                strcat(current, ".");
            }
            expression[0] = '\0';
            break;

        case 16: /* clear */
            strcpy(current, "0");
            expression[0] = '\0';
            pendingOperator = 0;
            firstNumber = 0;
            startNewNumber = 1;
            break;

        case 17: /* backspace */
            if (!startNewNumber)
            {
                length = (int)strlen(current);

                if (length > 1)
                    current[length - 1] = '\0';
                else
                    strcpy(current, "0");

                if (strcmp(current, "-") == 0 ||
                    strcmp(current, "") == 0)
                    strcpy(current, "0");
            }
            expression[0] = '\0';
            break;

        case 18: /* percentage */
            applyUnary("percent", atof(current) / 100.0);
            break;

        case 19: /* equals */
            calculate();
            break;
    }
}

void drawText(HDC hdc, const char *text,
              int x, int y, int w, int h,
              COLORREF color, int size,
              int bold, UINT alignment)
{
    HFONT font = CreateFontA(
        size, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"
    );

    HFONT oldFont = (HFONT)SelectObject(hdc, font);

    RECT rect = { x, y, x + w, y + h };

    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);

    DrawTextA(hdc, text, -1, &rect,
              alignment | DT_VCENTER | DT_SINGLELINE |
              DT_END_ELLIPSIS);

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void roundedPanel(HDC hdc, int x, int y,
                  int w, int h, COLORREF color, int radius)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);

    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);

    RoundRect(hdc, x, y, x + w, y + h, radius, radius);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);

    DeleteObject(brush);
    DeleteObject(pen);
}

void drawButton(HDC hdc, int x, int y,
                int w, int h, const char *label,
                COLORREF fill, COLORREF foreground,
                int fontSize)
{
    roundedPanel(hdc, x, y, w, h, fill, 14);

    drawText(hdc, label, x + 2, y + 2,
             w - 4, h - 4, foreground,
             fontSize, 1, DT_CENTER);
}

void paintCalculator(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT client;
    GetClientRect(hwnd, &client);

    HBRUSH bg = CreateSolidBrush(bgColor);
    FillRect(hdc, &client, bg);
    DeleteObject(bg);

    roundedPanel(hdc, 15, 15, 420, 800,
                 panelColor, 24);

    roundedPanel(hdc, 455, 15, 400, 800,
                 panelColor, 24);

    drawText(hdc, "PREMIUM GLASS",
             32, 25, 245, 25,
             mutedColor, 12, 1, DT_LEFT);

    drawText(hdc, scientificMode ?
             "Scientific Calculator" : "Calculator",
             32, 51, 275, 32,
             textColor, 21, 1, DT_LEFT);

    drawButton(hdc, 320, 29, 96, 38,
               scientificMode ? "STANDARD" : "SCI",
               accentColor, RGB(255,255,255), 11);

    roundedPanel(hdc, 30, 100, 390, 91,
                 RGB(29, 25, 49), 17);

    drawText(hdc, expression,
             43, 105, 363, 25,
             mutedColor, 12, 0, DT_RIGHT);

    drawText(hdc, current,
             42, 131, 364, 51,
             textColor, 25, 1, DT_RIGHT);

    if (scientificMode)
    {
        for (int i = 0; i < 15; i++)
        {
            int row = i / 5;
            int col = i % 5;
            int x = 30 + col * 78;
            int y = 205 + row * 61;

            COLORREF fill = RGB(67, 53, 101);

            if (i == 11)
                fill = accentColor;

            drawButton(hdc, x, y, 69, 51,
                       scientificButtons[i],
                       fill, textColor, 12);
        }
    }

    for (int i = 0; i < 20; i++)
    {
        int row = i / 4;
        int col = i % 4;

        int x = 30 + col * 95;
        int y = scientificMode ?
                405 + row * 70 : 220 + row * 70;

        COLORREF fill = buttonColor;
        COLORREF foreground = textColor;

        if (i == 3 || i == 7 || i == 11 ||
            i == 15 || i == 19)
        {
            fill = accentColor;
            foreground = RGB(255,255,255);
        }

        if (i == 16 || i == 17 || i == 18)
            fill = RGB(72, 57, 105);

        drawButton(hdc, x, y, 84, 58,
                   standardButtons[i],
                   fill, foreground, 17);
    }

    drawText(hdc, degreesMode ?
             "Angle mode: DEG" : "Angle mode: RAD",
             32, 765, 370, 25,
             mutedColor, 11, 0, DT_LEFT);

    drawText(hdc, "RECENT CALCULATIONS",
             477, 30, 350, 30,
             textColor, 16, 1, DT_LEFT);

    drawText(hdc, "Your calculation history",
             477, 62, 350, 24,
             mutedColor, 11, 0, DT_LEFT);

    HPEN separator = CreatePen(
        PS_SOLID, 1, RGB(70, 59, 102));

    HPEN oldPen = (HPEN)SelectObject(hdc, separator);
    MoveToEx(hdc, 477, 98, NULL);
    LineTo(hdc, 832, 98);
    SelectObject(hdc, oldPen);
    DeleteObject(separator);

    if (historyCount == 0)
    {
        drawText(hdc, "No calculations yet",
                 477, 118, 350, 35,
                 mutedColor, 13, 0, DT_LEFT);
    }
    else
    {
        for (int i = 0; i < historyCount; i++)
        {
            int y = 115 + i * 85;

            roundedPanel(hdc, 473, y, 364, 70,
                         RGB(44, 37, 71), 12);

            char number[20];
            sprintf(number, "%02d", i + 1);

            drawText(hdc, number,
                     485, y + 6, 28, 22,
                     accentColor, 11, 1, DT_LEFT);

            drawText(hdc, history[i],
                     485, y + 29, 335, 30,
                     textColor, 12, 0, DT_LEFT);
        }
    }

    drawText(hdc, "Calculations are performed in C",
             477, 765, 350, 22,
             mutedColor, 11, 0, DT_LEFT);

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WindowProcedure(
    HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_PAINT:
            paintCalculator(hwnd);
            return 0;

        case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lp);
            int y = HIWORD(lp);

            /* Mode switch */
            if (x >= 320 && x < 416 &&
                y >= 29 && y < 67)
            {
                scientificMode = !scientificMode;
                refresh();
                return 0;
            }

            /* Scientific function buttons */
            if (scientificMode &&
                x >= 30 && x < 420 &&
                y >= 205 && y < 376)
            {
                int col = (x - 30) / 78;
                int row = (y - 205) / 61;

                if (col >= 0 && col < 5 &&
                    row >= 0 && row < 3)
                {
                    int bx = 30 + col * 78;
                    int by = 205 + row * 61;

                    if (x < bx + 69 && y < by + 51)
                    {
                        scientificAction(row * 5 + col);
                        refresh();
                        return 0;
                    }
                }
            }

            /* Standard keypad */
            {
                int top = scientificMode ? 405 : 220;

                if (x >= 30 && x < 410 &&
                    y >= top && y < top + 350)
                {
                    int col = (x - 30) / 95;
                    int row = (y - top) / 70;

                    if (col >= 0 && col < 4 &&
                        row >= 0 && row < 5)
                    {
                        int bx = 30 + col * 95;
                        int by = top + row * 70;

                        if (x < bx + 84 && y < by + 58)
                        {
                            standardAction(row * 4 + col);
                            refresh();
                            return 0;
                        }
                    }
                }
            }

            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(
    HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSA wc = {0};

    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = hInstance;
    wc.lpszClassName = "PremiumScientificCalculator";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(bgColor);

    RegisterClassA(&wc);

    mainWindow = CreateWindowA(
        "PremiumScientificCalculator",
        "Premium Glass Scientific Calculator",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 880,
        NULL, NULL, hInstance, NULL
    );

    if (mainWindow == NULL)
    {
        MessageBoxA(NULL,
                    "Could not create calculator window.",
                    "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(mainWindow, nCmdShow);
    UpdateWindow(mainWindow);

    MSG msg;

    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}
