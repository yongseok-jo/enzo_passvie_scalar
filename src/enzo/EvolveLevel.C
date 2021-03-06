/***********************************************************************
/
/  EVOLVE LEVEL FUNCTION
/
/  written by: Greg Bryan
/  date:       November, 1994
/  modified1:  February, 1995 by GB
/              Overhauled to make sure that all the subgrid's of a grid
/              advance with in lock step (i.e. with the same timestep and
/              in order).  This was done to allow a subgrid to get it's
/              boundary values from another subgrid (with the same parent).
/              Previously, a subgrid' BVs were always interpolated from its
/              parent.
/  modified2:  August, 1995 by GB
/                1) All grids on a level are processed at the same time
/                 (rather than all the subgrids of one parent).
/                2) C routines are called to loop over subgrids
/                 (so parallelizing C compilers can be used).
/                3) Subgrid timesteps are not constant over top grid step.
/              June, 1999 by GB -- Clean up somewhat
/
/  modified3:  August, 2001 by Alexei Kritsuk
/                Added 2nd call of PrepareDensityField() to compute
/                grav. potential (to be written with other baryon fields).
/  modified4:  January, 2004 by Alexei Kritsuk
/                Added support for RandomForcing
/  modified5:  February, 2006 by Daniel Reynolds
/                Added PotentialBdry to EvolveLevel and 
/                PrepareDensityField calls, so that it can be used
/                within computing isolating BCs for self-gravity.
/  modified6:  January, 2007 by Robert Harkness
/                Group and in-core i/o
/  modified7:  December, 2007 by Robert Harkness
/                Optional StaticSiblingList for root grid
/  modified8:  April, 2009 by John Wise
/                Added star particle class and radiative transfer
/  modified9:  June, 2009 by MJT, DC, JHW, TA
/                Cleaned up error handling and created new routines for
/                computing the timestep, output, handling fluxes
/  modified10: July, 2009 by Sam Skillman
/                Added shock and cosmic ray analysis
/
/  PURPOSE:
/    This routine is the main grid evolution function.  It assumes that the
/    grids of level-1 have already been advanced by dt (passed
/    in the argument) and that their boundary values are properly set.
/    We then perform a complete update on all grids on level, including:
/       - for each grid: set the boundary values from parent/subgrids
/       - for each grid: get a list of its subgrids
/       - determine the timestep based on the minimum timestep for all grids
/       - subcycle over the grid timestep and for each grid:
/           - copy the fields to the old fields
/           - solve the hydro equations (and save fluxes around subgrid)
/           - set the boundary values from parent and/or other grids
/           - update time and check dt(min) for that grid against dt(cycle)
/           - call EvolveLevel(level+1)
/           - accumulate flux around this grid
/       - correct the solution on this grid based on subgrid solutions
/       - correct the solution on this grid based on improved subgrid fluxes
/
/    This routine essentially solves (completely) the grids of this level
/       and all finer levels and then corrects the solution of
/       grids on this level based on the improved subgrid solutions/fluxes.
/
/    Note: as a convenience, we store the current grid's fluxes (i.e. the
/          fluxes around the exterior of this grid) as the last entry in
/          the list of subgrids.
/
************************************************************************/
#include "preincludes.h"
 
#ifdef USE_MPI
#include "mpi.h"
#endif /* USE_MPI */
 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include "EnzoTiming.h" 
#include "performance.h"
#include "ErrorExceptions.h"
#include "macros_and_parameters.h"
#include "typedefs.h"
#include "global_data.h"
#include "Fluxes.h"
#include "GridList.h"
#include "ExternalBoundary.h"
#include "Grid.h"
#include "Hierarchy.h"
#include "TopGridData.h"
#include "LevelHierarchy.h"
#include "CommunicationUtilities.h"
#ifdef TRANSFER
#include "ImplicitProblemABC.h"
#endif
#ifdef NEW_PROBLEM_TYPES
#include "EventHooks.h"
#else
void RunEventHooks(char *, HierarchyEntry *Grid[], TopGridData &MetaData) {}
#endif
 
/* function prototypes */
 

