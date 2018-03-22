#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <time.h>

#define FORKSRV_FD          198

uint64_t total = 0, smallest = (uint64_t) -1, largest = 0;
int32_t fsrv_ctl_fd, fsrv_st_fd, shm_id;
uint32_t verbose = 0, no = 0;
uint8_t *trace_bits = NULL, *prog = NULL;

void init_forkserver(char **argv) {
  int32_t st_pipe[2], ctl_pipe[2], i = 0;
  uint8_t id_string[256], ex[4096] = "", *pre;

  if (pipe(st_pipe) || pipe(ctl_pipe)) {
    fprintf(stderr, "Error: pipe() failed \n");
    exit(-1);
  }

  int32_t forksrv_pid = fork();

  if (forksrv_pid < 0)
    fprintf(stderr, "Error: fork() failed \n");

  if (forksrv_pid == 0) {
    setsid();
    if (dup2(ctl_pipe[0], FORKSRV_FD) < 0)
      fprintf(stderr, "Error: dup2() failed \n");
    if (dup2(st_pipe[1], FORKSRV_FD + 1) < 0)
      fprintf(stderr, "Error: dup2() failed \n");

    close(ctl_pipe[0]);
    close(ctl_pipe[1]);
    close(st_pipe[0]);
    close(st_pipe[1]);
    i = 0;
    while (argv[i] != NULL) {
      strcat(ex, argv[i]);
      strcat(ex, " ");
      i++;
    }

    sprintf(id_string, "%d", shm_id);
    setenv("__AFL_SHM_ID", id_string, 1);
    
    if (verbose) fprintf(stderr, "SHM_ID=%s\n", id_string);
    if ((pre = getenv("AFL_PRELOAD")) != NULL) {
      setenv("LD_PRELOAD", pre, 1);
      if (verbose) fprintf(stderr, "LD_PRELOAD=%s\n", pre);
    }
    if (verbose) fprintf(stderr, "cmdline=%s\n", ex);

    (system(ex)+1);

    fprintf(stderr, "END=client finished\n");

    exit(0);
  }

  close(ctl_pipe[0]);
  close(st_pipe[1]);

  fsrv_ctl_fd = ctl_pipe[1];
  fsrv_st_fd = st_pipe[0];

  int32_t status = 0;
  int32_t rlen = read(fsrv_st_fd, &status, 4);

  if (rlen == 4) {
    if (verbose) fprintf(stderr, "READY=fork server is up.\n");
    return;
  }

  fprintf(stderr, "Error: init fork server fail\n");
  exit(-1);
}

void run() {
  int32_t res, child_pid, sth = 0, status;
  uint64_t fills = 0, diff;
  uint32_t buckets = 0, counter, sec, tsec;
  struct timespec ts1, ts2;

  memset(trace_bits, 0, 65536);

  if ((res = write(fsrv_ctl_fd, &sth, 4)) != 4) {
    fprintf(stderr, "Error: unable to request new process from fork server -- write fd!\n");
    return;
  }

  res = read(fsrv_st_fd, &child_pid, 4);
  clock_gettime(CLOCK_REALTIME, &ts1);
  if (res != 4) {
    fprintf(stderr, "Error: unable to request new preocess from fork server -- recv child_pid!\n");
    return;
  }

  if (child_pid <= 0) {
    fprintf(stderr, "Error: invalid pid received\n");
    return;
  }
  if (verbose) fprintf(stderr, "child_pid=%d\n", child_pid);

  res = read(fsrv_st_fd, &status, 4);
  clock_gettime(CLOCK_REALTIME, &ts2);
  if (res != 4) {
    fprintf(stderr, "Error: unable to communicate with fork server !\n");
    return;
  }

  if (verbose) fprintf(stderr, "result=%d\n", status);

  no++;
  for (counter = 0; counter < 65536; counter++) {
    if (trace_bits[counter] != 0) {
      buckets++;
      fills += trace_bits[counter];
    }
  }
  diff = ((ts2.tv_sec - ts1.tv_sec) * 1000000000) + (ts2.tv_nsec - ts1.tv_nsec);
  if (smallest > diff)
    smallest = diff;
  if (largest < diff)
    largest = diff;
  total += diff;
  sec = diff / 1000000000;
  tsec = (diff % 1000000000) / 1000;
  fprintf(stderr, "%s run=%d time=%u.%06d result=%d buckets=%u fills=%lu\n", prog, no, sec, tsec, status, buckets, fills);
}

int main(int argc, char **argv) {
  uint32_t iter = 10, sec1, sec2, sec3, tsec1, tsec2, tsec3, i;
  int32_t sth = 0;

  if (argc < 2 || strcmp(argv[1], "-h") == 0) {
    printf("Syntax: %s [-i count] program args\n", argv[0]);
    printf("Default iterations are 10\n");
    return 0;
  }
  if (strcmp(argv[1], "-i") == 0) {
    iter = atoi(argv[2]);
    argv += 2;
    argc -= 2;
  }
  prog = argv[1];
  
  if (getenv("AFL_VERBOSE") != NULL)
    verbose = 1;

  shm_id = shmget(IPC_PRIVATE, 65536, IPC_CREAT | IPC_EXCL | 0600);

  init_forkserver(&argv[1]);

  if ((trace_bits = (uint8_t *) shmat(shm_id, NULL, 0)) == (void *) -1 || trace_bits == NULL) {
    fprintf(stderr, "Error: can not access shm\n");
    exit(-1);
  }

  for (i = 0; i < iter; i++)
    run();

  // end it
  if (write(fsrv_ctl_fd, &sth, 2) != 2) {
    fprintf(stderr, "Error: failed to break loop\n");
  }
  sleep(1);

  total = total / no;
  sec1 = total / 1000000000;
  tsec1 = (total % 1000000000) / 1000;
  sec2 = smallest / 1000000000;
  tsec2 = (smallest % 1000000000) / 1000;
  sec3 = largest / 1000000000;
  tsec3 = (largest % 1000000000) / 1000;
  fprintf(stderr, "Average=%u.%06u min=%u.%06u max=%u.%06u\n", sec1, tsec1, sec2, tsec2, sec3, tsec3);

  return 0;
}
