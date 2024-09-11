//
//  sim5.c
//  Sim5Prod
//
//  Created by Kyle Vega on 4/4/24.
//
#include "sim5.h"
#include <stdio.h>

//ID Phase
void extract_instructionFields(WORD instruction, InstructionFields *fieldsOut) {
    fieldsOut->opcode = (instruction & 0xFC000000) >> 26;
    fieldsOut->rs = (instruction & 0x03E00000) >> 21;
    fieldsOut->rt = (instruction & 0x001F0000) >> 16;
    fieldsOut->rd = (instruction & 0x0000F800) >> 11;
    fieldsOut->shamt = (instruction & 0x000007C0) >> 6;
    fieldsOut->funct = (instruction & 0x0000003F);
    fieldsOut->imm16 = (instruction & 0x0000FFFF);
    fieldsOut->imm32 = signExtend16to32(fieldsOut->imm16);
    fieldsOut->address = (instruction & 0x03FFFFFF);
}
//If the next instruction uses the curr rd as the rs or rt return 1 else return 0
//If the opcode is invalid return 0
int IDtoIF_get_stall(InstructionFields *fields, ID_EX *old_idex, EX_MEM *old_exmem) {
    int opcode = fields->opcode;
    switch (opcode) {
        case 0x00:  // R-type instructions
            //Handles lw that may come before
            if(((old_idex->memRead && old_idex->regWrite) && (old_idex->rt == fields->rs || old_idex->rt == fields->rt))){
                return 1;
            }
            break;
            
        case 0x04:  // beq
        case 0x05:  // bne
        case 0x08:  // addi
        case 0x09:  // addiu
        case 0x0A:  //slti
        case 0x0C:  // andi
        case 0x0D:  // ori
        case 0x0F:  // lui
        case 0x23: // lw
            //Works for the I types
            if(((old_idex->memRead && old_idex->regWrite) && (old_idex->rt == fields->rs))){
                return 1;
            }
            break;
        case 0x02:  // j
            break;
        case 0x2B: // sw
            //Check if the instruction ahead is I or R type
            if(old_idex->regDst == 0){
                //If the rt from the instruction 1 ahead matches the
                // curr rt then it does not matter what happend 2 instructions ahead
                if(old_idex->rt == fields->rt){
                    return 0;
                }
            }else{
                //Same as if the rt from the I type but instead the rd for the R type
                if(old_idex->rd == fields->rt){
                    return 0;
                }
            }
            //Checks if the iinstruction 2 ahead is writng to the same register as the sw's rt
            if((old_exmem->regWrite && old_exmem->writeReg == fields -> rt) && fields -> rt != 0){
                return 1;
            }
            break;
        default:
            return 0;
            break;
    }
    return 0;
}

//Handles if the instruction branches or jumps
int IDtoIF_get_branchControl(InstructionFields *fields, WORD rsVal, WORD rtVal){
    int op = fields->opcode;
    //BEQ
    if(rtVal == rsVal && op == 0x04){
        return 1;
    }else if(rtVal != rsVal && op == 0x05){ //BNE
        return 1;
    }else if(op == 0x02){ //J
        return 2;
    }
    return 0;
}

WORD calc_branchAddr(WORD pcPlus4, InstructionFields *fields){
    WORD imm32 = fields->imm32;
    imm32 = imm32 << 2;
    WORD branchAddress = pcPlus4 + imm32;
    return branchAddress;
}

WORD calc_jumpAddr  (WORD pcPlus4, InstructionFields *fields){
    //The 26 bit field for the j format
    WORD jumpLoc = fields->address;
    jumpLoc = jumpLoc << 2;
    //Mask for the 4 most significant bits
    WORD jumpAddress = (pcPlus4 & 0xF0000000) | jumpLoc;
    return jumpAddress;
}

