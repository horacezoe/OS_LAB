section .data
ErrMessage:         db  'Error',0x0d,0x0a
BinaryMsg        db      '0b', 0h
OctalMsg       db      '0o', 0h
HexMsg        db      '0x', 0h
newline db 0x0a, 0      ; 存储换行符
msg_end:

input:			times	33	db 0 ;用于接受输入,最长32字节
number:		times	31	db 0 ;用于存储输入的十进制数字,最长30字节
type:		         	db 0 ;用于记录转换的进制类型
len:						dd 0 ;用于记录十进制数字的长度
remain:            db 0  ;用于记录在高精度除法中的余数
outputNum:  times   110  db 0;用于记录高精度除法后的结果数字
outputLen:              dd 0;用于记录结果数字的长度

section .text
  global _start
_start:

      ;接收输入
	mov eax,3
	mov ebx,0
	mov ecx,input
	mov edx,33
	int 80h

	call getNumAndTyp  ;获取了数字和类型
	call transType     ;进行进制转换
	call _start

;---------------------------------------------------------------------------------------

getNumAndTyp:
	cmp byte [input],'q'   ;判断第一个字符是不是q
	jz q_exit         ;如果第一个字符是q,就直接退出
	call getNum
	ret
	getNum:
		mov ebx,input
		mov esi,0    ;相当于一个累加器,用于记录需要转换的数字的长度,将累加器置0
		mov byte al,[ebx+esi]
	getNumLoop:
        sub al, '0'           ; 转换为数字
	    mov [number+esi],al
		inc esi                         ;长度加一
		mov byte al,[ebx+esi]
		cmp al,' ' 						;判断是否遇到空格,遇到零标志位置1
		jnz getNumLoop            ;如果不是空格就继续读入数字，如果是空格就读入将要转换的进制类型
		mov [len],esi				; 将获得的十进制数字长度保存下来,例如'12',esi就是2,从零开始计数长度
	getTyp:
	    inc esi
	    mov byte al,[ebx+esi] ;获取type类型
	    mov byte [type],al          ;存储type类型
	    ret

	q_exit:;输入q退出程序的情况
        mov ebx, 0							; 参数一：退出代码
        mov eax, 1							; 系统调用号(sys_exit)
        int 0x80

transType:
    cmp byte [type],'b'
    jz toBinary    ;转换到二进制
    cmp byte [type],'o'
    jz toOctal    ;转换到八进制
    cmp byte [type],'h'
    jz toHex    ;转换到十六进制
    call printErr       ;说明类型无法处理
    call resetVariables ;将所有空间置零
    call _start
