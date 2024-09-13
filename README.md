# Simulated-Pipelined-CPU

## Overview
This `sim5.c` file simulates parts of a MIPS-like processor pipeline. The code processes MIPS instructions through different stages of the pipeline: Instruction Decode (ID), Execute (EX), Memory Access (MEM), and Write Back (WB). It handles instruction parsing, branch control, forwarding, stalling, and ALU operations.

The pipeline stages included are:
- **Instruction Decode (ID)**
- **Execution (EX)**
- **Memory Access (MEM)**
- **Write Back (WB)**

## Key Components

### 1. Instruction Fields
The structure `InstructionFields` holds fields parsed from the 32-bit MIPS instruction such as opcode, source registers (rs, rt), destination register (rd), shift amount (shamt), function (funct), and immediate values. 

```c
void extract_instructionFields(WORD instruction, InstructionFields *fieldsOut);
```

### 2. Stalling (Data Hazard Handling)
The function `IDtoIF_get_stall` checks if there is a data hazard by comparing the current instruction's registers with the previous instruction's destination register. It checks if the next instruction depends on the results of the current instruction, causing a pipeline stall.

```c
int IDtoIF_get_stall(InstructionFields *fields, ID_EX *old_idex, EX_MEM *old_exmem);
```

### 3. Branch Control
The function `IDtoIF_get_branchControl` determines if a branch (e.g., BEQ or BNE) or jump (J) should be taken based on opcode and register values.

```c
int IDtoIF_get_branchControl(InstructionFields *fields, WORD rsVal, WORD rtVal);
```

### 4. ALU Operations
The ALU (Arithmetic Logic Unit) processes instructions during the EX phase, performing operations like addition, subtraction, and bitwise logic (AND, OR, XOR, NOR).

```c
void execute_EX(ID_EX *in, WORD input1, WORD input2, EX_MEM *new_exMem);
```

### 5. Forwarding
To avoid pipeline stalls due to data hazards, the EX stage implements data forwarding from the EX/MEM or MEM/WB pipeline stages, preventing unnecessary delays.

```c
WORD EX_getALUinput1(ID_EX *in, EX_MEM *old_exMem, MEM_WB *old_memWb);
WORD EX_getALUinput2(ID_EX *in, EX_MEM *old_exMem, MEM_WB *old_memWb);
```

### 6. Memory Access
The `execute_MEM` function manages memory read and write operations. If data forwarding is needed, it pulls the correct values from the pipeline to store into memory or registers.

```c
void execute_MEM(EX_MEM *in, MEM_WB *old_memWb, WORD *mem, MEM_WB *new_memwb);
```

### 7. Write Back (WB)
The final stage of the pipeline is Write Back, where results from the ALU or memory are written into the register file if needed.

```c
void execute_WB(MEM_WB *in, WORD *regs);
```

## Functions

### Instruction Decode (ID)
- `extract_instructionFields()`: Parses the instruction into its component fields.
- `IDtoIF_get_stall()`: Checks for pipeline stalls due to data hazards.
- `IDtoIF_get_branchControl()`: Determines if a branch or jump should occur.
- `calc_branchAddr()`: Calculates the target address for branch instructions.
- `calc_jumpAddr()`: Calculates the target address for jump instructions.
- `execute_ID()`: Handles the ID phase, setting up control signals and forwarding necessary data to the EX phase.

### Execute (EX)
- `EX_getALUinput1()`, `EX_getALUinput2()`: Forwards the correct inputs to the ALU based on the pipeline state.
- `execute_EX()`: Performs the ALU operation and forwards results to the MEM phase.

### Memory Access (MEM)
- `execute_MEM()`: Handles reading and writing from memory. Implements forwarding for memory write operations.

### Write Back (WB)
- `execute_WB()`: Writes the result to the appropriate register in the register file.

## Usage

The program is typically used in a pipeline simulator where each function corresponds to a stage of the processor's pipeline. The instruction fields are extracted, checked for hazards, and processed through the EX, MEM, and WB phases.

## Instructions Supported
- **R-type instructions**: add, addu, sub, subu, and, or, xor, nor, slt.
- **I-type instructions**: addi, addiu, lw, sw, andi, ori, slti, lui.
- **Branch instructions**: BEQ, BNE.
- **Jump instruction**: J.

## Stalling and Forwarding
The program efficiently handles stalling by checking for dependencies between instructions. Forwarding from the EX and MEM stages helps mitigate the effects of data hazards. Stalls are introduced if the next instruction depends on the current instruction's result before it is written back to the register file.

## Conclusion
This file forms the core of a MIPS pipeline simulator. It covers instruction decoding, data forwarding, stalling, and execution across pipeline stages, making it a foundational component of pipeline-based MIPS CPU simulation.