int execute_ID(int IDstall, InstructionFields *fieldsIn, WORD pcPlus4, WORD rsVal, WORD rtVal, ID_EX *new_idex){
    new_idex->ALUsrc = 0;
    new_idex->ALU.op = 0;
    new_idex->ALU.bNegate = 0;
    new_idex->memRead = 0;
    new_idex->memWrite = 0;
    new_idex->memToReg = 0;
    new_idex->regDst = 0;
    new_idex->regWrite = 0;
    new_idex->extra1 = 0;
    new_idex->extra2 = 0;
    new_idex->extra3 = 0;
    new_idex->rs = 0;
    new_idex->rt = 0;
    new_idex->rd = 0;
    new_idex->rd = 0;
    new_idex->rsVal = 0;
    new_idex->rtVal = 0;
    //Leaves everything set to 0 if there is a stall
    if(IDstall){
        return 1;
    }
    //Propogates the data if there is not stall
    new_idex->imm16 = fieldsIn->imm16;
    new_idex->imm32 = fieldsIn->imm32;
    new_idex->rs = fieldsIn->rs;
    new_idex->rt = fieldsIn->rt;
    new_idex->rd = fieldsIn->rd;
    
    new_idex->rsVal = rsVal;
    new_idex->rtVal = rtVal;
    int opcode = fieldsIn->opcode;
    int funct = fieldsIn->funct;
    

    switch (opcode) {
        case 0x00:  // R-type instructions
            switch (funct) {
                case 0x00:  // nop
                    new_idex->ALU.op = 5;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    break;
                case 32:  // add
                    new_idex->ALU.op = 2;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    break;
                case 33:  // addu
                    new_idex->ALU.op = 2;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    break;
                case 34:  // sub
                    new_idex->ALU.op = 2;
                    new_idex->ALU.bNegate = 1;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    break;
                case 35:  // subu
                    new_idex->ALU.op = 2;
                    new_idex->ALU.bNegate = 1;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    break;
                case 36:  // and
                    new_idex->ALU.op = 0;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    break;
                case 37:  // or
                    new_idex->ALU.op = 1;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    break;
                case 38:  // xor
                    new_idex->ALU.op = 4;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    break;
                case 39:  // nor
                    new_idex->ALU.op = 1;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    new_idex->ALU.bNegate = 1;
                    break;
                case 42:  // slt
                    new_idex->ALU.op = 3;
                    new_idex->ALU.bNegate = 1;
                    new_idex->regDst = 1;
                    new_idex->regWrite = 1;
                    break;
                default:
                    return 0;  // Invalid funct
            }
            break;
        case 0x04:  // beq
            
            new_idex->rs = 0;
            new_idex->rt = 0;
            new_idex->rd = 0;
            
            new_idex->rsVal = 0;
            new_idex->rtVal = 0;
            
            new_idex->ALUsrc = 0;
            break;
        case 0x05:  // bne
            new_idex->rs = 0;
            new_idex->rt = 0;
            new_idex->rd = 0;
            
            new_idex->rsVal = 0;
            new_idex->rtVal = 0;
            break;
        case 0x08:  // addi
            new_idex->ALUsrc = 1;
            new_idex->ALU.op = 2;
            new_idex->regWrite = 1;
            break;
        case 0x09:  // addiu
            new_idex->ALUsrc = 1;
            new_idex->ALU.op = 2;
            new_idex->regWrite = 1;
            break;
        case 0x0A:  //slti
            new_idex->ALUsrc = 1;
            new_idex->ALU.op = 3;
            new_idex->ALU.bNegate = 1;
            new_idex->regWrite = 1;
            break;
        case 0x0C:  // andi
            new_idex->ALUsrc = 2;
            new_idex->ALU.op = 0;
            new_idex->regWrite = 1;
            break;
        case 0x0D:  // ori
            new_idex->ALUsrc = 2;
            new_idex->ALU.op = 1;
            new_idex->regWrite = 1;
            break;
        case 0x0F:  // lui
            new_idex->ALUsrc = 3;
            new_idex->ALU.op = 2;
            new_idex->regWrite = 1;
            break;
        case 0x02:  // j
            new_idex->rs = 0;
            new_idex->rt = 0;
            new_idex->rd = 0;
            
            new_idex->rsVal = 0;
            new_idex->rtVal = 0;
            break;
        case 0x23:  // lw
            new_idex->ALUsrc = 1;
            new_idex->ALU.op = 2;
            new_idex->memRead = 1;
            new_idex->memToReg = 1;
            new_idex->regWrite = 1;
            break;
        case 0x2B:  // sw
            new_idex->ALUsrc = 1;
            new_idex->ALU.op = 2;
            new_idex->memWrite = 1;
            break;
        default:
            return 0;  // Invalid opcode
    }

    return 1;
}

