// ====================================================================
// Copyright 2009
// Yann COLLETTE
// This file is released into the public domain
// ====================================================================

sparsecomp_path = get_absolute_file_path('builder_c.sce');

Files = ['bdfactor.c','hessen.c','machine.c','spchfctr.c', ...
         'bkpfacto.c','hsehldr.c','matlab.c','norm.c','splufctr.c','update.c', ...
	 'chfactor.c','init.c','matop.c','otherio.c','sprow.c','vecop.c', ...
	 'copy.c','matrixio.c','pxop.c','spswap.c','version.c', ...
	 'iter0.c','qrfactor.c','err.c','iternsym.c','meminfo.c','schur.c','submat.c', ...
	 'extras.c','itersym.c','memory.c','solve.c','svd.c', ...
	 'fft.c','memstat.c','sparse.c','symmeig.c', ...
	 'ivecop.c','sparseio.c','givens.c','lufactor.c','mfunc.c','spbkp.c', ...
	 'zcopy.c','zhessen.c','zmachine.c','zmatop.c','zqrfctr.c','zvecop.c', ...
	 'zfunc.c','zhsehldr.c','zmatio.c','zmemory.c','zschur.c', ...
	 'zgivens.c','zlufctr.c','zmatlab.c','znorm.c','zsolve.c'];
Symbols = ['sp_get','sp_set_val','spICHfactor','sp_col_access','spILUfactor'];

libs = [];
ldflags = '';
cflags = '-I' + sparsecomp_path + ' -I' + sparsecomp_path + '/MACHINES/GCC';

tbx_build_src(Symbols, Files, 'c', sparsecomp_path, libs, ldflags, cflags);

clear tbx_build_src;