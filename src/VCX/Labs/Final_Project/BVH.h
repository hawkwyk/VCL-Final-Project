#ifndef  _BVH_H_
#define  _BVH_H_


#include <vector>
#include <map>
#include <string>
#include <fstream>

using namespace  std;



//
//  BVH format motion data
//
class  BVH
{
  public:
	/*  Internal structures  */

	// Channel types
	enum  ChannelEnum
	{
		X_ROTATION, Y_ROTATION, Z_ROTATION,
		X_POSITION, Y_POSITION, Z_POSITION
	};
	struct  Joint;

	// Channel information
	struct  Channel
	{
		// Corresponding joint
		Joint *              joint;
		
		// Channel type
		ChannelEnum          type;

		// Channel index
		int                  index;
	};

	// Joint information
	struct  Joint
	{
		// Joint name
		string               name;
		// Joint index
		int                  index;

		// Joint hierarchy (parent joint)
		Joint *              parent;
		// Joint hierarchy (child joints)
		vector< Joint * >    children;

		// Connection position
		double               offset[3];

		// Flag indicating whether it has end site information
		bool                 has_site;
		// End site position
		double               site[3];

		// Rotation axes
		vector< Channel * >  channels;
	};


  private:
	// Flag indicating whether loading was successful
	bool                     is_load_success;

	/*  File information  */
	string                   file_name;   // File name
	string                   motion_name; // Motion name

	/*  Hierarchy information  */
	int                      num_channel; // Number of channels
	vector< Channel * >      channels;    // Channel information [channel index]
	vector< Joint * >        joints;      // Joint information [part index]
	map< string, Joint * >   joint_index; // Index from joint name to joint information

	/*  Motion data information  */
	int                      num_frame;   // Number of frames
	double                   interval;    // Time interval between frames
	double *                 motion;      // [frame number][channel number]


  public:
	// Constructor and destructor
	BVH();
	BVH( const char * bvh_file_name );
	~BVH();

	// Clear all information
	void  Clear();

	// Set all information (joint hierarchy and motion data)
	void  Init( const char * name, 
		int n_joi, const Joint ** a_joi, int n_chan, const Channel ** a_chan, 
		int n_frame, double interval, const double * mo );

	// Set joint hierarchy
	void  SetSkeleton( const char * name, 
		int n_joi, const Joint ** a_joi, int n_chan, const Channel ** a_chan );

	// Set motion data
	void  SetMotion( int n_frame, double interval, const double * mo = NULL );

	// Load BVH file
	void  Load( const char * bvh_file_name );

	// Save BVH file
	void  Save( const char * bvh_file_name );

  public:
	/*  Data access functions  */

	// Check if loading was successful
	bool  IsLoadSuccess() const { return is_load_success; }

	// Get file information
	const string &  GetFileName() const { return file_name; }
	const string &  GetMotionName() const { return motion_name; }

	// Get hierarchy information
	const int       GetNumJoint() const { return  joints.size(); }
	const Joint *   GetJoint( int no ) const { return  joints[no]; }
	const int       GetNumChannel() const { return  channels.size(); }
	const Channel * GetChannel( int no ) const { return  channels[no]; }

	const Joint *   GetJoint( const string & j ) const  {
		map< string, Joint * >::const_iterator  i = joint_index.find( j );
		return  ( i != joint_index.end() ) ? (*i).second : NULL; }
	const Joint *   GetJoint( const char * j ) const  {
		map< string, Joint * >::const_iterator  i = joint_index.find( j );
		return  ( i != joint_index.end() ) ? (*i).second : NULL; }

	// Get motion data information
	int     GetNumFrame() const { return  num_frame; }
	double  GetInterval() const { return  interval; }
	double  GetMotion( int f, int c ) const { return  motion[ f*num_channel + c ]; }

	// Modify motion data information
	void  SetMotion( int f, int c, double v ) { motion[ f*num_channel + c ] = v; }

  protected:
	/*  Helper functions for saving  */
	
	// Recursively output hierarchy structure
	void  OutputHierarchy( ofstream & file, const Joint * joint, int indent_level,
		vector< int > & channel_list );
  
  public:
	/*  Rendering functions  */
	
	// Render the pose of a specified frame
	void  RenderFigure( int frame_no, float scale = 1.0f );

	// Render the specified BVH skeleton and pose (class function)
	static void  RenderFigure( const Joint * root, const double * data, float scale = 1.0f );

	// Render a single link of the BVH skeleton (class function)
	static void  RenderBone( float x0, float y0, float z0, float x1, float y1, float z1 );
};



#endif // _BVH_H_