int  RebuildHierarchy(TopGridData *MetaData,
		      LevelHierarchyEntry *LevelArray[], int level);
int  ReportMemoryUsage(char *header = NULL);
int  UpdateParticlePositions(grid *Grid);
int  CheckEnergyConservation(HierarchyEntry *Grids[], int grid,
			     int NumberOfGrids, int level, float dt);
int GenerateGridArray(LevelHierarchyEntry *LevelArray[], int level,
		      HierarchyEntry **Grids[]);
int WriteStreamData(LevelHierarchyEntry *LevelArray[], int level,
		    TopGridData *MetaData, int *CycleCount, int open=FALSE);
int CallProblemSpecificRoutines(TopGridData * MetaData, HierarchyEntry *ThisGrid,
				int GridNum, float *norm, float TopGridTimeStep, 
				int level, int LevelCycleCount[]);  //moo

#ifdef FAST_SIB
int PrepareDensityField(LevelHierarchyEntry *LevelArray[],
			SiblingGridList SiblingList[],
			int level, TopGridData *MetaData, FLOAT When);
#else  // !FAST_SIB
int PrepareDensityField(LevelHierarchyEntry *LevelArray[],
                        int level, TopGridData *MetaData, FLOAT When);
#endif  // end FAST_SIB
 
#ifdef FAST_SIB
int SetBoundaryConditions(HierarchyEntry *Grids[], int NumberOfGrids,
			  SiblingGridList SiblingList[],
			  int level, TopGridData *MetaData,
			  ExternalBoundary *Exterior, LevelHierarchyEntry * Level);
#else
int SetBoundaryConditions(HierarchyEntry *Grids[], int NumberOfGrids,
                          int level, TopGridData *MetaData,
                          ExternalBoundary *Exterior, LevelHierarchyEntry * Level);
#endif



#ifdef SAB
#ifdef FAST_SIB
int SetAccelerationBoundary(HierarchyEntry *Grids[], int NumberOfGrids,
			    SiblingGridList SiblingList[],
			    int level, TopGridData *MetaData,
			    ExternalBoundary *Exterior,
			    LevelHierarchyEntry * Level,
			    int CycleNumber);
#else
int SetAccelerationBoundary(HierarchyEntry *Grids[], int NumberOfGrids,
			    int level, TopGridData *MetaData, 
			    ExternalBoundary *Exterior,
			    LevelHierarchyEntry * Level,
			    int CycleNumber);
#endif
#endif

#ifdef FLUX_FIX
int UpdateFromFinerGrids(int level, HierarchyEntry *Grids[], int NumberOfGrids,
			 int NumberOfSubgrids[],
			 fluxes **SubgridFluxesEstimate[],
			 LevelHierarchyEntry *SUBlingList[],
			 TopGridData *MetaData);
#else
int UpdateFromFinerGrids(int level, HierarchyEntry *Grids[], int NumberOfGrids,
			 int NumberOfSubgrids[],
			 fluxes **SubgridFluxesEstimate[]);
#endif
int CreateFluxes(HierarchyEntry *Grids[],fluxes **SubgridFluxesEstimate[],
		 int NumberOfGrids,int NumberOfSubgrids[]);		 
int FinalizeFluxes(HierarchyEntry *Grids[],fluxes **SubgridFluxesEstimate[],
		 int NumberOfGrids,int NumberOfSubgrids[]);		 
int RadiationFieldUpdate(LevelHierarchyEntry *LevelArray[], int level,
			 TopGridData *MetaData);


int OutputFromEvolveLevel(LevelHierarchyEntry *LevelArray[],TopGridData *MetaData,
			  int level, ExternalBoundary *Exterior
#ifdef TRANSFER
			  , ImplicitProblemABC *ImplicitSolver
#endif
			  );
 
int ComputeRandomForcingNormalization(LevelHierarchyEntry *LevelArray[],
                                      int level, TopGridData *MetaData,
                                      float * norm, float * pTopGridTimeStep);
int CreateSiblingList(HierarchyEntry ** Grids, int NumberOfGrids, SiblingGridList *SiblingList, 
		      int StaticLevelZero,TopGridData * MetaData,int level);

