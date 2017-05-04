#include "bsg.h"
#include "bsgMenagerie.h"
#include "bsgObjModel.h"

#include <api/MinVR.h>
#include <math/VRMath.h>
#include <main/VREventInternal.h>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"
 
class DemoVRApp: public MinVR::VRApp {

  // Data values that were global in the demo2.cpp file are defined as
  // private members of the VRApp.
private:

  // The scene and the objects in it must be available from the main()
  // function where it is created and the renderScene() function where
  // it is drawn.  The scene object contains all the drawable objects
  // that make up the scene.
  bsg::scene _scene;

  // These are the shapes that make up the scene.  They are out here in
  // the global variables so they can be available in both the main()
  // function and the renderScene() function.
  bsg::drawableCompound* _axesSet;
  bsg::drawableCollection* _modelGroup;
  bsg::drawableObjModel* _model;
  bsg::drawableObjModel* _model2;
  bsg::drawableObjModel* _wand;
bsg::drawableCollection* _wandGroup;
    bsg::drawableCompound* _laser;

  //std::vector<bsg::drawableCollection*> _modelList;
  //bsg::drawableCollection* _activeModel;
  std::vector<bsg::drawableCompound*> _modelList;
  bsg::drawableCompound* _activeModel;
  int _activeID;

  

  // These variables were not global before, but their scope has been
  // divided into several functions here, so they are class-wide
  // private data objects.
  bsg::bsgPtr<bsg::shaderMgr> _shader;
  bsg::bsgPtr<bsg::shaderMgr> _axesShader;
  bsg::bsgPtr<bsg::lightList> _lights;

  // Here are the drawable objects that make up the compound object
  // that make up the scene.
  bsg::drawableObj _axes;
  bsg::drawableObj _topShape;
  bsg::drawableObj _bottomShape;

  std::string _vertexFile;
  std::string _fragmentFile;

  bool _moving;
  glm::mat4 _wandDrag;
  glm::mat4 _lastWandPos;
  glm::vec3 _lastTranslation;
  glm::quat _lastRotation;
  float _scaleChange;
  bool _showLaser;
  bool _activeToggleVisibility;



void printMat4(glm::mat4 m){
  for(int i = 0; i < 4; i++){
    for(int j = 0; j < 4; j++){
      printf("%6.2f ", m[j][i]);
    }
    printf("\n");
  }
}
  
  // These functions from demo2.cpp are not needed here:
  //
  //    init()
  //    makeWindow()
  //    resizeWindow()
  //    ... also most of the processKeys() methods.
  //
  // The functionality of these methods is assumed by the MinVR apparatus.

  // This contains a bunch of sanity checks from the graphics
  // initialization of demo2.cpp.  They are still useful with MinVR.
  void _checkContext() {
    
    // There is one more graphics library used here, called GLEW.  This
    // library sorts through the various OpenGL updates and changes and
    // allows a user to pretend that it's all a consistent and simple
    // system.  The 'core profile' refers to some modern OpenGL
    // enhancements that are not so modern as of 2017.  Set this to true
    // to get those enhancements.
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
      throw std::runtime_error("Failed to initialize GLEW");
    }

    // Now that we have a graphics context, let's look at what's inside.
    std::cout << "Hardware check: "
              << glGetString(GL_RENDERER)  // e.g. Intel 3000 OpenGL Engine
              << " / "
              << glGetString(GL_VERSION)    // e.g. 3.2 INTEL-8.0.61
              << std::endl;

    if (glewIsSupported("GL_VERSION_2_1")) {
      std::cout << "Software check: Ready for OpenGL 2.1." << std::endl;
    } else {
      throw std::runtime_error("Software check: OpenGL 2.1 not supported.");
    }

    // This is the background color of the viewport.
    glClearColor(0.1 , 0.1, 0.1, 1.0);

    // Now we're ready to start issuing OpenGL calls.  Start by enabling
    // the modes we want.  The DEPTH_TEST is how you get hidden faces.
    glEnable(GL_DEPTH_TEST);

    if (glIsEnabled(GL_DEPTH_TEST)) {
      std::cout << "Depth test enabled" << std::endl;
    } else {
      std::cout << "No depth test enabled" << std::endl;
    }

