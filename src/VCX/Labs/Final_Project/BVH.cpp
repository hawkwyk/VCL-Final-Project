#include <fstream>
#include <string.h>

#include "BVH.h"


// Constructor
BVH::BVH()
{
	motion = NULL;
	Clear();
}

// Constructor
BVH::BVH( const char * bvh_file_name )
{
	motion = NULL;
	Clear();

	Load( bvh_file_name );
}

// Destructor
BVH::~BVH()
{
	Clear();
}


// Clear all information
void  BVH::Clear()
{
	int  i;
	for ( i=0; i<channels.size(); i++ )
		delete  channels[ i ];
	for ( i=0; i<joints.size(); i++ )
		delete  joints[ i ];
	if ( motion != NULL )
		delete  motion;

	is_load_success = false;
	
	file_name = "";
	motion_name = "";
	
	num_channel = 0;
	channels.clear();
	joints.clear();
	joint_index.clear();
	
	num_frame = 0;
	interval = 0.0;
	motion = NULL;
}


// Set all information
void  BVH::Init( const char * name, 
	int n_joi, const Joint ** a_joi, int n_chan, const Channel ** a_chan, 
	int n_frame, double inter, const double * mo )
{
	// Set joint hierarchy structure
	SetSkeleton( name, n_joi, a_joi, n_chan, a_chan );

	// Set motion data
	SetMotion( n_frame, inter, mo );
}


// Set joint hierarchy structure
void  BVH::SetSkeleton( const char * name, 
	int n_joi, const Joint ** a_joi, int n_chan, const Channel ** a_chan )
{
	int  i, j;

	// Initialization
	Clear();

	// Set basic information
	file_name = "";
	if ( name )
		motion_name = name;
	num_channel = n_chan;

	// Initialize channel/joint arrays
	channels.resize( num_channel );
	for ( i=0; i<num_channel; i++ )
		channels[ i ] = new Channel();
	joints.resize( n_joi );
	for ( i=0; i<n_joi; i++ )
		joints[ i ] = new Joint();

	// Copy channel/joint information
	for ( i=0; i<num_channel; i++ )
	{
		*channels[ i ] = *a_chan[ i ];
		channels[ i ]->joint = joints[ a_chan[ i ]->joint->index ];
	}
	for ( i=0; i<n_joi; i++ )
	{
		*joints[ i ] = *a_joi[ i ];
		for ( j=0; j<joints[ i ]->channels.size(); j++ )
			joints[ i ]->channels[ j ] = channels[ a_joi[ i ]->channels[ j ]->index ];

		if ( a_joi[ i ]->parent == NULL )
			joints[ i ]->parent = NULL;
		else
			joints[ i ]->parent = joints[ a_joi[ i ]->parent->index ];

		for ( j=0; j<joints[ i ]->children.size(); j++ )
			joints[ i ]->children[ j ] = joints[ a_joi[ i ]->children[ j ]->index ];

		if ( joints[ i ]->name.size() > 0 )
			joint_index[ joints[ i ]->name ] = joints[ i ];
	}
}


// Set motion data
void  BVH::SetMotion( int n_frame, double inter, const double * mo )
{
	num_frame = n_frame;
	interval = inter;
	motion = new double[ num_frame * num_channel ];
	if ( mo != NULL )
		memcpy( motion, mo, sizeof( double ) * num_frame * num_channel );
}


