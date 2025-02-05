#include "monitor/tracer.h"

static bool elf_parse_flag = false;

typedef struct {
    char name[32];  // symbol name
    paddr_t addr; // address of the function head 
    Elf32_Xword size;
} Symbol;

Symbol *symbol = NULL;  // dynamic allocation of symbol array
int func_num = 0;       // function counter
int depth = 1;          // function stack depth

/*  assist function to handle file reading errors  */
void check_read(int n, size_t size, FILE *fp, const char *msg) {
  if (n < size) {
    // if acturally read n bytes and not eauql to expect size
    perror(msg);
    fclose(fp);
    exit(EXIT_FAILURE);
  }
}

/*  parse ELF file and extract function symbols  */
void parse_elf(const char *elf_file) {
  if (elf_file == NULL) {
    Log("Invalid ELF file path. Parse unavailable");
    return;
  }

  Log("parsing ELF file: %s", elf_file);

  FILE *fp = fopen(elf_file, "rb");
  if (fp == NULL) {
    perror("Fail to open ELF file");
    return;
  }

  // read ELF header
  Elf32_Ehdr elf_header;
  check_read(fread(&elf_header, sizeof(Elf32_Ehdr), 1, fp),
              1, fp, "Fail to read ELF header");

  // check if the file is a valid ELF
  if (elf_header.e_ident[0] != 0x7f || elf_header.e_ident[1] != 'E' ||
      elf_header.e_ident[2] != 'L'  || elf_header.e_ident[3] != 'F') {
        // e_ident is not equal with 7f 45 4c 46
        fprintf(stderr, "Not a valid ELF file\n");
        fclose(fp);
        return;
  }

  // read section headers
  fseek(fp, elf_header.e_shoff, SEEK_SET); // move offset pointer to the beginning
  Elf32_Shdr *section_headers = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr) * elf_header.e_shnum); // section struct * num of section
  check_read(fread(section_headers, sizeof(Elf32_Shdr), elf_header.e_shnum, fp), 
              elf_header.e_shnum, fp, "Fail to read section headers");

  // get the string table 
  char *string_table = NULL;
  for (int i = 0; i < elf_header.e_shnum; i++) {
    if (section_headers[i].sh_type == SHT_STRTAB) {
      // find string table
      string_table = (char *)malloc(section_headers[i].sh_size);
      fseek(fp, section_headers[i].sh_offset, SEEK_SET); // move offset pointer to the begin of string table
      check_read(fread(string_table, section_headers[i].sh_size, 1, fp), // fread it to string_table
                  1, fp, "Fail to read string table");
      break;
    }
  }

  // get the symbol table
  fseek(fp, elf_header.e_shoff, SEEK_SET);
  Elf32_Sym *symbol_table = NULL;
  for (int i = 0; i < elf_header.e_shnum; i++) {
    if (section_headers[i].sh_type == SHT_SYMTAB) {
      // find symbol table
      fseek(fp, section_headers[i].sh_offset, SEEK_SET); // move offset pointer to the begin of symbol table
      size_t symbol_count = section_headers[i].sh_size / section_headers[i].sh_entsize; // size of symbol table / size of single symbol
      symbol_table = (Elf32_Sym *)malloc(sizeof(Elf32_Sym) * symbol_count);  // allocate for symbol table
      check_read(fread(symbol_table, section_headers[i].sh_entsize, symbol_count, fp), // fread it to symbol_table
                  symbol_count, fp, "Fail to read symbol table");
      symbol = (Symbol *)malloc(sizeof(Symbol) * symbol_count); // allocate for extracted symbol array

      // get function symbols
      for (size_t j = 0; j < symbol_count; j++) {
        if (ELF32_ST_TYPE(symbol_table[j].st_info) == STT_FUNC) {
          // symbol type is FUNC
          const char *name = string_table + symbol_table[j].st_name; // func name = string_table + st_name(offset/index) 
          strncpy(symbol[func_num].name, name, sizeof(symbol[func_num].name) - 1); // copy func name to symbol.name
          symbol[func_num].addr = symbol_table[j].st_value; // get func initial addr
          symbol[func_num].size = symbol_table[j].st_size;  // get func size
          func_num++;
        }
      }
      break;
    }
  }

  elf_parse_flag = true;  // set flag to true

  // close the file and clean up
  fclose(fp);
  free(section_headers);
  free(string_table);
  free(symbol_table);
}


int find_func(vaddr_t addr) {
  for(int i = 0; i < func_num; i++) {
      if(addr >= symbol[i].addr && addr < (symbol[i].addr + symbol[i].size)) {
          return i;
      }
  }
  return -1; // unfind
}

void print_trace(vaddr_t addr_curr, int func_index, const char* type) {
  printf("0x%08x:", addr_curr);
  for(int k = 0; k < depth; k++) printf("  ");
  printf("%s  [%s@0x%08x]\n", type, symbol[func_index].name, symbol[func_index].addr);
}

void func_call_trace(vaddr_t addr_curr, vaddr_t addr_func) {
  if (!elf_parse_flag) {
    printf("ELF file not parsed, ftrace invalid\n");
    return;
  }

  int func_index = find_func(addr_func);
  if (func_index == -1) {
    return;
  } 

  print_trace(addr_curr, func_index, "call");
  depth++; // add depth after printing, make sure call and ret of sigle function keep in same level
}

void func_ret_trace(vaddr_t addr_curr) {
  if (!elf_parse_flag) {
    printf("ELF file not parsed, ftrace invalid\n");
    return;
  }
  
  int func_index = find_func(addr_curr);
  if (func_index == -1) {
    return;
  }

  depth--; // sub depth before ret printing 
  print_trace(addr_curr, func_index, "ret");
}
