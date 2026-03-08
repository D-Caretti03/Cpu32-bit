#include <ios>
#include <iostream>
#include <map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

class Encoder {
  public:
    // Enum to define all the possible operations
    enum class Opcode: uint8_t{
      OP_LOAD,
      OP_MOV,
      OP_ADD,
      OP_SUB,
      OP_MUL,
      OP_DIV,
      OP_JMP,
      OP_JZ,
      OP_JNZ,
      OP_INC,
      OP_DEC,
      OP_CALL,
      OP_RET,
      OP_JE,
      OP_JNE,
      OP_PRINT,
      OP_MOV_RES,
      OP_DONE
    };

    // Constructor of the encoder, calls LoadLabels, LoadAndParse and WriteBinary
    // @input name of the input file
    // @output name of the ouput file
    Encoder(const std::string& input, const std::string& output);

    // Default destructor
    ~Encoder() = default;

  private:
    // Read the file and insert all the labels into a map <string, size_t>
    // @filename name of the input file
    void LoadLabels(const std::string& filename);

    // Read the file and insert all the instruction encoded in the program vector
    // @filename name of the input file
    void LoadAndParse(const std::string& filename);

    // Write the program vector in binary mode inside the output file
    // @filename name of the output file
    void WriteBinary(const std::string& filename);

    // Check if a register is inside the bounds (0-15), if not the program exits with an error message
    // @reg number of the register taken from the source program
    // @line number of the actual line ( for a more precise message )
    void CheckReg(int reg, size_t line);

    // Vector containing all the instructions
    std::vector<uint32_t> mProgram;

    // Map of the labels
    std::map<std::string, uint16_t> mLabels;
};

Encoder::Encoder(const std::string& input, const std::string& output){
  LoadLabels(input);
  LoadAndParse(input);
  WriteBinary(output);
}

void Encoder::LoadLabels(const std::string& filename){
  std::ifstream file(filename);
  if (!file.is_open()){
    std::cerr << "Error: Unable to open file " << filename << '\n';
    std::exit(1);
  }

  std::string line;
  size_t lineCount = 1;

  while(std::getline(file, line)){
    // Trim the line
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos)
      continue;

    // Get the exact end of the line
    size_t end = line.find_last_not_of(" \t\r\n");
    line = line.substr(start, end - start + 1);

    // If it's a comment ';' put the end of the input line right before the comment 
    size_t comment = line.find(";");
    if (comment != std::string::npos) line = line.substr(0, comment);

    // If the line contains only a comment go to next line
    if (line.empty()) continue;

    // If a line ends with ':' it's a label
    if (line.back() == ':'){
      // Get the name of the label ( without the ':' )
      std::string label = line.substr(0, line.size() - 1);

      // If the map already contains this label -> error duplicate label
      if (mLabels.count(label)){
        std::cerr << "Error: Label " << label << " already exists\n";
        std::exit(1);
      }

      // Insert the label into the map
      mLabels[label] = lineCount;
      lineCount--; // the label line is gonna be removed so we decrement the line counter
      line.clear();
    }

    lineCount++;
  }
  file.close();
}