//
//  Load BVH file
//
void  BVH::Load( const char * bvh_file_name )
{
	#define  BUFFER_LENGTH  1024*4

	ifstream  file;
	char      line[ BUFFER_LENGTH ];
	char *    token;
	char      separater[] = " :,\t";
	vector< Joint * >   joint_stack;
	Joint *   joint = NULL;
	Joint *   new_joint = NULL;
	bool      is_site = false;
	double    x, y ,z;
	int       i, j;

	// Initialization
	Clear();

	// Set file information (file name/motion name)
	file_name = bvh_file_name;
	const char *  mn_first = bvh_file_name;
	const char *  mn_last = bvh_file_name + strlen( bvh_file_name );
	if ( strrchr( bvh_file_name, '\\' ) != NULL )
		mn_first = strrchr( bvh_file_name, '\\' ) + 1;
	else if ( strrchr( bvh_file_name, '/' ) != NULL )
		mn_first = strrchr( bvh_file_name, '/' ) + 1;
	if ( strrchr( bvh_file_name, '.' ) != NULL )
		mn_last = strrchr( bvh_file_name, '.' );
	if ( mn_last < mn_first )
		mn_last = bvh_file_name + strlen( bvh_file_name );
	motion_name.assign( mn_first, mn_last );

	// Open file
	file.open( bvh_file_name, ios::in );
	if ( file.is_open() == 0 )  return; // Exit if file cannot be opened

	// Read hierarchy information
	while ( ! file.eof() )
	{
		// Abnormal termination if end of file is reached
		if ( file.eof() )  goto bvh_error;

		// Read one line and get first word
		file.getline( line, BUFFER_LENGTH );
		token = strtok( line, separater );

		// Skip empty lines
		if ( token == NULL )  continue;

		// Start of joint block
		if ( strcmp( token, "{" ) == 0 )
		{
			// Push current joint to stack
			joint_stack.push_back( joint );
			joint = new_joint;
			continue;
		}
		// End of joint block
		if ( strcmp( token, "}" ) == 0 )
		{
			// Pop current joint from stack
			joint = joint_stack.back();
			joint_stack.pop_back();
			is_site = false;
			continue;
		}

		// Start of joint information
		if ( ( strcmp( token, "ROOT" ) == 0 ) ||
		     ( strcmp( token, "JOINT" ) == 0 ) )
		{
			// Create joint data
			new_joint = new Joint();
			new_joint->index = joints.size();
			new_joint->parent = joint;
			new_joint->has_site = false;
			new_joint->offset[0] = 0.0;  new_joint->offset[1] = 0.0;  new_joint->offset[2] = 0.0;
			new_joint->site[0] = 0.0;  new_joint->site[1] = 0.0;  new_joint->site[2] = 0.0;
			joints.push_back( new_joint );
			if ( joint )
				joint->children.push_back( new_joint );

			// Read joint name
			token = strtok( NULL, "" );
			while ( *token == ' ' )  token ++;
			new_joint->name = token;

			// Add to index
			joint_index[ new_joint->name ] = new_joint;
			continue;
		}

		// Start of end site information
		if ( ( strcmp( token, "End" ) == 0 ) )
		{
			new_joint = joint;
			is_site = true;
			continue;
		}

		// Joint offset or end site position information
		if ( strcmp( token, "OFFSET" ) == 0 )
		{
			// Read coordinate values
			token = strtok( NULL, separater );
			x = token ? atof( token ) : 0.0;
			token = strtok( NULL, separater );
			y = token ? atof( token ) : 0.0;
			token = strtok( NULL, separater );
			z = token ? atof( token ) : 0.0;
			
			// Set coordinate values to joint offset
			if ( is_site )
			{
				joint->has_site = true;
				joint->site[0] = x;
				joint->site[1] = y;
				joint->site[2] = z;
			}
			else
			// Set coordinate values to end site position
			{
				joint->offset[0] = x;
				joint->offset[1] = y;
				joint->offset[2] = z;
			}
			continue;
		}

		// Joint channel information
		if ( strcmp( token, "CHANNELS" ) == 0 )
		{
			// Read number of channels
			token = strtok( NULL, separater );
			joint->channels.resize( token ? atoi( token ) : 0 );

			// Read channel information
			for ( i=0; i<joint->channels.size(); i++ )
			{
				// Create channel
				Channel *  channel = new Channel();
				channel->joint = joint;
				channel->index = channels.size();
				channels.push_back( channel );
				joint->channels[ i ] = channel;

				// Determine channel type
				token = strtok( NULL, separater );
				if ( strcmp( token, "Xrotation" ) == 0 )
					channel->type = X_ROTATION;
				else if ( strcmp( token, "Yrotation" ) == 0 )
					channel->type = Y_ROTATION;
				else if ( strcmp( token, "Zrotation" ) == 0 )
					channel->type = Z_ROTATION;
				else if ( strcmp( token, "Xposition" ) == 0 )
					channel->type = X_POSITION;
				else if ( strcmp( token, "Yposition" ) == 0 )
					channel->type = Y_POSITION;
				else if ( strcmp( token, "Zposition" ) == 0 )
					channel->type = Z_POSITION;
			}
		}

		// Move to Motion data section
		if ( strcmp( token, "MOTION" ) == 0 )
			break;
	}


	// Read motion information
	while ( ! file.eof() )
	{
		file.getline( line, BUFFER_LENGTH );
		token = strtok( line, separater );
		if ( !token )
			continue;
		if ( strcmp( token, "Frames" ) == 0 )
			break;
	}
	if ( file.eof() )  goto bvh_error;
	token = strtok( NULL, separater );
	if ( token == NULL )  goto bvh_error;
	num_frame = atoi( token );

	while ( ! file.eof() )
	{
		file.getline( line, BUFFER_LENGTH );
		token = strtok( line, ":" );
		if ( !token )
			continue;
		if ( strcmp( token, "Frame Time" ) == 0 )
			break;
	}
	if ( file.eof() )  goto bvh_error;
	token = strtok( NULL, separater );
	if ( token == NULL )  goto bvh_error;
	interval = atof( token );

	num_channel = channels.size();
	motion = new double[ num_frame * num_channel ];

	// Read motion data
	for ( i=0; i<num_frame; i++ )
	{
		file.getline( line, BUFFER_LENGTH );
		token = strtok( line, separater );
		for ( j=0; j<num_channel; j++ )
		{
			if ( token == NULL )
				goto bvh_error;
			motion[ i*num_channel + j ] = atof( token );
			token = strtok( NULL, separater );
		}
	}

	// Close file
	file.close();

	// Load successful
	is_load_success = true;

	return;

bvh_error:
	file.close();
}


