#include "memory/paddr.h"

void sdb_set_batch_mode();
void difftest_init(char *ref_so_file, long img_size, int port);
void parse_elf(const char *elf_file);
void log_init(const char *log_file);
void mem_init();
void sdb_init();
void device_init();

static char *elf_file = NULL;
static char *log_file = NULL;
static char *img_file = NULL;
static char *diff_so_file = NULL;
static int difftest_port = 3614;

void welcome() {
  Log("ITrace: %s", MUXONE(CONFIG_ITRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  Log("MTrace: %s", MUXONE(CONFIG_MTRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  Log("FTrace: %s", MUXONE(CONFIG_FTRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  Log("DTrace: %s", MUXONE(CONFIG_DTRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  Log("Difftest: %s", MUXONE(CONFIG_DIFFTEST, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));

  Log("Build time: %s, %s", __TIME__, __DATE__);

  printf("Welcome to RockBottom-%s\n", ANSI_FMT(str(CONFIG_ISA), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("\"It's only up from there\"\n");
  printf("For help, type \"help\"\n");
}

static long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}

static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"elf"      , required_argument, NULL, 'e'},
    {"diff"     , required_argument, NULL, 'd'},
    {"help"     , no_argument      , NULL, 'h'},
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bl:e:d:h", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;
      case 'l': log_file = optarg; break;
      case 'e': elf_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 1: img_file = optarg; return 0;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");  // batch mode 
        printf("\t-l,--log=FILE           output log to FILE\n");   // output log
        printf("\t-e,--elf=FILE           parse given ELF FILE\n"); // parse elf  
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");  // diffset
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void monitor_init(int argc, char *argv[]) {
  /* Parse arguments. */
  parse_args(argc, argv);

  /* Open the log file. */
  log_init(log_file);

  /* Parse elf file. */
  IFONE(CONFIG_FTRACE, parse_elf(elf_file));

  /* Initialize memory. */
  mem_init();  

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Initialize differential testing. */
  IFONE(CONFIG_DIFFTEST, difftest_init(diff_so_file, img_size, difftest_port));

  IFONE(CONFIG_DEVICE, device_init());

  /* Display welcome message. */
  welcome();

  /* Initialize the simple debugger. */
  sdb_init();
}