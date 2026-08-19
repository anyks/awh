;
; Обёртка вызова порождённого кода, приметы в оберегаемые регистры укладывающая.
;
; Файл заведён ради связки вооружения Visual Studio: вставок на языке ассемблера
; она на наборе x86-64 не имеет вовсе - «__asm» там доступен лишь на наборе
; x86, - отчего проверка сохранности регистров отчитывалась пропуском. Заменить
; обёртку кодом на языке C нельзя: она обязана занять оберегаемые регистры перед
; самым вызовом и снять их сразу по возвращении, а распределитель регистров
; такого не обещает.
;
; Тело обёртки повторяет вставку, в tools/regex/conformance.cpp размещённую.
; Соглашение Microsoft подаёт доводы в rcx, rdx, r8 и r9, доводы пятый и шестой -
; кадром вызова, и числит оберегаемыми rbx, rsi, rdi, r12, r13 и r14 - их шесть
; и проверяется. Прежде вызова отводятся тридцать два байта под доводы
; вызываемого, как того соглашение и требует.
;
; Смещения доводов пятого и шестого: шесть укладок и отвод пятидесяти шести байтов
; дают сто четыре байта поверх адреса возврата, отчего довод пятый лежит
; по смещению 40 + 104 = 144, а шестой - по 48 + 104 = 152.
;
_TEXT	SEGMENT

	PUBLIC	awh_regex_preserving

awh_regex_preserving PROC
	push	rbx
	push	rsi
	push	rdi
	push	r12
	push	r13
	push	r14
	sub	rsp, 56
	mov	r10, rcx
	mov	r11, QWORD PTR [rsp + 144]
	mov	rax, QWORD PTR [rsp + 152]
	mov	QWORD PTR [rsp + 32], rax
	mov	rcx, rdx
	mov	rdx, r8
	mov	r8, r9
	mov	r9, r11
	mov	rbx, 1122334455667788h
	mov	r12, 99AABBCCDDEEFF00h
	mov	r13, 0F1E2D3C4B5A6978h
	mov	rsi, 2468ACE013579BDFh
	mov	rdi, 76543210FEDCBA98h
	mov	r14, 0123456789ABCDEFh
	call	r10
	mov	rax, rbx
	xor	rax, r12
	xor	rax, r13
	xor	rax, rsi
	xor	rax, rdi
	xor	rax, r14
	add	rsp, 56
	pop	r14
	pop	r13
	pop	r12
	pop	rdi
	pop	rsi
	pop	rbx
	ret
awh_regex_preserving ENDP

_TEXT	ENDS

	END
