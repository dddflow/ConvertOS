// Эта инструкция обязательно должна быть первой, т.к. этот код компилируется в бинарный,
// и загрузчик передает управление по адресу первой инструкции бинарного образа ядра ОС
extern "C" int kmain();

__declspec(naked) void startup()
{
    __asm  {
        call kmain;
    }
}

#define VIDEO_BUF_PTR (0xb8000)
#define COLOR_ADR (0xccc)
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80) // Ширина текстового экрана
#define PIC1_PORT (0x20)

#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)
#define GDT_CS (0x8) // Селектор секции кода, установленный загрузчиком ОС

#define gray_clr (0x01)
#define white_clr (0x02)
#define yellow_clr (0x03)
#define blue_clr (0x04)
#define red_clr (0x05)
#define green_clr (0x06)

#define KEYS_NUM 59
#define INT_MAX 2147483647
#define LL_MAX "9223372036854775807"


int strnum = 0;
int posnum = 0;
const char* start_str = "#";
bool shift = false;
char cmd[41];
int cmd_cnt = 0;

char scan_codes[] =
{
        '\0','\0',
        '1','2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
        '\0' /*Backspace*/,'\0' /*Tab*/,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
        '\0' /*Enter*/,'\0' /*Ctrl left*/,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'','`',
        '\0' /*Shift left*/,'\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
        0 /*Shift right*/,'\0' /*nop*/,'\0' /*nop*/, ' ' /*Space*/,'\0' /*CapsLock*/
};

#pragma pack(push, 1) // Выравнивание членов структуры запрещено
struct idt_entry  // Структура описывает данные об обработчике прерывания
{
    unsigned short base_lo; // Младшие биты адреса обработчика
    unsigned short segm_sel; // Селектор сегмента кода
    unsigned char always0; // Этот байт всегда 0
    unsigned char flags; // Флаги тип. Флаги: P, DPL, Типы - это константы - IDT_TYPE...
    unsigned short base_hi; // Старшие биты адреса обработчика
};
struct idt_ptr     // Структура, адрес которой передается как аргумент команды lidt
{
    unsigned short limit;
    unsigned int base;
};
#pragma pack(pop)

struct idt_entry g_idt[256]; // Реальная таблица IDT
struct idt_ptr g_idtp; // Описатель таблицы для команды lidt

__inline unsigned char inb (unsigned short port); // чтение в порт ввода-вывода
__inline void outb (unsigned short port, unsigned char data); // запись в порт ввода-вывода

void on_key (unsigned char scan_code); // определение нажатой клавиши
void backspace_on_key (); // нажата клавиша backspace
void enter_on_key (); // нажата клавиша enter
void keyb_process_keys(); // считывание скан-кода клавиши для PS/2 клавиатуры
void keyb_init(); // регистрирует обработчик прерывания клавиатуры и разрешает контроллеру прерываний его вызывать в случае нажатия пользователем клавиши клавиатуры
/*__declspec(naked)*/ void keyb_handler(); // Обработчик прерываний

/*__declspec(naked)*/ void default_intr_handler();  // Пустой обработчик прерываний. Другие обработчики могут быть реализованы по этому шаблону
typedef void (*intr_handler)();
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr); // Регистрация необходимых обработчиков
void intr_init();     // Функция инициализации системы прерываний: заполнение массива с адресами обработчиков
void intr_start(); // Регистрация таблицы дескрипторов прерываний
void intr_enable(); // Включение прерываний
void intr_disable(); // Выключение прерываний

void cursor_moveto(unsigned int strnum, unsigned int pos); // Функция переводит курсор на строку strnum (0 – самая верхняя) в позицию pos на этой строке (0 – самое левое положение).
void out_str(int color, const char* ptr, unsigned int strnum); // Вывод строки
void out_chr(int color, unsigned char chr); // Вывод символа

int color_switch (); // Функция выбора цвета шрифта

