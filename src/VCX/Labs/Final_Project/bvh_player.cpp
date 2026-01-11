#ifdef  WIN32
	#include <windows.h>
#endif

#include <GL/glut.h>

#include <stdio.h>

#include "BVH.h"


//
//  Global variables for camera and GLUT input processing
//

// Variables for camera rotation
static float   camera_yaw = 0.0f;      // Rotation angle around the Y-axis
static float   camera_pitch = -20.0f;  // Rotation angle around the X-axis
static float   camera_distance = 5.0f; // Distance from the center to the camera

// Variables for mouse dragging
static int     drag_mouse_r = 0; // Flag for right button dragging (1: dragging, 0: not dragging)
static int     drag_mouse_l = 0; // Flag for left button dragging (1: dragging, 0: not dragging)
static int     drag_mouse_m = 0; // Flag for middle button dragging (1: dragging, 0: not dragging)
static int     last_mouse_x, last_mouse_y; // Last recorded mouse cursor coordinates

// Window size
static int     win_width, win_height;


//
//  Global variables related to animation
//

// Flag indicating whether animation is in progress
bool   on_animation = true;

// Animation playback time
float  animation_time = 0.0f;

// Current display frame number
int    frame_no = 0;

// BVH motion data
BVH *   bvh = NULL;



//
//  Draw text
//
void  drawMessage( int line_no, const char * message )
{
	int   i;
	if ( message == NULL )
		return;

	// Initialize projection matrix (save current matrix before initialization)
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D( 0.0, win_width, win_height, 0.0 );

	// Initialize modelview matrix (save current matrix before initialization)
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	// Turn off Z-buffer and lighting
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );

	// Draw message
	glColor3f( 1.0, 0.0, 0.0 );
	glRasterPos2i( 8, 24 + 18 * line_no );
	for ( i=0; message[i]!='\0'; i++ )
		glutBitmapCharacter( GLUT_BITMAP_HELVETICA_18, message[i] );

	// Restore all settings
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_LIGHTING );
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}


//
//  Callback function called when the window is redrawn
//
void  display( void )
{
	// Clear the screen
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	// Set transformation matrix (model coordinate system → camera coordinate system)
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glTranslatef( 0.0, 0.0, - camera_distance );
	glRotatef( - camera_pitch, 1.0, 0.0, 0.0 );
	glRotatef( - camera_yaw, 0.0, 1.0, 0.0 );
	glTranslatef( 0.0, -0.5, 0.0 );

	// Reset light source position
	float  light0_position[] = { 10.0, 10.0, 10.0, 1.0 };
	glLightfv( GL_LIGHT0, GL_POSITION, light0_position );

	// Draw ground
	float  size = 1.5f;
	int  num_x = 10, num_z = 10;
	double  ox, oz;
	glBegin( GL_QUADS );
		glNormal3d( 0.0, 1.0, 0.0 );
		ox = -(num_x * size) / 2;
		for ( int x=0; x<num_x; x++, ox+=size )
		{
			oz = -(num_z * size) / 2;
			for ( int z=0; z<num_z; z++, oz+=size )
			{
				if ( ((x + z) % 2) == 0 )
					glColor3f( 1.0, 1.0, 1.0 );
				else
					glColor3f( 0.8, 0.8, 0.8 );
				glVertex3d( ox, 0.0, oz );
				glVertex3d( ox, 0.0, oz+size );
				glVertex3d( ox+size, 0.0, oz+size );
				glVertex3d( ox+size, 0.0, oz );
			}
		}
	glEnd();

	// Draw character
	glColor3f( 1.0f, 0.0f, 0.0f );
	if ( bvh )
		bvh->RenderFigure( frame_no, 0.02f );

	// Display time and frame number
	char  message[ 64 ];
	if ( bvh )
		sprintf( message, "%.2f (%d)", animation_time, frame_no );
	else
		sprintf( message, "Press 'L' key to Load a BVH file" );
	drawMessage( 0, message );

	// Display the screen drawn in the back buffer to the front buffer
	glutSwapBuffers();
}