;---------------------------------------------------------------------------------------------------
    toBinary:
        ; 转换为二进制,辗转相除,将一个十进制数除以二，得到的商再除以二，依此类推直到商等于一或零时为止，倒取除得的余数，即换算为二进制数的结果。
        ; 这里对超大的十进制数除以二的除法实现主要思路就是高精度除法，逐次试商
        ; 在number的高位逐次除以2,循环直到最低位,在每次除以二的运算中如果余数为1,在进入下一次循环运算时,被除数加上10
        ; 一直循环做高精度除法直到被除数的最后一位被除到
		; 主要就是两个除法的循环，注意rax是储存被除数的，也是储存商的，（rdx存余数）->这个有问题啊，要被除数是32位

         ; 初始化寄存器
            mov esi, number     ; rsi 指向被除数
            xor ecx, ecx        ; 循环计数器清零，这个ecx是记录二进制数字长度的
            push ecx           ;保存现在ecx的偏移量，用于记录得到的二进制数字的长度

        BinaryDivideLoop: ;这是辗转相除法的过程

            xor ecx,ecx         ;将ecx置0.用于作为高精度除法中的偏移量
            

            DivideTwoLoop:
                mov eax,[len]
                cmp  ecx, eax ;比较ecx和被除数的长度
                je BinaryLoopHandle ;如果ecx长度与被除数一样，说明处理完了被除数所有位，一次高精度除法完成了，应当跳转到辗转相除的处理阶段
                
                xor eax,eax            ;清空eax
                mov al, [esi+ecx]   ; 取出当前位的被除数
                
                ;如果指令给出操作数为16位，最终得到的商放在ax，余数放在eax的高16位。
                ;如果指令给出操作数为8位，最终得到的商放在al，余数放在ah。
                ; 进行高精度除法运算，将被除数除以 2注意这里有问题需要改,需要完善这里的高精度除法
                xor ebx,ebx         ;清零ebx
                mov bl, 2
                cmp byte [remain],1     ;比较上次的余数是否是1
                jz   BinaryTenadd                   ;如果余数是1，将al加10

            continueTwo:
                ;判断al是否是0，如果是0需要跳过除法运算
                cmp al,0
                je BinaryDivZero ;跳转到被除数是0的处理中
                jmp BinaryDivNotZero ;跳转到被除数不是0的处理中
            
            BinaryDivNotZero:
                div bl                ; edx = rax % 2, rax = rax / 2
                jmp BinaryDivSavRemain
            BinaryDivZero:
                mov ah,0                ;如果被除数是0，那余数自然也是零
                jmp BinaryDivSavRemain
                
            BinaryDivSavRemain:
                mov [esi+ecx],al       ;将商存回被除数中
                mov [remain],ah   ;将余数存入余数变量中
                inc ecx             ;ecx加1
                jmp DivideTwoLoop   ;继续循环高精度除法

            BinaryTenadd:         ;将al加10
                inc al
                inc al
                inc al
                inc al
                inc al
                inc al
                inc al
                inc al
                inc al
                inc al
                jmp continueTwo


        BinaryLoopHandle:
            ;这是高精度除法后的处理
            ; 根据余数填充二进制数的相应位
            xor eax,eax
            mov al,[remain]
            mov byte [remain],0      ;填完后将余数恢复为0
            xor ecx,ecx
            pop ecx           ;拿到存取的偏移量
            mov byte [outputNum + ecx],al          ; 如果余数为1,则在二进制数的相应位置填入1,如果余数为0,则在二进制数的相应位置填入0. 注意这个是倒着存的
            inc ecx    ;ecx加1
            push ecx  ;将ecx保存起来
            
            ; 逐位检查被除数是否全是0
            xor edx,edx             ;用edx作为计数器，清零
        BinaryZeroCheck:
            mov al, [esi + edx]   ; 取出被除数的某一位
            cmp al, 0              ; 判断是否为0
            jne BinaryDivideLoop        ; 如果不是0，说明除法还没结束，继续辗转相除除法
            inc edx
            cmp edx,[len]           ;如果是0，则拿edx与被除数长度比较
            jne BinaryZeroCheck           ;如果edx没有被除数长，说明还要继续检查

        ; 如果是最后一位，结束循环
        xor ecx,ecx
        pop ecx            ;释放存放的ecx值
        mov  [outputLen],ecx    ;保存现在二进制数字的长度，如'001',长度就是3
        call printOutput
;--------------------------------------------------------------------------------------------------
    toOctal:
        ; 转换为八进制,主要思路和二进制类似
        ; 初始化寄存器
            mov esi, number     ; rsi 指向被除数
            xor ecx, ecx        ; 循环计数器清零，这个ecx是记录二进制数字长度的
            push ecx           ;保存现在ecx的偏移量，用于记录得到的二进制数字的长度

        OctalDivideLoop:

            xor ecx,ecx         ;将ecx置0.用于作为高精度除法中的偏移量
            
            DivideEightLoop:
                mov eax,[len]
                cmp  ecx, eax ;比较ecx和被除数的长度
                je OctalLoopHandle ;如果ecx长度与被除数一样，说明处理完了被除数所有位，一次高精度除法完成了，应当跳转到辗转相除的处理阶段
                
                xor eax,eax            ;清空eax
                mov al, [esi+ecx]   ; 取出当前位的被除数
                
                ;如果指令给出操作数为16位，最终得到的商放在ax，余数放在eax的高16位。
                ;如果指令给出操作数为8位，最终得到的商放在al，余数放在ah。
                ; 进行高精度除法运算，将被除数除以8
                ;由于需要用到乘法，al将会被用到，这里决定用dl临时存一下al的数据
                ;乘法中，操作数为8位时，默认另一个乘数存在al中，结果存在ax中
                xor ebx,ebx         ;清零ebx
                xor edx,edx         ;清零edx
                mov dl,al
                xor eax,eax
                mov al,10            ;在al存入乘数10
                mul byte [remain]        ;上一次高精度除法留下的余数作为另一个乘数
                add al,dl           ;获得正确的被除数
                mov bl, 8           ;高精度除法用到的除数8               

                ;判断al是否是0
                cmp al,0
                je OctalDivZero
                jmp OctalDivNotZero

            OctalDivZero:
                mov ah,0        ;如果被除数是0，那余数自然也是零
                jmp OctalDivSavRemain
            OctalDivNotZero:
                div bl                ; edx = rax % 8, rax = rax / 8
                jmp OctalDivSavRemain
            OctalDivSavRemain:
                mov [esi+ecx],al        ;将商存回被除数中
                mov [remain],ah         ;将余数存入余数变量中
                inc ecx             ;ecx加1
                jmp DivideEightLoop   ;继续循环高精度除法

        OctalLoopHandle:
            ;这是高精度除法后的处理
            ; 根据余数填充八进制数的相应位
            xor eax,eax
            mov al,[remain]
            mov byte [remain],0      ;填完后将余数恢复为0
            xor ecx,ecx
            pop ecx           ;拿到存取的偏移量
            ; 注意这里是倒着存的
            ;将余数存下来
            mov byte [outputNum + ecx],al          
            inc ecx    ;ecx加1
            push ecx  ;将ecx保存起来
            
            ; 逐位检查被除数是否全是0
            xor edx,edx             ;用edx作为计数器，清零
        OctalZeroCheck:
            mov al, [esi + edx]   ; 取出被除数的某一位
            cmp al, 0              ; 判断是否为0
            jne OctalDivideLoop        ; 如果不是0，说明除法还没结束，继续辗转相除除法
            inc edx
            cmp edx,[len]           ;如果是0，则拿edx与被除数长度比较
            jne OctalZeroCheck           ;如果edx没有被除数长，说明还要继续检查

        ; 如果是最后一位，结束循环
        xor ecx,ecx
        pop ecx            ;释放存放的ecx值
        mov  [outputLen],ecx    ;保存现在八进制数字的长度，如'001',长度就是3
        call printOutput