#ifdef FLUX_FIX
#ifdef FAST_SIB 
int CreateSUBlingList(TopGridData *MetaData,
		      LevelHierarchyEntry *LevelArray[], int level,
		      SiblingGridList SiblingList[],
		      LevelHierarchyEntry ***SUBlingList);
#else
int CreateSUBlingList(TopGridData *MetaData,
		      LevelHierarchyEntry *LevelArray[], int level,
		      LevelHierarchyEntry ***SUBlingList);
#endif /* FAST_SIB */
int DeleteSUBlingList(int NumberOfGrids,
		      LevelHierarchyEntry **SUBlingList);
#endif

int StarParticleInitialize(HierarchyEntry *Grids[], TopGridData *MetaData,
			   int NumberOfGrids, LevelHierarchyEntry *LevelArray[], 
			   int ThisLevel, Star *&AllStars,
			   int TotalStarParticleCountPrevious[]);
int StarParticleFinalize(HierarchyEntry *Grids[], TopGridData *MetaData,
			 int NumberOfGrids, LevelHierarchyEntry *LevelArray[], 
			 int level, Star *&AllStars,
			 int TotalStarParticleCountPrevious[]);
int AdjustRefineRegion(LevelHierarchyEntry *LevelArray[], 
		       TopGridData *MetaData, int EL_level);
int AdjustMustRefineParticlesRefineToLevel(TopGridData *MetaData, int EL_level);

#ifdef TRANSFER
int EvolvePhotons(TopGridData *MetaData, LevelHierarchyEntry *LevelArray[],
		  Star *&AllStars, FLOAT GridTime, int level, int LoopTime = TRUE);
int RadiativeTransferPrepare(LevelHierarchyEntry *LevelArray[], int level,
			     TopGridData *MetaData, Star *&AllStars,
			     float dtLevelAbove);
int RadiativeTransferCallFLD(LevelHierarchyEntry *LevelArray[], int level,
			     TopGridData *MetaData, Star *AllStars, 
			     ImplicitProblemABC *ImplicitSolver);
#endif

int SetLevelTimeStep(HierarchyEntry *Grids[],
        int NumberOfGrids, int level,
        float *dtThisLevelSoFar, float *dtThisLevel,
        float dtLevelAbove);

void my_exit(int status);
 
int CallPython(LevelHierarchyEntry *LevelArray[], TopGridData *MetaData,
               int level, int from_topgrid);
int MovieCycleCount[MAX_DEPTH_OF_HIERARCHY];
double LevelWallTime[MAX_DEPTH_OF_HIERARCHY];
double LevelZoneCycleCount[MAX_DEPTH_OF_HIERARCHY];
double LevelZoneCycleCountPerProc[MAX_DEPTH_OF_HIERARCHY];
 
static float norm = 0.0;            //AK
static float TopGridTimeStep = 0.0; //AK
#ifdef STATIC_SIBLING_LIST
static int StaticLevelZero = 1;
#else
static int StaticLevelZero = 0;
#endif

extern int RK2SecondStepBaryonDeposit;