//
//  Callback function called when the window size is changed
//
void  reshape( int w, int h )
{
	// Set the drawing range within the window (draw to the entire window here)
	glViewport(0, 0, w, h);
	
	// Set transformation matrix (camera coordinate system → screen coordinate system)
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 45, (double)w/h, 1, 500 );

	// Record window size (for text drawing processing)
	win_width = w;
	win_height = h;
}


//
//  Callback function called when a mouse button is clicked
//
void  mouse( int button, int state, int mx, int my )
{
	// Start dragging when left button is pressed
	if ( ( button == GLUT_LEFT_BUTTON ) && ( state == GLUT_DOWN ) )
		drag_mouse_l = 1;
	// End dragging when left button is released
	else if ( ( button == GLUT_LEFT_BUTTON ) && ( state == GLUT_UP ) )
		drag_mouse_l = 0;

	// Start dragging when right button is pressed
	if ( ( button == GLUT_RIGHT_BUTTON ) && ( state == GLUT_DOWN ) )
		drag_mouse_r = 1;
	// End dragging when right button is released
	else if ( ( button == GLUT_RIGHT_BUTTON ) && ( state == GLUT_UP ) )
		drag_mouse_r = 0;

	// Start dragging when middle button is pressed
	if ( ( button == GLUT_MIDDLE_BUTTON ) && ( state == GLUT_DOWN ) )
		drag_mouse_m = 1;
	// End dragging when middle button is released
	else if ( ( button == GLUT_MIDDLE_BUTTON ) && ( state == GLUT_UP ) )
		drag_mouse_m = 0;

	// Redraw
	glutPostRedisplay();

	// Record current mouse coordinates
	last_mouse_x = mx;
	last_mouse_y = my;
}


//
//  Callback function called during mouse dragging
//
void  motion( int mx, int my )
{
	// Rotate viewpoint during right button dragging
	if ( drag_mouse_r )
	{
		// Rotate viewpoint according to the difference between previous and current mouse coordinates

		// Rotate around Y-axis according to horizontal mouse movement
		camera_yaw -= ( mx - last_mouse_x ) * 1.0;
		if ( camera_yaw < 0.0 )
			camera_yaw += 360.0;
		else if ( camera_yaw > 360.0 )
			camera_yaw -= 360.0;

		// Rotate around X-axis according to vertical mouse movement
		camera_pitch -= ( my - last_mouse_y ) * 1.0;
		if ( camera_pitch < -90.0 )
			camera_pitch = -90.0;
		else if ( camera_pitch > 90.0 )
			camera_pitch = 90.0;
	}
	// Change distance between viewpoint and camera during left button dragging
	if ( drag_mouse_l )
	{
		// Adjust distance according to vertical mouse movement
		camera_distance += ( my - last_mouse_y ) * 0.2;
		if ( camera_distance < 2.0 )
			camera_distance = 2.0;
	}

	// Record current mouse coordinates
	last_mouse_x = mx;
	last_mouse_y = my;

	// Redraw
	glutPostRedisplay();
}


