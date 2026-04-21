#ifndef QSPLAT_GUIMAIN_H
#define QSPLAT_GUIMAIN_H
/*
Szymon Rusinkiewicz

qsplat_gui.h
This is the generic interface to the QSplat GUI.  A given system must define
a subclass of this...

Copyright (c) 1999-2000 The Board of Trustees of the
Leland Stanford Junior University.  All Rights Reserved.
*/

#include "qsplat_util.h"
#include "qsplat_gui_camera.h"
#include "qsplat_model.h"


// Types
typedef void (*splatter)(float cx, float cy, float cz,
			 float r, float splatsize,
			 const float *norm, const float *col);
enum Driver { OPENGL_POINTS, OPENGL_POINTS_CIRC,
	      OPENGL_QUADS, OPENGL_POLYS_CIRC, OPENGL_POLYS_ELLIP,
	      OPENGL_POLYS_ELLIP_SMALL, OPENGL_SPHERES,
	      OPENGL_GAUSSIAN_ELLIP,
	      SOFTWARE_GLDRAWPIXELS, SOFTWARE,
	      SOFTWARE_TILES_GLDRAWPIXELS, SOFTWARE_TILES,
	      SOFTWARE_BEST_GLDRAWPIXELS, SOFTWARE_BEST,
	      DONT_DRAW };


#include "qsplat_shader.h"
#include <vector>
#include <string>

struct QSplatVertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
};


// The GUI class.  System-specific GUIs are derived from this
class QSplatGUI {
public:
	enum mousebutton { NO_BUTTON, ROT_BUTTON, TRANSXY_BUTTON, TRANSZ_BUTTON,
			   LIGHT_BUTTON, UP_WHEEL, DOWN_WHEEL };

    virtual ~QSplatGUI();

    // Modern rendering members
    std::vector<QSplatVertex> vertexBuffer;
    Shader* pointShader = nullptr;
    GLuint vao = 0, vbo = 0;
    void init_modern_rendering();
    void flush_modern_rendering();

private:
	// Parameters the drawing routine cares about
	Camera thecamera;
	vec lightdir;
	float specular;
	bool backfacecull;

	// Utility functions
        void setupGLstate();
	void setup_matrices(bool dolighting);
	void stop_refine();
	void update_depth(bool update_rot_depth=false, float hint=0.0f);
	void rotate(float, float, float, float);
	void startspin();
	void move(float, float, float);
	void relight(float, float);
	void mouse_i2f(int, int, float *, float *);
	void draw_light();
	void draw_progressbar();

	// GUI modes and internal state
	bool dorefine;
	float desiredrate;
	bool touristmode;
	enum { SHOWLIGHT_OFF, SHOWLIGHT_ON, SHOWLIGHT_NEVER } showlight;
	enum { PROGRESS_OFF, PROGRESS_ON, PROGRESS_NEVER } showprogressbar;
	float last_mousex, last_mousey;
	timestamp last_event_time;
	timestamp last_relight_time;
	timestamp last_progress_time;

	// State related to rotating and spinning
	float rot_depth;
	float surface_depth;
	bool dospin;
	float spinspeed;
	vec spinaxis;


protected:
	mousebutton last_button;
	// The QSplat structure
	QSplat_Model *theQSplat_Model;


public:
	// These are things that system-specific subclasses must implement //

	// Enter the main event loop
	virtual void Go() = 0;

	// Return size of screen and origin of viewport
	virtual int screenx() = 0;    virtual int screeny() = 0;
	virtual int viewportx() = 0;  virtual int viewporty() = 0;

        // Go into fullscreen mode
        virtual void fullscreen(bool go_fullscreen) = 0;
        
	// Called to indicate that a redraw needs to be performed
	virtual void need_redraw() = 0;

	// Swap buffers
	virtual void swapbuffers() = 0;

	// Update status bar
	virtual void updatestatus(const char *text) = 0;

	// Update rate bar
	virtual void updaterate(const char *text) = 0;

        // Update the menu bar
        virtual void updatemenus() = 0;

	// Display information about model
	virtual void aboutmodel() = 0;

        // Display information about QSplat
        virtual void aboutqsplat() = 0;
        
        // Display an error dialog in an appropriate fashion
        virtual void display_dialog(const char *title, const char *message) = 0;

        
	// Callbacks and event handlers to be used by the subclasses //
        void setmodel(QSplat_Model *q);
        void redraw();

	void mouse(int, int, mousebutton);
	bool idle();

	void resetviewer(bool splatsize_only = false);

        void set_shiny(bool);
	void set_backfacecull(bool);
	void set_desiredrate(float);
	void set_touristmode(bool);
	void set_showlight(bool);
	void set_showprogressbar(bool);
        

	// Initialize
	QSplatGUI() : pointShader(nullptr), vao(0), vbo(0), last_mousex(-1), last_mousey(-1),
		      last_button(NO_BUTTON), theQSplat_Model(NULL)
	{
                set_shiny(true);
		set_backfacecull(true);
		set_touristmode(false);
		set_showlight(false);
		set_showprogressbar(true);
	}

        // Open and display a model
        void OpenModel(const char *filename);
        std::string AutoMakeQS(const char *filename);

	// Query whether we are dragging the object
	bool dragging()
	{
		return (last_button != NO_BUTTON);
	}

	// Query the desired frame rate
	float rate()
	{
		return desiredrate;
	}

	// Number of points splatted
	int pts_splatted;

	// The splat rasterizer
	Driver whichDriver;
	virtual void start_drawing(bool havecolor);
	splatter drawpoint;
	virtual bool abort_drawing(float time_elapsed) = 0;
	virtual bool second_pass(bool havecolor);
	virtual void end_drawing(bool bailed);
};

extern QSplatGUI *theQSplatGUI;
#endif
