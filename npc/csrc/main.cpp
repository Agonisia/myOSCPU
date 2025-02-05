#include <memory/paddr.h>
#include <monitor/sdb.h>

void monitor_init(int, char *[]);
int is_exit_status_bad();

int main(int argc, char** argv) {
  monitor_init(argc, argv);

  sim_init(argc, argv);

  sdb_mainloop();

  sim_exit();

  return is_exit_status_bad();
}
