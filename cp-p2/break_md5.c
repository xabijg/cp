// @author : xabier.jimenez.gomez
// @author : martin.vieites

#include <sys/types.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define PASS_LEN 6
#define NUM_THREADS 8

struct args {
    int thread_num;
    char *passwd;
    bool *found;
    long start;
    long finish;
};

struct thread_info {
    pthread_t id;
    struct args *args;
};

long ipow(long base, int exp)
{
    long res = 1;
    for (;;)
    {
        if (exp & 1)
            res *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return res;
}

long pass_to_long(char *str) {
    long res = 0;

    for(int i=0; i < PASS_LEN; i++)
        res = res * 26 + str[i]-'a';

    return res;
};

void long_to_pass(long n, unsigned char *str) {  // str should have size PASS_SIZE+1
    for(int i=PASS_LEN-1; i >= 0; i--) {
        str[i] = n % 26 + 'a';
        n /= 26;
    }
    str[PASS_LEN] = '\0';
}

void to_hex(unsigned char *res, char *hex_res) {
    for(int i=0; i < MD5_DIGEST_LENGTH; i++) {
        snprintf(&hex_res[i*2], 3, "%.2hhx", res[i]);
    }
    hex_res[MD5_DIGEST_LENGTH * 2] = '\0';
}

void *break_pass (void *ptr) {

    struct args *args = ptr;
    long i;

    unsigned char res[MD5_DIGEST_LENGTH];
    char hex_res[MD5_DIGEST_LENGTH * 2 + 1];
    unsigned char *pass = malloc((PASS_LEN + 1) * sizeof(char));

    long start = args->start;
    long finish = args->finish;

    for(i=start; i < finish; i++) {
        if (*args->found == false){

            long_to_pass(i, pass);

            MD5(pass, PASS_LEN, res);
            to_hex(res, hex_res);

            if(!strcmp(hex_res, args->passwd)) {
                printf("%s: %s\n", args->passwd, (char *)pass);
                *args -> found = true;
                break;
            } // Found it!
        }
    }
    free(pass);
    return NULL;
}

struct thread_info *start_threads (char *passwd) {

    struct thread_info *threads;
    bool passwd_found = false;
    long bound = ipow(26, PASS_LEN);

    printf("Creating %d threads \n", NUM_THREADS);
    threads = malloc(sizeof(struct thread_info) * NUM_THREADS);
    for (int k = 0; k < NUM_THREADS; k++) {
        threads[k].args = malloc (sizeof(struct args));
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].args -> thread_num = i;
        threads[i].args -> passwd = passwd;
        threads[i].args -> found = &passwd_found;
        threads[i].args -> start = (bound/NUM_THREADS)*i;
        if (i == NUM_THREADS-1) {
            threads[i].args -> finish = bound;
        } else {
            threads[i].args -> finish = (bound/NUM_THREADS)*(i+1);
        }

        if (0 != pthread_create(&threads[i].id, NULL, break_pass, threads[i].args)) {
            printf("Could not create thread #%d\n", i);
            exit(1);
        }
    }
    return threads;
}

void wait (struct thread_info *threads) {

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i].id, NULL);
        free (threads[i].args);
    }
    free(threads);
}

int main(int argc, char *argv[]) {

    struct thread_info *threads;

    if(argc < 2) {
        printf("Use: %s string\n", argv[0]);
        exit(0);
    }

    threads = start_threads(argv[1]);
    wait(threads);

    return 0;
}