bool cmdcmp (const char *str); // Проверка на соотвесвтие команде
void cmdclr (); // Очистка буфера команды
void info_cmd(); // Команда info
void shutdown_cmd(); // Команда shutdown
void clear_cmd(); // Команда clear
void posix_cmd(); // Команда posixtime
void wint_cmd(); // Команда wintime
void nsconv_cmd(); // Команда nsconv

void overflow_err(); // Ошибка переполнения int
void ll_overflow_err(); // Ошибка переполнения long long
void inns_err(); // Неправильное значение исходной сс
void outns_err(); // Неправильное значение конечной сс
void wrongdig_err(char d); // Неправильная цифра

int max (int n1, int n2); // Возвращает максимальное число
size_t strlen(const char* str); // Длина строки
int str_comprasion (char s1[41], char s2[41]); // Сравнение строк
void str_diff (char s1[41], char s2[41], char ans[41]); // Разность строк как чисел


int max (int n1, int n2)
{
    if (n1 > n2) return n1;
    else return n2;
}
size_t strlen(const char* str)
{
    size_t length = 0;
    while (*str++)
        length++;
    return length;
}
int str_comprasion (char s1[41], char s2[41])
{
    int n1 = (int)strlen(s1), n2 = (int)strlen(s2);
    if (s1[0] == '-' && s2[0] != '-') return -1;
    if (s1[0] != '-' && s2[0] == '-') return 1;
    if (s1[0] != '-' && s2[0] != '-')
    {
        if (n1 > n2) return 1;
        else if (n1 < n2) return -1;
        else
        {
            int n = n1;
            for (int i = 0; i < n; i++)
            {
                if (s1[i] > s2[i]) return 1;
                if (s1[i] < s2[i]) return -1;
            }
            return 0;
        }
    }
    if (s1[0] == '-' && s2[0] == '-')
    {
        if (n1 > n2) return -1;
        else if (n1 < n2) return 1;
        else
        {
            int n = n1;
            for (int i = 0; i < n; i++)
            {
                if (s1[i] > s2[i]) return -1;
                if (s1[i] < s2[i]) return 1;
            }
            return 0;
        }
    }
    return 0;
}
void str_diff (char s1[41], char s2[41], char ans[41])
{
    int n1 = (int)strlen(s1), n2 = (int)strlen(s2);
    int n = max(n1, n2) + 1;
    n1--;
    n2--;
    for (int i = 0; i < n; i++) ans[i] = '0';
    ans[n-1] = '\0';
    
    if (str_comprasion(s1, s2) == 0)
    {
        ans[0] = '0';
        ans[1] = '\0';
    }
    else
    {
        int t = 0;
        for (int i = n-1; i >= 0; i--)
        {
            if (n2 < 0)
            {
                if (s1[n1] + t >= '0')
                {
                    if (s1[n1] + t != '0') ans[n1] = s1[n1] + t;
                    t = 0;
                }
                else
                {
                    if (10 + s1[n1] + t != '0') ans[n1] = 10 + s1[n1] + t;
                    t = -1;
                }
            }
            else
            {
                if (s1[n1] + t >= s2[n2])
                {
                    ans[n1] = s1[n1] + t - s2[n2] + '0';
                    t = 0;
                }
                else
                {
                    ans[n1] = 10 + s1[n1] + t - s2[n2] + '0';
                    t = -1;
                }
            }
            n1--;
            n2--;
            if (n1 < 0 && n2 < 0) break;
        }
        ans[n] = '\0';
        
        int p = 0;
        while (ans[p] == '0' || ans[p] == '\0') p++;
        for (int i = 0; i <= n-p; i++) ans[i] = ans[i+p];
        for (int i = n-p+1; i <= n; i++) ans[i] = '\0';
    }
}


__inline unsigned char inb (unsigned short port)
{
    unsigned char data;
    __asm {
    push dx
    mov dx, port
    in al, dx
    mov data, al
    pop dx
    }
    return data;
}
__inline void outb (unsigned short port, unsigned char data)
{
    __asm {
    push dx
    mov dx, port
    mov al, data
    out dx, al
    pop dx
    }
}