int EvolveLevel(TopGridData *MetaData, LevelHierarchyEntry *LevelArray[],
		int level, float dtLevelAbove, ExternalBoundary *Exterior
#ifdef TRANSFER
		, ImplicitProblemABC *ImplicitSolver
#endif
		)
{

#define no_debug_PS // by Jo
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Grid level %d has started!\n", level);
#endif
  /* Declarations */

  int dbx = 0;

  FLOAT When, GridTime;
  //float dtThisLevelSoFar = 0.0, dtThisLevel, dtGrid, dtActual, dtLimit;
  //float dtThisLevelSoFar = 0.0, dtThisLevel;
  int cycle = 0, counter = 0, grid1, subgrid, grid2;
  HierarchyEntry *NextGrid;
  int dummy_int;

  char level_name[MAX_LINE_LENGTH];
  sprintf(level_name, "Level_%02"ISYM, level);

  // Update lcaperf "level" attribute

  Eint32 lcaperf_level = level;
#ifdef USE_LCAPERF
  lcaperf.attribute ("level",&lcaperf_level,LCAPERF_INT);
#endif

  /* Create an array (Grids) of all the grids. */

  typedef HierarchyEntry* HierarchyEntryPointer;
  HierarchyEntry **Grids;
  int NumberOfGrids = GenerateGridArray(LevelArray, level, &Grids);
  int *NumberOfSubgrids = new int[NumberOfGrids];
  fluxes ***SubgridFluxesEstimate = new fluxes **[NumberOfGrids];
  int *TotalStarParticleCountPrevious = new int[NumberOfGrids];
  RunEventHooks("EvolveLevelTop", Grids, *MetaData);

#ifdef FLUX_FIX
  /* Create a SUBling list of the subgrids */
  LevelHierarchyEntry **SUBlingList;
#endif

  /* Initialize the chaining mesh used in the FastSiblingLocator. */

  if (dbx) fprintf(stderr, "EL: Initialize FSL \n"); 
  SiblingGridList *SiblingList = new SiblingGridList[NumberOfGrids];
  CreateSiblingList(Grids, NumberOfGrids, SiblingList, StaticLevelZero,MetaData,level);
  
  /* Adjust the refine region so that only the finest particles 
     are included.  We don't want the more massive particles
     to contaminate the high-resolution region. */

  AdjustRefineRegion(LevelArray, MetaData, level);

  //EMISSIVITY if cleared here will not reach the FLD solver in 2.0, finding better place
  /* Adjust MustRefineParticlesRefineToLevel parameter if requested */
  AdjustMustRefineParticlesRefineToLevel(MetaData, level);

  /* ================================================================== */
  /* For each grid: a) interpolate boundaries from its parent.
                    b) copy any overlapping zones.  */


#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right before BoundaryCondition1!\n");
#endif
 
  if (CheckpointRestart == FALSE) {
#ifdef FAST_SIB
    if (SetBoundaryConditions(Grids, NumberOfGrids, SiblingList,
                  level, MetaData, Exterior, LevelArray[level]) == FAIL)
      ENZO_FAIL("Error in SetBoundaryConditions (FastSib)");
#else
    if (SetBoundaryConditions(Grids, NumberOfGrids, level, MetaData,
                              Exterior, LevelArray[level]) == FAIL)
      ENZO_FAIL("Error in SetBoundaryConditions (SlowSib)");
#endif
  }
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right before BoundaryCondition2!\n");
#endif
 
  /* Clear the boundary fluxes for all Grids (this will be accumulated over
     the subcycles below (i.e. during one current grid step) and used to by the
     current grid to correct the zones surrounding this subgrid (step #18). 

     If we're just coming in off a CheckpointRestart, instead we take the
     fluxes that were stored in the file and then in the Grid object, and we 
     put them into the SubgridFluxesEstimate array. */
 
  if(CheckpointRestart == TRUE) {
    for (grid1 = 0; grid1 < NumberOfGrids; grid1++) {
      if (Grids[grid1]->GridData->FillFluxesFromStorage(
        &NumberOfSubgrids[grid1],
        &SubgridFluxesEstimate[grid1]) != -1) {
        fprintf(stderr, "Level: %"ISYM" Grid: %"ISYM" NS: %"ISYM"\n",
            level, grid1, NumberOfSubgrids[grid1]);
      }
    }
  } else {
    for (grid1 = 0; grid1 < NumberOfGrids; grid1++)
      Grids[grid1]->GridData->ClearBoundaryFluxes();
  }
 
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right before BoundaryCondition3!\n");
#endif
  /* After we calculate the ghost zones, we can initialize streaming
     data files (only on level 0) */

  if (MetaData->FirstTimestepAfterRestart == TRUE && level == 0)
    WriteStreamData(LevelArray, level, MetaData, MovieCycleCount);
    //float leakage_sf = 0;
    //leakage_sf = 0; // initialize mass tracing variable when restart by Jo
  
  
  //if (MetaData->FirstTimestepAfterRestart == TRUE){
    //float leakage_sf = 0;
    //leakage_sf = 0; // initialize mass tracing variable when restart by Jo
  //}
  /* ================================================================== */
  /* Loop over grid timesteps until the elapsed time equals the timestep
     from the level above (or loop once for the top level). */
 
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right before BoundaryCondition4!\n");
#endif
  while ((CheckpointRestart == TRUE)
        || (dtThisLevelSoFar[level] < dtLevelAbove)) {
    if(CheckpointRestart == FALSE) {

      TIMER_START(level_name);

 
    SetLevelTimeStep(Grids, NumberOfGrids, level, 
        &dtThisLevelSoFar[level], &dtThisLevel[level], dtLevelAbove);

    /* Streaming movie output (write after all parent grids are
       updated) */

    WriteStreamData(LevelArray, level, MetaData, MovieCycleCount);

    /* Initialize the star particles */

#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right before Initializer!\n");
#endif
    Star *AllStars = NULL;
    StarParticleInitialize(Grids, MetaData, NumberOfGrids, LevelArray,
			   level, AllStars, TotalStarParticleCountPrevious);


#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "1\n");
#endif
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "After Init.!\n");
#endif
#ifdef TRANSFER
    /* Initialize the radiative transfer */

    TIMER_STOP(level_name);

    RadiativeTransferPrepare(LevelArray, level, MetaData, AllStars, 
			     dtLevelAbove);
    RadiativeTransferCallFLD(LevelArray, level, MetaData, AllStars, 
			     ImplicitSolver);

#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "1\n");
#endif
    /* Solve the radiative transfer */
	
    GridTime = Grids[0]->GridData->ReturnTime() + dtThisLevel[level];
    EvolvePhotons(MetaData, LevelArray, AllStars, GridTime, level);
    TIMER_START(level_name);
 
#endif /* TRANSFER */

    /* trying to clear Emissivity here after FLD uses it, doesn't work */
 
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "1\n");
#endif
    CreateFluxes(Grids,SubgridFluxesEstimate,NumberOfGrids,NumberOfSubgrids);

    /* ------------------------------------------------------- */
    /* Prepare the density field (including particle density). */

    When = 0.5;

#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "1\n");
#endif
#ifdef FAST_SIB
     PrepareDensityField(LevelArray, SiblingList, level, MetaData, When);
#else   // !FAST_SIB
     PrepareDensityField(LevelArray, level, MetaData, When);
#endif  // end FAST_SIB
 
 
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "2\n");
#endif
    /* Prepare normalization for random forcing. Involves top grid only. */
 
    ComputeRandomForcingNormalization(LevelArray, 0, MetaData,
				      &norm, &TopGridTimeStep);

    /* ------------------------------------------------------- */
    /* Evolve all grids by timestep dtThisLevel. */
 
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "2\n");
#endif
    for (grid1 = 0; grid1 < NumberOfGrids; grid1++) {
 
      CallProblemSpecificRoutines(MetaData, Grids[grid1], grid1, &norm, 
				  TopGridTimeStep, level, LevelCycleCount);

      /* Gravity: compute acceleration field for grid and particles. */
 
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "2\n");
#endif
      if (SelfGravity) {
	if (level <= MaximumGravityRefinementLevel) {
 
	  /* Compute the potential. */
 
	  if (level > 0)
	    Grids[grid1]->GridData->SolveForPotential(level);
	  Grids[grid1]->GridData->ComputeAccelerations(level);
	  Grids[grid1]->GridData->CopyPotentialToBaryonField();
	}
	  /* otherwise, interpolate potential from coarser grid, which is
	     now done in PrepareDensity. */
 
      } // end: if (SelfGravity)
 
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "3\n");
#endif
      /* Gravity: compute field due to preset sources. */
 
      Grids[grid1]->GridData->ComputeAccelerationFieldExternal();
 
      /* Radiation Pressure: add to acceleration field */