void Encoder::LoadAndParse(const std::string& filename){
  std::ifstream file(filename);
  if (!file.is_open()){
    std::cerr << "Error: Unable to open file " << filename << '\n';
    std::exit(1);
  }

  // Define the map of the opcodes
  std::map<std::string, Opcode> opcodeMap = {
    {"LOAD", Opcode::OP_LOAD},
    {"MOV", Opcode::OP_MOV},
    {"ADD", Opcode::OP_ADD},
    {"SUB", Opcode::OP_SUB},
    {"MUL", Opcode::OP_MUL},
    {"DIV", Opcode::OP_DIV},
    {"JMP", Opcode::OP_JMP},
    {"JZ", Opcode::OP_JZ},
    {"JNZ", Opcode::OP_JNZ},
    {"INC", Opcode::OP_INC},
    {"DEC", Opcode::OP_DEC},
    {"CALL", Opcode::OP_CALL},
    {"RET", Opcode::OP_RET},
    {"JE", Opcode::OP_JE},
    {"JNE", Opcode::OP_JNE},
    {"PRINT", Opcode::OP_PRINT},
    {"MOV_RES", Opcode::OP_MOV_RES},
    {"DONE", Opcode::OP_DONE}
  };

  std::string line;
  size_t lineNum = 0;

  while (std::getline(file, line)){
    ++lineNum;

    // Trim the line
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    size_t end = line.find_last_not_of(" \r\t\n");
    line = line.substr(start, end - start + 1);

    // Handles the comments
    size_t comment = line.find(";");
    if (comment != std::string::npos) line = line.substr(0, comment);

    // If it's a label it's already been handled
    if (!line.empty() && line.back() == ':'){
      line.clear();
    }

    // If it's an empty line, a comment or a label goto next line
    if (line.empty()) continue;
    
    // Get the opcode from the name of the operation
    std::istringstream iss(line);
    std::string opStr;
    iss >> opStr;

    // Check if exists, if not -> error  unknown opcode
    auto it = opcodeMap.find(opStr);
    if (it == opcodeMap.end()){
      std::cerr << "Error line " << lineNum << ": Unknown opcode\n";
      std::exit(1);
    }

    // Set the opcode value
    Opcode op = it->second;
    uint8_t opcode = static_cast<uint8_t>(op);

    uint8_t reg = 0;
    uint16_t imm = 0;

    /**
      * Handles all the opcodes and set the reg and imm
      * Then insert them into instruction with bit manipulation
      */
    if (op == Opcode::OP_LOAD){
      int regNum, immNum;
      if (!(iss >> regNum >> immNum)){
        std::cerr << "Error line " << lineNum << ": LOAD needs register and immediate\n";
        std::exit(1);
      }

      CheckReg(regNum, lineNum);

      reg = static_cast<uint8_t>(regNum);
      imm = static_cast<uint16_t>(immNum);
    } else if (op == Opcode::OP_MOV){
      int regDest, regSrc;
      if (!(iss >> regDest >> regSrc)){
        std::cerr << "Error line " << lineNum << ":MOV needs dest register and src register\n";
        std::exit(1);
      }

      CheckReg(regDest, lineNum);
      CheckReg(regSrc, lineNum);

      reg = static_cast<uint8_t>(regDest);
      imm = (static_cast<uint16_t>(regSrc) << 8);
    } else if (op == Opcode::OP_ADD || op == Opcode::OP_SUB || op == Opcode::OP_MUL || op == Opcode::OP_DIV){
      int regDest, regSrc1, regSrc2;
      if (!(iss >> regDest >> regSrc1 >> regSrc2)){
        std::cerr << "Error line " << lineNum << ": Arithmetics need 3 registers: dst, src1, src2\n";
        std::exit(1);
      }

      CheckReg(regDest, lineNum);
      CheckReg(regSrc1, lineNum);
      CheckReg(regSrc2, lineNum);

      reg = static_cast<uint8_t>(regDest);
      imm = (static_cast<uint16_t>(regSrc1) << 8) | static_cast<uint16_t>(regSrc2);
    } else if (op == Opcode::OP_JMP) {
      std::string label;
      if (!(iss >> label)){
        std::cerr << "Error line " << lineNum << ": JMP needs a label\n";
        std::exit(1);
      }

      auto iter = mLabels.find(label);
      if (iter == mLabels.end()){
        std::cerr << "Error line " << lineNum << ": Label " << label << " does not exists\n"; 
        std::exit(1);
      }

      imm = static_cast<uint16_t>(iter->second);
    } else if (op == Opcode::OP_JZ || op == Opcode::OP_JNZ){
      int regNum;
      std::string label;
      if (!(iss >> regNum >> label)){
        std::cerr << "Error line " << lineNum << ": Jumps Zero need the register and a label\n";
        std::exit(1);
      }

      auto iter = mLabels.find(label);
      if (iter == mLabels.end()){
        std::cerr << "Error line " << lineNum << ": Label " << label << " does not exists\n";
        std::exit(1);
      }

      reg = static_cast<uint8_t>(regNum);
      imm = static_cast<uint16_t>(iter->second);
    } else if (op == Opcode::OP_INC || op == Opcode::OP_DEC){
      int regNum, immNum;
      if (!(iss >> regNum >> immNum)){
        std::cerr << "Error line " << lineNum << ": INC and DEC need a register and a value\n";
        std::exit(1);
      }

      CheckReg(regNum, lineNum);

      reg = static_cast<uint8_t>(regNum);
      imm = static_cast<uint16_t>(immNum);
    } else if (op == Opcode::OP_CALL){
      std::string func;
      if(!(iss >> func)){
        std::cerr << "Error line " << lineNum << ": CALL needs a function name\n";
        std::exit(1);
      }

      auto iter = mLabels.find(func);
      if (iter == mLabels.end()){
        std::cerr << "Error line " << lineNum << ": Function " << func << " not found\n";
        std::exit(1);
      }

      imm = static_cast<uint16_t>(iter->second);
    } else if (op == Opcode::OP_JE || op == Opcode::OP_JNE){
      int reg1Num, reg2Num;
      std::string label;
      if (!(iss >> reg1Num >> reg2Num >> label)) {
        std::cerr << "Error line " << lineNum << ": JR need two registers and a label\n";
        std::exit(1);
      }

      auto iter = mLabels.find(label);
      if (iter == mLabels.end()){
        std::cerr << "Error line " << lineNum << ": Label " << label << " not found\n";
        std::exit(1);
      }

      reg = (static_cast<uint8_t>(reg1Num) << 4) | static_cast<uint8_t>(reg2Num & 0xF);
      imm = static_cast<uint16_t>(iter->second);
    } else if (op == Opcode::OP_PRINT){
      int regNum;
      if (!(iss >> regNum)) {
        std::cout << "Error line " << lineNum << ": Print needs a register\n";
        std::exit(1);
      }

      CheckReg(regNum, lineNum);

      reg = static_cast<uint8_t>(regNum);
    }
    else if (op == Opcode::OP_MOV_RES){
      int regNum;
      if (!(iss >> regNum)){
        std::cerr << "Error line " << lineNum << ": MOV_RES needs a register\n";
        std::exit(1);
      }

      CheckReg(regNum, lineNum);

      reg = static_cast<uint8_t>(regNum);
    }

    uint32_t instruction = (opcode << 24) | (reg << 16) | imm;
    mProgram.push_back(instruction);
  }

  file.close();
}

