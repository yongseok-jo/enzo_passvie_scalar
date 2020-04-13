/***********************************************************************
/
/  CREATES STAR PARTICLES FROM EXISTING PARTICLES
/
io/  written by: Yongseok Jo
/  date:       May, 2019
/  modified1:
/
/  NOTES:  Find MBH particle
/
************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ErrorExceptions.h"
#include "macros_and_parameters.h"
#include "typedefs.h"
#include "global_data.h"
#include "Fluxes.h"
#include "GridList.h"
#include "ExternalBoundary.h"
#include "Grid.h"


int grid::FindMBHParticles(int level, FLOAT *sbuf)
{

  if (MyProcessorNumber != ProcessorNumber)
    return SUCCESS;

  if (NumberOfParticles == 0)
    return SUCCESS;
  
  int i;
#define GMC_TEST  
#ifdef GMC_TEST /* GMC_TEST */
  for (i = 0; i < NumberOfParticles; i++){

    if (ParticleType[i] == PARTICLE_TYPE_SINGLE_STAR ||
	ParticleType[i] == PARTICLE_TYPE_BLACK_HOLE ||
	ParticleType[i] == PARTICLE_TYPE_CLUSTER ||
        ParticleType[i] == PARTICLE_TYPE_COLOR_STAR ||
	ParticleType[i] == PARTICLE_TYPE_MBH ||
	ParticleType[i] == PARTICLE_TYPE_SIMPLE_SOURCE) {
      if (ParticleAttribute[0][i] < 1){
	printf("MBH Position of rank %d :", MyProcessorNumber);
	  for (int dim = 0; dim < MAX_DIMENSION; dim++){
	    sbuf[dim] = ParticlePosition[dim][i];
	    printf("%"GSYM", ", sbuf[dim]);
	  }
	  printf("\n");
      }
    }
  }
#endif  /* GMC_TEST */
  
  return SUCCESS;
  
}
