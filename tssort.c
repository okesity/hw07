#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "float_vec.h"
#include "barrier.h"
#include "utils.h"

const char* outfname;

typedef struct info {
  floats* data;
  long data_size;
  floats* samp;
  barrier* bb;
  int index;
  long* sum;
} info;

info* 
make_info(floats* data, long data_size, floats* samp, barrier* bb, int index, long* sum) {
  info* res = malloc(sizeof(info));
  res->data = data;
  res->data_size = data_size;
  res->samp = samp;
  res->bb = bb;
  res->index = index;
  res->sum = sum;
  return res;
}

int compar(const void * a, const void * b) {
    float v1 = *((const float *)a);
    float v2 = *((const float *)b);
    return (v1 > v2) - (v1 < v2);
}
void
qsort_floats(floats* xs)
{
    qsort(xs->data, xs->size, sizeof(float), compar);
}

floats*
sample(floats* data, long size, int P)
{
    int num = 3 * (P - 1);
    floats* samp = make_floats(num);
    for(int i=0;i<num;i++) {
        long ran = random() % size;
        floats_push(samp, data->data[ran]);
    }
    qsort_floats(samp);
    
    floats* res = make_floats(P + 1);
    floats_push(res, 0);
    for(int i=0;i<num;i++) {
        if(i % 3 == 1) {
            floats_push(res, samp->data[i]);
        }
    }
    floats_push(res, __FLT_MAX__);
    return res;
}

void*
sort_worker_thread(void* arg) {
  info* in = (info*) arg;
  float lower = in->samp->data[in->index];
  float upper = in->samp->data[in->index + 1];

  float* data = in->data->data;
  floats* xs = make_floats(10);
  for(int i=0; i<in->data_size; ++i) {
    if(compar(&data[i], &lower) >= 0 && compar(&data[i], &upper) < 0) {
        floats_push(xs, data[i]);
    }
  }
  qsort_floats(xs);
  in->sum[in->index] = xs->size;

  barrier_wait(in->bb);
  printf("%d: start %.04f, count %ld\n", in->index, lower, xs->size);

  int fd;
  fd = open(outfname, O_CREAT | O_WRONLY, 0644);
  check_rv(fd);

  long start_i = 0;
  for(int i=0; i<in->index; i++) {
    start_i += in->sum[i];
  }

  // printf("wrting starting at %ld, size %ld\n", start_i, xs->size);
  off_t rv;
  rv = lseek(fd, sizeof(float)*start_i, SEEK_SET);
  check_rv(rv);
  ssize_t wrv;
  wrv = write(fd, xs->data, xs->size * sizeof(float));
  check_rv(wrv);

  return 0;
}

void* 
run_sort_workers(floats* data, long size, int P, floats* samps, barrier* bb)
{
  int rv;
  pthread_t threads[P];
  long sum[P];
  for (int i = 0; i < P; ++i) {
        info* in = make_info(data, size, samps, bb, i, sum);   
        rv = pthread_create(&(threads[i]), 0, sort_worker_thread, in);
        check_rv(rv);
  }
  for (int i = 0; i < P; ++i) {
    rv = pthread_join(threads[i], 0);
    check_rv(rv);
  }
  return 0;
}


void
sample_sort(floats* data, long size, int P, barrier* bb)
{
  floats* samps = sample(data, size, P);
  // puts("sampled:");
  // floats_print(samps);

  run_sort_workers(data, size, P, samps, bb);
}

int main(int argc, char* argv[]) {

  
  if (argc != 4) {
      printf("Usage:\n");
      printf("\t%s P data.dat data.out\n", argv[0]);
      return 1;
  }

  const long P = atol(argv[1]);
  const char* fname = argv[2];
  outfname = argv[3];

  seed_rng();

  int rv;
  struct stat st;
  rv = stat(fname, &st);
  check_rv(rv);
  
  const int fsize = st.st_size;
  if (fsize < 8) {
      printf("File too small.\n");
      return 1;
  }
  long num;
  int fd = open(fname, O_RDONLY);
  check_rv(fd);

  read(fd, &num, sizeof(long));

  floats* data = make_floats(num);
  // printf("size: %ld\n", num);
  
  float buf[num];
  rv = read(fd, buf, num *sizeof(float));
  check_rv(rv);  
  for(long i=0;i<num;++i) {
    floats_push(data, buf[i]);
  }
  // floats_print(data);

  barrier* bb = make_barrier(P);
  sample_sort(data, num, P, bb);

  // float res[num];
  // int outfd = open(outfname, O_RDONLY);
  // check_rv(fd);
  // rv = read(outfd, res, num*sizeof(float));
  // for(long i=0;i<num; i++) {
  //   printf("res: %f\n", res[i]);
  // }

  return 0;
}
