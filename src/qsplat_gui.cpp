/*
Szymon Rusinkiewicz

qsplat_gui.cpp
The QSplat GUI.

Copyright (c) 1999-2000 The Board of Trustees of the
Leland Stanford Junior University.  All Rights Reserved.
*/

#define GL_SILENCE_DEPRECATION
#include <stdio.h>

#include "qsplat_gui.h"
#include "qsplat_util.h"

#ifdef DARWIN
# include <OpenGL/glu.h>
#else
# include <GL/glu.h>
# include <GL/gl.h>
#endif

// Various configuration parameters
#define SPECULAR	0.25f
#define AMBIENT		0.05f
#define LIGHT		0.85f
#define SHININESS	16.0f
#define DOF		30.0f
#define ROT_MULT	1.2f
#define TRANSZ_MULT	1.2f
#define TRANSXY_MULT	1.0f
#define WHEEL_MOVE	0.05f
#define RTSIZE		10
#define DEPTH_FUDGE	0.2f
#define REFINE_DELAY	0.5f
#define SHOWLIGHT_TIME	2.5f
#define SHOWPROG_TIME	1.5f
#define MOUSE_DEADZONE	0.001f


#include "qsplat_make_from_mesh.h"

// The GUI global variable
QSplatGUI *theQSplatGUI;

QSplatGUI::~QSplatGUI()
{
    if (pointShader) delete pointShader;
    if (vao) glDeleteVertexArraysAPPLE(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
}

std::string QSplatGUI::AutoMakeQS(const char *filename)
{
    std::string in(filename);
    const char *ext = strrchr(filename, '.');
    if (ext && strcasecmp(ext, ".qs") == 0)
        return in;

    printf("Auto-converting %s to QS format...\n", filename);
    
    // Generate a temporary filename in /tmp/
    char tmpname[255];
    static int counter = 0;
    snprintf(tmpname, sizeof(tmpname), "/tmp/qsplat_auto_%d.qs", counter++);

    if (RunMakeQS(filename, tmpname)) {
        printf("Conversion successful: %s\n", tmpname);
        return std::string(tmpname);
    }

    fprintf(stderr, "Auto-conversion failed for %s\n", filename);
    return "";
}

void QSplatGUI::OpenModel(const char *filename)
{
    std::string actual_file = AutoMakeQS(filename);
    if (actual_file == "") {
        display_dialog("Error", "Could not open or convert mesh file.");
        return;
    }

    setmodel(NULL);
    QSplat_Model *q;
    q = QSplat_Model::Open(actual_file.c_str());
    if (!q) {
        display_dialog("Error", "Couldn't open QSplat file");
        return;
    }
    setmodel(q);
    updatemenus();
    resetviewer();
    need_redraw();    
}

// Tell the GUI to use this model
void QSplatGUI::setmodel(QSplat_Model *q)
{
    if (theQSplat_Model)
            delete theQSplat_Model;

    theQSplat_Model = q;
    if (!q)
            return;

    // Set to default rate
    theQSplat_Model->reset_rate();

    // Look for a .xf file, and set "home" camera position
    char xffilename[255];
    strcpy(xffilename, q->filename.c_str());
    xffilename[strlen(xffilename)-2] = 'x';
    xffilename[strlen(xffilename)-1] = 'f';
    if (!thecamera.SetHome(xffilename)) {
            point homepos = { q->center[0],
                                q->center[1],
                                q->center[2] + 3.0f * q->radius };
            point homerotaxis = { 0, 0, 1 };
            thecamera.SetHome(homepos, 0, homerotaxis, DEFAULT_FOV);
    }
         
}


void QSplatGUI::draw_bbox()
{
	if (!theQSplat_Model) return;

	float xmin = theQSplat_Model->center[0] - theQSplat_Model->radius;
	float xmax = theQSplat_Model->center[0] + theQSplat_Model->radius;
	float ymin = theQSplat_Model->center[1] - theQSplat_Model->radius;
	float ymax = theQSplat_Model->center[1] + theQSplat_Model->radius;
	float zmin = theQSplat_Model->center[2] - theQSplat_Model->radius;
	float zmax = theQSplat_Model->center[2] + theQSplat_Model->radius;

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);

	glBegin(GL_LINE_LOOP);
	glVertex3f(xmin, ymin, zmin); glVertex3f(xmax, ymin, zmin);
	glVertex3f(xmax, ymax, zmin); glVertex3f(xmin, ymax, zmin);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3f(xmin, ymin, zmax); glVertex3f(xmax, ymin, zmax);
	glVertex3f(xmax, ymax, zmax); glVertex3f(xmin, ymax, zmax);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(xmin, ymin, zmin); glVertex3f(xmin, ymin, zmax);
	glVertex3f(xmax, ymin, zmin); glVertex3f(xmax, ymin, zmax);
	glVertex3f(xmax, ymax, zmin); glVertex3f(xmax, ymax, zmax);
	glVertex3f(xmin, ymax, zmin); glVertex3f(xmin, ymax, zmax);
	glEnd();

	glPopAttrib();
}

