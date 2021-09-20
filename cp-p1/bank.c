//Xabier Jimenez Gomez/xabier.jimenez.gomez@udc.es & Martin Vieites Garcia martin.vieites@udc.es 

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include "options.h"

#define MAX_AMOUNT 20
#define TIMEOUT 10

struct account {
    int balance;
    pthread_mutex_t mutex;
    pthread_cond_t not_enough_money;
};

struct bank {
	int num_accounts;               // number of accounts
	struct account *accounts;       // balance array
};

struct args {
	int		thread_num;       // application defined thread #
	int		delay;			  // delay between operations
	int		iterations;       // number of operations
	int     net_total;        // total amount deposited by this thread
	struct bank *bank;        // pointer to the bank (shared with other threads)
};

struct thread_info {
	pthread_t    id;        // id returned by pthread_create()
	struct args *args;      // pointer to the arguments
};

// Threads run on this function
void *deposit(void *ptr)
{
	struct args *args =  ptr;
	int amount, account, balance;

	while(args->iterations--) {
        //printf("Iterations deposit %d\n", args->iterations);
		amount  = rand() % MAX_AMOUNT;
		account = rand() % args->bank->num_accounts;
        pthread_mutex_lock(&args->bank->accounts[account].mutex);

		printf("Thread %d depositing %d on account %d\n",
			args->thread_num, amount, account);

		balance = args->bank->accounts[account].balance;
		if(args->delay) usleep(args->delay); // Force a context switch

		balance += amount;
		if(args->delay) usleep(args->delay);

		args->bank->accounts[account].balance = balance;
		if(args->delay) usleep(args->delay);

		args->net_total += amount;
        pthread_cond_broadcast(&args->bank->accounts[account].not_enough_money);
        pthread_mutex_unlock(&args->bank->accounts[account].mutex);
	}
	return NULL;
}

void *transfer (void *ptr)
{

    struct args *args =  ptr;
    int amount, account1, account2, balance1, balance2;
    //pthread_mutex_t m_valor1, m_valor2;

    while (args->iterations--) {
        //printf("Iterations transfer %d\n", args->iterations);
        account1 = rand() % args->bank->num_accounts;
        account2 = rand() % args->bank->num_accounts;

        if (account1 == account2) {
            while (account1 == account2) {
                account2 = rand() % args->bank->num_accounts;
            }
        }
		if(account1 < account2) {
			pthread_mutex_lock(&args->bank->accounts[account1].mutex);
			pthread_mutex_lock(&args->bank->accounts[account2].mutex);
		} else {
			pthread_mutex_lock(&args->bank->accounts[account2].mutex);
			pthread_mutex_lock(&args->bank->accounts[account1].mutex);
		}

        if(args->bank->accounts[account1].balance != 0) {
            amount = rand() % args->bank->accounts[account1].balance;
        }

        printf("Thread %d transfering %d from account %d to account %d\n",args->thread_num ,amount, account1, account2);

        balance1 = args->bank->accounts[account1].balance;
        if(args->delay) usleep(args->delay); // Force a context switch

        balance2 = args->bank->accounts[account2].balance;
        if(args->delay) usleep(args->delay); // Force a context switch

        balance1 -= amount;
        if(args->delay) usleep(args->delay);

        balance2 += amount;
        if(args->delay) usleep(args->delay);

        args->bank->accounts[account1].balance = balance1;
        args->bank->accounts[account2].balance = balance2;
        if(args->delay) usleep(args->delay);

        args->net_total += amount;

        pthread_cond_broadcast(&args->bank->accounts[account2].not_enough_money);
        pthread_mutex_unlock(&args->bank->accounts[account1].mutex);
        pthread_mutex_unlock(&args->bank->accounts[account2].mutex);
    }
    return NULL;
}

void *retire (void *ptr)
{

    struct args *args =  ptr;
    int account, retire, balance;
    bool error;
    struct timespec to;
    to.tv_sec = time(NULL) + TIMEOUT;
    to.tv_nsec = 0;

    while (args->iterations--) {
        //printf("Iterations retire %d\n", args->iterations);
        error = false;
        account = rand() % args->bank->num_accounts;
        pthread_mutex_lock (&args->bank->accounts[account].mutex);

        retire = rand() % MAX_AMOUNT;
        while (args->bank->accounts[account].balance < retire) {
            int err = pthread_cond_timedwait(&args->bank->accounts[account].not_enough_money,
                &args->bank->accounts[account].mutex, &to);
            if (err == ETIMEDOUT) {
                printf("Thread %d could not retire %d from account %d -Timeout error\n", args->thread_num, retire, account);
                error = true;
                break;
            }
        }
        if (!error) {
            printf("Thread %d retiring %d from account %d \n", args->thread_num, retire, account);
            balance = args->bank->accounts[account].balance;
		    if(args->delay) usleep(args->delay); // Force a context switch

		    balance = balance - retire;
		    if(args->delay) usleep(args->delay);

		    args->bank->accounts[account].balance = balance;
		    if(args->delay) usleep(args->delay);

		    args->net_total += retire;
        }

        pthread_mutex_unlock(&args->bank->accounts[account].mutex);
    }
    return NULL;
}

