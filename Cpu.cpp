#include <algorithm>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <iomanip>
#include <ostream>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <time.h>
#include <functional>

#define DEBUG
//#define DUMP
//#define STATS

const unsigned int MEMORY_SIZE = 16384;

class Cpu {
  public:
    Cpu();

    void Reset(){
      Pc = 0;
      halted = false;
      std::fill(std::begin(mRegisters), std::end(mRegisters), 0);
      std::fill(std::begin(mMemory), std::end(mMemory), 0);
    }

    bool LoadProgram (const std::string& filename);
    void Run();

  private:
    uint32_t Fetch();
    void Execute();
    void DumpRegisters();

    void Load();
    void Mov();
    void Add();
    void Sub();
    void Mul();
    void Div();
    void Jmp();
    void Jz();
    void Jnz();
    void Inc();
    void Dec();
    void Call();
    void Ret();
    void Je();
    void Jne();
    void Print();
    void MovRes();
    void Done();
    
    uint8_t mMemory[MEMORY_SIZE]{};
    uint64_t mRegisters[16] {};
    uint16_t Pc = 0;
    std::vector<uint16_t> Sp {};
    std::vector<uint16_t> Stack {};
    uint32_t mInstr;
    bool halted = false;

    #ifdef STATS
    size_t numInstr = 0;
    #endif

    typedef void (Cpu::*CpuFunc) ();
    using CpuF = std::function<void()>;
    CpuF table[0xFF];
};

Cpu::Cpu(){
  Reset();

  table[0x0] = [this](){Load();};
  table[0x1] = [this](){Mov();};
  table[0x2] = [this](){Add();};
  table[0x3] = [this](){Sub();};
  table[0x4] = [this](){Mul();};
  table[0x5] = [this](){Div();};
  table[0x6] = [this](){Jmp();};
  table[0x7] = [this](){Jz();};
  table[0x8] = [this](){Jnz();};
  table[0x9] = [this](){Inc();};
  table[0xA] = [this](){Dec();};
  table[0xB] = [this](){Call();};
  table[0xC] = [this](){Ret();};
  table[0xD] = [this](){Je();};
  table[0xE] = [this](){Jne();};
  table[0xF] = [this](){Print();};
  table[0x10] = [this](){MovRes();};
  table[0x11] = [this](){Done();};
}

bool Cpu::LoadProgram(const std::string& filename){
  std::ifstream file(filename, std::ios::binary);

  if (!file.is_open()){
    std::cerr << "Error while opening file " << filename << "\n";
    std::exit(1);
  }

  file.read(reinterpret_cast<char *>(mMemory), MEMORY_SIZE);  
  size_t bytesRead = file.gcount();
  file.close();

  if (bytesRead % 2 != 0){
    std::cerr << "Warning: Odd number of bytes in program\n";
  }

  #ifdef DEBUG
  std::cout << "Loaded " << bytesRead << " bytes into memory\n";
  #endif
  return true;
}

void Cpu::Run(){
  while(!halted && Pc < MEMORY_SIZE - 1){
    Execute();
    #ifdef STATS
    numInstr++;
    #endif
  }

  DumpRegisters();
  std::cout << "Final result (R0): " << static_cast<int>(mRegisters[0]) << '\n';
  #ifdef STATS
  std::cout << "Completed in " << numInstr << " instructions\n";
  #endif
}

uint32_t Cpu::Fetch(){
  if (Pc + 1u >= MEMORY_SIZE){
    std::cerr << "Error: Pc out of bounds\n";
    halted = true;
    return 0;
  }

  uint32_t instr = mMemory[Pc] | (mMemory[Pc + 1] << 8) | (mMemory[Pc + 2] << 16) | (mMemory[Pc + 3] << 24);
  #ifdef DEBUG
  std::cout << "\x1b[1J\x1b[H";
  usleep(500000);
  DumpRegisters();
  std::cout << "#" << static_cast<int>((Pc /2) + 1) << ": ";
  #endif
  Pc += 4;
  return instr;
}

