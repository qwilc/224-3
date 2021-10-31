#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

const int MAX_MEM_SIZE  = (1 << 13);

void fetchStage(int *icode, int *ifun, int *rA, int *rB, wordType *valC, wordType *valP) {
	pc = getPC();
	byteType byte = getByteFromMemory(pc);
	*ifun = byte&0xf;
	*icode = byte >> 4;

	if(*icode == NOP || *icode == HALT || *icode == RET) {
		*valP = pc + 1;
	}
	else if(*icode == IRMOVQ || *icode == RMMOVQ || *icode == MRMOVQ) {
		byte = getByteFromMemory(pc + 1);
		*rA = byte >> 4;
		*rB = byte&0xf;
		*valC = getWordFromMemory(pc+2);
		*valP = pc + 10;
	}
	else if(*icode == RRMOVQ || *icode == OPQ || *icode == PUSHQ || *icode == POPQ) {
		byte = getByteFromMemory(pc + 1);
		*rA = byte >> 4;
		*rB = byte&0xf;
		*valP = pc + 2;
	}
	else if(*icode == JXX || *icode == CALL) {
		*valC = getWordFromMemory(pc + 1);
		*valP = pc + 9;
	}
}

void decodeStage(int icode, int rA, int rB, wordType *valA, wordType *valB) {
	if(icode == RRMOVQ) {
		*valA = getRegister(rA);
	}
	else if(icode == RMMOVQ || icode == OPQ) {
		*valA = getRegister(rA);
		*valB = getRegister(rB);
	}
	else if(icode == MRMOVQ) {
		*valB = getRegister(rB);
	}
	else if(icode == CALL) {
		*valB = getRegister(RSP);
	}
	else if(icode == RET) {
		*valA = getRegister(RSP);
		*valB = getRegister(RSP);
	}
	else if(icode == PUSHQ) {
		
	}
}

void executeStage(int icode, int ifun, wordType valA, wordType valB, wordType valC, wordType *valE, bool *Cnd) {
	if(icode == IRMOVQ) {
		*valE = valC;
	}
	else if(icode == RRMOVQ) {
		*valE = valA;
	}
	else if(icode == RMMOVQ || icode == MRMOVQ) {
		*valE = valB + valC;
	}
	else if(icode == OPQ) {
		if(ifun == ADD) {
			*valE = valA + valB;
			overflowFlag = ((valA<0)==(valB<0) && (*valE<0)!=(valA<0));
		} 
		else if(ifun == SUB) {
			valA = -valA;
			*valE = valA + valB;
			overflowFlag = ((valA<0)==(valB<0) && (*valE<0)!=(valA<0));
		} 
		else if(ifun == AND) {
			*valE = valA & valB;
			overflowFlag = 0;
		} 
		else if(ifun == XOR) {
			*valE = valA ^ valB;
			overflowFlag = 0;
		}
		zeroFlag = *valE == 0;
		signFlag = *valE < 0;
	}
	else if(icode == JXX) {
		*Cnd = Cond(ifun);
	}
	else if(icode == CALL) {
		*valE = valB + (-8);
	}
	else if(icode == RET) {
		*valE = valB + 8;
	}
  
}

void memoryStage(int icode, wordType valA, wordType valP, wordType valE, wordType *valM) {
	if(icode == RMMOVQ) {
		setWordInMemory(valE, valA);
	}
	else if(icode == MRMOVQ) {
		*valM = getWordFromMemory(valE);
	}
	else if(icode == CALL) {
		setWordInMemory(valE, valP);
	}
	else if(icode == RET) {
		*valM = getWordFromMemory(valA);
	}
}

void writebackStage(int icode, int rA, int rB, wordType valE, wordType valM) {
	if(icode == IRMOVQ || icode == RRMOVQ || icode == OPQ) {
		setRegister(rB, valE);
	}
	else if(icode == MRMOVQ) {
		setRegister(rA, valM);
	}
	else if(icode == CALL || icode == RET) {
		setRegister(RSP, valE);
	}
}

void pcUpdateStage(int icode, wordType valC, wordType valP, bool Cnd, wordType valM) {
  if(icode == NOP || icode == IRMOVQ || icode == RRMOVQ || icode == RMMOVQ || icode == MRMOVQ || icode == OPQ) {
		setPC(valP);
	}
	else if(icode == HALT) {
		setStatus(STAT_HLT);
		setPC(valP);
	}
	else if(icode == JXX) {
		if(Cnd == 1) {
			setPC(valC);
		}
		else {
			setPC(valP);
		}
	}
	else if(icode == CALL) {
		setPC(valC);
	}
	else if(icode == RET) {
		setPC(valM);
	}
}

void stepMachine(int stepMode) {
  /* FETCH STAGE */
  int icode = 0, ifun = 0;
  int rA = 0, rB = 0;
  wordType valC = 0;
  wordType valP = 0;
 
  /* DECODE STAGE */
  wordType valA = 0;
  wordType valB = 0;

  /* EXECUTE STAGE */
  wordType valE = 0;
  bool Cnd = 0;

  /* MEMORY STAGE */
  wordType valM = 0;

  fetchStage(&icode, &ifun, &rA, &rB, &valC, &valP);
  applyStageStepMode(stepMode, "Fetch", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);

  decodeStage(icode, rA, rB, &valA, &valB);
  applyStageStepMode(stepMode, "Decode", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  executeStage(icode, ifun, valA, valB, valC, &valE, &Cnd);
  applyStageStepMode(stepMode, "Execute", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  memoryStage(icode, valA, valP, valE, &valM);
  applyStageStepMode(stepMode, "Memory", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  writebackStage(icode, rA, rB, valE, valM);
  applyStageStepMode(stepMode, "Writeback", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  pcUpdateStage(icode, valC, valP, Cnd, valM);
  applyStageStepMode(stepMode, "PC update", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);

  incrementCycleCounter();
}

/** 
 * main
 * */
int main(int argc, char **argv) {
  int stepMode = 0;
  FILE *input = parseCommandLine(argc, argv, &stepMode);

  initializeMemory(MAX_MEM_SIZE);
  initializeRegisters();
  loadMemory(input);

  applyStepMode(stepMode);
  while (getStatus() != STAT_HLT) {
    stepMachine(stepMode);
    applyStepMode(stepMode);
#ifdef DEBUG
    printMachineState();
    printf("\n");
#endif
  }
  printMachineState();
  return 0;
}