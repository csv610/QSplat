#ifndef QSPLAT_MAKE_FROM_MESH_H
#define QSPLAT_MAKE_FROM_MESH_H
/*
Szymon Rusinkiewicz

qsplat_make_from_mesh.h
Code for reading in a triangle mesh from a ply file and initializing the
QSplat data structure.

Copyright (c) 1999-2000 The Board of Trustees of the
Leland Stanford Junior University.  All Rights Reserved.
*/

#include "qsplat_make_qtree_v11.h"

extern bool read_ply(const char *plyfile,
		      int &numleaves, QTree_Node * &leaves,
		      int &numfaces, face * &faces,
		      bool &have_colors,
		      std::string &comments);

extern bool read_mesh(const char *filename,
		      int &numleaves, QTree_Node * &leaves,
		      int &numfaces, face * &faces,
		      bool &have_colors,
		      std::string &comments);

extern bool RunMakeQS(const char *infilename, const char *outfilename, float threshold = 0.0f);

extern void find_normals(int numleaves, QTree_Node *leaves,
		         int numfaces, const face *faces);
extern void merge_nodes(int &numleaves, QTree_Node *leaves,
			int &numfaces, face *faces,
			bool havecolor, float thresh);
extern void find_splat_sizes(int numleaves, QTree_Node *leaves,
		             int numfaces, const face *faces);

#endif