void Cpu::Execute(){
  mInstr = Fetch();

  table[(mInstr >> 24) & 0xFF]();

  #ifdef DUMP
  DumpRegisters();
  #endif
}

void Cpu::DumpRegisters(){
  for (int i = 0; i < 16; ++i){
    std::cout << "R" << std::dec << i << ": " << std::setw(3) << static_cast<int>(mRegisters[i]);
    if ((i + 1) % 8 == 0) std::cout << '\n';
    else std::cout << "  ";
  }
  std::cout << '\n';
}

void Cpu::Load(){
  uint8_t reg = (mInstr >> 16) & 0xFF;
  uint16_t imm = mInstr & 0xFFFF;

  mRegisters[reg] = imm;
  #ifdef DEBUG
  std::cout << "LOAD " << static_cast<int>(reg) << ", " << static_cast<int>(imm) << '\n';
  #endif
}

void Cpu::Mov(){
  uint8_t dest = (mInstr >> 16) & 0xFF;
  uint8_t src = (mInstr >> 8) & 0xFF;

  mRegisters[dest] = mRegisters[src];
  #ifdef DEBUG
  std::cout << "MOV " << static_cast<int>(dest) << ", " << static_cast<int>(src) << '\n';
  #endif
}

void Cpu::Add(){
  uint8_t dest = (mInstr >> 16) & 0xFF;
  uint8_t src1 = (mInstr >> 8) & 0xFF;
  uint8_t src2 = mInstr & 0xFF;

  mRegisters[dest] = mRegisters[src1] + mRegisters[src2];
  #ifdef DEBUG
  std::cout << "ADD " << static_cast<int>(dest) << ", " << static_cast<int>(src1) << ", " << static_cast<int>(src2) << '\n';
  #endif
}

void Cpu::Sub(){
  uint8_t dest = (mInstr >> 16) & 0xFF;
  uint8_t src1 = (mInstr >> 8) & 0xFF;
  uint8_t src2 = mInstr & 0xFF;

  mRegisters[dest] = mRegisters[src1] - mRegisters[src2];
  #ifdef DEBUG
  std::cout << "SUB " << static_cast<int>(dest) << ", " << static_cast<int>(src1) << ", " << static_cast<int>(src2) << '\n';
  #endif
}

void Cpu::Mul(){
  uint8_t dest = (mInstr >> 16) & 0xFF;
  uint8_t src1 = (mInstr >> 8) & 0xFF;
  uint8_t src2 = mInstr & 0xFF;

  mRegisters[dest] = mRegisters[src1] * mRegisters[src2];
  #ifdef DEBUG
  std::cout << "MUL " << static_cast<int>(dest) << ", " << static_cast<int>(src1) << ", " << static_cast<int>(src2) << '\n';
  #endif
}

void Cpu::Div(){
  uint8_t dest = (mInstr >> 16) & 0xFF;
  uint8_t src1 = (mInstr >> 8) & 0xFF;
  uint8_t src2 = mInstr & 0xFF;

  if (mRegisters[src2] == 0){
    std::cerr << "ERROR: division by zero\n";
    std::exit(1);
  }

  mRegisters[dest] = mRegisters[src1] / mRegisters[src2];
  #ifdef DEBUG
  std::cout << "DIV " << static_cast<int>(dest) << ", " << static_cast<int>(src1) << ", " << static_cast<int>(src2) << '\n';
  #endif
}

void Cpu::Jmp(){
  uint16_t pc = mInstr & 0xFFFF; 

  Pc = (pc-1) * 4;
  #ifdef DEBUG
  std::cout << "JMP " << static_cast<int>(pc) << '\n';
  #endif
}

void Cpu::Jz(){
  uint8_t reg = (mInstr >> 16) & 0xFF;
  uint16_t pc = mInstr & 0xFFFF;

  if (!mRegisters[reg]){
    Pc = (pc - 1) * 4;
  }
  #ifdef DEBUG
  std::cout << "JZ " << static_cast<int>(reg) << ", " << static_cast<int>(pc) << '\n';
  #endif
}

