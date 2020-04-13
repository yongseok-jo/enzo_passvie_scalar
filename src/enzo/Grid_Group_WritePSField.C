/***********************************************************************
/  GRID CLASS (WRITE OUT GRID)
/
/  written by: Yongseok Jo
/  date:       December, 2019 
/
/  PURPOSE:
/
************************************************************************/
 
//  Write grid to file pointer fptr
//  //     (we assume that the grid is at an appropriate stopping point,
//  //      where the Old values aren't required)
//#include <hdf5.h>


#ifdef USE_MPI
#include "mpi.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ErrorExceptions.h"
#include "macros_and_parameters.h"
#include "typedefs.h"
#include "global_data.h"
#include "Fluxes.h"
#include "GridList.h"
#include "ExternalBoundary.h"
#include "Grid.h"


int grid::Group_WritePSField(int grid_id, char *basename){
  int i, j, k, dim, field, size, active_size, ActiveDim[MAX_DIMENSION];
  float Old, New; 
  float *recv_old, *recv_new;

  char *filename = new char[MAX_LINE_LENGTH];
  
  
  if (MyProcessorNumber == ProcessorNumber) {
  fprintf(stderr,"Group_WritePSField has started!\n");
  strcpy(filename, basename);
  strcat(filename, "_ForStarFormation.txt");
  FILE *fp = fopen(filename, "a+");

/***
  for (dim = GridRank; dim < 3; dim++) {
    GridDimension[dim] = 1;
    GridStartIndex[dim] = 0;
    GridEndIndex[dim] = 0;
  }

 
  for (dim = 0; dim < 3; dim++)
    ActiveDim[dim] = GridEndIndex[dim] - GridStartIndex[dim] +1;
*/ 
   /* copy active part of field into grid */
/* 
  for (k = GridStartIndex[2]; k <= GridEndIndex[2]; k++)
   for (j = GridStartIndex[1]; j <= GridEndIndex[1]; j++)
    for (i = GridStartIndex[0]; i <= GridEndIndex[0]; i++)
	    Old +=
	    OldPSFieldForStarFormation[i+j*GridDimension[0]+
		             k*GridDimension[0]*GridDimension[1]];

  for (k = GridStartIndex[2]; k <= GridEndIndex[2]; k++)
   for (j = GridStartIndex[1]; j <= GridEndIndex[1]; j++)
    for (i = GridStartIndex[0]; i <= GridEndIndex[0]; i++)
	    New +=
	    NewPSFieldForStarFormation[i+j*GridDimension[0]+
		             k*GridDimension[0]*GridDimension[1]];


  MPI_Allreduce(&Old, recv_old, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  MPI_Allreduce(&New, recv_new, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
 ***/ 
  if (MyProcessorNumber == ROOT_PROCESSOR)
    if (fp != NULL) 
      fprintf(fp, "%lf\n", leakage_sf);
  fclose(fp);
  }
  leakage_sf = 0;
  return SUCCESS;
}  
