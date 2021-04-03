/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer changed for the assignment 1 of RTES course.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.
 *
 *	Long 	:gg
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:George Spaias
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>

#define QUEUESIZE 200
#define LOOP 1000
#define PROD_T_NUM 2
#define CONS_T_NUM 1

void *producer(void *args);
void *consumer(void *args);
pthread_cond_t cond_main;
pthread_mutex_t mut_main;
bool areProducersFinished = false; // false if there are producers else true

typedef struct workFunction {
  void *(*work)(void *);
  void *arg;
} workfunction;

struct timeval t;
struct timeval *tic;
struct timeval *toc;

FILE *f;
int flag = 0;
int count = 0;

typedef struct {
  workfunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit(void);
void queueDelete(queue *q);
void queueAdd(queue *q, workfunction in);
void queueDel(queue *q, workfunction *out);

void *task_function() { printf("Executing Task...\n"); }
void prod_t(long tail,struct timeval *t) ;
void cons_t() ;

typedef struct {
  queue *q;
  int tid;
} pthread_data;

int main() {
  queue *fifo;
  pthread_t pro[PROD_T_NUM], con[CONS_T_NUM];
  pthread_mutex_init(&mut_main, NULL);
  pthread_cond_init(&cond_main, NULL);

  tic = (struct timeval *)malloc(QUEUESIZE * sizeof(struct timeval));
  toc = (struct timeval *)malloc(QUEUESIZE * sizeof(struct timeval));

 
  f = fopen("time_t.csv", "a");
  if (f == NULL) {
    printf("Error opening file3!\n");
    exit(1);
  }

  fifo = queueInit();
  if (fifo == NULL) {
    fprintf(stderr, "main: Queue Init failed.\n");
    exit(1);
  }
  
 // fprintf(f, "LOOP: %d, QUEUESIZE: %d ,PROD_T_NUM: %d , CONS_T_NUM: %d \n",LOOP,QUEUESIZE,PROD_T_NUM,CONS_T_NUM);

  pthread_data dataPro[PROD_T_NUM];
  pthread_data dataCon[CONS_T_NUM];

  int i;
  flag = 0;
  for (i = 0; i < PROD_T_NUM + CONS_T_NUM; i++) {
    if (i < PROD_T_NUM) {
      dataPro[i].tid = i;
      dataPro[i].q = fifo;
      if (pthread_create(&pro[i], NULL, producer, &dataPro[i]) != 0) {
        perror("Failed to create thread");
      }
    } else {
      dataCon[i-PROD_T_NUM].tid = i-PROD_T_NUM;
      dataCon[i-PROD_T_NUM].q = fifo;
      if (pthread_create(&con[i - PROD_T_NUM], NULL, consumer, &dataCon[i-PROD_T_NUM]) != 0) {
        perror("Failed to create thread");
      }
    }
  }
  for (i = 0; i < PROD_T_NUM; i++) {
    if (pthread_join(pro[i], NULL) != 0) {
      perror("Failed to join thread");
    }
  }
  areProducersFinished = true; // all producers exited
  printf("Producers joined successfuly\n");
  usleep(100000);

  
  pthread_mutex_lock(&mut_main);
  while (flag < CONS_T_NUM) {
    //printf("main thread waiting for consumers \n");
    pthread_cond_broadcast(fifo->notEmpty);
    //pthread_cond_wait(&cond_main, &mut_main);
  }
  printf("flag %d\n", flag);
   
  
  pthread_mutex_unlock(&mut_main);

  for (i = 0; i < CONS_T_NUM; i++) {
    if (pthread_join(con[i], NULL) != 0) {
      perror("Failed to join thread");
    }
  }

  queueDelete(fifo);

  
  fclose(f);

  pthread_mutex_destroy(&mut_main);
  pthread_cond_destroy(&cond_main);
  free(tic);
  free(toc);
  return 0;
}

void *producer(void *args) {

  queue *fifo;
  pthread_data *data = (pthread_data *)args;
  fifo = data->q;

  int i;
  for (i = 0; i < LOOP; i++) {
    workfunction task = {
        .work = task_function,

     };
    pthread_mutex_lock(fifo->mut);
    while (fifo->full) {
      printf("producer: queue FULL.\n");
      pthread_cond_wait(fifo->notFull, fifo->mut);
     }
    printf("producer: Created Task number %d.\n", i);
    
    queueAdd(fifo, task);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notEmpty);
    // usleep (100000);
  }
  return (NULL);
}

void *consumer(void *args) {
  queue *fifo;
  pthread_data *data = (pthread_data *)args;
  fifo = data->q;
  int tid = data->tid;

  workfunction d;

  while (1) {

    pthread_mutex_lock(fifo->mut);
    while (fifo->empty && !areProducersFinished) {
      printf("consumer: queue EMPTY %d.\n", tid);
      pthread_cond_wait(fifo->notEmpty, fifo->mut);
    }
    if (fifo->empty && areProducersFinished) {
        printf("thread waiting for flag with id %d\n", tid);
        //pthread_cond_signal(&cond_main);
        
        flag++;
        pthread_mutex_unlock(fifo->mut);
      break;
      }
   

    printf("consumer: recieved task %d.\n", count++);
    queueDel(fifo, &d);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notFull);

    // usleep(200000);
  }
  return (NULL);
}

/*
  typedef struct {
  int buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  } queue;
*/

queue *queueInit(void) {
  queue *q;

  q = (queue *)malloc(sizeof(queue));
  if (q == NULL)
    return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(q->mut, NULL);
  q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notEmpty, NULL);

  return (q);
}

void queueDelete(queue *q) {
  pthread_mutex_destroy(q->mut);
  free(q->mut);
  pthread_cond_destroy(q->notFull);
  free(q->notFull);
  pthread_cond_destroy(q->notEmpty);
  free(q->notEmpty);
  free(q);
}

void queueAdd(queue *q, workfunction in) {
  q->buf[q->tail] = in;
  prod_t(q->tail,tic);
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel(queue *q, workfunction *out) {
  *out = q->buf[q->head];
  cons_t(q->head,tic,toc) ;
  *(out->work)(NULL);
  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}

void prod_t(long tail,struct timeval *t){
    gettimeofday(t+tail,NULL);
    
}

void cons_t(long head,struct timeval *t1,struct timeval *t2){
    gettimeofday(t2+head,NULL);

    int t_micro = ((t2+head)->tv_usec)-((t1+head)->tv_usec);
    //int t_sec = ((t2+head)->tv_sec-(t1+head)->tv_sec)*1e6;
    
    if(t_micro<0){t_micro=(1000000+t_micro);}
    
    fprintf(f," %d \n",t_micro);
}
