
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_STUDENTS 500
#define FILE_NAME "students.txt"

#define ID_ID       101
#define ID_NAME     102
#define ID_AGE      103
#define ID_COURSE   104
#define ID_ADD      105
#define ID_UPDATE   106
#define ID_DELETE   107
#define ID_CLEAR    108
#define ID_SEARCH   109
#define ID_SHOWALL  110
#define ID_EXPORT   111
#define ID_TABLE    112

typedef struct {
    int id;
    char name[100];
    int age;
    char course[100];
} Student;

Student students[MAX_STUDENTS];
int studentCount = 0;
int selectedOriginalId = -1;

HWND mainWindow;
HWND editID, editName, editAge, editCourse;
HWND editSearch, studentTable;
HWND labelTotal, statusLabel;

HBRUSH backgroundBrush;
HFONT titleFont, normalFont, headingFont;

const COLORREF PURPLE = RGB(91, 54, 160);
const COLORREF DARK_PURPLE = RGB(35, 24, 58);
const COLORREF LIGHT_PURPLE = RGB(239, 232, 255);
const COLORREF WHITE = RGB(255, 255, 255);

void setStatus(const char *message)
{
    SetWindowTextA(statusLabel, message);
}

void getControlText(HWND control, char *buffer, int size)
{
    GetWindowTextA(control, buffer, size);
}

int findStudentById(int id)
{
    int i;
    for (i = 0; i < studentCount; i++) {
        if (students[i].id == id)
            return i;
    }
    return -1;
}

int containsIgnoreCase(const char *text, const char *query)
{
    size_t queryLength = strlen(query);
    size_t i;

    if (queryLength == 0)
        return 1;

    if (strlen(text) < queryLength)
        return 0;

    for (i = 0; text[i] != '\0'; i++) {
        if (_strnicmp(text + i, query, queryLength) == 0)
            return 1;
    }

    return 0;
}

void saveStudents(void)
{
    FILE *file = fopen(FILE_NAME, "w");
    int i;

    if (file == NULL) {
        MessageBoxA(mainWindow,
                    "Could not save the student records.",
                    "File Error", MB_OK | MB_ICONERROR);
        return;
    }

    for (i = 0; i < studentCount; i++) {
        fprintf(file, "%d\t%s\t%d\t%s\n",
                students[i].id,
                students[i].name,
                students[i].age,
                students[i].course);
    }

    fclose(file);
}

void loadStudents(void)
{
    FILE *file = fopen(FILE_NAME, "r");
    char line[400];

    if (file == NULL)
        return;

    studentCount = 0;

    while (fgets(line, sizeof(line), file) != NULL &&
           studentCount < MAX_STUDENTS) {
        char *idText;
        char *nameText;
        char *ageText;
        char *courseText;
        char *newline;

        newline = strpbrk(line, "\r\n");
        if (newline != NULL)
            *newline = '\0';

        idText = strtok(line, "\t");
        nameText = strtok(NULL, "\t");
        ageText = strtok(NULL, "\t");
        courseText = strtok(NULL, "\t");

        if (idText == NULL || nameText == NULL ||
            ageText == NULL || courseText == NULL)
            continue;

        students[studentCount].id = atoi(idText);
        students[studentCount].age = atoi(ageText);

        strncpy(students[studentCount].name,
                nameText, sizeof(students[studentCount].name) - 1);
        students[studentCount].name[
            sizeof(students[studentCount].name) - 1] = '\0';

        strncpy(students[studentCount].course,
                courseText, sizeof(students[studentCount].course) - 1);
        students[studentCount].course[
            sizeof(students[studentCount].course) - 1] = '\0';

        studentCount++;
    }

    fclose(file);
}

void addTableColumn(int index, const char *heading, int width)
{
    LVCOLUMNA column;

    ZeroMemory(&column, sizeof(column));
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.pszText = (LPSTR)heading;
    column.cx = width;
    column.iSubItem = index;

    ListView_InsertColumn(studentTable, index, &column);
}