void on_key (unsigned char scan_code)
{
    if (scan_code == 42 || scan_code == 54) shift = true;
    if (scan_code == 14) backspace_on_key();
    else if (scan_code == 28) enter_on_key();
    else {
        char c = scan_codes[(unsigned int) scan_code];
        if (c != '\0' && posnum <= 41)
        {
            if (shift && scan_code == 9)
            {
                out_chr(color_switch(), '*');
                cmd[cmd_cnt] = '*';
                cmd_cnt++;
            }
            else if (shift && scan_code == 13)
            {
                out_chr(color_switch(), '+');
                cmd[cmd_cnt] = '+';
                cmd_cnt++;
            }
            else
            {
                out_chr(color_switch(), c);
                cmd[cmd_cnt] = c;
                cmd_cnt++;
            }
        }
    }
}
void backspace_on_key()
{
    if (posnum >= 3)
    {
        unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
        video_buf += 2*(strnum * VIDEO_WIDTH + posnum - 1);
        video_buf[0] = '\0';
        cursor_moveto(strnum, --posnum);
        cmd_cnt--;
        cmd[cmd_cnt] = '\0';
    }
}
void enter_on_key()
{
    if (strnum >= 23) clear_cmd();

    if (cmdcmp("info")) info_cmd();
    else if (cmdcmp("shutdown")) shutdown_cmd();
    else if (cmdcmp("clear")) clear_cmd();
    else if (cmdcmp("posixtime")) posix_cmd();
    else if (cmdcmp("wintime")) wint_cmd();
    else if (cmdcmp("nsconv")) nsconv_cmd();
    else
    {
        strnum++;
        out_str(color_switch(), "command not found", strnum);
    }

    strnum++;
    posnum = 0;
    out_chr(color_switch(), '#');
    posnum++;
    cursor_moveto(strnum, posnum);

    cmdclr();
}