// Do a redraw!  That's why we're here...
void QSplatGUI::redraw()
{
    // If we have no model, nothing to do
    if (!theQSplat_Model) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        swapbuffers();
        updatestatus("No model");
        updaterate("");
        return;
    }


    // We need to do resetviewer() before we start drawing, but
    // we can't do it until we're sure we have an OpenGL context.
    // So, we do it here...
    static bool first_time = true;
    if (first_time) {
        resetviewer();
        first_time = false;
    }

    // We're going to time how long the following takes
    timestamp start_time;
    get_timestamp(start_time);

    // Get the distance to the surface, so we know where to put clipping
    // planes.
    update_depth();

    // Set the viewport and clear the screen
    setupGLstate();
    if ((int)whichDriver < (int)SOFTWARE_GLDRAWPIXELS)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Set up projection and modelview matrices, and lighting
    setup_matrices(true);


    // Draw...
    pts_splatted = 0;
    bool bailed = theQSplat_Model->draw();

    // Examine what happened
    if (bailed) {
        if (dorefine) {
            // A run-of-the-mill move-the-mouse-to-stop-refine
            // sort of event happened
            stop_refine();
            return;
        } else {
            // That took waaaay longer than expected, and user
            // got bored.  Do a random fudge to try to avoid this
            // next time.
            theQSplat_Model->adjust_rate(4.0f);
            return;
        }
    }

    if (whichDriver != SOFTWARE &&
        whichDriver != SOFTWARE_TILES &&
        whichDriver != SOFTWARE_BEST) {
        if (showbbox) draw_bbox();
        draw_light();
        draw_progressbar();
        swapbuffers();
    }

    // Figure out how long that took
    timestamp end_time;
    get_timestamp(end_time);
    float elapsed = end_time - start_time;

    char ratemsg[255];
    snprintf(ratemsg, sizeof(ratemsg), "%d points, %.3f sec.", pts_splatted, elapsed);
    updaterate(ratemsg);

    if (dorefine) {
        if (elapsed <= 1.0f/desiredrate)
            // We're refining, but this frame took shorter than the
            // desired rendering time.  Therefore, we can save
            // the current minsize and when we exit refining
            // it'll still be at an acceptable rate.
            theQSplat_Model->start_refine();
        return;
    }

    // Try to adjust the rate for the next frame based on how long
    // this one actually took
    float correction = elapsed * desiredrate;
    if (correction > 1.0f)
        correction *= sqrtf(sqrtf(correction));
    else
        correction /= sqrtf(sqrtf(correction));
    theQSplat_Model->adjust_rate(correction);
    return;
}


