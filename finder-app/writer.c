#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
  openlog(NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
  if (argc < 3) {
    syslog(LOG_ERR, "!!! One or more aruguments not specified !!!");
    closelog();
    return 1;
  } else if (argc > 3) {
    syslog(LOG_ERR, "!!! More aruguments specified than expected!!!");
    closelog();
    return 1;
  }
  if ((argv[2] == NULL) || (strlen(argv[2]) <= 0)) {
    syslog(LOG_ERR, "Incorrect string");
    closelog();
    return 1;
  }
  int fd = 0;
  fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    syslog(LOG_ERR, "Failed to open file %s", argv[1]);
  } else {
    write(fd, argv[2], strlen(argv[2]));
    write(fd,"\n", strlen("\n"));
    syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
  }

  close(fd);
  closelog();
  return 0;
}
