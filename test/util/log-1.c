
#include "log.h"

#include <time.h>

int
main()
{
  setbuf(stdout, NULL);
  mcsh_logger logger;
  mcsh_log_init(&logger, 0, MCSH_INFO);

  time_t my_time_t = time(NULL);
  struct tm my_tm;
  localtime_r(&my_time_t, &my_tm);
  char s[128];
  size_t n = strftime(s, 128, "%Y-%m-%d %H:%M:%S", &my_tm);
  printf("n=%zi\n", n);
  printf("s='%s'\n", s);

  mcsh_log(&logger, MCSH_LOG_NULL,  MCSH_INFO, "HELLO");
  mcsh_log(&logger, MCSH_LOG_PARSE, MCSH_INFO, "P ");
  mcsh_log(&logger, MCSH_LOG_EXEC,  MCSH_INFO, "E ");

  printf("OK\n");

  return 0;
}