//
//  Save
//
void  BVH::Save( const char * bvh_file_name )
{
	int  i, j;
	ofstream  file;

	// Output order of channels for motion data
	vector< int >   channel_order;

	// Open file
	file.open( bvh_file_name, ios::out );
	if ( file.is_open() == 0 )  return; // Exit if file cannot be opened

	// Set output format
	file.flags( ios::showpoint | ios::fixed );
	file.precision( 6 );
//	int  value_widht = 11;

	// Output hierarchy structure
	file << "HIERARCHY" << endl;
	OutputHierarchy( file, joints[ 0 ], 0, channel_order );

	// Output motion data
	file << "MOTION" << endl;
	file << "Frames: " << num_frame << endl;
	file << "Frame Time: " << interval << endl;
	for ( i=0; i<num_frame; i++ )
	{
		for ( j=0; j<channel_order.size(); j++ )
		{
			// Set character width if outputting numeric values with fixed width
//			if ( value_widht > 0 )
//				file.width( value_widht );

			// Output motion data
			file << GetMotion( i, channel_order[ j ] );

			// Output space or end-of-line character
			if ( j != channel_order.size() - 1 )
				file << "  ";
			else
				file << endl;
		}
	}

	// Close file
	file.close();
}


//
//  Helper function for Save
//

