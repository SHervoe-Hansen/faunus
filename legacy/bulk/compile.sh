# Generic linux compile
gfortran -O3 -o bulk.run bulk.f

#fort77 -O2 -funroll-loops -finit-local-zero -fno-automatic -o bulk.run bulk.f

#Compile options for IRIX/SGI
#f77  -Ofast -static -OPT:IEEE_arithmetic=3:roundoff=3 -n32 -o bulk.run bulk.f

