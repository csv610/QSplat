/*
Szymon Rusinkiewicz

qsplat_make_qtree_v11.cpp
Builds the tree.

Copyright (c) 1999-2000 The Board of Trustees of the
Leland Stanford Junior University.  All Rights Reserved.
*/

#include <stdio.h>
#include "qsplat_util.h"
#include "qsplat_make_qtree_v11.h"
#include "qsplat_spherequant.h"
#include "qsplat_normquant.h"
#include "qsplat_colorquant.h"
#include <vector>
#include <queue>


// A few random #defines
#define QSPLAT_MAGIC "QSplat"
#define QSPLAT_FILE_VERSION 11
#define ANGLE(x,y) (acos(min(max(Dot(x,y), 0.0f), 1.0f)))


// Static QTree_Node class variable
PoolAlloc QTree_Node::memPool(sizeof(QTree_Node));


// Initializes a Qtree
void QTree::InitLeaves()
{
	QSplat_ColorQuant::Init();
	QSplat_NormQuant::Init();
	QSplat_SphereQuant::Init();

	printf("Initializing tree... "); fflush(stdout);
	int i, next = 0;
	for (i=0; i < numleaves; i++) {
		if (!leaves[i].m.refcount || leaves[i].r == 0.0f)
			continue;
		if (i != next)
			memcpy(&leaves[next], &leaves[i], sizeof(QTree_Node));
		next++;
	}

	if (numleaves != next) {
		printf("Removed %d vertices... ", numleaves - next); fflush(stdout);
		numleaves = next;
	}

	for (i=0; i < numleaves; i++) {
		leaves[i].child[0] = NULL;
		leaves[i].normcone = 0;
		vec &n = leaves[i].norm;
		float l = Len(n);
		if (l == 0.0f) {
			n[0] = n[1] = 0.0f;
			n[2] = 1.0f;
		} else {
			l = 1.0f / l;
			n[0] *= l;  n[1] *= l;  n[2] *= l;
		}
	}

	printf("Done.\n");
}


// Assume the leaves have been filled in, and build the rest of the tree
void QTree::BuildTree()
{
	printf("Building tree... "); fflush(stdout);
	leafptr = new QTree_Node * [numleaves];
	for (int i = 0; i < numleaves; i++)
		leafptr[i] = &(leaves[i]);
	root = BuildLeaves(0, numleaves);
	delete [] leafptr;
	printf("Done.\n");
}


// Return the bounding sphere of the leaves in positions [begin..end)
QTree_Node *QTree::BuildLeaves(int begin, int end)
{
#ifdef DEBUG
	if (begin == end) {
		fprintf(stderr, "Yikes! begin == end in BuildLeaves()\n");
		return NULL;
	}
#endif

	if (end - begin <= 4)
		return CombineLeaves(begin, end);

	// Recursive case
	int middle = Partition(begin, end);

	if (middle - begin <= 4) {
		if (end - middle <= 4) {
			return CombineNodes(CombineLeaves(begin, middle),
					    CombineLeaves(middle, end));
		} else {
			int m2 = Partition(middle, end);
			return CombineNodes(CombineLeaves(begin, middle),
					    BuildLeaves(middle, m2),
					    BuildLeaves(m2, end));
		}
	} else {
		if (end - middle <= 4) {
			int m1 = Partition(begin, middle);
			return CombineNodes(BuildLeaves(begin, m1),
					    BuildLeaves(m1, middle),
					    CombineLeaves(middle, end));
		} else {
			int m1 = Partition(begin, middle);
			int m2 = Partition(middle, end);
			return CombineNodes(BuildLeaves(begin, m1),
					    BuildLeaves(m1, middle),
					    BuildLeaves(middle, m2),
					    BuildLeaves(m2, end));
		}
	}
}


