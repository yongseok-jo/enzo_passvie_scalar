/*------------------------------------------------------------------------
  ADJUST REFINE REGION ACCORDING TO MBH PARTICLE
  By Jo

  History:
     28 MAY 2019 : JO - created
------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "macros_and_parameters.h"
#include "typedefs.h"
#include "global_data.h"
#include "TopGridData.h"
#include "CosmologyParameters.h"

int CosmologyComputeExpansionFactor(FLOAT time, FLOAT *a, FLOAT *dadt);


int AdjustRefineRegionByMBH(TopGridData &MetaData) 
{

  int staticRegion, i;
  FLOAT dd=0.0005;    // I should take this value as a parameter from an enzo parameter file.
  FLOAT a, dadt, redshift, static_resolution;

  fprintf(stderr,"AdjustedEvolveRegion Started\n");

  if (ComovingCoordinates) {
    CosmologyComputeExpansionFactor(MetaData.Time, &a, &dadt);
    redshift = (1 + InitialRedshift)/a - 1;
  }

  /* Set RefineRegion to EvolveRefineRegion or innermost StaticRefineRegion */
  if (bh_pos[0] != 0.0){  // should be !=
    staticRegion = 0;
    while (StaticRefineRegionLevel[staticRegion] != INT_UNDEFINED){
      /*for (i=0; i<MAX_DIMENSION; i++){
	dd[i] = (StaticRefineRegionRightEdge[staticRegion][i] - StaticRefineRegionLeftEdge[staticRegion][i])/2;
	StaticRefineRegionRightEdge[staticRegion][i] = bh_pos[i]+dd[i]; 
	StaticRefineRegionLeftEdge[staticRegion][i] = bh_pos[i]-dd[i];
	}*/
      staticRegion++;
    }
    static_resolution =  1/(MetaData.TopGridDims[0]*pow(2,staticRegion));
    for (i=0; i< MAX_DIMENSION; i++){
      if ((RefineRegionRightEdge[i]-bh_pos[i]) > dd+static_resolution){
	RefineRegionRightEdge[i] -= static_resolution;
      }
      else if((RefineRegionRightEdge[i]-bh_pos[i]) < dd){
	RefineRegionRightEdge[i] += static_resolution;
      }
      if ((bh_pos[i]-RefineRegionLeftEdge[i]) > dd+static_resolution){
	RefineRegionLeftEdge[i] += static_resolution;
      }
      else if((bh_pos[i]-RefineRegionLeftEdge[i]) < dd){
	RefineRegionLeftEdge[i] -= static_resolution;
      }
      //fprintf(stderr, "dd[%d]: %"FSYM",bh_pos[%d] :  %"FSYM"", i, dd[i],i,bh_pos[i]);
      //RefineRegionRightEdge[i] = bh_pos[i]+dd[i]; 
      //RefineRegionLeftEdge[i] = bh_pos[i]-dd[i];
      /*if (MyProcessorNumber==ROOT_PROCESSOR){
	fprintf(stderr,"Last Static Region resolution : %"FSYM"\n", static_resolution);
	}*/
    }
    fprintf(stderr,"\n");
    if (true) //debug
      fprintf(stderr, "AdjustedEvolveRegion: %"FSYM" %"FSYM" %"FSYM" %"FSYM" %"FSYM" %"FSYM"\n",
	      RefineRegionLeftEdge[0], RefineRegionLeftEdge[1], 
	      RefineRegionLeftEdge[2], RefineRegionRightEdge[0],
	      RefineRegionRightEdge[1], RefineRegionRightEdge[2]);
  }else{
    fprintf(stderr,"AdjustedEvolveRegion Failed\n");
    //fprintf(stderr,"%d %d %d", MetaData.TopGridDims[0], MetaData.TopGridDims[1], MetaData.TopGridDims[2]);
  }
  return SUCCESS;
}
