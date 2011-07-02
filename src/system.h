/*
 * filename :	System.h
 *
 * programmer :	Cao Jiayin
 */

#ifndef	SORT_SYSTEM
#define	SORT_SYSTEM

// include the header file
#include "geometry/scene.h"
#include "integrator/integrator.h"

// declare classes
class Camera;
class RenderTarget;

/////////////////////////////////////////////////////////////////////
//	definition of the system
class	System
{
// public method
public:
	// default constructor
	System();
	// destructor
	~System();

	// pre-process before rendering
	void PreProcess();
	// post-process after rendering
	void PostProcess();
	// render the image
	void Render();
	// output the render target
	void OutputRT( const char* str );

	// load the scene
	bool LoadScene( const string& str );

//private field:
private:
	// the render target for the system
	RenderTarget*	m_rt;
	// the camera for the system
	Camera*			m_camera;

	// the scene for rendering
	Scene			m_Scene;

	// the integrator for the renderer
	Integrator*		m_pIntegrator;

	// pre-Initialize
	void	_preInit();
	// post-Uninitialize
	void	_postUninit();
};

#endif