void info_cmd()
{
    if (strnum >= 21) clear_cmd();

    strnum++;
    out_str(color_switch(), "ConvertOS: v.01. Developer: Danilov Denis, 4851003/00002, SpbPU, 2022", strnum);

    strnum++;
    out_str(color_switch(), "Compilers: bootloader: FASM, kernel: Microsoft C Compiler", strnum);

    strnum++;
    switch (*(unsigned char*)COLOR_ADR) {
        case gray_clr:
            out_str(color_switch(), "Bootloader parameters: gray color.", strnum);
            break;
        case white_clr:
            out_str(color_switch(), "Bootloader parameters: white color.", strnum);
            break;
        case yellow_clr:
            out_str(color_switch(), "Bootloader parameters: yellow color.", strnum);
            break;
        case blue_clr:
            out_str(color_switch(), "Bootloader parameters: blue color.", strnum);
            break;
        case red_clr:
            out_str(color_switch(), "Bootloader parameters: red color.", strnum);
            break;
        case green_clr:
            out_str(color_switch(), "Bootloader parameters: green color.", strnum);
            break;
    }
}
void shutdown_cmd()
{
    strnum++;
    const char* pwr = "Powering off...";
    out_str(color_switch(), pwr, strnum);
    __asm
    {
        push dx
        mov dx, 0x604
        mov ax, 0x2000
        out dx, ax
        pop dx
    }
}
void clear_cmd()
{
    for (strnum = 0; strnum < 25; strnum++)
        for (posnum = 0; posnum < 80;)
            out_chr(color_switch(), ' ');

    posnum = 0;
    strnum = -1;
}
void posix_cmd()
{
    int time = 0;
    for (int i = 10; i < cmd_cnt; i++)
    {
        if (time - '0' > INT_MAX - cmd[i] || time > (INT_MAX - cmd[i] + '0') / 10)
        {
            overflow_err();
            return;
        }
        else time = time * 10 + cmd[i] - '0';
    }
    
    int year = 1970, month = 1, day = 1, hour = 0, min = 0, sec = 0;
    
    int yr = 31536000, leap_yr = 31622400;
    int years4 = 3*yr + leap_yr;
    year += (time/years4)*4;
    time %= years4;
    
    bool leap = false;
    if (time >= yr)
    {
        time -= yr;
        year++;
    }
    if (time >= yr)
    {
        time -= yr;
        year++;
        leap = true;
    }
    if (time >= leap_yr)
    {
        time -= leap_yr;
        year++;
        leap = false;
    }
    
    int mnth[12] = {31*86400, 28*86400, 31*86400, 30*86400, 31*86400, 30*86400, 31*86400, 31*86400, 30*86400, 31*86400, 30*86400, 31*86400};
    int leap_mnth[12] = {31*86400, 29*86400, 31*86400, 30*86400, 31*86400, 30*86400, 31*86400, 31*86400, 30*86400, 31*86400, 30*86400, 31*86400};
    
    int mnth_num = 0;
    if (!leap)
    {
        for (int i = 0; i < 12; i++)
        {
            if (time < mnth[i])    break;
            time -= mnth[i];
            month++;
        }
    }
    else
    {
        for (int i = 0; i < 12; i++)
        {
            if (time < leap_mnth[i]) break;
            time -= leap_mnth[i];
            month++;
        }
    }
    
    while (time >= 86400)
    {
        time -= 86400;
        day++;
    }
    
    while (time >= 3600)
    {
        time -= 3600;
        hour++;
    }
    
    while (time >= 60)
    {
        time -= 60;
        min++;
    }
    
    sec = time;
    
    char ans[30];
    for (int i = 0; i < 30; i++) ans[i] = '\0';
    
    ans[1] = day % 10 + '0';
    day /= 10;
    ans[0] = day % 10 + '0';
    ans[2] = '.';
    
    ans[4] = month % 10 + '0';
    month /= 10;
    ans[3] = month % 10 + '0';
    ans[5] = '.';
    
    ans[9] = year % 10 + '0';
    year /= 10;
    ans[8] = year % 10 + '0';
    year /= 10;
    ans[7] = year % 10 + '0';
    year /= 10;
    ans[6] = year % 10 + '0';
    ans[10] = ' ';
    
    ans[12] = hour % 10 + '0';
    hour /= 10;
    ans[11] = hour % 10 + '0';
    ans[13] = ':';
    
    ans[15] = min % 10 + '0';
    min /= 10;
    ans[14] = min % 10 + '0';
    ans[16] = ':';
    
    ans[18] = sec % 10 + '0';
    sec /= 10;
    ans[17] = sec % 10 + '0';

    strnum++;
    out_str(color_switch(), ans, strnum);
}
void wint_cmd()
{
    char time_c[41], time_c0[41];;
    for (int i = 0; i < 41; i++)
    {
        time_c[i] = '\0';
        time_c0[i] = '\0';
    }
    for (int i = 8; i < cmd_cnt-7; i++)
        time_c[i-8] = cmd[i];
    for (int i = 8; i < cmd_cnt; i++)
        time_c0[i-8] = cmd[i];
    
    //out_str(color_switch(), time_c0, 20);
    //out_str(color_switch(), LL_MAX, 21);
    
    if (str_comprasion(time_c0, LL_MAX) == 1)
    {
        ll_overflow_err();
        return;
    }
    
    int year = 1601, month = 1, day = 1, hour = 0, min = 0, sec = 0;
    bool leap = false;
    
    while (true)
    {
        for (int i = 0; i < 41; i++) time_c0[i] = '\0';
        
        char *yr = "31536000", *leap_yr = "31622400";
        if ((year % 4 == 0 && year % 100 != 0) ||(year % 400 == 0))
        {
            leap = true;
            if (str_comprasion(time_c, leap_yr) == -1) break;
            str_diff(time_c, leap_yr, time_c0);
            year++;
        }
        else
        {
            leap = false;
            if (str_comprasion(time_c, yr) == -1) break;
            str_diff(time_c, yr, time_c0);
            year++;
        }
        
        for (int i = 0; i < 41; i++) time_c[i] = time_c0[i];
    }
    
    int time = 0;
    for (int i = 0; i < strlen(time_c); i++)
    {
            time *= 10;
            time += time_c[i] - '0';
    }
    
    int mnth[12] = {31*86400, 28*86400, 31*86400, 30*86400, 31*86400, 30*86400, 31*86400, 31*86400, 30*86400, 31*86400, 30*86400, 31*86400};
    int leap_mnth[12] = {31*86400, 29*86400, 31*86400, 30*86400, 31*86400, 30*86400, 31*86400, 31*86400, 30*86400, 31*86400, 30*86400, 31*86400};
    
    int mnth_num = 0;
    if (!leap)
    {
        for (int i = 0; i < 12; i++)
        {
            if (time < mnth[i])    break;
            time -= mnth[i];
            month++;
        }
    }
    else
    {
        for (int i = 0; i < 12; i++)
        {
            if (time < leap_mnth[i]) break;
            time -= leap_mnth[i];
            month++;
        }
    }
    
    while (time >= 86400)
    {
        time -= 86400;
        day++;
    }
    
    while (time >= 3600)
    {
        time -= 3600;
        hour++;
    }
    
    while (time >= 60)
    {
        time -= 60;
        min++;
    }
    
    sec = time;

    char ans[30];
    for (int i = 0; i < 30; i++) ans[i] = '\0';
    
    ans[1] = day % 10 + '0';
    day /= 10;
    ans[0] = day % 10 + '0';
    ans[2] = '.';
    
    ans[4] = month % 10 + '0';
    month /= 10;
    ans[3] = month % 10 + '0';
    ans[5] = '.';
    
    if (year < 10000)
    {
        ans[9] = year % 10 + '0';
        year /= 10;
        ans[8] = year % 10 + '0';
        year /= 10;
        ans[7] = year % 10 + '0';
        year /= 10;
        ans[6] = year % 10 + '0';
        ans[10] = ' ';
        
        ans[12] = hour % 10 + '0';
        hour /= 10;
        ans[11] = hour % 10 + '0';
        ans[13] = ':';
        
        ans[15] = min % 10 + '0';
        min /= 10;
        ans[14] = min % 10 + '0';
        ans[16] = ':';
        
        ans[18] = sec % 10 + '0';
        sec /= 10;
        ans[17] = sec % 10 + '0';
    }
    else
    {
        ans[10] = year % 10 + '0';
        year /= 10;
        ans[9] = year % 10 + '0';
        year /= 10;
        ans[8] = year % 10 + '0';
        year /= 10;
        ans[7] = year % 10 + '0';
        year /= 10;
        ans[6] = year % 10 + '0';
        ans[11] = ' ';
        
        ans[13] = hour % 10 + '0';
        hour /= 10;
        ans[12] = hour % 10 + '0';
        ans[14] = ':';
        
        ans[16] = min % 10 + '0';
        min /= 10;
        ans[15] = min % 10 + '0';
        ans[17] = ':';
        
        ans[19] = sec % 10 + '0';
        sec /= 10;
        ans[18] = sec % 10 + '0';
    }

    strnum++;
    out_str(color_switch(), ans, strnum);
}
void nsconv_cmd()
{
    int n = 0, n_len = 7, i = 7;
    for (;cmd[i] != ' ' && cmd[i] != '\0'; i++)
        n_len++;
    n_len--;

    int in_ns = 0;
    i++;
    for (; cmd[i] != ' ' && cmd[i] != '\0'; i++)
    {
        if (in_ns - '0' > INT_MAX - cmd[i] || in_ns > (INT_MAX - cmd[i] + '0') / 10)
        {
            overflow_err();
            return;
        }
        else in_ns = in_ns * 10 + cmd[i] - '0';
    }
    if (in_ns > 36 || in_ns < 2)
    {
        inns_err();
        return;
    }

    int out_ns = 0;
    i++;
    for (; cmd[i] != ' ' && cmd[i] != '\0'; i++)
    {
        if (out_ns - '0' > INT_MAX - cmd[i] || out_ns > (INT_MAX - cmd[i] + '0') / 10)
        {
            overflow_err();
            return;
        }
        else out_ns = out_ns * 10 + cmd[i] - '0';
    }
    if (out_ns > 36 || out_ns < 2)
    {
        outns_err();
        return;
    }

    int n10 = 0, cnt = 1;
    for (i = n_len; cmd[i] != ' ' && cmd[i] != '\0'; i--)
    {
        if (cmd[i] - '0' >= 0 && cmd[i] - '0' < 10)
        {
            if (cmd[i] - '0' >= in_ns)
            {
                wrongdig_err(cmd[i]);
                return;
            }
            if (n10 > INT_MAX - (cmd[i] + '0')*cnt)
            {
                overflow_err();
                return;
            }
            else n10 += (cmd[i] - '0') * cnt;
        }
       else
        {
            if (cmd[i] - 'a' + 10 >= in_ns)
            {
                wrongdig_err(cmd[i]);
                return;
            }
            if (n10 > INT_MAX - (cmd[i] + 'a' - 10)*cnt)
            {
                overflow_err();
                return;
            }
            else n10 += (cmd[i] - 'a' + 10) * cnt;
        }
        cnt *= in_ns;
    }

    char ans[33];
    for (int i = 0; i < 33; i++) ans[i] = '\0';
    cnt = 0;
    while (n10 > 0)
    {
        if (n10 % out_ns >= 0 && n10 % out_ns < 10)
            ans[cnt] = n10 % out_ns + '0';
        else
            ans[cnt] = n10 % out_ns + 'a' - 10;
        n10 /= out_ns;
        cnt++;
    }
    cnt--;

    for (int j = 0; j <= cnt/2; j++)
    {
        char temp = ans[j];
        ans[j] = ans[cnt-j];
        ans[cnt-j] = temp;
    }

    strnum++;
    out_str(color_switch(), ans, strnum);

}