    // This is just a performance enhancement that allows OpenGL to
    // ignore faces that are facing away from the camera.
    glEnable(GL_CULL_FACE);
    glLineWidth(4);
    glEnable(GL_LINE_SMOOTH);

  }

  // Just a little debug function so that a user can see what's going on
  // in a non-graphical sense.
  void _showCameraPosition() {

    std::cout << "Camera is at ("
              << _scene.getCameraPosition().x << ", "
              << _scene.getCameraPosition().y << ", "
              << _scene.getCameraPosition().z << ")... ";
    std::cout << "looking at ("
              << _scene.getLookAtPosition().x << ", "
              << _scene.getLookAtPosition().y << ", "
              << _scene.getLookAtPosition().z << ")." << std::endl; 
  }

  void _initializeScene() {

    // Create a list of lights.  If the shader you're using doesn't use
    // lighting, and the shapes don't have textures, this is irrelevant.
    _lights->addLight(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                      glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
                      0.5f, 0.8f, 0.0f, 0);

    // Create a shader manager and load the light list.
    _shader->addLights(_lights);


    _vertexFile = "D:\\workspace\\demo-graphic-experimental\\src\\tex2.vp";
    _fragmentFile = "D:\\workspace\\demo-graphic-experimental\\src\\tex2.fp";
    
    // Add the shaders to the manager, first the vertex shader...
    _shader->addShader(bsg::GLSHADER_VERTEX, _vertexFile);

    // ... then the fragment shader.  You could potentially add a
    // geometry shader at this point.
    _shader->addShader(bsg::GLSHADER_FRAGMENT, _fragmentFile);

    // The shaders are loaded, now compile them.
    _shader->compileShaders();

    // Add a texture to our shader manager object.
    bsg::bsgPtr<bsg::textureMgr> texture = new bsg::textureMgr();

    texture->readFile(bsg::textureCHK, "");
    _shader->addTexture(texture);
    
    // We could put the axes and the object in the same compound
    // shape, but we leave them separate so they can be moved
    // separately.

    _model = new bsg::drawableObjModel(_shader, "D:\\workspace\\demo-graphic\\data\\test-v.obj", false);
    //_model = new bsg::drawableObjModel(_shader, "../../demo-graphic/data/CasA_Supernova_Remnant.obj", false);
    _model->setPosition(glm::vec3(-5.0f, 0.0f, -8.0f));
    _model->setScale(glm::vec3(0.4f, 0.4f, 0.4f));
	_model->setVisible(true);

    //_model2 = new bsg::drawableObjModel(_shader, "../data/tubes.fluid.lw.obj", true);
    _model2 = new bsg::drawableObjModel(_shader, "D:\\workspace\\demo-graphic\\data\\test-v.obj", false);
    _model2->setPosition(glm::vec3(-5.0f, 0.0f, -10.0f));
    _model2->setScale(glm::vec3(0.15f, 0.15f, 0.15f));
	_model2->setVisible(true);
/*
    _model3 = new bsg::drawableObjModel(_shader, "../data/tubes.fluid.lw.obj", true);
    //_model3 = new bsg::drawableObjModel(_shader, "../data/test-v.obj", false);
    _model3->setPosition(glm::vec3(-5.0f, 0.0f, -10.0f));
    _model3->setScale(glm::vec3(0.15f, 0.15f, 0.15f));

    _model4 = new bsg::drawableObjModel(_shader, "../data/tubes.fluid.lw.obj", true);
    //_model4 = new bsg::drawableObjModel(_shader, "../data/test-v.obj", false);
    _model4->setPosition(glm::vec3(-5.0f, 0.0f, -10.0f));
    _model4->setScale(glm::vec3(0.15f, 0.15f, 0.15f));

    _model5 = new bsg::drawableObjModel(_shader, "../data/tubes.fluid.lw.obj", true);
    //_model5 = new bsg::drawableObjModel(_shader, "../data/test-v.obj", false);
    _model5->setPosition(glm::vec3(-5.0f, 0.0f, -10.0f));
    _model5->setScale(glm::vec3(0.15f, 0.15f, 0.15f));

    _model6 = new bsg::drawableObjModel(_shader, "../data/tubes.fluid.lw.obj", true);
    //_model6 = new bsg::drawableObjModel(_shader, "../data/test-v.obj", false);
    _model6->setPosition(glm::vec3(-5.0f, 0.0f, -10.0f));
    _model6->setScale(glm::vec3(0.15f, 0.15f, 0.15f));
*/


    _wand = new bsg::drawableObjModel(_shader, "D:\\workspace\\demo-graphic-experimental\\data\\pointer.obj", false);


    _modelList.push_back(_model);
    _modelList.push_back(_model2);
/*    _modelList.push_back(_model3);
    _modelList.push_back(_model4);
    _modelList.push_back(_model5);
    _modelList.push_back(_model6);*/

    _activeModel = _modelList.front();
    _activeID = 0;





    _modelGroup = new bsg::drawableCollection();
    _wandGroup = new bsg::drawableCollection();

    /*for (std::vector<bsg::drawableCollection*>::iterator it = _modelList.begin(); it != _modelList.end(); ++it) {
    	_modelGroup->addObject(*it);
    }*/

    for (std::vector<bsg::drawableCompound*>::iterator it = _modelList.begin(); it != _modelList.end(); ++it) {
    	_scene.addObject(*it);
    }
    //_modelGroup->addObject(_model);
    //_modelGroup->addObject(_model2);
	//_modelGroup->setVisible(true);
    //_scene.addObject(_modelGroup);
    //
    
    
    _axesShader->addLights(_lights);
    _axesShader->addShader(bsg::GLSHADER_VERTEX, "D:\\workspace\\demo-graphic-experimental\\src\\shader2.vp");
    _axesShader->addShader(bsg::GLSHADER_FRAGMENT, "D:\\workspace\\demo-graphic-experimental\\src\\shader.fp");
    _axesShader->addTexture(texture);

    _axesShader->compileShaders();

    _axesSet = new bsg::drawableAxes(_axesShader, 100.0f);

    // Now add the axes.
    _scene.addObject(_axesSet);

    _laser = new bsg::drawableLine(_axesShader, 100.0f);
    _laser->setVisible(false);
    

    _wandGroup->addObject(_wand);
    _wandGroup->addObject(_laser);
    _scene.addObject(_wandGroup);
    _wand->setPosition(glm::vec3(0.0f, 0.0f, -0.3f));
    _wand->setRotation(-1.5708f, 0.0f, 1.5708f);
    _wand->setScale(0.1f);

    // All the shapes are now added to the scene.
  }

  