;--------------------------------------------------------------------------------------

    toHex:
        ; 转换为十六进制,主要思路和二进制以及八进制类似
        ;但是在处理余数的时候注意会有要存不是数字类型的情况出现
        ; 初始化寄存器
            mov esi, number     ; rsi 指向被除数
            xor ecx, ecx        ; 循环计数器清零，这个ecx是记录二进制数字长度的
            push ecx           ;保存现在ecx的偏移量，用于记录得到的二进制数字的长度

        HexDivideLoop:

            xor ecx,ecx         ;将ecx置0.用于作为高精度除法中的偏移量
            

            DivideSixteenLoop:
                mov eax,[len]
                cmp  ecx, eax ;比较ecx和被除数的长度
                je HexLoopHandle ;如果ecx长度与被除数一样，说明处理完了被除数所有位，一次高精度除法完成了，应当跳转到辗转相除的处理阶段
                
                xor eax,eax            ;清空eax
                mov al, [esi+ecx]   ; 取出当前位的被除数
                
                
                ;如果指令给出操作数为16位，最终得到的商放在ax，余数放在eax的高16位。
                ;如果指令给出操作数为8位，最终得到的商放在al，余数放在ah。
                ; 进行高精度除法运算，将被除数除以16
                ;由于需要用到乘法，al将会被用到，这里决定用dl临时存一下al的数据
                ;乘法中，操作数为8位时，默认另一个乘数存在al中，结果存在ax中
                xor ebx,ebx         ;清零ebx
                xor edx,edx         ;清零edx
                mov dl,al
                xor eax,eax
                mov al,10            ;在al存入乘数10
                mul byte [remain]        ;上一次高精度除法留下的余数作为另一个乘数
                add al,dl           ;获得正确的被除数
                mov bl, 16           ;高精度除法用到的除数16
                
                ;判断al是否是0，如果是0需要跳过除法
                cmp al,0
                je HexDivZreo
                jmp HexDivNotZero
            HexDivZreo:
                mov ah,0                ;如果被除数是0，那余数自然也是零
                jmp HexDivSavRemain
            HexDivNotZero:
                div bl                ; edx = rax % 16, rax = rax / 16
                jmp HexDivSavRemain
            HexDivSavRemain:
                mov [esi+ecx],al        ;将商存回被除数中
                mov [remain],ah         ;将余数存入余数变量中
                inc ecx             ;ecx加1
                jmp DivideSixteenLoop   ;继续循环高精度除法

        HexLoopHandle:
            ;这是高精度除法后的处理
            ; 根据余数填充十六进制数的相应位
            xor eax,eax
            mov al,[remain]
            mov byte[remain],0      ;填完后将余数恢复为0
            xor ecx,ecx
            pop ecx           ;拿到存取的偏移量

            ; 注意这里是倒着存的
            ;将余数存下来,这里需要有判断是否余数大于10，进而选择不同的存储方法
            cmp byte al,9
            jg SaveChar
            jmp SaveNum

        SaveChar:
            ;为了便于output统一的输出，char将也以'x'(x代表某一个char)-'0'的数值保存
            add al,'a'-':'
            mov byte [outputNum + ecx],al ;
            jmp AfterSave
            
        SaveNum:
            mov byte [outputNum + ecx],al ;存储数字
            jmp AfterSave     

        AfterSave:     
            inc ecx    ;ecx加1
            push ecx  ;将ecx保存起来
            
            ; 逐位检查被除数是否全是0
            xor edx,edx             ;用edx作为计数器，清零
        HexZeroCheck:
            mov al, [esi + edx]   ; 取出被除数的某一位
            cmp al, 0              ; 判断是否为0
            jne HexDivideLoop        ; 如果不是0，说明除法还没结束，继续辗转相除除法
            inc edx
            cmp edx,[len]           ;如果是0，则拿edx与被除数长度比较
            jne HexZeroCheck           ;如果edx没有被除数长，说明还要继续检查

        ; 如果是最后一位，结束循环
        xor ecx,ecx
        pop ecx            ;释放存放的ecx值
        mov  [outputLen],ecx    ;保存现在八进制数字的长度，如'001',长度就是3
        call printOutput

