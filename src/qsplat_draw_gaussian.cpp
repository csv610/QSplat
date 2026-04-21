#define GL_SILENCE_DEPRECATION
/*
Szymon Rusinkiewicz

qsplat_draw_gaussian.cpp
Two-pass method for drawing fuzzy splats using OpenGL

Copyright (c) 1999-2000 The Board of Trustees of the
Leland Stanford Junior University.  All Rights Reserved.
*/

#include "qsplat_gui.h"
#ifdef DARWIN
# include <OpenGL/gl.h>
#else
# include <GL/gl.h>
#endif


// Local variables
static bool secondpass;
static vec rdir, uldir, lldir;
static point campos;
static const float sqrt3 = 1.7320508f;
static const float depthfudge = 5.0f;
static const float splatscale = 2.2f;


// Set up texture map...
static void setuptexturing()
{
	static unsigned char *thetexture;
	static unsigned texcontext = 0;

	if (!glIsTexture(texcontext)) {
		glGenTextures(1, (GLuint *) &texcontext);
		glBindTexture(GL_TEXTURE_2D, texcontext);
		const int texsize = 32;
		thetexture = new unsigned char[sqr(texsize) * 2];
		for (int x=0; x < texsize; x++) {
			for (int y=0; y < texsize; y++) {
				int index = x*texsize + y;
				float r2 = sqr(0.5f+x-texsize/2)+sqr(0.5f+y-texsize/2);
				r2 /= sqr(texsize/2);
				r2 *= sqr(1.05f);
				if (r2 >= 1.0f) {
					thetexture[2*index  ] = 0;
					thetexture[2*index+1] = 0;
				} else {
					thetexture[2*index  ] = 255;
					if (r2 < 0.25f)
						thetexture[2*index+1] = int(255.99f*(1.0f-2.0f*r2));
					else
						thetexture[2*index+1] = int(255.99f*2.0f*sqr(1.0f-sqrtf(r2)));
				}
			}
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                             texsize, texsize, 0,
                             GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                             thetexture);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
	glBindTexture(GL_TEXTURE_2D, texcontext);
}



// Little function called right before we start drawing
void start_drawing_gaussian(bool havecolor)
{
	secondpass = false;

	float M[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, M);

	rdir[0]  = 2.0f * M[0];
	rdir[1]  = 2.0f * M[4];
	rdir[2]  = 2.0f * M[8];
	uldir[0] = sqrt3 * M[1] - M[0];
	uldir[1] = sqrt3 * M[5] - M[4];
	uldir[2] = sqrt3 * M[9] - M[8];
	lldir[0] = -sqrt3 * M[1] - M[0];
	lldir[1] = -sqrt3 * M[5] - M[4];
	lldir[2] = -sqrt3 * M[9] - M[8];

	campos[0] = -(M[0]*M[12] + M[1]*M[13] + M[2]*M[14]);
	campos[1] = -(M[4]*M[12] + M[5]*M[13] + M[6]*M[14]);
	campos[2] = -(M[8]*M[12] + M[9]*M[13] + M[10]*M[14]);

	setuptexturing();
	glAlphaFunc(GL_GREATER, 0.0f);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_TRUE);
	glColor4f(0, 0, 0, 1);
	glBegin(GL_TRIANGLES);
}