// Draw an indication of light source direction
void QSplatGUI::draw_light()
{
	if (showlight != SHOWLIGHT_ON)
		return;
	timestamp now;
	get_timestamp(now);
	if (now - last_relight_time > SHOWLIGHT_TIME) {
		showlight = SHOWLIGHT_OFF;
		return;
	}


	glPushAttrib(GL_VIEWPORT_BIT);
	int subwin_size = (int)((float)min(screenx(), screeny()) * 0.25f);
	int xoff = (screenx()-subwin_size) / 2;
	int yoff = (screeny()-subwin_size) / 2;
	glViewport(viewportx()+xoff, viewporty()+yoff, subwin_size, subwin_size);
	glScissor(viewportx()+xoff, viewporty()+yoff, subwin_size, subwin_size);
	glEnable(GL_SCISSOR_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-.03, .03, -.03, .03, .08, 20);
	glTranslatef(0, 0, -10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	{
		GLfloat mat_diffuse[] = { 0.5f, 0.5f, 0.5f, 0.5f };
		GLfloat mat_specular[] = { 0.5f, 0.5f, 0.5f, 0.5f };
		GLfloat mat_shininess[] = { SHININESS };
		GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 0.2f };
		GLfloat light_diffuse[] = { 0.5f, 0.5f, 0.5f, 0.5f };
		GLfloat light_specular[] = { 0.5f, 0.5f, 0.5f, 0.5f };
		GLfloat light_position[] = { lightdir[0], lightdir[1], lightdir[2], 0 };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	}

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_COLOR_MATERIAL);

	GLUquadricObj *q = gluNewQuadric();
#ifdef MESA
	gluQuadricDrawStyle(q, (GLenum)GLU_FILL);
	gluQuadricNormals(q, (GLenum)GLU_SMOOTH);
#else
	gluQuadricDrawStyle(q, GLU_FILL);
	gluQuadricNormals(q, GLU_SMOOTH);
#endif
	
	gluSphere(q, 1.0f, 16, 8);
	if (lightdir[2] != 1.0f) {
		glRotatef(acos(lightdir[2])*180.0f/M_PI,
			  -lightdir[1], lightdir[0], 0);
	}
	glTranslatef(0, 0, 1.9f);

	{
		GLfloat mat_diffuse[] = { 0.5f, 1.0f, 0.5f, 1.0f };
		GLfloat light_position[] = { 0.0f, 0.0f, 1.0f, 0.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	}

	glRotatef(180, 1, 0, 0);
	gluCylinder(q, 0.3f, 0.0f, 0.8f, 8, 1);
	gluDisk(q, 0.0f, 0.3f, 8, 1);
	glRotatef(180, 1, 0, 0);
	glTranslatef(0, 0, 1.7f);

	{
		GLfloat mat_diffuse[] = { 0.5f, 0.5f, 1.0f, 1 };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);
	}

	glRotatef(180, 1, 0, 0);
	gluCylinder(q, 0.1f, 0, 2.5f, 8, 1);

	gluDeleteQuadric(q);
	glPopAttrib();
}


