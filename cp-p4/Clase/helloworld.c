// mpicc helloworld.c -o helloworld.o
// mpirun -np _ ./helloworld.o
// MPI_FlattreeColectiva = práctica 1

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main ( int argc , char * argv []) {
    int numprocs , rank , namelen , i , n ;
    MPI_Status status;
    char processor_name [ MPI_MAX_PROCESSOR_NAME ];

    MPI_Init (&argc , &argv ) ;
    MPI_Comm_size ( MPI_COMM_WORLD , &numprocs );                               // siempre copy-paste, comunicador global, comunica todos os procesos, numprocs indica cantos procesos tes
    MPI_Comm_rank ( MPI_COMM_WORLD , &rank );                                   // rank indica "quen é", como un identificador

    MPI_Get_processor_name ( processor_name , &namelen );

    printf (" Process %d on %s out of %d\n" , rank , processor_name , numprocs ) ;

    if (rank == 0) {
        printf("Enter the number to distribute: \n");
        scanf("%d", &n);
        for (i = 1; i < numprocs; i++) {
            MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }

    printf("Process %d on %s out of %d says number is %d\n", rank, processor_name, numprocs, n);

    int my_num = (rank+1) * n;

    if (rank == 0) {
        for (int i = 1; i < numprocs; i++) {
            MPI_Recv(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            printf("Num of process %d on %s is %d\n", i, processor_name, n);
        }
    } else {
        MPI_Send(&my_num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize () ;
}