void overflow_err()
{
    if (strnum >= 23) clear_cmd();
    strnum++;
    out_str(color_switch(), "error: int overflow", strnum);
}
void ll_overflow_err()
{
    if (strnum >= 23) clear_cmd();
    strnum++;
    out_str(color_switch(), "error: long long overflow", strnum);
}
void inns_err()
{
    if (strnum >= 23) clear_cmd();
    strnum++;
    out_str(color_switch(), "error: wrong initial number system", strnum);
}
void outns_err()
{
    if (strnum >= 23) clear_cmd();
    strnum++;
    out_str(color_switch(), "error: wrong final number system", strnum);
}
void wrongdig_err(char d)
{
    if (strnum >= 23) clear_cmd();
    char err[30];
    for (int i = 0; i < 30; i++)
        err[i] = '\0';
    
    char* msg  = "error: wrong digit : ";
    for (int i = 0; i < 22; i++) err[i] = msg[i];
    err[21] = d;
    
    strnum++;
    out_str(color_switch(), err, strnum);
}

void keyb_process_keys()
{
    // Проверка что буфер PS/2 клавиатуры не пуст (младший бит присутствует)
    if (inb(0x64) & 0x01)
    {
        unsigned char scan_code = inb(0x60); // Считывание символа с PS/2 клавиатуры
        if (scan_code < 128) // Скан-коды выше 128 - это отпускание клавиши
            on_key(scan_code);
        if (scan_code == 170 || scan_code == 182) shift = false;
    }
}
void keyb_init()
{
    // Регистрация обработчика прерывания
    intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler);
    // segm_sel=0x8, P=1, DPL=0, Type=Intr
    // Разрешение только прерываний клавиатуры от контроллера 8259
    outb(PIC1_PORT + 1, 0xFF ^ 0x02); // 0xFF - все прерывания, 0x02 - бит IRQ1 (клавиатура).
    // Разрешены будут только прерывания, чьи биты установлены в 0
}

