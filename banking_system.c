
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACCOUNTS 500
#define DATA_FILE "bank_accounts.txt"

#define ID_ACCOUNT  101
#define ID_NAME     102
#define ID_AMOUNT   103
#define ID_CREATE   104
#define ID_DEPOSIT  105
#define ID_WITHDRAW 106
#define ID_BALANCE  107
#define ID_CLEAR    108
#define ID_LIST     109
#define ID_REFRESH  110

typedef struct {
    int accountNumber;
    char name[100];
    double balance;
} BankAccount;

BankAccount accounts[MAX_ACCOUNTS];
int accountCount = 0;

HWND mainWindow;
HWND editAccount, editName, editAmount;
HWND accountList, statusLabel, balanceLabel;

HBRUSH backgroundBrush;
HFONT titleFont, normalFont, headingFont;

const COLORREF DARK_PURPLE = RGB(35, 24, 58);
const COLORREF PURPLE = RGB(112, 70, 190);
const COLORREF WHITE = RGB(255, 255, 255);
const COLORREF LIGHT_PURPLE = RGB(239, 232, 255);

void setStatus(const char *message)
{
    SetWindowTextA(statusLabel, message);
}

void getText(HWND control, char *buffer, int size)
{
    GetWindowTextA(control, buffer, size);
}

int findAccount(int accountNumber)
{
    int i;

    for (i = 0; i < accountCount; i++) {
        if (accounts[i].accountNumber == accountNumber)
            return i;
    }

    return -1;
}

void saveAccounts(void)
{
    FILE *file = fopen(DATA_FILE, "w");
    int i;

    if (file == NULL) {
        MessageBoxA(mainWindow,
                    "Could not save bank records.",
                    "File Error", MB_OK | MB_ICONERROR);
        return;
    }

    for (i = 0; i < accountCount; i++) {
        fprintf(file, "%d\t%s\t%.2f\n",
                accounts[i].accountNumber,
                accounts[i].name,
                accounts[i].balance);
    }

    fclose(file);
}

void loadAccounts(void)
{
    FILE *file = fopen(DATA_FILE, "r");
    char line[300];

    if (file == NULL)
        return;

    accountCount = 0;

    while (fgets(line, sizeof(line), file) != NULL &&
           accountCount < MAX_ACCOUNTS) {
        char *numberText;
        char *nameText;
        char *balanceText;
        char *newline;
        int number;

        newline = strpbrk(line, "\r\n");
        if (newline != NULL)
            *newline = '\0';

        numberText = strtok(line, "\t");
        nameText = strtok(NULL, "\t");
        balanceText = strtok(NULL, "\t");

        if (numberText == NULL ||
            nameText == NULL ||
            balanceText == NULL)
            continue;

        number = atoi(numberText);

        if (number <= 0 || findAccount(number) != -1)
            continue;

        accounts[accountCount].accountNumber = number;

        strncpy(accounts[accountCount].name,
                nameText,
                sizeof(accounts[accountCount].name) - 1);

        accounts[accountCount].name[
            sizeof(accounts[accountCount].name) - 1] = '\0';

        accounts[accountCount].balance = atof(balanceText);

        accountCount++;
    }

    fclose(file);
}

void refreshAccountList(void)
{
    int i;
    char line[250];

    SendMessageA(accountList, LB_RESETCONTENT, 0, 0);

    for (i = 0; i < accountCount; i++) {
        sprintf(line, "A/C %d   |   %-22s   |   Rs. %.2f",
                accounts[i].accountNumber,
                accounts[i].name,
                accounts[i].balance);

        SendMessageA(accountList, LB_ADDSTRING, 0, (LPARAM)line);

        SendMessageA(accountList, LB_SETITEMDATA,
                     i, (LPARAM)i);
    }

    sprintf(line, "TOTAL ACCOUNTS: %d", accountCount);
    SetWindowTextA(balanceLabel, line);
}

void clearFields(void)
{
    SetWindowTextA(editAccount, "");
    SetWindowTextA(editName, "");
    SetWindowTextA(editAmount, "");

    SendMessageA(accountList, LB_SETCURSEL, (WPARAM)-1, 0);

    setStatus("Ready. Select an action to continue.");
    SetFocus(editAccount);
}

