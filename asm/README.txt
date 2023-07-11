ROS-CHIP-8 Assembler

SYNTAX
======
[ label ] : mnemonic [ op1 ], [ op2 ], ...

NOTE: Elements in [] are optional.

MNEMONICS
=========
CROS    <imm12>                 | Call ROS-API
RET                             | Return from procedure
CLS                             | Clear screen
GOTO    <imm12>/<V0 + imm12>    | Direct jump
CALL    <imm12>                 | Call procedure
SE      <reg8>, <reg8>/<imm8>   | Skip next if equal
SNE     <reg8>, <reg8>/<imm8>   | Skip next if not equal
SET*    <dst8>/<dst12>/delay, <reg8>/<imm8>/<reg8 - dst8>/<imm12>/delay/key
                                | Set register value ( VF is affected in case of <reg8 - dst8> )
ADD^    <reg8>/<reg12>, <reg8>/<imm8>   | Add operation
OR*     <reg8>, <reg8>/<imm8>   | Or operation ( Assembler uses Vf register as temporary, in case of <imm8> )
AND*    <reg8>, <reg8>/<imm8>   | And operation ( Assembler uses Vf register as temporary, in case of <imm8> )
XOR*    <reg8>, <reg8>/<imm8>   | Xor operation ( Assembler uses Vf register as temporary, in case of <imm8> )
SUB^    <reg8>, <reg8>/<imm8>   | Sub operation ( Assembler uses Vf register as temporary, in case of <imm8> )
SHR^    <reg8>                  | Right binary shift
SHL^    <reg8>                  | Left binary shift
RND     <reg8>, <imm8>          | Get random value masked by immidiate value
BCD     <reg8>                  | BCD representation
DUMP    V0, <reg8>              | Dump registers to I
LOAD    V0, <reg8>              | Load registers from I

DRAW    <reg8>, <reg8>, <imm4>  | [DEPRECATED, DO NOT USE]
SKE     <reg8>                  | [DEPRECATED, DO NOT USE]
SKNE    <reg8>                  | [DEPRECATED, DO NOT USE]
SPR     <reg8>                  | [DEPRECATED, DO NOT USE]

* - vF is affected in particular case
^ - vF is always affected

CONSTRAINTS
===========
SET     <reg8>, <imm12>
SET     <reg12>, <reg8>/delay/key/<reg8 - dst8>
SET     delay, <imm8>/<reg8 - dst8>/<imm12>/delay/key

PSEUDO INSTRUCTIONS SYNTAX
==========================
DATASEG <IMM8>/<CHAR>/<STRING>, <IMM8>/<CHAR>/<STRING> ... DATAEND  | Declare data segment
RESERVE <IMM8>/<IMM12>                                              | Declare reserved data segment
DEFINE <name>, <IMM8>/<IMM12>/<CHAR>                                | Create name
INCLUDE <file>                                                      | Include header

Segments can be combined with instructions in ANY order.

-------------------------------------------------------------------------------------------------------

ROS-CHIP8 API

ABOUT
=====
There is a special instruction CROS in ROS-CHIP-8 instruction set.
It corresponds to syscall. Syscalls are performed by OS each time
application calls this instruction. Logical code of syscall is stored
directly in CROS argument as IMM12. There are defines for all provided
syscalls in "rosapi.rch8".

CALLING CONVENTION
==================
From "rosapi.rch8":
;; ----------------------- ;;
;; ROS calling convention  ;;
;;                         ;;
;; Addresses (IMM12):      ;;
;; I                       ;;
;;                         ;;
;; Arguments (IMM8):       ;;
;; v1 ... v5               ;;
;;                         ;;
;; Return value:           ;;
;; Addresses (IMM12):      ;;
;; I                       ;;
;; Constants (IMM8):       ;;
;; v0                      ;;
;; ----------------------- ;;

DOCUMENTATION STRUCTURE
=======================
SYSCALL CODE (IMM12, HEX) | SYSCALL SIGNATURE (ACCORDING TO CALLING CONVENTION)
    SHORT DESCRIPTION

    NOTE: SOME NOTES

    arg1         : ARGUMENT DESCRIPTION
    ...
    Return value : RETURN VALUE DESCRIPTION

I/O
===
000h | imm8 r_puts(imm12 cstr)
    PUT String to standart output.
    
    cstr         : String to output
    Return value : Number of characters printed

001h | imm8 r_putc(imm8 ch)
    PUT Character to standart output.

    ch           : Character to output
    Return value : Character, which was outputed

002h | imm8 r_putf(imm12 fmt, __VA_ARGS_OUT__)
    PUT Formatted string to standart output.

    fmt             : Format string
    __VA_ARGS_OUT__ : Values ( From v1 )
    Return value    : Number of characters printed

003h | imm8 r_gets(imm12 buf, imm8 max)
    GET String from standart input.

    NOTE: Blocking operation

    buf          : String buffer
    max          : String buffer size
    Return value : Number of characters read

004h | imm8 r_getc(void)
    GET Character from standart input.

    NOTE: Blocking operation

    Return value : Character

005h | imm8 r_getf(imm12 fmt, __VA_ARGS_IN__)
    GET Formatted string from standart input.

    NOTE: Blocking operation

    fmt             : Format string
    __VA_ARGS_IN__  : Values ( From v1 )
    Return value    : Number of values read

ARGUMENTS PARSING
=================
006h | imm8 r_geta(imm8 idx)
    GET Argument from command line by index.

    NOTE: Argument will be located at address 1h,
          and zero terminated.

    idx          : Argument index
    Return value : 0 if success

007h | imm8 r_getb(void)
    GET Buffer from command line.

    NOTE: Buffer will be located at address 1h,
          zero terminated, and without new line
          at the end.

    Return value : 0 if success

CURSOR
======
008h | imm8 r_shcr(void)
    SHow CuRsor

    Return value : 0 if success

009h | imm8 r_hdcr(void) 
    HiDe CuRsor

    Return value : 0 if success

00Ah | imm8 r_stcr(imm8 state)
    SeT CuRsor

    state        : 0 if invisible, else visible
    Return value : 0 if success

00Bh | imm8 r_mvcr(imm8 row, imm8 col)
    MoVe CuRsor

    row          : Screen row
    col          : Screen column
    Return value : 0 if success

FLOW
====
00Ch | void r_paus(void)
    PAUSe

    NOTE: Switches system to SYS_IDLE mode and prints:
    "Press any key to continue. . .".
    NOTE: Blocking operation

00Dh | void r_slms(imm12 ms)
    SLeep MilliSeconds

    NOTE: Blocking operation

    ms           : Amount of time to sleep in milliseconds

00Eh | void r_exit(imm8 code)
    EXIT

    NOTE: This syscall does not return to program

    code         : Exit code

00Fh | void r_abrt(void)
    ABoRT

    NOTE: This syscall does not return to program

LOG
===
010h | imm8 r_log(imm8 type, imm12 fmt, __VA_ARGS_OUT__)
    LOG

    NOTE: You cannot raise hard screen with this syscall

    type            : 0 ( LG_INFO ), 1 ( LG_ERROR ), 2 ( LG_WARN )
    fmt             : Format string
    __VA_ARGS_OUT__ : Values ( From v1 )

SYSTEM ONLY
===========
011h | void s_hder(imm8 code)
    HarD ERrror

    NOTE: Raises red screen
    NOTE: This syscall does not return to program

    code         : fault code