#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "3\n");
#endif
#ifdef TRANSFER
      Grids[grid1]->GridData->AddRadiationPressureAcceleration();
#endif /* TRANSFER */

      /* Check for energy conservation. */
/*
      if (ComputePotential)
	if (CheckEnergyConservation(Grids, grid, NumberOfGrids, level,
				    dtThisLevel) == FAIL) {
	  ENZO_FAIL("Error in CheckEnergyConservation.\n");
	}
*/
#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "3\n");
#endif
#ifdef SAB
    } // End of loop over grids
    
    //Ensure the consistency of the AccelerationField
    SetAccelerationBoundary(Grids, NumberOfGrids,SiblingList,level, MetaData,
			    Exterior, LevelArray[level], LevelCycleCount[level]);
    
    for (grid1 = 0; grid1 < NumberOfGrids; grid1++) {
#endif //SAB.
      /* Copy current fields (with their boundaries) to the old fields
	  in preparation for the new step. */
#ifdef debug_PS
  //if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right before CopyBaryon!\n");
#endif
 
      Grids[grid1]->GridData->CopyBaryonFieldToOldBaryonField();

      /* Call hydro solver and save fluxes around subgrids. */

      Grids[grid1]->GridData->SolveHydroEquations(LevelCycleCount[level],
	    NumberOfSubgrids[grid1], SubgridFluxesEstimate[grid1], level);

      /* Solve the cooling and species rate equations. */
 
      Grids[grid1]->GridData->MultiSpeciesHandler();

      /* Update particle positions (if present). */
 
      UpdateParticlePositions(Grids[grid1]->GridData);

    /*Trying after solving for radiative transfer */
#ifdef EMISSIVITY
    /*                                                                                                           
        clear the Emissivity of the level below, after the level below                                            
        updated the current level (it's parent) and before the next
        timestep at the current level.                                                                            
    */
      /*    if (StarMakerEmissivityField > 0) {
    LevelHierarchyEntry *Temp;
    Temp = LevelArray[level];
    while (Temp != NULL) {
      Temp->GridData->ClearEmissivity();
      Temp = Temp->NextGridThisLevel;
      }
      }*/

#endif

#ifdef debug_PS
  //if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right before Handler!\n");
#endif

// by Jo
//FLOAT mass_leakage_total = 0.0;
//leakage_sf_tmp = 0.0;

      /* Include 'star' particle creation and feedback. */
      Grids[grid1]->GridData->StarParticleHandler
	(Grids[grid1]->NextGridNextLevel, level ,dtLevelAbove);
      //fprintf(stderr, "EvolveLevel right after StarParticleHandler\n"); // by Jo
      /* Include shock-finding and cosmic ray acceleration */


#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right before BoundaryCondition!\n");
  if (MyProcessorNumber == ROOT_PROCESSOR){
	fprintf(stderr, "Level of this grid is %d\n", level);
	fprintf(stderr, "Mass Leakage: %e\n", leakage_sf_tmp);
	}
#endif

// by Jo
//MPI_Allreduce(&leakage_sf_tmp, &leakage_sf, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
//  if (MyProcessorNumber == ROOT_PROCESSOR){
	//fprintf(stderr, "Total Mass Leakage: %e\n", leakage_sf);
  //}
//CommunicationBarrier();

#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right after Handler\n");
#endif
      Grids[grid1]->GridData->ShocksHandler();

#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right after Shocks\n");
#endif
      /* Compute and apply thermal conduction. */
      if(IsotropicConduction || AnisotropicConduction){
	if(Grids[grid1]->GridData->ConductHeat() == FAIL){
	  ENZO_FAIL("Error in grid->ConductHeat.\n");
	}
      }

#ifdef debug_PS
  if (MyProcessorNumber == ROOT_PROCESSOR)
     fprintf(stderr, "Right after Particle Handler!\n");
#endif 
    //fprintf(stderr, "EvolveLevel right after ConductHeat\n"); // by Jo
      /* Gravity: clean up AccelerationField. */

      if ((level != MaximumGravityRefinementLevel ||
	   MaximumGravityRefinementLevel == MaximumRefinementLevel) &&
	  !PressureFree)
	Grids[grid1]->GridData->DeleteAccelerationField();

      Grids[grid1]->GridData->DeleteParticleAcceleration();
 
      /* Update current problem time of this subgrid. */
 
      Grids[grid1]->GridData->SetTimeNextTimestep();
 
    //fprintf(stderr, "EvolveLevel right after SetTimeNextTimestep\n"); // by Jo
      /* If using comoving co-ordinates, do the expansion terms now. */
 
      if (ComovingCoordinates)
	Grids[grid1]->GridData->ComovingExpansionTerms();

    //fprintf(stderr, "EvolveLevel right after ComovingExpansionTerms\n"); // by Jo
    }  // end loop over grids
 
    /* Finalize (accretion, feedback, etc.) star particles */

    //fprintf(stderr, "EvolveLevel right before StarParticleFinalize\n"); // by Jo
    StarParticleFinalize(Grids, MetaData, NumberOfGrids, LevelArray,
			 level, AllStars, TotalStarParticleCountPrevious);

    /* For each grid: a) interpolate boundaries from the parent grid.
                      b) copy any overlapping zones from siblings. */
 
