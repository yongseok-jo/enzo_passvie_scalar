#include <stdio.h>
void auto_show_flags(FILE *fp) {
   fprintf (fp,"\n");
   fprintf (fp,"CPP = /usr/bin/cpp\n");
   fprintf (fp,"CC  = /nasa/intel/Compiler/11.1/046/bin/intel64/icc\n");
   fprintf (fp,"CXX = /nasa/intel/Compiler/11.1/046/bin/intel64/icpc\n");
   fprintf (fp,"FC  = /nasa/intel/Compiler/11.1/046/bin/intel64/ifort\n");
   fprintf (fp,"F90 = /nasa/intel/Compiler/11.1/046/bin/intel64/ifort\n");
   fprintf (fp,"LD  = /nasa/intel/Compiler/11.1/046/bin/intel64/icpc\n");
   fprintf (fp,"\n");
   fprintf (fp,"DEFINES = -DLINUX -DH5_USE_16_API   -D__max_subgrids=100000 -D__max_baryons=30 -D__max_cpu_per_node=8 -D__memory_pool_size=100000 -DINITS64 -DSMALL_INTS -DCONFIG_PINT_4 -DIO_32     -DUSE_MPI   -DCONFIG_PFLOAT_8 -DCONFIG_BFLOAT_8  -DUSE_HDF5_GROUPS    -DTRANSFER   -DNEW_GRID_IO -DFAST_SIB -DBITWISE_IDENTICALITY -DFLUX_FIX     -DSET_ACCELERATION_BOUNDARY\n");
   fprintf (fp,"\n");
   fprintf (fp,"INCLUDES = -I/nasa/hdf5/1.6.5/serial/include -I/nasa/sgi/mpt/1.25/include          -I.\n");
   fprintf (fp,"\n");
   fprintf (fp,"CPPFLAGS = -P -traditional\n");
   fprintf (fp,"CFLAGS   =  -O2\n");
   fprintf (fp,"CXXFLAGS =  -O2\n");
   fprintf (fp,"FFLAGS   =  -r8 -i4 -O2\n");
   fprintf (fp,"F90FLAGS =  -r8 -i4 -O2\n");
   fprintf (fp,"LDFLAGS  =  -O2\n");
   fprintf (fp,"\n");
   fprintf (fp,"LIBS     = -L/nasa/hdf5/1.6.5/serial/lib -lhdf5 -lz  -lifcore -lifport  -L/nasa/sgi/mpt/1.25/lib -lmpi -lmpi++       \n");
   fprintf (fp,"\n");
}