__declspec(naked) void keyb_handler()
{
    __asm pusha;
    // Обработка поступивших данных
    keyb_process_keys();
    // Отправка контроллеру 8259 нотификации о том, что прерывание
    //обработано. Если не отправлять нотификацию, то контроллер не будет посылать
    //новых сигналов о прерываниях до тех пор, пока ему не сообщать что
    //прерывание обработано.
    outb(PIC1_PORT, 0x20);
    __asm {
    popa
    iretd
    }
}
__declspec(naked) void default_intr_handler()
{
    __asm {
    pusha
    }
    // ... (реализация обработки)
    __asm {
    popa
    iretd
    }
}

void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr)
{
    unsigned int hndlr_addr = (unsigned int) hndlr;
    g_idt[num].base_lo = (unsigned short) (hndlr_addr & 0xFFFF);
    g_idt[num].segm_sel = segm_sel;
    g_idt[num].always0 = 0;
    g_idt[num].flags = flags;
    g_idt[num].base_hi = (unsigned short) (hndlr_addr >> 16);
}
void intr_init()
{
    int i;
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
    for(i = 0; i < idt_count; i++)
        intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,default_intr_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
}
void intr_start()
{
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
    g_idtp.base = (unsigned int) (&g_idt[0]);
    g_idtp.limit = (sizeof (struct idt_entry) * idt_count) - 1;
    __asm {
    lidt g_idtp
    }
    //__lidt(&g_idtp);
}
void intr_enable()
{
    __asm sti;
}
void intr_disable()
{
    __asm cli;
}

