/***********************************************************************
/
/  COMPUTE STELLAR PHOTON EMISSION RATES
/
/  written by: John Wise
/  date:       November, 2005
/  modified1:
/
/  ---------- SPECIES --------
/  0 : HI
/  1 : HeI
/  2 : HeII
/  3 : Lyman-Werner (H2)
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
#include "Hierarchy.h"
#include "TopGridData.h"
#include "LevelHierarchy.h"

float ReturnValuesFromSpectrumTable(float ColumnDensity, float dColumnDensity, int mode);

int Star::ComputePhotonRates(int &nbins, float E[], double Q[])
{

  int i;
  float x, x2, _mass, EnergyFractionLW, MeanEnergy, XrayLuminosityFraction;
  float EnergyFractionHeI, EnergyFractionHeII;
  x = log10((float)(this->Mass));
  x2 = x*x;

  switch(this->type) {

    /* Luminosities from Schaerer (2002) */

  case PopIII:
    nbins = (PopIIIHeliumIonization &&
	     !RadiativeTransferHydrogenOnly) ? 3 : 1;
#ifdef TRANSFER    
    if (!RadiativeTransferOpticallyThinH2) nbins++;
#endif
    E[0] = 28.0;
    E[1] = 30.0;
    E[2] = 58.0;
    E[3] = 12.8;
    _mass = max(min((float)(this->Mass), 500), 5);
    if (_mass > 9 && _mass <= 500) {
      Q[0] = pow(10.0, 43.61 + 4.9*x   - 0.83*x2);
      Q[1] = pow(10.0, 42.51 + 5.69*x  - 1.01*x2);
      Q[2] = pow(10.0, 26.71 + 18.14*x - 3.58*x2);
      Q[3] = pow(10.0, 44.03 + 4.59*x  - 0.77*x2);
    } else if (_mass > 5 && _mass <= 9) {
      Q[0] = pow(10.0, 39.29 + 8.55*x);
      Q[1] = pow(10.0, 29.24 + 18.49*x);
      Q[2] = pow(10.0, 26.71 + 18.14*x - 3.58*x2);
      Q[3] = pow(10.0, 44.03 + 4.59*x  - 0.77*x2);
    } // ENDELSE
    else {
      for (i = 0; i < nbins; i++) Q[i] = 0.0;
    }
    break;

    /* Average energy from Schaerer (2003) */

  case PopII:
    nbins = (StarClusterHeliumIonization && 
	     !RadiativeTransferHydrogenOnly) ? 3 : 1;
#ifdef TRANSFER    
    if (!RadiativeTransferOpticallyThinH2 &&
	MultiSpecies > 1) nbins++;
#endif
    EnergyFractionLW   = 0.01;
    EnergyFractionHeI  = 0.4284;
    EnergyFractionHeII = 0.0282;
    E[0] = 21.62; // eV (good for a standard, low-Z IMF)
    E[1] = 30.0;
    E[2] = 60.0;
    E[3] = 12.8;
    Q[0] = StarClusterIonizingLuminosity * this->Mass;
    if (StarClusterHeliumIonization) {
      Q[1] = EnergyFractionHeI * Q[0];
      Q[2] = EnergyFractionHeII * Q[0];
      Q[0] *= 1.0 - EnergyFractionHeI - EnergyFractionHeII;
    } else {
      Q[1] = 0.0;
      Q[2] = 0.0;
    }
    Q[3] = EnergyFractionLW * Q[0];
    break;

    /* Approximation to the multi-color disk and power law of an
       accreting BH (Kuhlen & Madau 2004; Alvarez et al. 2009) */

  case BlackHole:
    nbins = 1;
    XrayLuminosityFraction = 0.43;
    EnergyFractionLW = 1.51e-3;
    MeanEnergy = 93.0;  // eV
    E[0] = 460.0;
    E[1] = 0.0;
    E[2] = 0.0;
    E[3] = 12.8;
    Q[0] = 1.12e66 * PopIIIBHLuminosityEfficiency * XrayLuminosityFraction *
      this->last_accretion_rate / E[0];
//    Below is wrong!
//    Q[0] = 3.54e58 * PopIIIBHLuminosityEfficiency * XrayLuminosityFraction *
//      this->DeltaMass / E[0];
    Q[1] = 0.0;
    Q[2] = 0.0;
    Q[3] = EnergyFractionLW * (E[0]/MeanEnergy) * Q[0];
    break;

    /* Average quasar SED by Sazonov et al.(2004), where associated 
       spectral temperature is 2 keV, for accreting massive BH */

  case MBH:
    nbins = 1;
    XrayLuminosityFraction = 1.0;
