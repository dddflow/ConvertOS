use16 ;������� � 16-������ �����
org 0x7C00 ;�������� ������ �������� ����

start: 	; ������������� ������� ���������. ��� �������� ��������� �� ��� ������ BIOS, �� �� ������������� ���������.
	mov ax, cs ; ���������� ������ �������� ���� � ax
	mov ds, ax ; ���������� ����� ������ ��� ������ �������� ������
	mov ss, ax ; � �������� �����
	mov sp, start ; ���������� ������ ����� ��� ����� ������ ���������� ����� ����. ���� ����� ����� ����� � �� ��������� ���.

    mov cx, 0x01 ; �����, ����������� ����� �����   
    
list:
    mov ax, 0x0002
	int 0x10 
    
    mov al, 0x0a ; ������� ������� �� ��������� ������
    mov ah, 0x0e
    int 0x10
    
    mov edi, 0xb8000 ; � ������� edi ���������� ����� ������ ������ �����������.
    
    ; ����� ����� 
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
    mov esi, gray ; � ������� esi ���������� ����� ������ ������
    call video_puts ; ���������� ������� ��� ����������� � ����� ������ ����������           
    ret
    
white_font:    
    mov esi, white ; � ������� esi ���������� ����� ������ ������
    call video_puts ; ���������� ������� ��� ����������� � ����� ������ ����������           
    ret      
    
yellow_font:    
    mov esi, yellow ; � ������� esi ���������� ����� ������ ������
    call video_puts ; ���������� ������� ��� ����������� � ����� ������ ����������           
    ret    

blue_font:    
    mov esi, blue ; � ������� esi ���������� ����� ������ ������
    call video_puts ; ���������� ������� ��� ����������� � ����� ������ ����������           
    ret 

red_font:    
    mov esi, red ; � ������� esi ���������� ����� ������ ������
    call video_puts ; ���������� ������� ��� ����������� � ����� ������ ����������           
    ret 
    
green_font:    
    mov esi, green ; � ������� esi ���������� ����� ������ ������
    call video_puts ; ���������� ������� ��� ����������� � ����� ������ ����������           
    ret 
     
check:
    mov ah, 0x00 ; ���������� ��� ���������� � �������� �����
    int 0x16
    cmp ah, 0x50
    je up_arrow 
    cmp ah, 0x48
    je down_arrow
    cmp ah, 0x1c ; ���� enter ��������� � ����
    je kernel

j:  jmp list 

up_arrow:  ; ������ ������� ������� �����
    add cx, 0x01
    cmp cx, 0x07
    jne j
    mov cx, 0x01
    jmp j
    
down_arrow:  ; ������ ������� ������� ����
    sub cx, 0x01
    cmp cx, 0x00
    jne j
    mov cx, 0x06
    jmp j    
       
    
video_puts:
    ; ������� ������� � ����� ����������� (���������� � edi) ������, �������������� 0 (���������� � esi)
    ; ����� ���������� edi �������� ����� �� �������� ����� ���������� ����� ��������� �����
    mov al, [esi]
    test al, al
    jz video_puts_end
    mov ah, 0x07 ; ���� ������� � ����. ��������� ��������: 0x00 is black-on-black, 0x07 is lightgrey-on-black, 0x1F is white-on-blue
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
	mov es, ax ; ������ ������ � ������ es	
    
    mov [0xccc], cx
	
	mov bx, 0x1000 ; �����
	mov dl, 1 ; ����� �����
	mov dh, 0 ; ����� �������
	mov ch, 0 ; ����� ��������
	mov cl, 2 ; ����� ��������
	mov al, 18 ; ���������� ��������
	mov ah, 0x02 ; ���������� ��������� ���������� �������� � ����� � ������
	int 0x13
    
	mov bx, 0x4000 ; �����
	mov dl, 1 ; ����� �����
	mov dh, 0 ; ����� �������
	mov ch, 0 ; ����� ��������
	mov cl, 20 ; ����� ��������
	mov al, 6 ; ���������� ��������
	mov ah, 0x02 ; ���������� ��������� ���������� �������� � ����� � ������
	int 0x13
	
	
	cli ; ���������� ����������
	; �������� ������� � ������ ������� ������������
	lgdt [gdt_info]
	; ��������� �������� ����� �20
	in al, 0x92
	or al, 2
	out 0x92, al
	; ��������� ���� PE �������� CR0 - ��������� �������� � ���������� �����
	mov eax, cr0
	or al, 1
	mov cr0, eax
	jmp 0x8:protected_mode ; "�������" ������� ��� �������� ���������� ���������� � cs (������������� ����������� �� ��������� ����� ������� ��������).

gdt:
	; ������� ����������
	db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	; ������� ����: base=0, size=4Gb, P=1, DPL=0, S=1(user),
	; Type=1(code), Access=00A, G=1, B=32bit
	db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
	; ������� ������: base=0, size=4Gb, P=1, DPL=0, S=1(user),
	; Type=0(data), Access=0W0, G=1, B=32bit
	db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_info: ; ������ � ������� GDT (������, ��������� � ������)
	dw gdt_info - gdt ; ������ ������� (2 �����)
	dw gdt, 0 ; 32-������ ���������� ����� �������.   
  

use32
protected_mode:
	; �������� ���������� ��������� ��� ����� � ������ � ��������
	mov ax, 0x10 ; ������������ ���������� � ������� 2 � GDT
	mov es, ax
	mov ds, ax
	mov ss, ax
	; �������� ���������� ������������ ����
	call 0x10000 ; ����� ����� ������ �������� � ������ ���� ���� �������������� � "�������" ���

	times (512 - ($ - start) - 2) db 0 ; ���������� ������ �� ������� 512 - 2 ������� �����
	db 0x55, 0xAA ; 2 ����������� ����� ����� ������ �������� �����������