void refreshTable(void)
{
    char query[100];
    char number[30];
    int i, displayed = 0;

    getControlText(editSearch, query, sizeof(query));
    ListView_DeleteAllItems(studentTable);

    for (i = 0; i < studentCount; i++) {
        LVITEMA item;
        int matches = 0;

        sprintf(number, "%d", students[i].id);

        if (query[0] == '\0' ||
            containsIgnoreCase(number, query) ||
            containsIgnoreCase(students[i].name, query) ||
            containsIgnoreCase(students[i].course, query)) {
            matches = 1;
        }

        if (!matches)
            continue;

        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_TEXT;
        item.iItem = displayed;
        item.iSubItem = 0;
        item.pszText = number;

        ListView_InsertItem(studentTable, &item);

        ListView_SetItemText(studentTable, displayed, 1,
                             students[i].name);

        sprintf(number, "%d", students[i].age);
        ListView_SetItemText(studentTable, displayed, 2, number);

        ListView_SetItemText(studentTable, displayed, 3,
                             students[i].course);

        displayed++;
    }

    sprintf(number, "Total students: %d", studentCount);
    SetWindowTextA(labelTotal, number);
}

void clearFields(void)
{
    SetWindowTextA(editID, "");
    SetWindowTextA(editName, "");
    SetWindowTextA(editAge, "");
    SetWindowTextA(editCourse, "");

    selectedOriginalId = -1;
    ListView_SetItemState(studentTable, -1, 0, LVIS_SELECTED);
    setStatus("Ready. Enter student details to begin.");
    SetFocus(editID);
}

int readForm(Student *student)
{
    char idText[30];
    char ageText[30];

    getControlText(editID, idText, sizeof(idText));
    getControlText(editName, student->name, sizeof(student->name));
    getControlText(editAge, ageText, sizeof(ageText));
    getControlText(editCourse, student->course,
                   sizeof(student->course));

    if (idText[0] == '\0' || student->name[0] == '\0' ||
        ageText[0] == '\0' || student->course[0] == '\0') {
        MessageBoxA(mainWindow,
                    "Please fill in all four fields.",
                    "Missing Information",
                    MB_OK | MB_ICONWARNING);
        return 0;
    }

    student->id = atoi(idText);
    student->age = atoi(ageText);

    if (student->id <= 0) {
        MessageBoxA(mainWindow,
                    "Student ID must be a positive number.",
                    "Invalid ID", MB_OK | MB_ICONWARNING);
        return 0;
    }

    if (student->age < 1 || student->age > 120) {
        MessageBoxA(mainWindow,
                    "Please enter a valid age between 1 and 120.",
                    "Invalid Age", MB_OK | MB_ICONWARNING);
        return 0;
    }

    if (strchr(student->name, '\t') != NULL ||
        strchr(student->course, '\t') != NULL) {
        MessageBoxA(mainWindow,
                    "Tabs are not allowed in names or courses.",
                    "Invalid Input", MB_OK | MB_ICONWARNING);
        return 0;
    }

    return 1;
}

void addStudent(void)
{
    Student student;

    if (!readForm(&student))
        return;

    if (studentCount >= MAX_STUDENTS) {
        MessageBoxA(mainWindow, "Student storage is full.",
                    "Limit Reached", MB_OK | MB_ICONWARNING);
        return;
    }

    if (findStudentById(student.id) != -1) {
        MessageBoxA(mainWindow,
                    "This student ID already exists.",
                    "Duplicate ID", MB_OK | MB_ICONWARNING);
        return;
    }

    students[studentCount++] = student;
    saveStudents();

    SetWindowTextA(editSearch, "");
    refreshTable();
    clearFields();
    setStatus("Student added successfully.");
}

