/*
Szymon Rusinkiewicz

qsplat_make_main.cpp
The routine for building .qs files.

Copyright (c) 1999-2000 The Board of Trustees of the
Leland Stanford Junior University.  All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "qsplat_make_qtree_v11.h"
#include "qsplat_make_from_mesh.h"


// Version stamp
const char *QSPLATMAKE_VERSION = "1.01";


static void usage(const char *myname)
{
	fprintf(stderr, "Usage: %s [-m threshold] in.{ply,obj,off,stl} out.qs\n", myname);
	exit(1);
}

int main(int argc, char *argv[])
{
#ifdef WIN32
	_fmode = _O_BINARY;
#endif
	printf("This is QSplatMake version %s\n", QSPLATMAKE_VERSION);

	// Parse command-line params
	if ((argc < 3) ||
	    !strncasecmp(argv[1], "-h", 2) ||
	    !strncasecmp(argv[1], "--h", 3))
		usage(argv[0]);

	const char *infilename = argv[1];
	const char *outfilename = argv[2];
	float threshold = 0;

	if (argc >= 5 && !strncasecmp(argv[1], "-m", 2)) {
		threshold = atof(argv[2]);
		infilename = argv[3];
		outfilename = argv[4];
	}


	if (!RunMakeQS(infilename, outfilename, threshold)) {
		fprintf(stderr, "Conversion failed.\n");
		exit(1);
	}
	return 0;
}