// 3-way overloaded function: return a new node in the tree, given the nodes
// of 2, 3, or 4 children.
QTree_Node *QTree::CombineNodes(QTree_Node *n1, QTree_Node *n2)
{
	QTree_Node *n = new QTree_Node;

	n->pos[0] = (n1->pos[0] + n2->pos[0])/2.0f;
	n->pos[1] = (n1->pos[1] + n2->pos[1])/2.0f;
	n->pos[2] = (n1->pos[2] + n2->pos[2])/2.0f;

	n->r = max(Dist(n->pos, n1->pos) + n1->r,
		   Dist(n->pos, n2->pos) + n2->r);

	n->norm[0] = n1->norm[0] + n2->norm[0];
	n->norm[1] = n1->norm[1] + n2->norm[1];
	n->norm[2] = n1->norm[2] + n2->norm[2];
	Normalize(n->norm);

	n->normcone = max(ANGLE(n->norm, n1->norm) + n1->normcone,
			  ANGLE(n->norm, n2->norm) + n2->normcone);

	if (havecolor) {
		n->col[0] = (n1->col[0] + n2->col[0]) / 2;
		n->col[1] = (n1->col[1] + n2->col[1]) / 2;
		n->col[2] = (n1->col[2] + n2->col[2]) / 2;
	}

	n->child[0] = n1;
	n->child[1] = n2;
	n->child[2] = NULL;

	return n;
}

QTree_Node *QTree::CombineNodes(QTree_Node *n1, QTree_Node *n2, QTree_Node *n3)
{
	QTree_Node *n = new QTree_Node;

	n->pos[0] = (n1->pos[0] + n2->pos[0] + n3->pos[0])/3.0f;
	n->pos[1] = (n1->pos[1] + n2->pos[1] + n3->pos[1])/3.0f;
	n->pos[2] = (n1->pos[2] + n2->pos[2] + n3->pos[2])/3.0f;

	n->r = max(max(Dist(n->pos, n1->pos) + n1->r,
		       Dist(n->pos, n2->pos) + n2->r),
		       Dist(n->pos, n3->pos) + n3->r);

	n->norm[0] = n1->norm[0] + n2->norm[0] + n3->norm[0];
	n->norm[1] = n1->norm[1] + n2->norm[1] + n3->norm[1];
	n->norm[2] = n1->norm[2] + n2->norm[2] + n3->norm[2];
	Normalize(n->norm);

	n->normcone = max(max(ANGLE(n->norm, n1->norm) + n1->normcone,
			      ANGLE(n->norm, n2->norm) + n2->normcone),
			      ANGLE(n->norm, n3->norm) + n3->normcone);

	if (havecolor) {
		n->col[0] = (n1->col[0] + n2->col[0] + n3->col[0]) / 3;
		n->col[1] = (n1->col[1] + n2->col[1] + n3->col[1]) / 3;
		n->col[2] = (n1->col[2] + n2->col[2] + n3->col[2]) / 3;
	}

	n->child[0] = n1;
	n->child[1] = n2;
	n->child[2] = n3;
	n->child[3] = NULL;

	return n;
}

QTree_Node *QTree::CombineNodes(QTree_Node *n1, QTree_Node *n2,
				QTree_Node *n3, QTree_Node *n4)
{
	QTree_Node *n = new QTree_Node;

	n->pos[0] = (n1->pos[0] + n2->pos[0] + n3->pos[0] + n4->pos[0])/4.0f;
	n->pos[1] = (n1->pos[1] + n2->pos[1] + n3->pos[1] + n4->pos[1])/4.0f;
	n->pos[2] = (n1->pos[2] + n2->pos[2] + n3->pos[2] + n4->pos[2])/4.0f;

	n->r = max(max(Dist(n->pos, n1->pos) + n1->r,
		       Dist(n->pos, n2->pos) + n2->r),
		   max(Dist(n->pos, n3->pos) + n3->r,
		       Dist(n->pos, n4->pos) + n4->r));

	n->norm[0] = n1->norm[0] + n2->norm[0] + n3->norm[0] + n4->norm[0];
	n->norm[1] = n1->norm[1] + n2->norm[1] + n3->norm[1] + n4->norm[1];
	n->norm[2] = n1->norm[2] + n2->norm[2] + n3->norm[2] + n4->norm[2];
	Normalize(n->norm);

	n->normcone = max(max(ANGLE(n->norm, n1->norm) + n1->normcone,
			      ANGLE(n->norm, n2->norm) + n2->normcone),
			  max(ANGLE(n->norm, n3->norm) + n3->normcone,
			      ANGLE(n->norm, n4->norm) + n4->normcone));

	if (havecolor) {
		n->col[0] = (n1->col[0] + n2->col[0] + n3->col[0] + n4->col[0]) / 4;
		n->col[1] = (n1->col[1] + n2->col[1] + n3->col[1] + n4->col[1]) / 4;
		n->col[2] = (n1->col[2] + n2->col[2] + n3->col[2] + n4->col[2]) / 4;
	}

	n->child[0] = n1;
	n->child[1] = n2;
	n->child[2] = n3;
	n->child[3] = n4;

	return n;
}


