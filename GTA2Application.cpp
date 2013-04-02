/*
-----------------------------------------------------------------------------
Filename:    GTA2Application.cpp
-----------------------------------------------------------------------------
*/
#include "PhysicsSimulator.h"
#include "GTA2Application.h"
#include "Player.h"
#include "Environment.h"
#include "Ball.h"
#include "Score.h"
#include "NetworkManager.h"
#include "gameUpdate.h"
#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Ogre/CEGUIOgreRenderer.h>
#include <cstdlib>
#include <stdlib.h> 
#include <SoundManager.h>
#include <ctime>

using namespace std;

#define PAD_LEFT  0
#define PAD_RIGHT 1
#define PAD_UP    2
#define PAD_DOWN  3
/*
static Ogre::Vector3 velocityVec = Ogre::Vector3::ZERO;
btVector3 startVelocity = btVector3(0,0, -250);
int maxVelocity = 300;
static int speedModifier = 3;
*/
static int edgeSize = 500;
static int wallScale = 4;

static PhysicsSimulator bullet;
SoundManager* sound_manager;
NetworkManager* network_manager;
vector<Player*> players;
static Environment env;
static Ball ball;
static Score score;
bool isMultiplayer;
//static btRigidBody* ball;
//Ogre::SceneNode* ballNode;

//float cooldownMax = 20.0; //# of frames the cooldown is in effect

//btVector3 currBallPos;
//btVector3 currBallDir;
//btVector3 cooldown;



//static int score = 0;
//static int maxScore = 0;
static char scoreString[16];
static char highScoreString[32];
CEGUI::Window *scorePointer;
CEGUI::Window *highScore;
static bool mouseCam = true;
static bool mute=false;
static bool paused = false;


//-------------------------------------------------------------------------------------
GTA2Application::GTA2Application(void)
{
	srand(time(0)); //seed random num generator with current time
}
//-------------------------------------------------------------------------------------
GTA2Application::~GTA2Application(void)
{
}
//-------------------------------------------------------------------------------------
void GTA2Application::createCamera(void)
{
	// Create the camera
    mCamera = mSceneMgr->createCamera("PlayerCam");
    // Position it at 500 in Z direction
    mCamera->setPosition(Ogre::Vector3(0,100,500));
    // Look back along -Z
    mCamera->lookAt(Ogre::Vector3(0,0,0));
    mCamera->setNearClipDistance(5);

    mCameraMan = new OgreBites::SdkCameraMan(mCamera);   // create a default camera controller
    //mCameraMan->setStyle(OgreBites::CS_MANUAL);
}
//-------------------------------------------------------------------------------------
void GTA2Application::createScene(void)
{

	// Set up CEGUI stuff
	mRenderer = &CEGUI::OgreRenderer::bootstrapSystem();
 
    CEGUI::Imageset::setDefaultResourceGroup("Imagesets");
    CEGUI::Font::setDefaultResourceGroup("Fonts");
    CEGUI::Scheme::setDefaultResourceGroup("Schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("LookNFeel");
    CEGUI::WindowManager::setDefaultResourceGroup("Layouts");
 
    CEGUI::SchemeManager::getSingleton().create("TaharezLook.scheme");
 
    CEGUI::System::getSingleton().setDefaultMouseCursor("TaharezLook", "MouseArrow");
 
    CEGUI::WindowManager &wmgr = CEGUI::WindowManager::getSingleton();
    CEGUI::Window *sheet = wmgr.createWindow("DefaultWindow", "CEGUIDemo/Sheet");
 
    //CEGUI::Window *quit = scorePointer;
	scorePointer = wmgr.createWindow("TaharezLook/Button", "CEGUIDemo/ScoreButton");
	sprintf(scoreString, "SCORE: %d", score.getScore());
    scorePointer->setText(scoreString);
    scorePointer->setSize(CEGUI::UVector2(CEGUI::UDim(0.15, 0), CEGUI::UDim(0.05, 0)));
	scorePointer->setPosition(CEGUI::UVector2(CEGUI::UDim(0.8f, 0), CEGUI::UDim(0.85f, 0))); 	

    sheet->addChildWindow(scorePointer);
    CEGUI::System::getSingleton().setGUISheet(sheet);
    
    //High score display
    highScore = wmgr.createWindow("TaharezLook/Button", "CEGUIDemo/HighScoreButton");
    sprintf(highScoreString, "HI-SCORE: %d", score.getMaxScore());
  	highScore->setText(highScoreString);
    highScore->setSize(CEGUI::UVector2(CEGUI::UDim(0.15, 0), CEGUI::UDim(0.05, 0)));
	highScore->setPosition(CEGUI::UVector2(CEGUI::UDim(0.8f, 0), CEGUI::UDim(0.80f, 0))); 	

    sheet->addChildWindow(highScore);
    CEGUI::System::getSingleton().setGUISheet(sheet);

	
	CEGUI::Window *quit = wmgr.createWindow("TaharezLook/Button", "CEGUIDemo/QuitButton");
    quit->setText("QUIT");
    quit->setSize(CEGUI::UVector2(CEGUI::UDim(0.15, 0), CEGUI::UDim(0.05, 0)));
	quit->setPosition(CEGUI::UVector2(CEGUI::UDim(0.8f, 0), CEGUI::UDim(0.9f, 0))); 	

    sheet->addChildWindow(quit);
    CEGUI::System::getSingleton().setGUISheet(sheet);
	
	quit->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&GTA2Application::quit, this));

	//Initialize bullet
	bullet.initPhysics(mSceneMgr);
	
    // Set the scene's ambient light
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5f, 0.5f, 0.5f));
 	
 	//Initialize sound manager
 	sound_manager = new SoundManager();
 	network_manager = new NetworkManager(false);
    // Create a ball
    ball.initBall(mSceneMgr, &bullet, sound_manager, &score);

    // Create a Light and set its position
    Ogre::Light* light = mSceneMgr->createLight("MainLight");
    light->setPosition(20.0f, 80.0f, 50.0f);
	
	env.initEnvironment(mSceneMgr, &bullet);
   
    //PADDLE ------------------------------------------------------------------
    players.push_back(new Player(mSceneMgr, &bullet, "paddlex0"));
   
	sound_manager->playBackground(-1);
    
	cout<<"SCENE CREATED"<<endl;
}