// Draw a point at (cx, cy, cz).  Radius is r in world coords, which must
// correspond to a screen-space size (diameter) of splatsize.
// Normal (3 floats) is pointed to by norm.
// Color (3 floats) is pointed to by col.
void drawpoint_gaussian(float cx, float cy, float cz,
			float r, float splatsize,
			const float *norm, const float *col)
{
	if (secondpass) {
		if (col) glColor3fv(col);
		glNormal3fv(norm);
	}

	vec p1, p2, p3;

	vec splat_to_cam = { campos[0] - cx, campos[1] - cy, campos[2] - cz };
	if (Dot(splat_to_cam, norm) <= 0.0f)
		return;

	vec x, y;
	CrossProd(splat_to_cam, norm, x);
	float lx = Len(x);
	if (lx == 0.0f) {
		p1[0] = cx + r * splatscale * rdir[0];
		p1[1] = cy + r * splatscale * rdir[1];
		p1[2] = cz + r * splatscale * rdir[2];
		p2[0] = cx + r * splatscale * uldir[0];
		p2[1] = cy + r * splatscale * uldir[1];
		p2[2] = cz + r * splatscale * uldir[2];
		p3[0] = cx + r * splatscale * lldir[0];
		p3[1] = cy + r * splatscale * lldir[1];
		p3[2] = cz + r * splatscale * lldir[2];
	} else {
		CrossProd(norm, x, y);

		float rdist2 = 1.0f / (sqr(splat_to_cam[0]) +
				sqr(splat_to_cam[1]) + sqr(splat_to_cam[2]));
		float xdot = Dot(x, splat_to_cam) * rdist2;
		x[0] -= splat_to_cam[0] * xdot;
		x[1] -= splat_to_cam[1] * xdot;
		x[2] -= splat_to_cam[2] * xdot;

		float ydot = Dot(y, splat_to_cam) * rdist2;
		y[0] -= splat_to_cam[0] * ydot;
		y[1] -= splat_to_cam[1] * ydot;
		y[2] -= splat_to_cam[2] * ydot;

		float scale = r * splatscale / lx;
		p1[0] = cx + scale * 2.0f * x[0];
		p1[1] = cy + scale * 2.0f * x[1];
		p1[2] = cz + scale * 2.0f * x[2];
		p2[0] = cx - scale * (x[0] - sqrt3 * y[0]);
		p2[1] = cy - scale * (x[1] - sqrt3 * y[1]);
		p2[2] = cz - scale * (x[2] - sqrt3 * y[2]);
		p3[0] = cx - scale * (x[0] + sqrt3 * y[0]);
		p3[1] = cy - scale * (x[1] + sqrt3 * y[1]);
		p3[2] = cz - scale * (x[2] + sqrt3 * y[2]);
	}

	if (!secondpass) {
		// First pass
		vec p1c = { campos[0] - p1[0], campos[1] - p1[1], campos[2] - p1[2] };
		float scale1 = depthfudge * r / Len(p1c);
		glTexCoord2f(1.5f, 0.5f);
		glVertex3f(p1[0] - scale1 * p1c[0],
			   p1[1] - scale1 * p1c[1],
			   p1[2] - scale1 * p1c[2]);

		vec p2c = { campos[0] - p2[0], campos[1] - p2[1], campos[2] - p2[2] };
		float scale2 = depthfudge * r / Len(p2c);
		glTexCoord2f(0.0f, 0.5f+0.5f*sqrt3);
		glVertex3f(p2[0] - scale2 * p2c[0],
			   p2[1] - scale2 * p2c[1],
			   p2[2] - scale2 * p2c[2]);

		vec p3c = { campos[0] - p3[0], campos[1] - p3[1], campos[2] - p3[2] };
		float scale3 = depthfudge * r / Len(p3c);
		glTexCoord2f(0.0f, 0.5f-0.5f*sqrt3);
		glVertex3f(p3[0] - scale3 * p3c[0],
			   p3[1] - scale3 * p3c[1],
			   p3[2] - scale3 * p3c[2]);
	} else {
		// Second pass
		glTexCoord2f(1.5f, 0.5f);
		glVertex3fv(p1);
		glTexCoord2f(0.0f, 0.5f+0.5f*sqrt3);
		glVertex3fv(p2);
		glTexCoord2f(0.0f, 0.5f-0.5f*sqrt3);
		glVertex3fv(p3);
		theQSplatGUI->pts_splatted++;
	}
}


// Prepare for the second rendering pass
void second_pass_gaussian(bool havecolor)
{
	secondpass = true;
	glEnd();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);  
	glEnable(GL_LIGHTING);
	if (havecolor) {
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);
	} else {
		glDisable(GL_COLOR_MATERIAL);
		float gray[] = { 0.9f, 0.9f, 0.9f, 0.9f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gray);
	}

	glBegin(GL_TRIANGLES);
}


// Little function called right after we finish drawing
void end_drawing_gaussian()
{
	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_LIGHTING);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
}