public:
  DemoVRApp(int argc, char** argv, const std::string& configFile) :
    MinVR::VRApp(argc, argv, configFile) {

    // This is the root of the scene graph.
    bsg::scene _scene = bsg::scene();

    // These are tracked separately because multiple objects might use
    // them.
    _shader = new bsg::shaderMgr();
    _axesShader = new bsg::shaderMgr();
    _lights = new bsg::lightList();
    _moving = false;
    _scaleChange = 0.0f;
    _showLaser = false;
    _activeToggleVisibility = false;
  }

  /// The MinVR apparatus invokes this method whenever there is a new
  /// event to process.
  void onVREvent(const MinVR::VREvent &event) {
	  

    if ((event.getName() != "Wand0_Move" && event.getName() != "FrameStart" && (event.getName().compare(0, 3, "HTC") != 0))|| (event.getName() == "Wand0_Move" && _moving)) {
    //    event.print();
    }

	if (event.getName().compare(0, 16, "HTC_Controller_1") == 0)
	{
		if (event.getInternal()->getDataIndex()->exists("/HTC_Controller_1/State/Axis1Button_Pressed") &&
			(int)event.getInternal()->getDataIndex()->getValue("/HTC_Controller_1/State/Axis1Button_Pressed")) {
			_moving = true;
		//	std::cout << "move" << std::endl;
		}
		else
		{
		//	std::cout << "stop" << std::endl;
			_moving = false;
		}

		if (event.getInternal()->getDataIndex()->exists("/HTC_Controller_1/Pose")) {
			//std::cout << "huzzah - " << event.getName() << std::endl;
			//event.print();
			MinVR::VRMatrix4 wandPosition(event.getDataAsFloatArray("Pose"));
			glm::mat4 wandPos = glm::make_mat4(wandPosition.getArray());

			//printMat4(wandPos);

		//}
			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 translation;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(wandPos, scale, rotation, translation, skew, perspective);

			if (_moving) {
				glm::mat4 wandChange = wandPos / _lastWandPos;
				_wandDrag = wandChange * _wandDrag;
			}

			_lastRotation = rotation;
			_lastTranslation = translation;
			_lastWandPos = wandPos;
		}
	}
		/*if (event.getInternal()->getDataIndex()->exists("/HTC_Controller_1/State/Axis0Button_Pressed") &&
			(int)event.getInternal()->getDataIndex()->getValue("/HTC_Controller_1/State/Axis0Button_Pressed")) {
			double x = event.getInternal()->getDataIndex()->getValue("/HTC_Controller_1/State/Axis0/XPos");
			double y = event.getInternal()->getDataIndex()->getValue("/HTC_Controller_1/State/Axis0/YPos");

			bool rotate = false;
			if (fabs(x) > fabs(y)) rotate = true;

			//if (!rotate)
			//{
				MinVR::VRVector3 offset = 0.05 * controllerpose * MinVR::VRVector3(0, 0, y);
				MinVR::VRMatrix4 trans = MinVR::VRMatrix4::translation(offset);

				roompose = trans * roompose;
			//}
			//else
			//{
				MinVR::VRMatrix4 rot = MinVR::VRMatrix4::rotationY(x / 10 / M_PI);
				roompose = rot * roompose;
			//}

		}
	}*/

        
    // This heartbeat event recurs at regular intervals, so you can do
    // animation with the model matrix here, as well as in the render
    // function.  
                // if (event.getName() == "FrameStart") {
    //   const double time = event.getDataAsDouble("ElapsedSeconds");
    //   return;
                // }

      if (event.getName() == "Wand0_Move"){
            MinVR::VRMatrix4 wandPosition(event.getDataAsFloatArray("Transform"));
            glm::mat4 wandPos = glm::make_mat4(wandPosition.getArray());
	    glm::vec3 scale;
	    glm::quat rotation;
	    glm::vec3 translation;
	    glm::vec3 skew;
	    glm::vec4 perspective;
	    glm::decompose(wandPos, scale, rotation, translation, skew, perspective);

            if(_moving){
	      glm::mat4 wandChange = wandPos/_lastWandPos;
	      _wandDrag = wandChange * _wandDrag;
            }

	    _lastRotation = rotation;
            _lastTranslation = translation;
            _lastWandPos = wandPos;
          }
	

    // Quit if the escape button is pressed
    if (event.getName() == "KbdEsc_Down" || event.getName() == "Wand_Select_Down") {
std::cout << "Shutting Down" << std::endl;
      shutdown();
      
    } else if (event.getName() == "MouseBtnLeft_Down" || event.getName() == "Wand_Bottom_Trigger_Down" || event.getName() == "Wand_Top_Trigger_Down"){
        _moving = true;
      } else if (event.getName() == "MouseBtnLeft_Up" || event.getName() == "Wand_Bottom_Trigger_Up" || event.getName() == "Wand_Top_Trigger_Up"){
        _moving = false;
      } else if (event.getName() == "Wand_Up_Down"){
        _showLaser = true;
      } else if (event.getName() == "Wand_Up_Up"){
        _showLaser = false;
      } else if (event.getName() == "Wand_Left_Down"){
	_activeID--;
        if (_activeID < 0) {
		_activeID = _modelList.size()-1;
	}
        
      } else if (event.getName() == "Wand_Right_Down"){
        _activeID++;
        if (_activeID >= _modelList.size()) {
		_activeID = 0;
	}
        
        
      } else if (event.getName() == "Wand_Down_Down"){
        _activeToggleVisibility = true;
      } else if (event.getName() == "Wand_Joystick_Y_Change"){
        if (abs(event.getDataAsFloat("AnalogValue")) > 0.05) {
        	_scaleChange = -event.getDataAsFloat("AnalogValue") * 0.005;
	} else {
		_scaleChange = 0.0f;
	}
      }

      

    // Print out where you are (where the camera is) and where you're
    // looking.
    // _showCameraPosition();
    
  }
  
  /// \brief Set the render context.
  ///
  /// The onVRRender methods are the heart of the MinVR rendering
  /// apparatus.  Some render calls are shared among multiple views,
  /// for example a stereo view has two renders, with the same render
  /// context.
  void onVRRenderGraphicsContext(const MinVR::VRGraphicsState &renderState) {

    // Check if this is the first call.  If so, do some initialization. 
    if (renderState.isInitialRenderCall()) {
      _checkContext();
      _initializeScene();
      _scene.prepare();
    }
  }

  /// \brief Draw the image.
  ///
  /// This is the heart of any graphics program, the render function.
  /// It is called each time through the main graphics loop, and
  /// re-draws the scene according to whatever has changed since the
  /// last time it was drawn.
  void onVRRenderGraphics(const MinVR::VRGraphicsState &renderState) {
    // Only draw if the application is still running.
    if (isRunning()) {

      // If you want to adjust the positions of the various objects in
      // your scene, you can do that here.

//printMat4(_wandDrag);
_activeModel = _modelList[_activeID];
if (_activeToggleVisibility) {
  _activeModel->setVisible(!_activeModel->getVisible());
  _activeToggleVisibility = false;
}

_activeModel->setTransformMatrix(_wandDrag);
if(_wandDrag != glm::mat4(1.0f)){
printMat4(_activeModel->getModelMatrix());
}
//_wandDrag = glm::mat4(1.0f);

_wandGroup->setPosition(_lastTranslation);
_wandGroup->setOrientation(glm::inverse(_lastRotation));


float newScale = _activeModel->getScale().x + _scaleChange;
if (newScale > 0.01 && newScale < 5) {
_activeModel->setScale(newScale); 
}

_laser->setVisible(_showLaser);

      // Now the preliminaries are done, on to the actual drawing.
  
      // First clear the display.
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  
      // Second the load() step.  We let MinVR give us the projection
      // matrix from the render state argument to this method.
      const float* pm = renderState.getProjectionMatrix();
      glm::mat4 projMatrix = glm::mat4( pm[0],  pm[1], pm[2], pm[3],
                                        pm[4],  pm[5], pm[6], pm[7],
                                        pm[8],  pm[9],pm[10],pm[11],
                                        pm[12],pm[13],pm[14],pm[15]);
      _scene.load();

      // The draw step.  We let MinVR give us the view matrix.
      const float* vm = renderState.getViewMatrix();
      glm::mat4 viewMatrix = glm::mat4( vm[0],  vm[1], vm[2], vm[3],
                                        vm[4],  vm[5], vm[6], vm[7],
                                        vm[8],  vm[9],vm[10],vm[11],
                                        vm[12],vm[13],vm[14],vm[15]);

      //in desktop mode, +x is away from camera, +z is right, +y is up
      //viewMatrix = _owm * viewMatrix;

      //bsg::bsgUtils::printMat("view", viewMatrix);
//	std::cout << "three" << std::endl;
      _scene.draw(viewMatrix, projMatrix);
//std::cout << "four" << std::endl;

if(_wandDrag != glm::mat4(1.0f)){
printMat4(_activeModel->getModelMatrix());
}
_wandDrag = glm::mat4(1.0f);

      // We let MinVR swap the graphics buffers.
      // glutSwapBuffers();
    }
  }
};

// The main function is just a shell of its former self.  Just
// initializes a MinVR graphics object and runs it.
int main(int argc, char **argv) {

  std::cout << "Invoked with argc=" << argc << " arguments." << std::endl;

  for (int i = 0; i < argc ; i++) {
    std::cout << "argv[" << i << "]: " << std::string(argv[i]) << std::endl;
  }

  // Now we load the shaders.  First check to see if any have been
  // specified on the command line.

  if (argc < 2) {
    throw std::runtime_error("\nNeed a config file.\nTry 'bin/objDemoMinVR ../config/desktop-freeglut.xml'");
  }
    
  // Initialize the app.
  DemoVRApp app(argc, argv, argv[1]);

  // Run it.
  app.run();

  // We never get here.
  return 0;
}



  