//EX Phase
WORD EX_getALUinput1(ID_EX *in, EX_MEM *old_exMem, MEM_WB *old_memWb){
    //Checking if the instruction 1 ahead is writng to the same register as the curr
    // instruction if it is it will return the aluResult from there
    if(old_exMem->writeReg == in->rs && old_exMem->regWrite == 1){
        return old_exMem->aluResult;
    }
    //Checking the same things but for the instruction 2 ahead
    else if(old_memWb->writeReg == in->rs && old_memWb->regWrite == 1){

        if(old_memWb->memToReg){
            return old_memWb->memResult;
        }else{
            return old_memWb->aluResult;
        }
    }else{
        return in->rsVal;
    }
}
WORD EX_getALUinput2(ID_EX *in, EX_MEM *old_exMem, MEM_WB *old_memWb){
    //Does the same thing as input 1 but checks for immediate values
    if (in->ALUsrc==1){
        return in->imm32;
    }else if (in->ALUsrc == 2){
        return in->imm16;
    }else if (in->ALUsrc == 3){//For lui
        return in->imm16 << 16;
    }else{
        if(old_exMem->writeReg == in->rt && old_exMem->regWrite == 1){
            return old_exMem->aluResult;
        }else if(old_memWb->writeReg == in->rt && old_memWb->regWrite == 1){
            if(old_memWb->memToReg){
                return old_memWb->memResult;
            }else{
                return old_memWb->aluResult;
            }
        }else{
            return in->rtVal;
        }
    }
}
void execute_EX(ID_EX *in, WORD input1, WORD input2, EX_MEM *new_exMem){
    //Propogating the data
    new_exMem->aluResult = 0;
    new_exMem->regWrite = in->regWrite;
    new_exMem->rt = in->rt;
    new_exMem->rtVal = in->rtVal;
    new_exMem->memRead = in->memRead;
    new_exMem->memWrite = in->memWrite;
    new_exMem->memToReg = in->memToReg;
    new_exMem->writeReg = in->regDst ? in->rd : in->rt;
    int bNeg = in -> ALU.bNegate;
    // Perform ALU operation based on ALU.op
        switch (in->ALU.op) {
            case 0:  // and
                new_exMem->aluResult = input1 & input2;
                break;
            case 1:  // or
                //Checks the bNegate to know if it needs to negate the or for NOR
                new_exMem->aluResult = bNeg ? ~(input1 | input2) : input1 | input2;
                break;
            case 2:  // add
                new_exMem->aluResult = input1 + (bNeg ? -input2 : input2);
                break;
            case 3:  // slt
                new_exMem->aluResult = input1 < input2;
                break;
            case 4: // xor
                new_exMem->aluResult = input1 ^ input2;
                break;
            case 5:
                break;
            default:
                //invalid ALU.op
                break;
        }
}

//MEM Phase
void execute_MEM(EX_MEM *in, MEM_WB *old_memWb, WORD *mem, MEM_WB *new_memwb){
    new_memwb->memResult = 0;
    new_memwb->memToReg= in->memToReg;
    new_memwb->aluResult = in->aluResult;
    
    new_memwb->writeReg = in->writeReg;
    new_memwb->regWrite = in->regWrite;
    if (in->memRead) {
        // Perform memory read operation
        new_memwb->memResult = mem[in->aluResult/4];
    } else if (in -> memWrite == 1){
        
        //first check if we need to update value were are writing to memory
        //from a value generated by the instruction 2 ahead
        
        if (old_memWb -> regWrite == 1 && old_memWb -> memToReg == 1){
            
            //if instuction 2 ahead is updating a register (from memory) and the register is
            //same as curr instruction - foward
            
            if (in -> rt == old_memWb -> writeReg){
                mem[(in -> aluResult)/4] = old_memWb -> memResult;
                //else use curr rtVal - no fowarding
                
            } else {
                mem[(in -> aluResult)/4] = in -> rtVal;
            }
            
            //check if we need to again foward value but not a value pulled from memory
            //indicates instuction 2 ahead is updating register with a value calculated by
            //ALU not read from memory - still foward but alu result
            
        } else if (old_memWb -> regWrite == 1 && old_memWb -> memToReg == 0){
            if (in -> rt == old_memWb -> writeReg){
                mem[(in -> aluResult)/4] = old_memWb -> aluResult;
                //else no fowarding
            } else {
                mem[(in -> aluResult)/4] = in -> rtVal;
            }
            //for all other cases where non fowarding memory write is required
        } else {
            mem[(in -> aluResult)/4] = in -> rtVal;
        }
        // Pass other data through
    }
}

//WB Phase
void execute_WB (MEM_WB *in, WORD *regs){
    //if curr instruction is not writing to a register do nothing
        if (in -> regWrite == 0){
            return;
        //else determine if value came from memory or from ALU calculation
        } else {
            //if curr instuction is writing a value from memory into a register
            if (in -> memToReg == 1){
                regs[in -> writeReg] = in -> memResult;
            //else value is coming from ALU
            } else {
                regs[in -> writeReg] = in -> aluResult;
            }
        }
}