int color_switch ()
{
    int color = 0x07;
    switch (*(unsigned char*)COLOR_ADR)
    {
        case gray_clr:
            color = 0x07; // ok
        break;
        case white_clr:
            color = 0x0F; // ok
        break;
        case yellow_clr:
            color = 0x0E;
        break;
        case blue_clr:
            color = 0x0B; // mb ok
        break;
        case red_clr:
            color = 0x0C; // mb ok
        break;
        case green_clr:
            color = 0x0A; // ok
        break;
    }

    return color;
}

bool cmdcmp (const char *str)
{
    bool ans = true;
    int i = 0;
    while (str[i] != '\0')
    {
        if (cmd[i] != str[i] || i == 39)
        {
            ans = false;
            break;
        }
        i++;
    }
    if (cmd[i] != ' ' && cmd[i] != '\0') ans = false;
    return ans;
}
void cmdclr ()
{
    for (int i = 0; i < 41; i++) cmd[i] = '\0';
    cmd_cnt = 0;
}

void cursor_moveto(unsigned int strnum, unsigned int pos)
{
    unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
        outb(CURSOR_PORT, 0x0F);
        outb(CURSOR_PORT + 1, (unsigned char) (new_pos & 0xFF));
        outb(CURSOR_PORT, 0x0E);
        outb(CURSOR_PORT + 1, (unsigned char) ((new_pos >> 8) & 0xFF));
}

void out_str(int color, const char* ptr, unsigned int strnum)
{
    unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
    video_buf += 80*2 * strnum;
    while (*ptr)
    {
        video_buf[0] = (unsigned char) *ptr;
        video_buf[1] = color;
        video_buf += 2;
        ptr++;
    }
}
void out_chr(int color, unsigned char chr)
{
    unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
    video_buf += 2*(strnum * VIDEO_WIDTH + posnum);
    video_buf[0] = chr;
    video_buf[1] = color;
    cursor_moveto(strnum, posnum);
    posnum++;
}


extern "C" int kmain()
{
    const char* hello = "Welcome to ConvertOS!";
    int color = color_switch();

    cmdclr();

    out_str(color, hello, 0);
    out_str(color, start_str, 1);
    strnum = 1;
    posnum = 2;
    cursor_moveto(strnum, posnum);

    intr_disable();
    intr_init();
    keyb_init();
    intr_start();
    intr_enable();

    while(1)
    {
        __asm hlt;
    }

    return 0;
}