#define GMC_TEST 
#ifdef GMC_TEST //#####
    //E[0] = 60.0; 
    E[0] = 16.0; // from Whalen (2004) or Whalen & Norman (2006) - Ji-hoon Kim, Jul.2011
#else
    E[0] = 2000.0;
#endif /* GMC_TEST */
    E[1] = 0.0;
    E[2] = 0.0;
    E[3] = 0.0;

#define GMC_TEST_5 //#####  for 121512_dGF-RTF
#ifdef GMC_TEST_5 //#####
    /* 3.86e33ergs/s/Ms * 6.24e11eV/ergs = 2.41e45 eV/s/Ms 
                                         = 2.41e48 eV/s/1000Ms 
       OR use mag = -18.1 from Fig.45b of Leitherer et al. (1999) to get 3.32e51 eV/s/1000Ms
       assumed constant bolometric luminosity for a few Myrs (or a few t_dyn) */
    //Q[0] = 3.32e48 * this->Mass / E[0]; 

    //below was a trick for making Q[0]=0 at the beginning
    //Q[0] = min(3.32e51 / E[0], 
    // 	       1.0e100 * this->last_accretion_rate);  
    
    /* The total ionizing photons, 6.3e46 /s/Msun from Eq.10 of Murray & Rahman (2010) */
    //Q[0] = 6.3e46 * this->Mass; 

    /* 1.18e34 erg/s/Msun for 40 Myrs matches 2e51 ergs of stellar feedback energy as in Ceverino & Klypin (2009)
       1.53e46/6.25e11 * (16-13.6) * (1-0.8) = 1.18e34 */
    Q[0] = 1.53e46 * (1 - StarMassEjectionFraction) * this->Mass; 
#else
    // 1.99e33g/Ms * (3e10cm/s)^2 * 6.24e11eV/ergs = 1.12e66 eV/Ms 
    Q[0] = 1.12e66 * MBHFeedbackRadiativeEfficiency * XrayLuminosityFraction *
      this->last_accretion_rate / E[0]; 
#endif /* GMC_TEST_5 */
    Q[1] = 0.0;
    Q[2] = 0.0;
    Q[3] = 0.0;  

    /* When MBHFeedback = 6 (radio-/quasar-mode duality) 
       change the luminosity of the radiative feedback based on the accretion rate;
       check Grid_AddFeedbackSphere.C for the change in the mechanical feedback */

    const double Grav = 6.673e-8, m_h = 1.673e-24, sigma_T = 6.65e-25, c = 3.0e10;
    float mdot_Edd = 4.0 * PI * Grav * this->Mass * m_h /
      max(MBHFeedbackRadiativeEfficiency, 0.1) / sigma_T / c;  //same as in Star_CalculateMassAccretion

    if (MBHFeedback == 6) {
      Q[0] *= 1 / (1 + exp(-50*(this->last_accretion_rate/mdot_Edd - 0.1))); //#####

//       fprintf(stdout, "star::ComputePhotonRates: mdot, mdot_Edd, radio-/quasar-mode factor = %g %g %g\n", 
// 	      this->last_accretion_rate, mdot_Edd, 
// 	      1 / (1 + exp(-50*(this->last_accretion_rate/mdot_Edd - 0.1))));
    }

#define NOT_HII_REGION_TEST
#ifdef HII_REGION_TEST
    Q[0] = 1.0e45 * MBHFeedbackRadiativeEfficiency * XrayLuminosityFraction / E[0];
#endif
    
//    fprintf(stdout, "star::ComputePhotonRates: this->last_accretion_rate = %g, Q[0]=%g\n", 
//    	    this->last_accretion_rate, Q[0]); 

#ifdef TRANSFER
    if (RadiativeTransferTraceSpectrum == TRUE) {
      nbins = 1;
      E[0] = ReturnValuesFromSpectrumTable(0.0, 0.0, 3); //##### mean energy if column density=0
      E[1] = 0.0;
      E[2] = 0.0;
      E[3] = 0.0;

      Q[0] = 1.12e66 * MBHFeedbackRadiativeEfficiency *
	this->last_accretion_rate / E[0]; 
      Q[1] = 0.0;
      Q[2] = 0.0;
      Q[3] = 0.0;  

      //better check the initial mean energy when tracing spectrum
      if (MyProcessorNumber == ROOT_PROCESSOR)
	fprintf(stdout, "star::CPR: check initial mean E of photon SED: E[0] = %g\n", E[0]); 
    }
#endif

    break;

  case SimpleSource:
    nbins = 1;
    // radiating particle that ramps with time, independant of mass
    E[0] = 20.0;
    Q[0] = SimpleQ; // ramping done in StarParticleRadTransfer.C
    break;

  default:
    ENZO_VFAIL("Star type = %"ISYM" not understood.\n", this->type)

  } // ENDSWITCH

  return SUCCESS;
}