int readAccountNumber(int *accountNumber)
{
    char text[50];
    char *end;
    long value;

    getText(editAccount, text, sizeof(text));

    if (text[0] == '\0') {
        MessageBoxA(mainWindow,
                    "Please enter the account number.",
                    "Missing Information",
                    MB_OK | MB_ICONWARNING);
        return 0;
    }

    value = strtol(text, &end, 10);

    if (*end != '\0' || value <= 0 || value > 2147483647L) {
        MessageBoxA(mainWindow,
                    "Enter a valid positive account number.",
                    "Invalid Account Number",
                    MB_OK | MB_ICONWARNING);
        return 0;
    }

    *accountNumber = (int)value;
    return 1;
}

int readAmount(double *amount)
{
    char text[50];
    char *end;
    double value;

    getText(editAmount, text, sizeof(text));

    if (text[0] == '\0') {
        MessageBoxA(mainWindow,
                    "Please enter an amount.",
                    "Missing Amount",
                    MB_OK | MB_ICONWARNING);
        return 0;
    }

    value = strtod(text, &end);

    if (*end != '\0' || value <= 0 || value > 1000000000000.0) {
        MessageBoxA(mainWindow,
                    "Enter a valid amount greater than zero.",
                    "Invalid Amount",
                    MB_OK | MB_ICONWARNING);
        return 0;
    }

    *amount = value;
    return 1;
}

