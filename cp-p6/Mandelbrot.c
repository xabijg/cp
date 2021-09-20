/*The Mandelbrot set is a fractal that is defined as the set of points c
in the complex plane for which the sequence z_{n+1} = z_n^2 + c
with z_0 = 0 does not tend to infinity.*/

/*This code computes an image of the Mandelbrot set.*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>

#define DEBUG 1

#define          X_RESN  1024  /* x resolution */
#define          Y_RESN  1024  /* y resolution */

/* Boundaries of the mandelbrot set */
#define           X_MIN  -2.0
#define           X_MAX   2.0
#define           Y_MIN  -2.0
#define           Y_MAX   2.0

/* More iterations -> more detailed image & higher computational cost */
#define   maxIterations  1000

typedef struct complextype
{
  float real, imag;
} Compl;

static inline double get_seconds(struct timeval t_ini, struct timeval t_end)
{
  return (t_end.tv_usec - t_ini.tv_usec) / 1E6 +
         (t_end.tv_sec - t_ini.tv_sec);
}

int main ( int argc, char *argv[] )
{

  /* MPI variables */
  int numprocs, rank;

  /* Partition variables */
  int parts;

  /* Performance variables */
  int count=0, flops=0;
  double loadBalance;

  /* Mandelbrot variables */
  int i, j, k;
  Compl   z, c;
  float   lengthsq, temp;
  int *vres, *res[Y_RESN];

  /* Timestamp variables */
  struct timeval  ti, tf;

  /* MPI init */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* Calculate parts */
  parts = ceil((float) Y_RESN/numprocs);

  if (rank == 0) {
    /* Allocate result matrix of parts x numprocs x X_RESN */
    vres = (int *) malloc(parts * numprocs * X_RESN * sizeof(int));
    if (!vres)
    {
      fprintf(stderr, "Error allocating memory\n");
      return 1;
    }
    for (i=0; i<Y_RESN; i++)
      res[i] = vres + i*X_RESN;

  }

  /* Allocate result of partial matrix */
  int *vresp = malloc(parts * X_RESN * sizeof(int));
  int *resp[parts];
  for (i = 0; i < parts; i++) {
    resp[i] = vresp + i*X_RESN;
  }

  /* Start measuring computing time */
  gettimeofday(&ti, NULL);

  /* Calculate and draw points */
  for(i=rank*parts; i < (rank+1)*parts; i++)
  {
    for(j=0; j < X_RESN; j++)
    {
      z.real = z.imag = 0.0;
      c.real = X_MIN + j * (X_MAX - X_MIN)/X_RESN;
      c.imag = Y_MAX - i * (Y_MAX - Y_MIN)/Y_RESN;
      k = 0;

      do
      {    /* iterate for pixel color */
        temp = z.real*z.real - z.imag*z.imag + c.real;
        z.imag = 2.0*z.real*z.imag + c.imag;
        z.real = temp;
        lengthsq = z.real*z.real+z.imag*z.imag;
        k++;
      } while (lengthsq < 4.0 && k < maxIterations);

      count += k*10; // Each do iteration makes 10 floating point operations

      if (k >= maxIterations) resp[i%parts][j] = 0;
      else resp[i%parts][j] = k;
    }
  }

  /* End measuring computing time */
  gettimeofday(&tf, NULL);
  fprintf (stderr, "(PERF)(PROCESS=%d) Computing Time (seconds) = %lf\n", rank, get_seconds(ti,tf));

  /* Start measuring communication time */
  gettimeofday(&ti, NULL);

  MPI_Gather(vresp, parts*X_RESN, MPI_INT, vres, parts*X_RESN, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Allreduce(&count, &flops, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  /* End measuring communication time */
  gettimeofday(&tf, NULL);
  fprintf(stderr, "(PERF)(PROCESS=%d) Communication Time (seconds) = %lf\n", rank, get_seconds(ti, tf));

  /* Calculate load balance */
  loadBalance = (double)flops/(count*numprocs);
  fprintf(stderr, "(PERF)(PROCESS=%d) Load Balance = %f\n", rank, loadBalance);

  /* Print result out */
  if( DEBUG && (rank == 0)) {
    for(i=0;i<Y_RESN;i++) {
      for(j=0;j<X_RESN;j++)
              printf("%3d ", res[i][j]);
      printf("\n");
    }
    free(vres);
  }

  MPI_Finalize();

  return 0;
}