#ifdef FAST_SIB
    SetBoundaryConditions(Grids, NumberOfGrids, SiblingList,
			  level, MetaData, Exterior, LevelArray[level]);
#else
    SetBoundaryConditions(Grids, NumberOfGrids, level, MetaData,
			  Exterior, LevelArray[level]);
#endif

    /* If cosmology, then compute grav. potential for output if needed. */


    /* For each grid, delete the GravitatingMassFieldParticles. */
 
    for (grid1 = 0; grid1 < NumberOfGrids; grid1++)
      Grids[grid1]->GridData->DeleteGravitatingMassFieldParticles();

    TIMER_STOP(level_name);

    /* ----------------------------------------- */
    /* Evolve the next level down (recursively). */
 
    MetaData->FirstTimestepAfterRestart = FALSE;

    } else { // CheckpointRestart
        // dtThisLevelSoFar set during restart
        // dtThisLevel set during restart
        // Set dtFixed on each grid to dtThisLevel
        for (grid1 = 0; grid1 < NumberOfGrids; grid1++)
          Grids[grid1]->GridData->SetTimeStep(dtThisLevel[level]);
    }

    if (LevelArray[level+1] != NULL) {
      if (EvolveLevel(MetaData, LevelArray, level+1, dtThisLevel[level], Exterior
#ifdef TRANSFER
		      , ImplicitSolver
#endif
		      ) == FAIL) {
	ENZO_VFAIL("Error in EvolveLevel (%"ISYM").\n", level)
      }
    }

