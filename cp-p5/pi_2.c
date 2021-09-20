#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

// precondición: se n = 0 o programa para no main, por tanto
// nunca chegaría aquí un n = 0.
double calculate_pi (int n, int rank, int numprocs) {

    int i;
    double h, x, sum;

    h = 1.0 / (double) n;
    sum = 0.0;

    for (i = rank+1; i <= n; i+=numprocs) {
        x = h * ((double)i - 0.5);
        sum += 4.0 / (1.0 + x*x);
    }

    return h * sum;

}

void print_pi (int rank, int root, double pi) {

    double PI25DT = 3.141592653589793238462643;

    if (rank == root) {
        printf("Pi is approximately %.16f, Error is %.16f \n\n", pi,
            fabs(pi - PI25DT));
    }

}


void MPI_FlattreeColectiva ( void * buf, int count, MPI_Datatype datatype, int root,
    MPI_Comm comm ){

    int i, numprocs, rank;
    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == root) {
        for (i = 0; i < numprocs; i++) { // se o proceso 0 vai ser sempre o root poderíase empezar en 1
            if (i!=root) { MPI_Send(buf, count, datatype, i, 0, comm); } // evita que se envie a si mesmo, util en caso de que o root sea distinto do 0
        }
    } else  {
        MPI_Recv(buf, count, datatype, root, 0, comm, &status);
    }

}

void MPI_BinomialColectiva ( void * buf, int count, MPI_Datatype datatype, int root,
    MPI_Comm comm ) {

    int i, numprocs, rank;
    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank != root) { MPI_Recv(buf, count, datatype, MPI_ANY_SOURCE, 0, comm,
        &status); } // pon a todos os procesos existentes a recibir menos o root

    for (i = 0;; i++) {

        if (rank < pow(2,i-1) ) {  // calculase se o proceso ten que enviar información
            if (pow(2,i-1)+rank >= numprocs) break; // se o destino non existe parase porque xa enviaron todos os que teñen que enviar
            MPI_Send(buf, count, datatype, pow(2,i-1)+rank, 0, comm);
        }

    }
}

int main( int argc , char * argv [] ) {

    int i, k, done = 0, n;
    double pi, h, sum, x, tmp, aux;

    int numprocs, rank;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while (!done) {

        if (rank==0) {
            printf("Enter the number of intervals (0 quits) \n");
            scanf("%d", &n);
        }

        // MPI_Bcast
        // if (rank == 0) { printf("Using MPI_Bcast \n"); }
        // MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // MPI_FlattreeColectiva
        if (rank == 0) { printf("Using MPI_FlattreeColectiva \n"); };
        MPI_FlattreeColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // MPI_BinomialColectiva
        // if (rank == 0) { printf("Using MPI_BinomialColectiva \n"); };
        // MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (n == 0) { break; }
        aux = calculate_pi(n, rank, numprocs);

        MPI_Reduce(&aux, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        print_pi(rank, 0, pi);

    }

    MPI_Finalize();
}