void Cpu::Jnz(){
  uint8_t reg = (mInstr >> 16) & 0xFF;
  uint16_t pc = mInstr & 0xFFFF;

  if (mRegisters[reg]){
    Pc = (pc - 1) * 4;
  }

  #ifdef DEBUG
  std::cout << "JNZ " << static_cast<int>(reg) << ", " << static_cast<int>(pc) << '\n';
  #endif
}

void Cpu::Inc(){
  uint8_t reg = (mInstr >> 16) & 0xFF;
  uint16_t imm = mInstr & 0xFFFF;

  mRegisters[reg] += imm;
  #ifdef DEBUG
  std::cout << "INC " << static_cast<int>(reg) << ", " << static_cast<int>(imm) << '\n';
  #endif
}

void Cpu::Dec(){
  uint8_t reg = (mInstr >> 16) & 0xFF;
  uint16_t imm = mInstr & 0xFFFF;

  mRegisters[reg] -= imm;
  #ifdef DEBUG
  std::cout << "DEC " << static_cast<int>(reg) << ", " << static_cast<int>(imm) << '\n';
  #endif
}

void Cpu::Call(){
  uint16_t pc = mInstr & 0xFFFF;

  Stack.push_back(mRegisters[15]);
  Sp.push_back(Pc);
  Pc = (pc - 1) * 4;
  #ifdef DEBUG
  std::cout << "CALL " << static_cast<int>(pc) << '\n';
  #endif
}

void Cpu::Ret(){
  mRegisters[15] = Stack.back();
  Stack.pop_back();

  Pc = Sp.back();
  Sp.pop_back();
  #ifdef DEBUG
  std::cout << "RET\n";
  #endif
}

void Cpu::Je(){
  uint8_t reg1 = (mInstr >> 20) & 0xF;
  uint8_t reg2 = (mInstr >> 16) & 0xF;
  uint16_t pc = mInstr & 0xFFFF;

  if (mRegisters[reg1] == mRegisters[reg2]){
    Pc = (pc - 1) * 4;
  }
  #ifdef DEBUG
  std::cout << "JRE " << static_cast<int>(reg1) << ", " << static_cast<int>(reg2) << ", " << static_cast<int>(pc) << '\n';
  #endif
}

void Cpu::Jne(){
  uint8_t reg1 = (mInstr >> 20) & 0xF;
  uint8_t reg2 = (mInstr >> 16) & 0xF;
  uint16_t pc = mInstr & 0xFFFF;

  if (mRegisters[reg1] != mRegisters[reg2]){
    Pc = (pc - 1) * 4;
  }
  #ifdef DEBUG
  std::cout << "JRNE " << static_cast<int>(reg1) << ", " << static_cast<int>(reg2) << ", " << static_cast<int>(pc) << '\n';
  #endif
}

void Cpu::Print(){
  uint8_t reg = (mInstr >> 16) & 0xF;

  std::cout << static_cast<int>(mRegisters[reg]) << std::endl;
  #ifdef DEBUG
  std::cout << "PRINT " << static_cast<int>(reg) << '\n';
  #endif
}

void Cpu::MovRes(){
  uint8_t reg = (mInstr >> 16) & 0xFF;
  
  mRegisters[0] = mRegisters[reg];
  #ifdef DEBUG
  std::cout << "MOV_RES " << static_cast<int>(reg) << '\n';
  #endif
}

void Cpu::Done(){
  halted = true;
  #ifdef DEBUG
  std::cout << "DONE\n";
  #endif
}

int main(int argc, char **argv){
  if (argc != 2){
    std::cerr << "Usage: " << argv[0] << " <filename>\n";
    std::exit(1);
  }

  std::string filename = argv[1];

  Cpu cpu = Cpu();
  cpu.LoadProgram(filename);
  cpu.Run();
}