// Run CombineNodes on the leaves in positions begin through end
QTree_Node *QTree::CombineLeaves(int begin, int end)
{
	switch (end - begin) {
		case 1:
			return leafptr[begin];
		case 2:
			return CombineNodes(leafptr[begin], leafptr[begin+1]);
		case 3:
			return CombineNodes(leafptr[begin], leafptr[begin+1],
					    leafptr[begin+2]);
		case 4:
			return CombineNodes(leafptr[begin], leafptr[begin+1],
					    leafptr[begin+2], leafptr[begin+3]);
	}

#ifdef DEBUG
	fprintf(stderr, "Yikes! end - begin not between 1 and 4 in CombineLeaves()\n");
#endif
	return NULL;
}


// Do partitioning on the range [begin..end) of thetree.  Returns index on
// which we partition (i.e. partitions are [begin..middle) and [middle..end) )
int QTree::Partition(int begin, int end)
{
#ifdef DEBUG
	if (end - begin <= 4)
		fprintf(stderr, "end - begin <= 4 in Partition()\n");
#endif

	float xmin, xmax, ymin, ymax, zmin, zmax;
	FindBBox(begin, end, xmin, xmax, ymin, ymax, zmin, zmax);
	float dx = xmax-xmin, dy = ymax-ymin, dz = zmax-zmin;
	if (!dx && !dy && !dz) {
#ifdef DEBUG
		fprintf(stderr, "dx, dy, dz all zero in Partition()\n");
#endif
		return (begin+end)/2;
	}


	// Split along longest axis
	int splitaxis;
	float axis_min, axis_max;
	if (dx > dy) {
		if (dx > dz) {
			splitaxis = 0;
			axis_min = xmin;  axis_max = xmax;
		} else {
			splitaxis = 2;
			axis_min = zmin;  axis_max = zmax;
		}
	} else {
		if (dy > dz) {
			splitaxis = 1;
			axis_min = ymin;  axis_max = ymax;
		} else {
			splitaxis = 2;
			axis_min = zmin;  axis_max = zmax;
		}
	}

	float splitval = 0.5f * (axis_min + axis_max);
	int middle = PartitionAlongAxis(begin, end, splitaxis, splitval);


	// If we're down to a small number of nodes, don't try to do
	// anything fancy, just split along the middle
	if (end - begin <= 64)
		return middle;


	// Otherwise, try a few more candidate splits
	splitval = 0.6f * axis_min + 0.4f * axis_max;
	int mid1 = PartitionAlongAxis(begin, middle, splitaxis, splitval);

	splitval = 0.4f * axis_min + 0.6f * axis_max;
	int mid2 = PartitionAlongAxis(middle, end, splitaxis, splitval);

	float xmin1, xmax1, ymin1, ymax1, zmin1, zmax1;
	float xmin2, xmax2, ymin2, ymax2, zmin2, zmax2;
	float xmin3, xmax3, ymin3, ymax3, zmin3, zmax3;
	float xmin4, xmax4, ymin4, ymax4, zmin4, zmax4;
	FindBBox(begin,  mid1,   xmin1, xmax1, ymin1, ymax1, zmin1, zmax1);
	FindBBox(mid1,   middle, xmin2, xmax2, ymin2, ymax2, zmin2, zmax2);
	FindBBox(middle, mid2,   xmin3, xmax3, ymin3, ymax3, zmin3, zmax3);
	FindBBox(mid2,   end,    xmin4, xmax4, ymin4, ymax4, zmin4, zmax4);

	float vol1 = (xmax1-xmin1) * (ymax1-ymin1) * (zmax1-zmin1) +
		     (max(max(xmax2, xmax3), xmax4) - min(min(xmin2, xmin3), xmin4)) *
		     (max(max(ymax2, ymax3), ymax4) - min(min(ymin2, ymin3), ymin4)) *
		     (max(max(zmax2, zmax3), zmax4) - min(min(zmin2, zmin3), zmin4));

	float vol2 = (max(xmax1, xmax2) - min(xmin1, xmin2)) *
		     (max(ymax1, ymax2) - min(ymin1, ymin2)) *
		     (max(zmax1, zmax2) - min(zmin1, zmin2))  +
		     (max(xmax3, xmax4) - min(xmin3, xmin4)) *
		     (max(ymax3, ymax4) - min(ymin3, ymin4)) *
		     (max(zmax3, zmax4) - min(zmin3, zmin4));
	// An even split is desirable, so we give this a slight bonus
	vol2 *= 0.95f;

	float vol3 = (max(max(xmax1, xmax2), xmax3) - min(min(xmin1, xmin2), xmin3)) *
		     (max(max(ymax1, ymax2), ymax3) - min(min(ymin1, ymin2), ymin3)) *
		     (max(max(zmax1, zmax2), zmax3) - min(min(zmin1, zmin2), zmin3))  +
		     (xmax4-xmin4) * (ymax4-ymin4) * (zmax4-zmin4);

	if (vol1 < vol3) {
		if (vol1 < vol2) return mid1;
	} else {
		if (vol3 < vol2) return mid2;
	}
	return middle;
}


