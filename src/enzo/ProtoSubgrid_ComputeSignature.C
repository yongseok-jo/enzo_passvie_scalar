/***********************************************************************
/
/  PROTOSUBGRID CLASS (COMPUTES A 1D SIGNATURE (PROJECTION))
/
/  written by: Greg Bryan
/  date:       October, 1995
/  modified1:
/
/  PURPOSE: This gives 3 lines of Signature[dim] into which FlaggingField is summed over 2d surface.
/
************************************************************************/
 
#include <stdio.h>
#include "ErrorExceptions.h"
#include "macros_and_parameters.h"
#include "typedefs.h"
#include "global_data.h"
#include "Fluxes.h"
#include "GridList.h"
#include "ExternalBoundary.h"
#include "Grid.h"
 
/* function prototypes */
 
extern "C" void FORTRAN_NAME(project)(int *rank, int *i1, int *i2, int *i3,
				      int *iline, int *pdim, int *field,
				      int *line);
 
int ProtoSubgrid::ComputeSignature(int dim)
{
 
  /* Error check. */
 
  if (dim >= GridRank) {
    ENZO_VFAIL("Project: dim = %"ISYM" > GridRank = %"ISYM"\n", dim, GridRank)
  }
 
  /* Already done? */
 
  if (Signature[dim] != NULL)
    return SUCCESS;
 
  /* Allocate space. */
 
  Signature[dim] = new int[GridDimension[dim]];   // This GridDimension is pretty much the same as just Grid object, rather ProtoSubgrid.
  for(int i = 0; i < GridDimension[dim]; i++) Signature[dim][i] = 0;
 
  /* Perform projection (and clear line)
     For 1D grids, no projection necessary, just copy. */
 
  if (GridRank == 1)
    for (int i = 0; i < GridDimension[dim]; i++)
      Signature[dim][i] += (GridFlaggingField[i]) ? 1 : 0;
  else
    FORTRAN_NAME(project)(&GridRank, GridDimension, GridDimension+1,
			  GridDimension+2, &GridDimension[dim], &dim,
			  GridFlaggingField, Signature[dim]);    // Projection of N-dim rectangular volume into N lines summing over (N-1)-dim.  
 
  /*  if (debug) {

      printf ("sig[%"ISYM"]=%"ISYM": ", dim, GridDimension[dim]);
      for (int j = 0; j < GridDimension[dim]; j++)
      printf("%"ISYM" ", Signature[dim][j]);
      printf("\n");
      } */
 
 
  return SUCCESS;
}