void updateStudent(void)
{
    Student student;
    int index;

    if (selectedOriginalId == -1) {
        MessageBoxA(mainWindow,
                    "Select a student from the table before updating.",
                    "Select Student", MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (!readForm(&student))
        return;

    index = findStudentById(selectedOriginalId);

    if (index == -1) {
        MessageBoxA(mainWindow,
                    "The selected record could not be found.",
                    "Record Not Found", MB_OK | MB_ICONERROR);
        return;
    }

    {
        int duplicate = findStudentById(student.id);
        if (duplicate != -1 && duplicate != index) {
            MessageBoxA(mainWindow,
                        "Another student already uses this ID.",
                        "Duplicate ID", MB_OK | MB_ICONWARNING);
            return;
        }
    }

    students[index] = student;
    saveStudents();

    SetWindowTextA(editSearch, "");
    refreshTable();
    clearFields();
    setStatus("Student record updated successfully.");
}

void deleteStudent(void)
{
    int index;
    int answer;

    if (selectedOriginalId == -1) {
        MessageBoxA(mainWindow,
                    "Select a student from the table before deleting.",
                    "Select Student", MB_OK | MB_ICONINFORMATION);
        return;
    }

    answer = MessageBoxA(mainWindow,
                         "Are you sure you want to delete this student?",
                         "Confirm Delete",
                         MB_YESNO | MB_ICONQUESTION);

    if (answer != IDYES)
        return;

    index = findStudentById(selectedOriginalId);

    if (index == -1)
        return;

    {
        int i;
        for (i = index; i < studentCount - 1; i++)
            students[i] = students[i + 1];
    }

    studentCount--;
    saveStudents();

    SetWindowTextA(editSearch, "");
    refreshTable();
    clearFields();
    setStatus("Student record deleted.");
}

void selectStudentFromTable(void)
{
    int row = ListView_GetNextItem(studentTable, -1, LVNI_SELECTED);
    char buffer[100];
    int id;

    if (row == -1)
        return;

    ListView_GetItemText(studentTable, row, 0, buffer, sizeof(buffer));
    id = atoi(buffer);

    {
        int index = findStudentById(id);

        if (index == -1)
            return;

        sprintf(buffer, "%d", students[index].id);
        SetWindowTextA(editID, buffer);

        SetWindowTextA(editName, students[index].name);

        sprintf(buffer, "%d", students[index].age);
        SetWindowTextA(editAge, buffer);

        SetWindowTextA(editCourse, students[index].course);

        selectedOriginalId = students[index].id;
        setStatus("Student selected. You can update or delete this record.");
    }
}

void exportCSV(void)
{
    FILE *file = fopen("students_export.csv", "w");
    int i;

    if (file == NULL) {
        MessageBoxA(mainWindow,
                    "Could not create the CSV file.",
                    "Export Error", MB_OK | MB_ICONERROR);
        return;
    }

    fprintf(file, "Student ID,Name,Age,Course\n");

    for (i = 0; i < studentCount; i++) {
        fprintf(file, "%d,\"%s\",%d,\"%s\"\n",
                students[i].id,
                students[i].name,
                students[i].age,
                students[i].course);
    }

    fclose(file);

    MessageBoxA(mainWindow,
                "Export complete!\n\nFile: students_export.csv",
                "Export Successful", MB_OK | MB_ICONINFORMATION);

    setStatus("Student records exported to students_export.csv.");
}

HWND makeLabel(HWND parent, const char *text,
               int x, int y, int width, int height)
{
    return CreateWindowA(
        "STATIC", text,
        WS_CHILD | WS_VISIBLE,
        x, y, width, height,
        parent, NULL, GetModuleHandle(NULL), NULL);
}

HWND makeEdit(HWND parent, int id,
              int x, int y, int width, int height)
{
    return CreateWindowA(
        "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        x, y, width, height,
        parent, (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL), NULL);
}

HWND makeButton(HWND parent, const char *text, int id,
                int x, int y, int width, int height)
{
    return CreateWindowA(
        "BUTTON", text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        x, y, width, height,
        parent, (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL), NULL);
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
    {
        INITCOMMONCONTROLSEX controls;

        controls.dwSize = sizeof(controls);
        controls.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&controls);

        titleFont = CreateFontA(
            28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        headingFont = CreateFontA(
            17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        normalFont = CreateFontA(
            15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        makeLabel(hwnd, "STUDENT HUB", 28, 18, 400, 40);
        SendMessageA(GetDlgItem(hwnd, -1), WM_SETFONT,
                     (WPARAM)titleFont, TRUE);

        makeLabel(hwnd, "STUDENT MANAGEMENT SYSTEM",
                  30, 58, 400, 24);

        makeLabel(hwnd, "STUDENT DETAILS",
                  30, 105, 250, 28);

        makeLabel(hwnd, "Student ID", 30, 145, 120, 22);
        editID = makeEdit(hwnd, ID_ID, 30, 169, 200, 30);

        makeLabel(hwnd, "Full Name", 260, 145, 160, 22);
        editName = makeEdit(hwnd, ID_NAME, 260, 169, 270, 30);

        makeLabel(hwnd, "Age", 560, 145, 80, 22);
        editAge = makeEdit(hwnd, ID_AGE, 560, 169, 100, 30);

        makeLabel(hwnd, "Course", 690, 145, 100, 22);
        editCourse = makeEdit(hwnd, ID_COURSE, 690, 169, 270, 30);

        makeButton(hwnd, "ADD STUDENT", ID_ADD,
                   30, 220, 145, 36);
        makeButton(hwnd, "UPDATE", ID_UPDATE,
                   185, 220, 110, 36);
        makeButton(hwnd, "DELETE", ID_DELETE,
                   305, 220, 105, 36);
        makeButton(hwnd, "CLEAR FORM", ID_CLEAR,
                   420, 220, 120, 36);

        makeLabel(hwnd, "STUDENT DIRECTORY",
                  30, 285, 300, 28);

        makeLabel(hwnd, "Search by ID, name or course:",
                  30, 322, 245, 25);
        editSearch = makeEdit(hwnd, 0, 275, 318, 310, 30);

        makeButton(hwnd, "SEARCH", ID_SEARCH,
                   595, 317, 100, 34);
        makeButton(hwnd, "SHOW ALL", ID_SHOWALL,
                   705, 317, 110, 34);
        makeButton(hwnd, "EXPORT CSV", ID_EXPORT,
                   825, 317, 135, 34);

        studentTable = CreateWindowA(
            WC_LISTVIEWA, "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            30, 365, 930, 235,
            hwnd, (HMENU)(INT_PTR)ID_TABLE,
            GetModuleHandle(NULL), NULL);

        ListView_SetExtendedListViewStyle(
            studentTable,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        addTableColumn(0, "Student ID", 130);
        addTableColumn(1, "Name", 320);
        addTableColumn(2, "Age", 100);
        addTableColumn(3, "Course", 340);

        labelTotal = makeLabel(hwnd, "Total students: 0",
                               30, 610, 250, 25);
        statusLabel = makeLabel(hwnd,
                                "Ready. Enter student details to begin.",
                                30, 640, 900, 25);

        {
            HWND controlsToStyle[] = {
                editID, editName, editAge, editCourse, editSearch,
                studentTable
            };
            int i;
            for (i = 0; i < 6; i++)
                SendMessageA(controlsToStyle[i], WM_SETFONT,
                             (WPARAM)normalFont, TRUE);
        }

        loadStudents();
        refreshTable();
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_ADD:
            addStudent();
            return 0;

        case ID_UPDATE:
            updateStudent();
            return 0;

        case ID_DELETE:
            deleteStudent();
            return 0;

        case ID_CLEAR:
            clearFields();
            return 0;

        case ID_SEARCH:
            refreshTable();
            setStatus("Search results updated.");
            return 0;

        case ID_SHOWALL:
            SetWindowTextA(editSearch, "");
            refreshTable();
            setStatus("Showing all student records.");
            return 0;

        case ID_EXPORT:
            exportCSV();
            return 0;
        }
        break;

    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == ID_TABLE) {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED) {
                LPNMLISTVIEW change = (LPNMLISTVIEW)lParam;

                if ((change->uNewState & LVIS_SELECTED) &&
                    !(change->uOldState & LVIS_SELECTED)) {
                    selectStudentFromTable();
                }
            }

            if (header->code == NM_DBLCLK) {
                selectStudentFromTable();
            }
        }
        return 0;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, WHITE);
        SetBkColor(hdc, DARK_PURPLE);
        return (LRESULT)backgroundBrush;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(35, 24, 58));
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
        saveStudents();

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
    windowClass.lpszClassName = "StudentHubWindow";
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = backgroundBrush;

    if (!RegisterClassA(&windowClass)) {
        MessageBoxA(NULL, "Could not register the application window.",
                    "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    mainWindow = CreateWindowA(
        "StudentHubWindow",
        "Student Hub | Management Dashboard",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1010, 730,
        NULL, NULL, instance, NULL);

    if (mainWindow == NULL) {
        MessageBoxA(NULL, "Could not open the application window.",
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
