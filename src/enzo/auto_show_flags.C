#include <stdio.h>
void auto_show_flags(FILE *fp) {
   fprintf (fp,"\n");
   fprintf (fp,"CPP = /usr/bin/cpp\n");
   fprintf (fp,"CC  = /apps/compiler/intel/19.0.5/impi/2019.5.281/intel64/bin/mpiicc\n");
   fprintf (fp,"CXX = /apps/compiler/intel/19.0.5/impi/2019.5.281/intel64/bin/mpiicpc\n");
   fprintf (fp,"FC  = /apps/compiler/intel/19.0.5/impi/2019.5.281/intel64/bin/mpiifort\n");
   fprintf (fp,"F90 = /apps/compiler/intel/19.0.5/impi/2019.5.281/intel64/bin/mpiifort\n");
   fprintf (fp,"LD  = /apps/compiler/intel/19.0.5/impi/2019.5.281/intel64/bin/mpiicpc\n");
   fprintf (fp,"\n");
   fprintf (fp,"DEFINES = -DLINUX -DH5_USE_16_API   -D__max_subgrids=100000 -D__max_baryons=30 -D__max_cpu_per_node=8 -D__memory_pool_size=100000 -DINITS64 -DSMALL_INTS -DCONFIG_PINT_4 -DIO_32     -DUSE_MPI   -DCONFIG_PFLOAT_8 -DCONFIG_BFLOAT_8  -DUSE_HDF5_GROUPS    -DTRANSFER   -DNEW_GRID_IO -DFAST_SIB -DBITWISE_IDENTICALITY -DFLUX_FIX     -DSET_ACCELERATION_BOUNDARY\n");
   fprintf (fp,"\n");
   fprintf (fp,"INCLUDES = -I/scratch/x1921a02/packages/hdf5-1.8.20/build/include            -I.\n");
   fprintf (fp,"\n");
   fprintf (fp,"CPPFLAGS = -P -traditional \n");
   fprintf (fp,"CFLAGS   = -O2 -O2\n");
   fprintf (fp,"CXXFLAGS = -O2  -O2\n");
   fprintf (fp,"FFLAGS   = -O2 -r8 -i4    -O2\n");
   fprintf (fp,"F90FLAGS = -O2 -r8 -i4    -O2\n");
   fprintf (fp,"LDFLAGS  = -O2  -O2\n");
   fprintf (fp,"\n");
   fprintf (fp,"LIBS     = -L/scratch/x1921a02/packages/hdf5-1.8.20/build/lib -lhdf5 -lz  -lifcore -lifport -limf -lstdc++           \n");
   fprintf (fp,"\n");
}