// Recursively output hierarchy structure
void  BVH::OutputHierarchy( 
	ofstream & file, const Joint * joint, int indent_level, vector< int > & channel_list )
{
	int  i;
	string  indent, space;
	Channel *  channel;

	// Initialize indent string
	indent.assign( indent_level * 4, ' ' );
	space.assign( "  " );

	// Output joint name (root name)
	if ( joint->parent )
		file << indent << "JOINT" << space << joint->name << endl;
	else
		file << indent << "ROOT" << space << joint->name << endl;

	// Start of joint block
	file << indent << "{" << endl;
	indent_level ++;
	indent.assign( indent_level * 4, ' ' );

	// Output offset position
	file << indent << "OFFSET" << space;
	file << joint->offset[0] << space;
	file << joint->offset[1] << space;
	file << joint->offset[2] << endl;

	// Output channel information
	file << indent << "CHANNELS"  << space << joint->channels.size() << space;
	for ( i=0; i<joint->channels.size(); i++ )
	{
		channel = joint->channels[ i ];
		switch ( channel->type )
		{
		  case X_ROTATION:
			file << "Xrotation";  break;
		  case Y_ROTATION:
			file << "Yrotation";  break;
		  case Z_ROTATION:
			file << "Zrotation";  break;
		  case X_POSITION:
			file << "Xposition";  break;
		  case Y_POSITION:
			file << "Yposition";  break;
		  case Z_POSITION:
			file << "Zposition";  break;
		}
		if ( i != joint->channels.size() - 1 )
			file << space;
		else
			file << endl;

		// Add to output channel list
		channel_list.push_back( channel->index );
	}

	// Output end site position information
	if ( joint->has_site )
	{
		file << indent << "End Site" << endl;
		file << indent << "{" << endl;

		indent_level ++;
		indent.assign( indent_level * 4, ' ' );

		// Output offset position
		file << indent << "OFFSET" << space;
		file << joint->site[0] << space;
		file << joint->site[1] << space;
		file << joint->site[2] << endl;

		indent_level --;
		indent.assign( indent_level * 4, ' ' );

		file << indent << "}" << endl;
	}

	// Recursively output all child joints
	for ( i=0; i<joint->children.size(); i++ )
	{
		OutputHierarchy( file, joint->children[ i ], indent_level, channel_list );
	}

	// End of joint block
	indent_level --;
	indent.assign( indent_level * 4, ' ' );
	file << indent << "}" << endl;
}


//
//  BVH skeleton/posture rendering functions
//

#include <math.h>
#define  FREEGLUT_STATIC
#include <gl/glut.h>


// Render posture of specified frame
void  BVH::RenderFigure( int frame_no, float scale )
{
	// Render BVH skeleton/posture with specified parameters
	RenderFigure( joints[ 0 ], motion + frame_no * num_channel, scale );
}


// Render specified BVH skeleton/posture (class function)
void  BVH::RenderFigure( const Joint * joint, const double * data, float scale )
{
	glPushMatrix();

	// Apply translation for root joint
	if ( joint->parent == NULL )
	{
		glTranslatef( data[ 0 ] * scale, data[ 1 ] * scale, data[ 2 ] * scale );
	}
	// Apply translation from parent joint for child joints
	else
	{
		glTranslatef( joint->offset[ 0 ] * scale, joint->offset[ 1 ] * scale, joint->offset[ 2 ] * scale );
	}

	// Apply rotation from parent joint (rotation from world coordinates for root joint)
	int  i, j;
	for ( i=0; i<joint->channels.size(); i++ )
	{
		Channel *  channel = joint->channels[ i ];
		if ( channel->type == X_ROTATION )
			glRotatef( data[ channel->index ], 1.0f, 0.0f, 0.0f );
		else if ( channel->type == Y_ROTATION )
			glRotatef( data[ channel->index ], 0.0f, 1.0f, 0.0f );
		else if ( channel->type == Z_ROTATION )
			glRotatef( data[ channel->index ], 0.0f, 0.0f, 1.0f );
	}

	// Render link
	// Render link from origin of joint coordinate system to end point
	if ( joint->children.size() == 0 )
	{
		RenderBone( 0.0f, 0.0f, 0.0f, joint->site[ 0 ] * scale, joint->site[ 1 ] * scale, joint->site[ 2 ] * scale );
	}
	// Render link from origin of joint coordinate system to connection position of next joint
	if ( joint->children.size() == 1 )
	{
		Joint *  child = joint->children[ 0 ];
		RenderBone( 0.0f, 0.0f, 0.0f, child->offset[ 0 ] * scale, child->offset[ 1 ] * scale, child->offset[ 2 ] * scale );
	}
	// Render cylinders from center point to connection positions of all joints
	if ( joint->children.size() > 1 )
	{
		// Calculate origin and center point to connection positions of all joints
		float  center[ 3 ] = { 0.0f, 0.0f, 0.0f };
		for ( i=0; i<joint->children.size(); i++ )
		{
			Joint *  child = joint->children[ i ];
			center[ 0 ] += child->offset[ 0 ];
			center[ 1 ] += child->offset[ 1 ];
			center[ 2 ] += child->offset[ 2 ];
		}
		center[ 0 ] /= joint->children.size() + 1;
		center[ 1 ] /= joint->children.size() + 1;
		center[ 2 ] /= joint->children.size() + 1;

		// Render link from origin to center point
		RenderBone(	0.0f, 0.0f, 0.0f, center[ 0 ] * scale, center[ 1 ] * scale, center[ 2 ] * scale );

		// Render link from center point to connection position of next joint
		for ( i=0; i<joint->children.size(); i++ )
		{
			Joint *  child = joint->children[ i ];
			RenderBone(	center[ 0 ] * scale, center[ 1 ] * scale, center[ 2 ] * scale,
				child->offset[ 0 ] * scale, child->offset[ 1 ] * scale, child->offset[ 2 ] * scale );
		}
	}

	// Recursive call for child joints
	for ( i=0; i<joint->children.size(); i++ )
	{
		RenderFigure( joint->children[ i ], data, scale );
	}

	glPopMatrix();
}