//
//  Callback function called when a keyboard key is pressed
//
void  keyboard( unsigned char key, int mx, int my )
{
	// Press 's' key to pause/resume animation
	if ( key == 's' )
		on_animation = !on_animation;

	// Press 'n' key to go to next frame
	if ( ( key == 'n' ) && !on_animation )
	{
		animation_time += bvh->GetInterval();
		frame_no ++;
		frame_no = frame_no % bvh->GetNumFrame();
	}

	// Press 'p' key to go to previous frame
	if ( ( key == 'p' ) && !on_animation && ( frame_no > 0 ) && bvh )
	{
		animation_time -= bvh->GetInterval();
		frame_no --;
		frame_no = frame_no % bvh->GetNumFrame();
	}

	// Press 'r' key to reset animation
	if ( key == 'r' )
	{
		animation_time = 0.0f;
		frame_no = 0;
	}

	// Press 'l' key to change playback motion
	if ( key == 'l' )
	{
#ifdef  WIN32
		const int  file_name_len = 256;
		char  file_name[ file_name_len ] = "";

		// Set up file dialog
		OPENFILENAME	open_file;
		memset( &open_file, 0, sizeof(OPENFILENAME) );
		open_file.lStructSize = sizeof(OPENFILENAME);
		open_file.hwndOwner = NULL;
		open_file.lpstrFilter = "BVH Motion Data (*.bvh)\0*.bvh\0All (*.*)\0*.*\0";
		open_file.nFilterIndex = 1;
		open_file.lpstrFile = file_name;
		open_file.nMaxFile = file_name_len;
		open_file.lpstrTitle = "Select a BVH file";
		open_file.lpstrDefExt = "bvh";
		open_file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		// Display file dialog
		BOOL  ret = GetOpenFileName( &open_file );

		// Set new motion if file is specified
		if( ret )
		{
			// Load motion data
			if ( bvh )
				delete  bvh;
			bvh = new BVH( file_name );

			// Delete if loading failed
			if ( !bvh->IsLoadSuccess() )
			{
				delete  bvh;
				bvh = NULL;
			}

			// Reset animation
			animation_time = 0.0f;
			frame_no = 0;
		}
#endif
	}

	glutPostRedisplay();
}


//
//  Callback function called during idle time
//
void  idle( void )
{
	// Animation processing
	if ( on_animation )
	{
#ifdef  WIN32
		// Get system time and determine Δt according to elapsed time from last time
		static DWORD  last_time = 0;
		DWORD  curr_time = timeGetTime();
		float  delta = ( curr_time - last_time ) * 0.001f;
		if ( delta > 0.03f )
			delta = 0.03f;
		last_time = curr_time;
		animation_time += delta;
#else
		// Use fixed Δt
		animation_time += 0.03f;
#endif
		// Calculate current frame number
		if ( bvh )
		{
			frame_no = animation_time / bvh->GetInterval();
			frame_no = frame_no % bvh->GetNumFrame();
		}
		else
			frame_no = 0;

		// Issue redraw instruction (redraw callback function will be called after this)
		glutPostRedisplay();
	}
}


//
//  Environment initialization function
//
void  initEnvironment( void )
{
	// Create light source
	float  light0_position[] = { 10.0, 10.0, 10.0, 1.0 };
	float  light0_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
	float  light0_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	float  light0_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
	glLightfv( GL_LIGHT0, GL_POSITION, light0_position );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, light0_diffuse );
	glLightfv( GL_LIGHT0, GL_SPECULAR, light0_specular );
	glLightfv( GL_LIGHT0, GL_AMBIENT, light0_ambient );
	glEnable( GL_LIGHT0 );

	// Enable light source calculation
	glEnable( GL_LIGHTING );

	// Enable object color information
	glEnable( GL_COLOR_MATERIAL );

	// Enable Z-test
	glEnable( GL_DEPTH_TEST );

	// Enable back face culling
	glCullFace( GL_BACK );
	glEnable( GL_CULL_FACE );

	// Set background color
	glClearColor( 0.5, 0.5, 0.8, 0.0 );

	// Load initial BVH motion data
//	bvh = new BVH( "???.bvh" );
}


//
//  Main function (program starts here)
//
int  main( int argc, char ** argv )
{
	// Initialize GLUT
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL );
	glutInitWindowSize( 640, 640 );
	glutInitWindowPosition( 0, 0 );
	glutCreateWindow("BVH Player Sample");
	
	// Register callback functions
	glutDisplayFunc( display );
	glutReshapeFunc( reshape );
	glutMouseFunc( mouse );
	glutMotionFunc( motion );
	glutKeyboardFunc( keyboard );
	glutIdleFunc( idle );

	// Initialize environment
	initEnvironment();

	// Transfer processing to GLUT's main loop
	glutMainLoop();
	return 0;
}