// Draw an indication of how much we've refined
void QSplatGUI::draw_progressbar()
{
    if (showprogressbar != PROGRESS_ON) {
        return;
    }

        glPushAttrib(GL_TRANSFORM_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(0, 0, 0, 0.3f);
	glBegin(GL_QUADS);
		glVertex2f(-1.0f,   -1.0f);
		glVertex2f(-0.695f, -1.0f);
		glVertex2f(-0.695f, -0.965f);
		glVertex2f(-1.0f,   -0.965f);
	glEnd();
	glDisable(GL_BLEND);
	glColor3f(1, 1, 1);
	glBegin(GL_LINE_LOOP);
		glVertex2f(-0.995f, -0.995f);
		glVertex2f(-0.7f,   -0.995f);
		glVertex2f(-0.7f,   -0.97f);
		glVertex2f(-0.995f, -0.97f);
	glEnd();
	float refinement =  1.0f / theQSplat_Model->minsize;
	glBegin(GL_QUADS);
		glVertex2f(-0.995f,                   -0.995f);
		glVertex2f(-0.995f+0.295f*refinement, -0.995f);
		glVertex2f(-0.995f+0.295f*refinement, -0.97f);
		glVertex2f(-0.995f,                   -0.97f);
	glEnd();
	glPopAttrib();
	get_timestamp(last_progress_time);
}


// Process a mouse event
void QSplatGUI::mouse(int x, int y, mousebutton this_button)
{
	if (!theQSplat_Model)
		return;

	// We are not interested in mouse motion with no buttons pressed.
	if ((this_button == NO_BUTTON) && (last_button == NO_BUTTON))
		return;

	float this_mousex, this_mousey;
	mouse_i2f(x, y, &this_mousex, &this_mousey);


	// We are not interested in small mouse motions (deadzone) or duplicate events
	float dx = this_mousex - last_mousex;
	float dy = this_mousey - last_mousey;
	if (dx*dx + dy*dy < MOUSE_DEADZONE*MOUSE_DEADZONE &&
	    this_button != UP_WHEEL && this_button != DOWN_WHEEL &&
	    this_button == last_button)
		return;

	// Handle rotation
	if ((this_button == ROT_BUTTON) && (last_button != ROT_BUTTON)) {
		spinspeed = 0;
	}
	if ((this_button == ROT_BUTTON) && (last_button == ROT_BUTTON)) {
		rotate(last_mousex, last_mousey, this_mousex, this_mousey);
		updatestatus("Rotating");
	}
	if ((this_button == NO_BUTTON)  && (last_button == ROT_BUTTON)) {
		startspin();
	}

	// Handle translation
	if ((this_button == TRANSXY_BUTTON) && (last_button == TRANSXY_BUTTON)) {
		move(this_mousex - last_mousex, this_mousey - last_mousey, 0);
		updatestatus("Panning");
	}
	if ((this_button == TRANSZ_BUTTON) && (last_button == TRANSZ_BUTTON)) {
		move(0, 0, this_mousey - last_mousey);
		updatestatus("Zooming");
	}
	if (this_button == UP_WHEEL) {
		move(0, 0, WHEEL_MOVE);
		updatestatus("Zooming");
	}
	if (this_button == DOWN_WHEEL) {
		move(0, 0, -WHEEL_MOVE);
		updatestatus("Zooming");
	}

	// Handle lighting
	if ((this_button == LIGHT_BUTTON) && (last_button == LIGHT_BUTTON)) {
		relight(this_mousex, this_mousey);
		updatestatus("Relighting");
	}

	// Various other housekeeping chores
	if ((last_button == NO_BUTTON) || (this_button == last_button)) {
		// Button press or drag
		get_timestamp(last_event_time);
                dospin = false;
		stop_refine();
	} else if (this_button == NO_BUTTON) {
		// Button release
		update_depth(!dospin);
		if (!dospin &&
		    last_button != UP_WHEEL && last_button != DOWN_WHEEL) {
			theQSplat_Model->start_refine();
			dorefine = true;
		}
	}

	last_mousex = this_mousex;
	last_mousey = this_mousey;
	last_button = this_button;
}


// Our idle loop.  Handles auto-spin and auto-refine.
// Returns true iff we have refined all the way and are not spinning
bool QSplatGUI::idle()
{
	if (!theQSplat_Model) {
		updatestatus("No model");
		updaterate("");
		return true;
	}

	// Measure elapsed time since the last mouse event or idle loop
	timestamp new_time;
	get_timestamp(new_time);
	float dt = new_time - last_event_time;

	if ((showlight == SHOWLIGHT_ON) &&
	    (new_time - last_relight_time > SHOWLIGHT_TIME)) {
		showlight = SHOWLIGHT_OFF;
		need_redraw();
	}
	if ((showprogressbar == PROGRESS_ON) &&
	    (new_time - last_progress_time > SHOWPROG_TIME)) {
		showprogressbar = PROGRESS_OFF;
		need_redraw();
	}


	if (dospin) {
		float q[4];
		RotAndAxis2Q(spinspeed*dt, spinaxis, q);
		if (rot_depth == 0) { 
			q[1] = -q[1];
			q[2] = -q[2];
		}
		thecamera.Rotate(q, rot_depth);
		need_redraw();
		last_event_time = new_time;
		updatestatus("Spinning");
		return false;
	}

	if (dorefine && !theQSplat_Model->can_refine()) {
		updatestatus("Done refining");
		if (showlight == SHOWLIGHT_ON || showprogressbar == PROGRESS_ON) {
			usleep(10000);
			return false;
		} else {
			return true;
		}
	}

	if (!dorefine) {
		if (dt < REFINE_DELAY)
			return false;
		theQSplat_Model->start_refine();
		dorefine = true;
	}

	theQSplat_Model->refine();
	if (showprogressbar == PROGRESS_OFF)
		showprogressbar = PROGRESS_ON;
	need_redraw();
	updatestatus("Refining");
	return false;
}


// Reset viewer to default parameters
void QSplatGUI::resetviewer(bool splatsize_only /*= false */)
{
	if (!theQSplat_Model)
		return;

	theQSplat_Model->reset_rate();
	theQSplat_Model->start_refine();
	dorefine = true;
	dospin = false;

	if (splatsize_only)
		return;

        thecamera.GoHome();

	rot_depth = surface_depth = Dist(thecamera.pos, theQSplat_Model->center);

	lightdir[0] = 0;  lightdir[1] = 0;  lightdir[2] = 1;

	update_depth(true);
}

// Set whether surface material should have a specular component
void QSplatGUI::set_shiny(bool is_shiny)
{
	if (is_shiny)
		specular = SPECULAR;
	else
		specular = 0;

	stop_refine();
}


// Should we do backface culling?
void QSplatGUI::set_backfacecull(bool do_cull)
{
	backfacecull = do_cull;
	stop_refine();
}



// User has moved the "desired rate" slider
void QSplatGUI::set_desiredrate(float rate)
{
	float rate_ratio = rate / desiredrate;
	desiredrate = rate;
	if (!theQSplat_Model)
		return;
	stop_refine();
	theQSplat_Model->adjust_rate(rate_ratio);
}


// Switch into or out of tourist (restricted functionality) mode.
void QSplatGUI::set_touristmode(bool i_am_confused)
{
	if (i_am_confused) {
		touristmode = true;
		dospin = false;
	} else {
		touristmode = false;
	}
	stop_refine();
}


// Should we display an indication of the light source direction?
void QSplatGUI::set_showlight(bool do_light)
{
	if (do_light)
		showlight = SHOWLIGHT_ON;
	else
		showlight = SHOWLIGHT_NEVER;
	get_timestamp(last_relight_time);
}


// Should we display a graphical indication of refinement?
void QSplatGUI::set_showprogressbar(bool do_prog)
{
	if (do_prog)
		showprogressbar = PROGRESS_ON;
	else
		showprogressbar = PROGRESS_NEVER;
	get_timestamp(last_progress_time);
}



// Set up a consistent set of OpenGL modes
void QSplatGUI::setupGLstate()
{
	glViewport(viewportx(), viewporty(), screenx(), screeny());

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_DITHER);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_FLAT);

	glClearColor(0, 0, 0, 0);
	glClearDepth(1);
}