#ifdef USE_LCAPERF
    // Update lcaperf "level" attribute

    lcaperf.attribute ("level",&lcaperf_level,LCAPERF_INT);
#endif

    OutputFromEvolveLevel(LevelArray, MetaData, level, Exterior
#ifdef TRANSFER
			  , ImplicitSolver
#endif
			  );
    CallPython(LevelArray, MetaData, level, 0);

    /* Update SubcycleNumber and the timestep counter for the
       streaming data if this is the bottom of the hierarchy -- Note
       that this not unique based on which level is the highest, it
       just keeps going */

    if (LevelArray[level+1] == NULL) {
      MetaData->SubcycleNumber++;
      MetaData->MovieTimestepCounter++;
    }

    /* Once MBH particles are inserted throughout the whole grid hierarchy,
       turn off MBH creation (at the bottom of the hierarchy) */

    if (STARMAKE_METHOD(MBH_PARTICLE) && (LevelArray[level+1] == NULL)) { 
      StarParticleCreation -= pow(2, MBH_PARTICLE);  
    }


    /* ------------------------------------------------------- */
    /* For each grid,
     * (a) project the subgrid's solution into this grid (step #18)
     * (b) correct for the difference between this grid's fluxes and the
     *     subgrid's fluxes. (step #19)
     */
 
#ifdef FLUX_FIX
    SUBlingList = new LevelHierarchyEntry*[NumberOfGrids];
#ifdef FAST_SIB
    CreateSUBlingList(MetaData, LevelArray, level, SiblingList,
		      &SUBlingList);
#else
    CreateSUBlingList(MetaData, LevelArray, level, &SUBlingList);
#endif /* FAST_SIB */
#endif /* FLUX_FIX */