void createAccount(void)
{
    int number;
    char name[100];
    char *end;
    double openingBalance = 0.0;
    char amountText[50];

    if (!readAccountNumber(&number))
        return;

    getText(editName, name, sizeof(name));
    getText(editAmount, amountText, sizeof(amountText));

    if (name[0] == '\0') {
        MessageBoxA(mainWindow,
                    "Please enter the account holder's name.",
                    "Missing Name",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    if (strchr(name, '\t') != NULL ||
        strchr(name, '\n') != NULL ||
        strchr(name, '\r') != NULL) {
        MessageBoxA(mainWindow,
                    "The account name cannot contain tabs or new lines.",
                    "Invalid Name",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    if (findAccount(number) != -1) {
        MessageBoxA(mainWindow,
                    "This account number already exists.",
                    "Duplicate Account",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    if (accountCount >= MAX_ACCOUNTS) {
        MessageBoxA(mainWindow,
                    "The account storage limit has been reached.",
                    "Storage Full",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    if (amountText[0] != '\0') {
        openingBalance = strtod(amountText, &end);

        if (*end != '\0' || openingBalance < 0 ||
            openingBalance > 1000000000000.0) {
            MessageBoxA(mainWindow,
                        "Enter a valid opening balance of zero or more.",
                        "Invalid Balance",
                        MB_OK | MB_ICONWARNING);
            return;
        }
    }

    accounts[accountCount].accountNumber = number;

    strncpy(accounts[accountCount].name, name,
            sizeof(accounts[accountCount].name) - 1);

    accounts[accountCount].name[
        sizeof(accounts[accountCount].name) - 1] = '\0';

    accounts[accountCount].balance = openingBalance;
    accountCount++;

    saveAccounts();
    refreshAccountList();

    {
        char message[200];
        sprintf(message,
                "Account created successfully!\n\nAccount number: %d\nOpening balance: Rs. %.2f",
                number, openingBalance);

        MessageBoxA(mainWindow, message,
                    "Account Created",
                    MB_OK | MB_ICONINFORMATION);
    }

    clearFields();
    setStatus("New bank account created successfully.");
}

void depositMoney(void)
{
    int number;
    int index;
    double amount;
    char message[200];

    if (!readAccountNumber(&number) || !readAmount(&amount))
        return;

    index = findAccount(number);

    if (index == -1) {
        MessageBoxA(mainWindow,
                    "Account not found. Please check the account number.",
                    "Account Not Found",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    if (accounts[index].balance + amount > 1000000000000.0) {
        MessageBoxA(mainWindow,
                    "This deposit exceeds the supported balance limit.",
                    "Amount Too Large",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    accounts[index].balance += amount;
    saveAccounts();
    refreshAccountList();

    sprintf(message,
            "Deposit successful!\n\nAmount: Rs. %.2f\nNew balance: Rs. %.2f",
            amount, accounts[index].balance);

    MessageBoxA(mainWindow, message,
                "Deposit Complete",
                MB_OK | MB_ICONINFORMATION);

    setStatus("Deposit completed successfully.");
}

void withdrawMoney(void)
{
    int number;
    int index;
    double amount;
    char message[200];

    if (!readAccountNumber(&number) || !readAmount(&amount))
        return;

    index = findAccount(number);

    if (index == -1) {
        MessageBoxA(mainWindow,
                    "Account not found. Please check the account number.",
                    "Account Not Found",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    if (amount > accounts[index].balance) {
        MessageBoxA(mainWindow,
                    "Insufficient balance. Withdrawal cancelled.",
                    "Insufficient Funds",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    accounts[index].balance -= amount;
    saveAccounts();
    refreshAccountList();

    sprintf(message,
            "Withdrawal successful!\n\nAmount: Rs. %.2f\nRemaining balance: Rs. %.2f",
            amount, accounts[index].balance);

    MessageBoxA(mainWindow, message,
                "Withdrawal Complete",
                MB_OK | MB_ICONINFORMATION);

    setStatus("Withdrawal completed successfully.");
}

void balanceEnquiry(void)
{
    int number;
    int index;
    char message[300];

    if (!readAccountNumber(&number))
        return;

    index = findAccount(number);

    if (index == -1) {
        MessageBoxA(mainWindow,
                    "Account not found. Please check the account number.",
                    "Account Not Found",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    sprintf(message,
            "ACCOUNT BALANCE\n\nAccount number: %d\nAccount holder: %s\nAvailable balance: Rs. %.2f",
            accounts[index].accountNumber,
            accounts[index].name,
            accounts[index].balance);

    MessageBoxA(mainWindow, message,
                "Balance Enquiry",
                MB_OK | MB_ICONINFORMATION);

    SetWindowTextA(editName, accounts[index].name);
    setStatus("Balance enquiry completed.");
}

HWND makeLabel(HWND parent, const char *text,
               int x, int y, int width, int height)
{
    HWND control = CreateWindowA(
        "STATIC", text,
        WS_CHILD | WS_VISIBLE,
        x, y, width, height,
        parent, NULL, GetModuleHandle(NULL), NULL);

    SendMessageA(control, WM_SETFONT, (WPARAM)normalFont, TRUE);
    return control;
}

HWND makeEdit(HWND parent, int id,
              int x, int y, int width, int height)
{
    HWND control = CreateWindowA(
        "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        x, y, width, height,
        parent, (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL), NULL);

    SendMessageA(control, WM_SETFONT, (WPARAM)normalFont, TRUE);
    return control;
}

HWND makeButton(HWND parent, const char *text, int id,
                int x, int y, int width, int height)
{
    HWND control = CreateWindowA(
        "BUTTON", text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        x, y, width, height,
        parent, (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL), NULL);

    SendMessageA(control, WM_SETFONT, (WPARAM)normalFont, TRUE);
    return control;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        titleFont = CreateFontA(
            30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        headingFont = CreateFontA(
            18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        normalFont = CreateFontA(
            15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        {
            HWND title = makeLabel(hwnd, "PURPLE BANK",
                                   30, 18, 400, 42);
            SendMessageA(title, WM_SETFONT,
                         (WPARAM)titleFont, TRUE);
        }

        makeLabel(hwnd, "SIMPLE BANKING MANAGEMENT SYSTEM",
                  33, 60, 450, 25);

        makeLabel(hwnd, "ACCOUNT DETAILS",
                  32, 108, 300, 28);

        makeLabel(hwnd, "Account Number", 32, 147, 170, 22);
        editAccount = makeEdit(hwnd, ID_ACCOUNT,
                               32, 172, 220, 32);

        makeLabel(hwnd, "Account Holder Name", 280, 147, 220, 22);
        editName = makeEdit(hwnd, ID_NAME,
                            280, 172, 290, 32);

        makeLabel(hwnd, "Amount (Rs.)", 600, 147, 150, 22);
        editAmount = makeEdit(hwnd, ID_AMOUNT,
                              600, 172, 220, 32);

        makeLabel(hwnd,
                  "For account creation, the amount is optional and is the opening balance.",
                  32, 210, 800, 24);

        makeLabel(hwnd, "BANKING SERVICES",
                  32, 251, 300, 28);

        makeButton(hwnd, "CREATE ACCOUNT", ID_CREATE,
                   32, 292, 175, 42);

        makeButton(hwnd, "DEPOSIT MONEY", ID_DEPOSIT,
                   222, 292, 175, 42);

        makeButton(hwnd, "WITHDRAW MONEY", ID_WITHDRAW,
                   412, 292, 175, 42);

        makeButton(hwnd, "BALANCE ENQUIRY", ID_BALANCE,
                   602, 292, 175, 42);

        makeButton(hwnd, "CLEAR", ID_CLEAR,
                   792, 292, 100, 42);

        makeLabel(hwnd, "ACCOUNT DIRECTORY",
                  32, 360, 300, 28);

        accountList = CreateWindowA(
            "LISTBOX", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            32, 398, 860, 190,
            hwnd, (HMENU)(INT_PTR)ID_LIST,
            GetModuleHandle(NULL), NULL);

        SendMessageA(accountList, WM_SETFONT,
                     (WPARAM)normalFont, TRUE);

        balanceLabel = makeLabel(hwnd, "TOTAL ACCOUNTS: 0",
                                 32, 604, 350, 25);

        statusLabel = makeLabel(hwnd,
                                "Ready. Select an action to continue.",
                                32, 640, 850, 25);

        loadAccounts();
        refreshAccountList();
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_CREATE:
            createAccount();
            return 0;

        case ID_DEPOSIT:
            depositMoney();
            return 0;

        case ID_WITHDRAW:
            withdrawMoney();
            return 0;

        case ID_BALANCE:
            balanceEnquiry();
            return 0;

        case ID_CLEAR:
            clearFields();
            return 0;

        case ID_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                int selection = (int)SendMessageA(
                    accountList, LB_GETCURSEL, 0, 0);

                if (selection != LB_ERR) {
                    int index = (int)SendMessageA(
                        accountList, LB_GETITEMDATA,
                        (WPARAM)selection, 0);

                    if (index >= 0 && index < accountCount) {
                        char number[40];

                        sprintf(number, "%d",
                                accounts[index].accountNumber);
                        SetWindowTextA(editAccount, number);
                        SetWindowTextA(editName, accounts[index].name);
                        SetWindowTextA(editAmount, "");

                        setStatus("Account selected. Choose a banking service.");
                    }
                }
            }
            return 0;
        }
        break;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, WHITE);
        SetBkColor(hdc, DARK_PURPLE);
        return (LRESULT)backgroundBrush;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, DARK_PURPLE);
        SetBkColor(hdc, WHITE);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;

        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, backgroundBrush);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        saveAccounts();

        if (backgroundBrush)
            DeleteObject(backgroundBrush);
        if (titleFont)
            DeleteObject(titleFont);
        if (normalFont)
            DeleteObject(normalFont);
        if (headingFont)
            DeleteObject(headingFont);

        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance,
                   LPSTR commandLine, int showCommand)
{
    WNDCLASSA windowClass;
    MSG message;

    (void)previousInstance;
    (void)commandLine;

    backgroundBrush = CreateSolidBrush(DARK_PURPLE);

    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "PurpleBankWindow";
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = backgroundBrush;

    if (!RegisterClassA(&windowClass)) {
        MessageBoxA(NULL,
                    "Could not register the banking application.",
                    "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    mainWindow = CreateWindowA(
        "PurpleBankWindow",
        "Purple Bank | Banking System",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        960, 740,
        NULL, NULL, instance, NULL);

    if (mainWindow == NULL) {
        MessageBoxA(NULL,
                    "Could not open the banking application.",
                    "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(mainWindow, showCommand);
    UpdateWindow(mainWindow);

    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return (int)message.wParam;
}
