ROS-CHIP-8 Assembler

SYNTAX
======
CROS    <api function>          | Call ROS-API
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

Segments can be combined with instructions in ANY order.