#ifdef FLUX_FIX
    UpdateFromFinerGrids(level, Grids, NumberOfGrids, NumberOfSubgrids,
			     SubgridFluxesEstimate,SUBlingList,MetaData);
#else
    UpdateFromFinerGrids(level, Grids, NumberOfGrids, NumberOfSubgrids,
			 SubgridFluxesEstimate);
#endif

#ifdef FLUX_FIX
    DeleteSUBlingList( NumberOfGrids, SUBlingList );
#endif

  /* ------------------------------------------------------- */
  /* Add the saved fluxes (in the last subsubgrid entry) to the exterior
     fluxes for this subgrid .
     (Note: this must be done after CorrectForRefinedFluxes). */

    FinalizeFluxes(Grids,SubgridFluxesEstimate,NumberOfGrids,NumberOfSubgrids); // This does nearly nothing. e.g., Deallocate and delete fluxes

    /* Recompute radiation field, if requested. */
    RadiationFieldUpdate(LevelArray, level, MetaData);
 
//     //dcc cut second potential cut: Duplicate?
 
//     if (SelfGravity && WritePotential) {
//       CopyGravPotential = TRUE;
//       When = 0.0;
 
// #ifdef FAST_SIB
//       PrepareDensityField(LevelArray, SiblingList, level, MetaData, When);
// #else   // !FAST_SIB
//       PrepareDensityField(LevelArray, level, MetaData, When);
// #endif  // end FAST_SIB
 
 
//       for (grid1 = 0; grid1 < NumberOfGrids; grid1++) {
//         if (level <= MaximumGravityRefinementLevel) {
 
//           /* Compute the potential. */
 
//           if (level > 0)
//             Grids[grid1]->GridData->SolveForPotential(level);
//           Grids[grid1]->GridData->CopyPotentialToBaryonField();
//         }
//       } //  end loop over grids
//        CopyGravPotential = FALSE;

//     } // if WritePotential
 

    /* Rebuild the Grids on the next level down.
       Don't bother on the last cycle, as we'll rebuild this grid soon. */
 
    if (dtThisLevelSoFar[level] < dtLevelAbove)
      RebuildHierarchy(MetaData, LevelArray, level);

    /* Count up number of grids on this level. */

    int GridMemory, NumberOfCells, CellsTotal, Particles;
    float AxialRatio, GridVolume;
    for (grid1 = 0; grid1 < NumberOfGrids; grid1++) {
      Grids[grid1]->GridData->CollectGridInformation
        (GridMemory, GridVolume, NumberOfCells, AxialRatio, CellsTotal, Particles);
      LevelZoneCycleCount[level] += NumberOfCells;
      TIMER_ADD_CELLS(level, NumberOfCells);
      if (MyProcessorNumber == Grids[grid1]->GridData->ReturnProcessorNumber())
	LevelZoneCycleCountPerProc[level] += NumberOfCells;
    }
    TIMER_SET_NGRIDS(level, NumberOfGrids);
 
    cycle++;
    LevelCycleCount[level]++;
 
  } // end of loop over subcycles
 
  if (debug)
    fprintf(stdout, "EvolveLevel[%"ISYM"]: NumberOfSubCycles = %"ISYM" (%"ISYM" total)\n", level,
           cycle, LevelCycleCount[level]);
 
  /* If possible & desired, report on memory usage. */
 
  ReportMemoryUsage("Memory usage report: Evolve Level");
 
#ifdef USE_LCAPERF
  lcaperf.attribute ("level",0,LCAPERF_NULL);
#endif

  /* Clean up. */
 
  delete [] NumberOfSubgrids;
  delete [] Grids;
  delete [] SubgridFluxesEstimate;
  delete [] TotalStarParticleCountPrevious;

  dtThisLevel[level] = dtThisLevelSoFar[level] = 0.0;
 
  /* Clean up the sibling list. */


  if ((NumberOfGrids >1) || ( StaticLevelZero == 1 && level != 0 ) || StaticLevelZero == 0 ) {

    for (grid1 = 0; grid1 < NumberOfGrids; grid1++)
      delete [] SiblingList[grid1].GridList;
    delete [] SiblingList;
  }

  return SUCCESS;
 
}