// Set up projection and modelview matrices, and (optionally) lighting
const char* pointVertexShaderSource = R"(
#version 120
attribute vec3 aPos;
attribute vec3 aColor;
attribute vec3 aNormal;
varying vec3 vColor;
uniform mat4 projection;
uniform mat4 view;
void main() {
    vColor = aColor;
    gl_Position = projection * view * vec4(aPos, 1.0);
    gl_PointSize = 2.0; // Fixed size for now
}
)";

const char* pointFragmentShaderSource = R"(
#version 120
varying vec3 vColor;
void main() {
    gl_FragColor = vec4(vColor, 1.0);
}
)";

void QSplatGUI::init_modern_rendering()
{
    if (pointShader) return;
    pointShader = new Shader(pointVertexShaderSource, pointFragmentShaderSource);

    glGenVertexArraysAPPLE(1, &vao);
    glBindVertexArrayAPPLE(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLint posAttrib = glGetAttribLocation(pointShader->ID, "aPos");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(QSplatVertex), (void*)offsetof(QSplatVertex, pos));

    GLint colAttrib = glGetAttribLocation(pointShader->ID, "aColor");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(QSplatVertex), (void*)offsetof(QSplatVertex, color));

    GLint normAttrib = glGetAttribLocation(pointShader->ID, "aNormal");
    glEnableVertexAttribArray(normAttrib);
    glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(QSplatVertex), (void*)offsetof(QSplatVertex, normal));

    glBindVertexArrayAPPLE(0);
}