// Partition the range [begin..end) of thetree along the given axis, at the
// given value.  Returns index on which we partition.
int QTree::PartitionAlongAxis(int begin, int end, int axis, float splitval)
{
	int left = begin, right = end-1;

	while (1) {
		// March the "left" pointer to the right
		while (leafptr[left]->pos[axis] < splitval)
			left++;

		// March the "right" pointer to the left
		while (leafptr[right]->pos[axis] >= splitval)
			right--;

		// If the pointers have crossed, we're done
		if (right < left) {
#ifdef DEBUG
			if (left == begin) {
				fprintf(stderr, "Yikes! left == begin in PartitionAlongAxis()\n");
				left++;
			} else if (left == end) {
				fprintf(stderr, "Yikes! left == end in PartitionAlongAxis()\n");
				left--;
			}
#endif
			return left;
		}

		// Else, swap and repeat
		swap(leafptr[left], leafptr[right]);
	}
}


// A couple of trivial helper functions
static inline void write_float(FILE *f, float F)
{
	FIX_FLOAT(F);
	fwrite((void *)&F, 4, 1, f);
}
static inline void write_int(FILE *f, int I)
{
	FIX_LONG(I);
	fwrite((void *)&I, 4, 1, f);
}

static inline void write_comments(FILE *f, const std::string &comments)
{
	int s = comments.size();
	int padding = (4 - (s % 4)) % 4;
	unsigned char buf[9];
	sprintf((char *)buf, "%s%02d", QSPLAT_MAGIC, QSPLAT_FILE_VERSION);
	fwrite((void *)buf, 8, 1, f);
	write_int(f, 20 + s + padding);
	write_int(f, 0);
	write_int(f, 2);
	fwrite((void *)comments.data(), s, 1, f);
	buf[0] = buf[1] = buf[2] = 0;
	if (padding) fwrite((void *)buf, padding, 1, f);
}