;---------------------------------------------------------------------------------------------

printOutput:
    cmp byte [type],'b'
    je printBinary      ;如果转换类型为二进制，去print'0b'
    cmp byte [type],'o'
    je printOctal      ;如果转换类型为八进制，去print'0o'
    cmp byte [type],'h'
    je printHex      ;如果转换类型为十六进制，去print'0x'
printBinary:
    mov eax, 4            ; sys_write
    mov ebx, 1            ; 标准输出
    mov ecx, BinaryMsg    ; 读取 BinaryMsg 的地址
    mov edx,2             ;BinaryMsg长度已知是2   
    int 0x80         ; 调用系统调用  
    jmp printOutputNum      ;跳转去打印结果数字

printOctal:
    mov eax, 4            ; sys_write
    mov ebx, 1            ; 标准输出
    mov ecx, OctalMsg    ; 读取 OctalMsg 的地址
    mov edx,2             ;BinaryMsg长度已知是2   
    int 0x80         ; 调用系统调用  
    jmp printOutputNum      ;跳转去打印结果数字


printHex:
    mov eax, 4            ; sys_write
    mov ebx, 1            ; 标准输出
    mov ecx, HexMsg    ; 读取 HexMsg 的地址
    mov edx,2             ;BinaryMsg长度已知是2   
    int 0x80         ; 调用系统调用  
    jmp printOutputNum      ;跳转去打印结果数字


printOutputNum:
    xor ecx,ecx     ;将ecx清零，作为输出结果数字从后往前的偏移量
    xor esi,esi         ;将esi清零
    mov esi,[outputLen]     ;将ecx赋予结果数字的长度
    printLoop:
        dec esi     ;将ecx减一作为真实偏移量

        ;注意这里在输出前要把数字转换回字符
        ;注意要判断是否为字符
        xor edi,edi
        mov edi,[outputNum+esi]
        add edi, 0x30
        mov [outputNum+esi],edi

        mov eax, 4            ; sys_write
        mov ebx, 1            ; 标准输出
        lea ecx,[outputNum+esi]    ; 读取结果数字该位数的地址
        mov edx,1             ;输出长度为1
        int 0x80         ; 调用系统调用
        cmp esi,0       ;将esi与0比较
        jnz printLoop ;如果不为0，说明还要继续打印
        call printNewLine
        call resetVariables ;将空间置零，等待下一次程序调用
        jmp _start          ;如果为0，说明打印完成，跳转回程序开始点







    printErr:
        ; 输出错误消息
        mov eax, 4            ; sys_write
        mov ebx, 1            ; 标准输出
        mov ecx, ErrMessage
        mov edx, 7            ; 错误消息长度
        int 0x80
        ret

resetVariables:
    ; 清除输入缓冲区
    mov ecx, 33
    mov edi, input
    xor eax, eax
    rep stosb  ; 用零填充输入缓冲区

    ; 清除数字缓冲区
    mov ecx, 31
    mov edi, number
    xor eax, eax
    rep stosb  ; 用零填充数字缓冲区

    ; 清除类型变量
    mov byte [type], 0

    ; 清除长度变量
    mov dword [len], 0

    ; 清除余数变量
    mov byte [remain], 0

    ; 清除outputNum缓冲区
    mov ecx, 110
    mov edi, outputNum
    xor eax, eax
    rep stosb  ; 用零填充outputNum缓冲区

    ; 清除outputLen变量
    mov dword [outputLen], 0

    ret
printNewLine:
    mov eax, 4            ; sys_write
    mov ebx, 1            ; 标准输出
    mov ecx, newline      ; newline是存储换行符的数据
    mov edx, 1            ; 输出长度为1
    int 0x80              ; 调用系统调用
    ret