//-------------------------------------------------------------------------------------
bool GTA2Application::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    bool ret = BaseApplication::frameRenderingQueued(evt);
    //Need to capture/update each device
    mKeyboard->capture();
    mMouse->capture();
	
	sprintf (scoreString, "SCORE: %d", score.getScore());
	sprintf (highScoreString, "HI-SCORE: %d", score.getMaxScore());
	scorePointer->setText(scoreString);
	highScore->setText(highScoreString);

	if (!paused)
	{
		bullet.updateWorld(evt);
		ball.update();
		players[0]->updatePosition(evt);

		if(mWindow->isClosed() || mShutDown)
		    return false;
 	}
	
    return ret;
}

void GTA2Application::setMute(bool val){
	cout<<"Calling setMute("<<val<<")"<<endl;
	sound_manager->setMute(val);
	if(val==false){
		sound_manager->playBackground(-1);	
	}
	mute=val;
}

//-------------------------------------------------------------------------------------
// OIS::KeyListener
bool GTA2Application::keyPressed( const OIS::KeyEvent& evt )
{
	switch (evt.key)
    {
    case OIS::KC_ESCAPE: 
        mShutDown = true;
        break;
    case OIS::KC_D:
        players[0]->updatePadDirection(PAD_RIGHT, true);
        break;
    case OIS::KC_A:
    	players[0]->updatePadDirection(PAD_LEFT, true);
        break;  
    case OIS::KC_W:
	    players[0]->updatePadDirection(PAD_UP, true);
        break;    
    case OIS::KC_S:
    	players[0]->updatePadDirection(PAD_DOWN, true);
        break;
	case OIS::KC_C:
		mouseCam = !mouseCam;
		break;
	case OIS::KC_P:
		paused = !paused;
		break;
	case OIS::KC_M:
		setMute(!mute);
		break;
    default:
        break;
    }

	return true;
}
bool GTA2Application::keyReleased( const OIS::KeyEvent& evt ) 
{
	switch (evt.key)
    {
    case OIS::KC_D:
    	players[0]->updatePadDirection(PAD_RIGHT, false);
        break;
    case OIS::KC_A:
       	players[0]->updatePadDirection(PAD_LEFT, false);
        break;  
    case OIS::KC_W:
    	players[0]->updatePadDirection(PAD_UP, false);
        break;    
    case OIS::KC_S:
    	players[0]->updatePadDirection(PAD_DOWN, false);
        break;            
    default:
        break;
    }
	if(CEGUI::System::getSingleton().injectKeyUp(evt.key)) return true;
    	mCameraMan->injectKeyUp(evt);
	return true;

} 
//-------------------------------------------------------------------------------------

CEGUI::MouseButton convertButton(OIS::MouseButtonID buttonID)
{
    switch (buttonID)
    {
    case OIS::MB_Left:
        return CEGUI::LeftButton;
        break;
 
    case OIS::MB_Right:
        return CEGUI::RightButton;
        break;
 
    case OIS::MB_Middle:
        return CEGUI::MiddleButton;
        break;
 
    default:
        return CEGUI::LeftButton;
        break;
    }
}

bool GTA2Application::mouseMoved( const OIS::MouseEvent &arg )
{
	if(mouseCam) {
		BaseApplication::mouseMoved( arg );}
	else {
		if(CEGUI::System::getSingleton().injectMouseMove(arg.state.X.rel, arg.state.Y.rel)) return true;
		mCameraMan->injectMouseMove(arg);
	}
    return true;
}
 
bool GTA2Application::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    if(CEGUI::System::getSingleton().injectMouseButtonDown(convertButton(id))) return true;
    mCameraMan->injectMouseDown(arg, id);
    return true;
}
 
bool GTA2Application::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    if(CEGUI::System::getSingleton().injectMouseButtonUp(convertButton(id))) return true;
    mCameraMan->injectMouseUp(arg, id);
    return true;
}

bool GTA2Application::quit(const CEGUI::EventArgs &e)
{
    mShutDown = true;
    return true;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
    int main(int argc, char *argv[])
#endif
    {
        // Create application object
        GTA2Application app;

        try {
            app.go();
        } catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
            std::cerr << "An exception has occured: " <<
                e.getFullDescription().c_str() << std::endl;
#endif
        }

        return 0;
    }

#ifdef __cplusplus
}
#endif