// Writes out the QTree to a .qs file.  Note: this messes up the values in
// the QTree!
void QTree::Write(const char *qsfile, const std::string &comments)
{
	FILE *f = fopen(qsfile, "w");
	if (!f) {
		fprintf(stderr, "Couldn't open %s for writing.\n", qsfile);
		return;
	}
	printf("Quantizing and writing... "); fflush(stdout);

	// Write out comment
	if (!comments.empty())
		write_comments(f, comments);

	// Write out magic number
	unsigned char buf[9];
	sprintf((char *)buf, "%s%02d", QSPLAT_MAGIC, QSPLAT_FILE_VERSION);
	fwrite((void *)buf, 8, 1, f);

	// Write out file length
	int file_len = 8			// Magic number
		     + 4			// File length
		     + 4			// Number of leaf nodes
		     + 4			// Options 'n parameters
		     + 4*3 + 4			// Center and R of top level
		     + 4			// # of children @ top level
		     + Treesize();		// The tree itself
	int padding = (4 - (file_len % 4)) % 4;
	if (padding == 4)
		padding = 0;
	else
		file_len += padding;
	write_int(f, file_len);

	// Write out more stuff in the header
	write_int(f, numleaves);
	write_int(f, havecolor);
	write_float(f, root->pos[0]);
	write_float(f, root->pos[1]);
	write_float(f, root->pos[2]);
	write_float(f, root->r);
	write_int(f, Num_Children(root));

	// Write out the nodes...
	std::queue<QTree_Node *> todolist;
	todolist.push(root);
	int todolistlen = Nodesize(root);

	while (!todolist.empty()) {

		QTree_Node *this_node = todolist.front();
		todolist.pop();
		int this_size = Nodesize(this_node);

		// Write out the children of this node
		if (Has_Grandchildren(this_node))
			write_int(f, todolistlen);
		todolistlen -= this_size;

		for (int i = 0; i < Num_Children(this_node); i++) {
			QTree_Node *n = this_node->child[i];

			QSplat_SphereQuant::quantize(
				this_node->pos[0], this_node->pos[1],
				this_node->pos[2], this_node->r,
				n->pos[0], n->pos[1],
				n->pos[2], n->r,
				buf);
#if 0
			float oldr = n->r;
#endif
			QSplat_SphereQuant::lookup(
				buf,
				this_node->pos[0], this_node->pos[1],
				this_node->pos[2], this_node->r,
				n->pos[0], n->pos[1],
				n->pos[2], n->r);
#if 0
			if (oldr > n->r || n->r / oldr > 5.0f) {
				printf("Ack! oldr = %f, n->r = %f\n", oldr, n->r);
				printf("     quantized / real = %f, parent / real = %f\n", n->r/oldr, this_node->r/oldr);
			}
#endif

			buf[1] |= Num_Children(n) ? Num_Children(n) - 1 : 0;
			if (Has_Grandchildren(n))
				buf[1] |= 4;

			QSplat_NormQuant::quantize(n->norm, buf+2);
			QSplat_NormQuant::quantize_cone(n->normcone, buf+2);
			if (havecolor) {
				QSplat_ColorQuant::quantize(n->col, buf+4);
				fwrite((void *)buf, 6, 1, f);
			} else {
				fwrite((void *)buf, 4, 1, f);
			}

			if (Num_Children(n)) {
				todolist.push(n);
				todolistlen += Nodesize(n);
			}
		}

	}

	if (padding)
		fwrite((void *)buf, padding, 1, f);
	fclose(f);
	printf("Done.\n");
}


// Number of children of this node
int QTree::Num_Children(QTree_Node *n)
{
	return (n->child[0] ? (n->child[1] ? (n->child[2] ? (n->child[3] ?
		4 : 3) : 2) : 1) : 0);
}


// Does this node have any grandchildren?
bool QTree::Has_Grandchildren(QTree_Node *n)
{
	if (!n->child[0])
		return false;
	if (n->child[0]->child[0])
		return true;
	if (!n->child[1])
		return false;
	if (n->child[1]->child[0])
		return true;
	if (!n->child[2])
		return false;
	if (n->child[2]->child[0])
		return true;
	if (!n->child[3])
		return false;
	if (n->child[3]->child[0])
		return true;
	return false;
}


// Return size in bytes of a group of nodes on disk
int QTree::Nodesize(QTree_Node *n)
{
	if (!n || !n->child[0])
		return 0;
	else
		return ((havecolor ? 6 : 4) * Num_Children(n)) +
		       (Has_Grandchildren(n) ? 4 : 0);
}

// Return size in bytes of the tree on disk
int QTree::Treesize(QTree_Node *n /* = NULL */ )
{
	if (!n)	
		n = root;

	if (!n->child[0])
		return 0;

	int size = Nodesize(n) + Treesize(n->child[0]);
	if (n->child[1]) {
		size += Treesize(n->child[1]);
		if (n->child[2]) {
			size += Treesize(n->child[2]);
			if (n->child[3])
				size += Treesize(n->child[3]);
		}
	}
	return size;
}

