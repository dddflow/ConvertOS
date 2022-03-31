use16 ;переход в 16-битный режим
org 0x7C00 ;указание адреса загрузки кода

start: 	; Инициализация адресов сегментов. Эти операции требуется не для любого BIOS, но их рекомендуется проводить.
	mov ax, cs ; Сохранение адреса сегмента кода в ax
	mov ds, ax ; Сохранение этого адреса как начало сегмента данных
	mov ss, ax ; И сегмента стека
	mov sp, start ; Сохранение адреса стека как адрес первой инструкции этого кода. Стек будет расти вверх и не перекроет код.

    mov cx, 0x01 ; номер, сохраняющий выбор цвета   
    
list:
    mov ax, 0x0002
	int 0x10 
    
    mov al, 0x0a ; перевод курсора на следующую строку
    mov ah, 0x0e
    int 0x10
    
    mov edi, 0xb8000 ; В регистр edi помещается адрес начала буфера видеопамяти.
    
    ; выбор цвета 
    cmp cx, 0x01
    je gray_font
    cmp cx, 0x02
    je white_font
    cmp cx, 0x03
    je yellow_font
    cmp cx, 0x04
    je blue_font
    cmp cx, 0x05
    je red_font
    cmp cx, 0x06
    je green_font
    
gray_font:    
    mov esi, gray ; В регистр esi помещается адрес начала строки
    call video_puts ; Вызывается функция для видеовывода в буфер памяти видеокарты           
    ret
    
white_font:    
    mov esi, white ; В регистр esi помещается адрес начала строки
    call video_puts ; Вызывается функция для видеовывода в буфер памяти видеокарты           
    ret      
    
yellow_font:    
    mov esi, yellow ; В регистр esi помещается адрес начала строки
    call video_puts ; Вызывается функция для видеовывода в буфер памяти видеокарты           
    ret    

blue_font:    
    mov esi, blue ; В регистр esi помещается адрес начала строки
    call video_puts ; Вызывается функция для видеовывода в буфер памяти видеокарты           
    ret 

red_font:    
    mov esi, red ; В регистр esi помещается адрес начала строки
    call video_puts ; Вызывается функция для видеовывода в буфер памяти видеокарты           
    ret 
    
green_font:    
    mov esi, green ; В регистр esi помещается адрес начала строки
    call video_puts ; Вызывается функция для видеовывода в буфер памяти видеокарты           
    ret 
     
check:
    mov ah, 0x00 ; прерывание для считывания и ожидание ввода
    int 0x16
    cmp ah, 0x50
    je up_arrow 
    cmp ah, 0x48
    je down_arrow
    cmp ah, 0x1c ; если enter переходим к ядру
    je kernel

j:  jmp list 

up_arrow:  ; случай нажатия стрелки вверх
    add cx, 0x01
    cmp cx, 0x07
    jne j
    mov cx, 0x01
    jmp j
    
down_arrow:  ; случай нажатия стрелки вниз
    sub cx, 0x01
    cmp cx, 0x00
    jne j
    mov cx, 0x06
    jmp j    
       
    
video_puts:
    ; Функция выводит в буфер видеопамяти (передается в edi) строку, оканчивающуюся 0 (передается в esi)
    ; После завершения edi содержит адрес по которому можно продолжать вывод следующих строк
    mov al, [esi]
    test al, al
    jz video_puts_end
    mov ah, 0x07 ; Цвет символа и фона. Возможные варианты: 0x00 is black-on-black, 0x07 is lightgrey-on-black, 0x1F is white-on-blue
    mov [edi], al
    mov [edi+1], ah
    add edi, 2
    add esi, 1
    jmp video_puts
    
video_puts_end:
    jmp check
    
gray:               ; 0x01
    db "Gray", 0    
white:              ; 0x02
    db "White", 0    
yellow:             ; 0x03
    db "Yellow", 0 
blue:               ; 0x04
    db "Blue", 0    
red:                ; 0x05
    db "Red", 0
green:              ; 0x06
    db "Green", 0 

kernel:
	mov ax, 0x1000
	mov es, ax ; запись адреса в реистр es	
    
    mov [0xccc], cx
	
	mov bx, 0x1000 ; адрес
	mov dl, 1 ; номер диска
	mov dh, 0 ; номер головки
	mov ch, 0 ; номер цилиндра
	mov cl, 2 ; номер сегмента
	mov al, 18 ; количсетво секторов
	mov ah, 0x02 ; считывание заданного количества секторов с диска в память
	int 0x13
    
	mov bx, 0x4000 ; адрес
	mov dl, 1 ; номер диска
	mov dh, 0 ; номер головки
	mov ch, 0 ; номер цилиндра
	mov cl, 20 ; номер сегмента
	mov al, 6 ; количсетво секторов
	mov ah, 0x02 ; считывание заданного количества секторов с диска в память
	int 0x13
	
	
	cli ; Отключение прерываний
	; Загрузка размера и адреса таблицы дескрипторов
	lgdt [gdt_info]
	; Включение адресной линии А20
	in al, 0x92
	or al, 2
	out 0x92, al
	; Установка бита PE регистра CR0 - процессор перейдет в защищенный режим
	mov eax, cr0
	or al, 1
	mov cr0, eax
	jmp 0x8:protected_mode ; "Дальний" переход для загрузки корректной информации в cs (архитектурные особенности не позволяют этого сделать напрямую).

gdt:
	; Нулевой дескриптор
	db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	; Сегмент кода: base=0, size=4Gb, P=1, DPL=0, S=1(user),
	; Type=1(code), Access=00A, G=1, B=32bit
	db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
	; Сегмент данных: base=0, size=4Gb, P=1, DPL=0, S=1(user),
	; Type=0(data), Access=0W0, G=1, B=32bit
	db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_info: ; Данные о таблице GDT (размер, положение в памяти)
	dw gdt_info - gdt ; Размер таблицы (2 байта)
	dw gdt, 0 ; 32-битный физический адрес таблицы.   
  

use32
protected_mode:
	; Загрузка селекторов сегментов для стека и данных в регистры
	mov ax, 0x10 ; Используется дескриптор с номером 2 в GDT
	mov es, ax
	mov ds, ax
	mov ss, ax
	; Передача управления загруженному ядру
	call 0x10000 ; Адрес равен адресу загрузки в случае если ядро скомпилировано в "плоский" код

	times (512 - ($ - start) - 2) db 0 ; Заполнение нулями до границы 512 - 2 текущей точки
	db 0x55, 0xAA ; 2 необходимых байта чтобы сектор считался загрузочным