void Encoder::CheckReg(int reg, size_t line){
  if (reg < 0 || reg > 15){
    std::cerr << "Error line " << line << ": Register out of bounds (0-15)\n";
    std::exit(1);
  }
}

void Encoder::WriteBinary(const std::string& filename){
  std::ofstream file(filename, std::ios::binary);

  if (!file.is_open()){
    std::cerr << "Error opening file " << filename << '\n';
    std::exit(1);
  }

  // Write in little endian
  // First write imm and then opcode
  // Example: LOAD 1 10
  // opcode: 0
  // reg: 1
  // imm: 0a
  // So the instruction will be: 010a
  // In little endian: 0a01
  for (uint32_t instr: mProgram){
    file.put(static_cast<uint8_t>(instr & 0xFF));
    file.put(static_cast<uint8_t>(instr >> 8) % 0xFF);
    file.put(static_cast<uint8_t>(instr >> 16) & 0xFF);
    file.put(static_cast<uint8_t>(instr >> 24) & 0xFF);
  }

  file.close();
}


int main(int argc, char **argv){
  if (argc != 3){
    std::cerr << "Usage: " << argv[0] << " <input file> <output file>\n";
    std::exit(1);
  }

  std::string input = argv[1];
  std::string output = argv[2];

  Encoder e = Encoder(input, output);
}