void QSplatGUI::flush_modern_rendering()
{
    if (vertexBuffer.empty()) return;

    init_modern_rendering();
    pointShader->use();

    // Get matrices from legacy OpenGL state (for now)
    float P[16], M[16];
    glGetFloatv(GL_PROJECTION_MATRIX, P);
    glGetFloatv(GL_MODELVIEW_MATRIX, M);
    
    glUniformMatrix4fv(glGetUniformLocation(pointShader->ID, "projection"), 1, GL_FALSE, P);
    glUniformMatrix4fv(glGetUniformLocation(pointShader->ID, "view"), 1, GL_FALSE, M);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(QSplatVertex), vertexBuffer.data(), GL_STREAM_DRAW);

    glBindVertexArrayAPPLE(vao);
    glDrawArrays(GL_POINTS, 0, vertexBuffer.size());
    glBindVertexArrayAPPLE(0);

    vertexBuffer.clear();
}

void QSplatGUI::setup_matrices(bool dolighting)
{
	// Reset everything to identity
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Set up light position before we load the modelview matrix
	if (dolighting) {
		GLfloat light0_position[] = { lightdir[0], lightdir[1], lightdir[2], 0 };
		glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	}

	// Set up projection and modelview matrices
	float near_depth = surface_depth;
	float min_near = 1e-6f * theQSplat_Model->radius;
	if (near_depth < min_near) near_depth = min_near;

	thecamera.SetGL(near_depth/DOF, near_depth*DOF,
			screenx(), screeny());

	// Set up lighting
	if (dolighting) {
		GLfloat mat_specular[4] = { specular, specular, specular, 0 };
		GLfloat mat_shininess[] = { 127 };
		GLfloat global_ambient[] = { AMBIENT, AMBIENT, AMBIENT, 1 };
		GLfloat light0_ambient[] = { 0, 0, 0, 1 };
		GLfloat light0_diffuse[] = { LIGHT, LIGHT, LIGHT, 1.0 };
		GLfloat light0_specular[] = { LIGHT, LIGHT, LIGHT, 1.0 };
		// light0_position is set above...

		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
		glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
	}

	// Backface culling
	if (backfacecull) {
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
}


// If we were auto-refining, stop it
void QSplatGUI::stop_refine()
{
	if (theQSplat_Model && dorefine)
		theQSplat_Model->stop_refine();
	dorefine = false;
	if (showprogressbar == PROGRESS_OFF)
		showprogressbar = PROGRESS_ON;
}


// Updates the distance from the camera center to the rotation center.
// (i.e. recomputes surface_depth and rot_depth)
void QSplatGUI::update_depth(bool update_rot_depth, float hint)
//   (  void QSplatGUI::update_depth(bool update_rot_depth=false, float hint= 0.0f)  )
{
	if (!theQSplat_Model)
		return;

	setup_matrices(false); // traceray reads camera params from OpenGL
	float depth = theQSplat_Model->traceray(screenx() / 2,
						screeny() / 2,
						RTSIZE);
	if (!depth && hint) {
		if (hint + surface_depth > 1.0e-5 * theQSplat_Model->radius)
			depth = surface_depth + hint;
	}

	if (!depth)
		return;

	surface_depth = depth;
	if (!update_rot_depth)
		return;

	// Rotate around the surface if we are close to it, else around
	// the center of the object.
	float dist2center = sqrtf(sqr(thecamera.pos[0]-theQSplat_Model->center[0]) +
				  sqr(thecamera.pos[1]-theQSplat_Model->center[1]) +
				  sqr(thecamera.pos[2]-theQSplat_Model->center[2]));

	rot_depth = min(depth*(1.0f+DEPTH_FUDGE*thecamera.fov), dist2center);

}


// Mouse event helper function - rotate
void QSplatGUI::rotate(float ox, float oy, float nx, float ny)
{
	// Random multiplicative factor - adjust to taste
	nx += (ROT_MULT-1)*(nx-ox);
	ny += (ROT_MULT-1)*(ny-oy);

	float q[4];
	Mouse2Q(ox, oy, nx, ny, q);

	if (rot_depth == 0) {
		q[1] = -q[1];
		q[2] = -q[2];
	}

	thecamera.Rotate(q, rot_depth);


	// Update autospin params
	float this_rot;
	Q2RotAndAxis(q, this_rot, spinaxis);

	timestamp new_time;
	get_timestamp(new_time);
	float dt = new_time - last_event_time;

	const float time_const = 0.05f + 1.0f / desiredrate;
	if (dt > time_const)
		spinspeed = this_rot / time_const;
	else
		// Exponential decay.  Math trick.  Think about it.
		spinspeed = (this_rot/time_const) +
				(1.0f-dt/time_const)*spinspeed;

	last_event_time = new_time;
	need_redraw();
}


// Mouse event helper function - user has let go of "rotate" mouse button. 
// Should we start auto-spin?
void QSplatGUI::startspin()
{
	// No spinning in tourist mode
	if (touristmode)
		return;

	timestamp new_time;
	get_timestamp(new_time);
	float dt = new_time - last_event_time;

	const float time_const = 0.05f + 1.0f / desiredrate;
        if ((dt < time_const) && (fabs(spinspeed) > 0.03f)) {
            dospin = true;
        }
}


// Mouse event helper function - translate
void QSplatGUI::move(float dx, float dy, float dz)
{
	float d_scale = surface_depth;
	float min_scale = 0.001f * theQSplat_Model->radius;
	if (d_scale < min_scale)
		d_scale = min_scale;

	float scalefactor = -0.5f*d_scale * TRANSXY_MULT;
	dx *= scalefactor*thecamera.fov;
	dy *= scalefactor*thecamera.fov;
	dz = d_scale * (exp(-0.2f * TRANSZ_MULT * dz) - 1.0f);

	thecamera.Move(dx, dy, dz);

	update_depth(true, dz);
	need_redraw();
}


// Mouse event helper function - change direction of light
void QSplatGUI::relight(float x, float y)
{
	float theta = M_PI*sqrtf(x*x+y*y);
	if (theta > float(M_PI))
		theta = float(M_PI);
	float phi = atan2(y, x);


	if (touristmode)
		// Too easy to shoot yourself in the foot with backlighting
		theta *= 0.4f;

	lightdir[0] = sin(theta)*cos(phi);
	lightdir[1] = sin(theta)*sin(phi);
	lightdir[2] = cos(theta);

	get_timestamp(last_relight_time);
	if (showlight == SHOWLIGHT_OFF)
		showlight = SHOWLIGHT_ON;

	need_redraw();
}


// Helper function - convert integer coordinates of mouse position to
// normalized floats
void QSplatGUI::mouse_i2f(int x, int y, float *xf, float *yf)
{
	int screenwidth = screenx();
	int screenheight = screeny();

	float r = (float)screenwidth/screenheight;
	if (r > 1.0) {
		*xf = r*(2.0f*x/screenwidth-1.0f);
		*yf = 1.0f - 2.0f*y/screenheight;
	} else {
		*xf = 2.0f*x/screenwidth-1.0f;
		*yf = (1.0f - 2.0f*y/screenheight)/r;
	}
}


// Various splat rasterizers...
extern void start_drawing_gl(bool,int,bool);
extern void drawpoint_gl(float,float,float,float,float,const float *,const float *);
extern void end_drawing_gl();

extern void start_drawing_gl_ellip(bool,bool);
extern void drawpoint_gl_ellip(float,float,float,float,float,const float *,const float *);
extern void end_drawing_gl_ellip();

extern void start_drawing_spheres(bool);
extern void drawpoint_spheres(float,float,float,float,float,const float *,const float *);
extern void end_drawing_spheres();

extern void drawpoint_software(float,float,float,float,float,const float *,const float *);
extern void start_drawing_software(bool);
extern void end_drawing_software(bool);

extern void drawpoint_software_tiles(float,float,float,float,float,const float*,const float*);
extern void start_drawing_software_tiles(bool);
extern void end_drawing_software_tiles(bool);

extern void drawpoint_gaussian(float,float,float,float,float,const float*,const float*);
extern void start_drawing_gaussian(bool);
extern void second_pass_gaussian(bool);
extern void end_drawing_gaussian();

void dont_draw(float,float,float,float,float,const float *,const float*) {}

#define SOFTWARE_TILES_CROSSOVER 7

// Set up the splat renderer
void QSplatGUI::start_drawing(bool havecolor)
{

    float ps[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, ps);

    switch (whichDriver) {
        case OPENGL_POINTS:
        case OPENGL_POINTS_CIRC:
            drawpoint = drawpoint_gl;
            start_drawing_gl(havecolor, int(ps[1]), whichDriver == OPENGL_POINTS_CIRC);
            break;
        case OPENGL_QUADS:
        case OPENGL_POLYS_CIRC:
            drawpoint = drawpoint_gl;
            start_drawing_gl(havecolor, 0, whichDriver == OPENGL_POLYS_CIRC);
            break;
        case OPENGL_POLYS_ELLIP:
        case OPENGL_POLYS_ELLIP_SMALL:
            drawpoint = drawpoint_gl_ellip;
            start_drawing_gl_ellip(havecolor, whichDriver == OPENGL_POLYS_ELLIP_SMALL);
            break;
        case OPENGL_SPHERES:
            drawpoint = drawpoint_spheres;
            start_drawing_spheres(havecolor);
            break;

        case OPENGL_GAUSSIAN_ELLIP:
            drawpoint = drawpoint_gaussian;
            start_drawing_gaussian(havecolor);
            break;

        case SOFTWARE_GLDRAWPIXELS:
        case SOFTWARE:
            drawpoint = drawpoint_software;
            start_drawing_software(whichDriver == SOFTWARE_GLDRAWPIXELS);
            break;
        case SOFTWARE_TILES_GLDRAWPIXELS:
        case SOFTWARE_TILES:
            drawpoint = drawpoint_software_tiles;
            start_drawing_software_tiles(whichDriver == SOFTWARE_TILES_GLDRAWPIXELS);
            break;
        case SOFTWARE_BEST_GLDRAWPIXELS:
        case SOFTWARE_BEST:
            if (theQSplat_Model->minsize < SOFTWARE_TILES_CROSSOVER) {
                drawpoint = drawpoint_software;
                start_drawing_software(whichDriver == SOFTWARE_BEST_GLDRAWPIXELS);
            } else {
                drawpoint = drawpoint_software_tiles;
                start_drawing_software_tiles(whichDriver == SOFTWARE_BEST_GLDRAWPIXELS);
            }
            break;
        case DONT_DRAW:
            drawpoint = dont_draw;
            break;
        default:
            drawpoint = drawpoint_gl;
            start_drawing_gl(havecolor, 0, false);
            break;
    }
}


// Prepare for the second pass of rendering.  Returns true iff a second pass
// is necessary.
bool QSplatGUI::second_pass(bool havecolor)
{
	switch (whichDriver) {
	case OPENGL_GAUSSIAN_ELLIP:
		second_pass_gaussian(havecolor);
		return true;
        default:
            return false;
	}
	return false;
}


// Wrap up the rendering
void QSplatGUI::end_drawing(bool bailed)
{
    flush_modern_rendering();

	switch (whichDriver) {
	case OPENGL_POINTS:
	case OPENGL_POINTS_CIRC:
	case OPENGL_QUADS:
	case OPENGL_POLYS_CIRC:
		end_drawing_gl();
		break;
	case OPENGL_POLYS_ELLIP:
	case OPENGL_POLYS_ELLIP_SMALL:
		end_drawing_gl_ellip();
		break;
	case OPENGL_SPHERES:
		end_drawing_spheres();
		break;

	case OPENGL_GAUSSIAN_ELLIP:
		end_drawing_gaussian();
		break;

	case SOFTWARE_GLDRAWPIXELS:
	case SOFTWARE:
		end_drawing_software(bailed);
		break;
	case SOFTWARE_TILES_GLDRAWPIXELS:
	case SOFTWARE_TILES:
		end_drawing_software_tiles(bailed);
		break;
	case SOFTWARE_BEST_GLDRAWPIXELS:
	case SOFTWARE_BEST:
		if (theQSplat_Model->minsize < SOFTWARE_TILES_CROSSOVER)
			end_drawing_software(bailed);
		else
			end_drawing_software_tiles(bailed);
		break;
	case DONT_DRAW:
		break;
	default:
		end_drawing_gl();
	}
}

