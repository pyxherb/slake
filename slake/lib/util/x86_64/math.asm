global _slake_stdlib_sinf_fast
_slake_stdlib_sinf_fast:
	endbr64
	sub rsp, 4

	movss [rsp], xmm0
	fld dword [rsp]
	fsin
	fst dword [rsp]
	movss xmm0, [rsp]

	add rsp, 4
	ret

global _slake_stdlib_sin_fast
_slake_stdlib_sin_fast:
	endbr64
	sub rsp, 8

	movsd [rsp], xmm0
	fld qword [rsp]
	fsin
	fst qword [rsp]
	movsd xmm0, [rsp]

	add rsp, 8
	ret

global _slake_stdlib_cosf_fast
_slake_stdlib_cosf_fast:
	endbr64
	sub rsp, 4

	movss [rsp], xmm0
	fld dword [rsp]
	fcos
	fst dword [rsp]
	movss xmm0, [rsp]

	add rsp, 4
	ret

global _slake_stdlib_cos_fast
_slake_stdlib_cos_fast:
	endbr64
	sub rsp, 8

	movsd [rsp], xmm0
	fld qword [rsp]
	fcos
	fst qword [rsp]
	movsd xmm0, [rsp]

	add rsp, 8
	ret

global _slake_stdlib_tanf_fast
_slake_stdlib_tanf_fast:
	endbr64
	sub rsp, 4

	movss [rsp], xmm0
	fld dword [rsp]
	fptan
	fld st1
	fst dword [rsp]
	movss xmm0, [rsp]

	add rsp, 4
	ret

global _slake_stdlib_tan_fast
_slake_stdlib_tan_fast:
	endbr64
	sub rsp, 8

	movsd [rsp], xmm0
	fld qword [rsp]
	fptan
	fld st1
	fst qword [rsp]
	movsd xmm0, [rsp]

	add rsp, 8
	ret

global _slake_stdlib_sqrtf_fast
_slake_stdlib_sqrtf_fast:
	endbr64
	sub rsp, 4

	movss [rsp], xmm0
	fld dword [rsp]
	fsqrt
	fst dword [rsp]
	movss xmm0, [rsp]

	add rsp, 4
	ret

global _slake_stdlib_sqrt_fast
_slake_stdlib_sqrt_fast:
	endbr64
	sub rsp, 8

	movsd [rsp], xmm0
	fld qword [rsp]
	fsqrt
	fst qword [rsp]
	movsd xmm0, [rsp]

	add rsp, 8
	ret