// Render one link of BVH skeleton (class function)
void  BVH::RenderBone( float x0, float y0, float z0, float x1, float y1, float z1 )
{
	// Render cylinder connecting two given points

	// Convert information of two end points of cylinder to origin/direction/length information
	GLdouble  dir_x = x1 - x0;
	GLdouble  dir_y = y1 - y0;
	GLdouble  dir_z = z1 - z0;
	GLdouble  bone_length = sqrt( dir_x*dir_x + dir_y*dir_y + dir_z*dir_z );

	// Set rendering parameters
	static GLUquadricObj *  quad_obj = NULL;
	if ( quad_obj == NULL )
		quad_obj = gluNewQuadric();
	gluQuadricDrawStyle( quad_obj, GLU_FILL );
	gluQuadricNormals( quad_obj, GLU_SMOOTH );

	glPushMatrix();

	// Set translation
	glTranslated( x0, y0, z0 );

	// Calculate matrix representing rotation of cylinder below

	// Normalize z-axis to unit vector
	double  length;
	length = sqrt( dir_x*dir_x + dir_y*dir_y + dir_z*dir_z );
	if ( length < 0.0001 ) { 
		dir_x = 0.0; dir_y = 0.0; dir_z = 1.0;  length = 1.0;
	}
	dir_x /= length;  dir_y /= length;  dir_z /= length;

	// Set direction of reference y-axis
	GLdouble  up_x, up_y, up_z;
	up_x = 0.0;
	up_y = 1.0;
	up_z = 0.0;

	// Calculate direction of x-axis from cross product of z-axis and y-axis
	double  side_x, side_y, side_z;
	side_x = up_y * dir_z - up_z * dir_y;
	side_y = up_z * dir_x - up_x * dir_z;
	side_z = up_x * dir_y - up_y * dir_x;

	// Normalize x-axis to unit vector
	length = sqrt( side_x*side_x + side_y*side_y + side_z*side_z );
	if ( length < 0.0001 ) {
		side_x = 1.0; side_y = 0.0; side_z = 0.0;  length = 1.0;
	}
	side_x /= length;  side_y /= length;  side_z /= length;

	// Calculate direction of y-axis from cross product of z-axis and x-axis
	up_x = dir_y * side_z - dir_z * side_y;
	up_y = dir_z * side_x - dir_x * side_z;
	up_z = dir_x * side_y - dir_y * side_x;

	// Set rotation matrix
	GLdouble  m[16] = { side_x, side_y, side_z, 0.0,
	                    up_x,   up_y,   up_z,   0.0,
	                    dir_x,  dir_y,  dir_z,  0.0,
	                    0.0,    0.0,    0.0,    1.0 };
	glMultMatrixd( m );

	// Set cylinder parameters
	GLdouble radius= 0.01; // Thickness of cylinder
	GLdouble slices = 8.0; // Number of radial subdivisions of cylinder (default 12)
	GLdouble stack = 3.0;  // Number of cross-sectional subdivisions of cylinder (default 1)

	// Render cylinder
	gluCylinder( quad_obj, radius, radius, bone_length, slices, stack ); 

	glPopMatrix();
}



// End of BVH.cpp