// start opt.num_threads threads running on deposit.
struct thread_info *start_threads(struct options opt, struct bank *bank)
{
	int i = 0, j = 0, k = 0;
	struct thread_info *threads;

	printf("creating %d threads\n", opt.num_threads);
	threads = malloc(sizeof(struct thread_info)* 3 * opt.num_threads);

	if (threads == NULL) {
		printf("Not enough memory\n");
		exit(1);
	}

	// Create num_thread threads running deposit()
	for (i = 0; i < opt.num_threads; i++) {
		threads[i].args = malloc(sizeof(struct args));

		threads[i].args -> thread_num = i;
		threads[i].args -> net_total  = 0;
		threads[i].args -> bank       = bank;
		threads[i].args -> delay      = opt.delay;
		threads[i].args -> iterations = opt.iterations;

		if (0 != pthread_create(&threads[i].id, NULL, deposit, threads[i].args)) {
			printf("Could not create thread #%d", i);
			exit(1);
		}
	}

    // Create num_thread threads running transfer()
    for (j = opt.num_threads; j < 2*opt.num_threads; j++) {
        threads[j].args = malloc(sizeof(struct args));

        threads[j].args -> thread_num = j;
        threads[j].args -> net_total  = 0;
        threads[j].args -> bank       = bank;
        threads[j].args -> delay      = opt.delay;
        threads[j].args -> iterations = opt.iterations;

        if (0 != pthread_create(&threads[j].id, NULL, transfer, threads[j].args)) {
            printf("Could not create thread #%d", j);
            exit(1);
        }
    }

    // Create num_thread threads running retire()
    for (k = 2*opt.num_threads; k < 3*opt.num_threads; k++) {
        threads[k].args = malloc(sizeof(struct args));

        threads[k].args -> thread_num = k;
        threads[k].args -> net_total  = 0;
        threads[k].args -> bank       = bank;
        threads[k].args -> delay      = opt.delay;
        threads[k].args -> iterations = opt.iterations;

        if (0 != pthread_create(&threads[k].id, NULL, retire, threads[k].args)) {
            printf("Could not create thread #%d", k);
            exit(1);
        }
    }

	return threads;
}

// Print the final balances of accounts and threads
void print_balances(struct bank *bank, struct thread_info *thrs, int num_threads) {
	int total_deposits=0, bank_total=0, retired_total = 0;

	printf("\nNet deposits by thread\n");
	for(int i=0; i < num_threads; i++) {
		printf("%d: %d\n", i, thrs[i].args->net_total);
		total_deposits += thrs[i].args->net_total;
	}
	printf("Total: %d\n", total_deposits);

    printf("\nAccount balance\n");
	for(int i=0; i < bank->num_accounts; i++) {
		printf("%d: %d\n", i, bank->accounts[i].balance);
		bank_total += bank->accounts[i].balance;
	}
	printf("Total: %d\n", bank_total);

    printf("\nNet retired by thread\n");
    for(int i=2*num_threads; i < 3*num_threads; i++) {
		printf("%d: %d\n", i, thrs[i].args->net_total);
		retired_total += thrs[i].args->net_total;
	}
	printf("Total: %d\n", retired_total);

}

// wait for all threads to finish, print totals, and free memory
void wait(struct options opt, struct bank *bank, struct thread_info *threads) {
	// Wait for the threads to finish
	for (int i = 0; i < 3*opt.num_threads; i++)
		pthread_join(threads[i].id, NULL);

	print_balances(bank, threads, opt.num_threads);

	for (int i = 0; i < 3*opt.num_threads; i++)
		{ free(threads[i].args); }

    for (int i = 0; i < bank->num_accounts; i++)
        { pthread_mutex_destroy (&bank->accounts[i].mutex); }

	free(threads);
	free(bank->accounts);
}

// allocate memory, and set all accounts to 0
void init_accounts(struct bank *bank, int num_accounts) {
	bank->num_accounts = num_accounts;
	bank->accounts     = malloc(bank->num_accounts * sizeof(struct account));

	for(int i=0; i < bank->num_accounts; i++) {
		bank->accounts[i].balance = 0;
        pthread_mutex_init(&bank->accounts[i].mutex, NULL);
        pthread_cond_init(&bank->accounts[i].not_enough_money, NULL);
    }
}

int main (int argc, char **argv)
{
	struct options      opt;
	struct bank         bank;
	struct thread_info *thrs;

	srand(time(NULL));

	// Default values for the options
	opt.num_threads  = 3;
	opt.num_accounts = 3;
	opt.iterations   = 2;
	opt.delay        = 10;

	read_options(argc, argv, &opt);

	init_accounts(&bank, opt.num_accounts);

	thrs = start_threads(opt, &bank);
    wait(opt, &bank, thrs);

	return 0;
}
