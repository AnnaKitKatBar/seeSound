/* P3 materials example - see SetMaterial and fragment shader
CPE 471 Cal Poly Z. Wood + S. Sueda
*/
#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"

#include "fmod.hpp"
#include "fmod_errors.h" // Only if you want error checking


#define blue 0
#define grey 1
#define brass 2
#define copper 3
#define perl 4
#define tin 5
#define gold 6
#define cyan 7
#define bronze 8
#define silver 9

using namespace std;
using namespace Eigen;
using namespace FMOD;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "../resources/"; // Where the resources are loaded from
shared_ptr<Program> phong;

shared_ptr<Shape> sphere;
shared_ptr<Shape> cube;
shared_ptr<Shape> ring;

int g_width, g_height;
float cursorX = (float)g_width / 2.0f;
float cursorY = (float)g_height / 2.0f;
float curShiftX = 0;  //in pixels
float curShiftY = 0;

//light and materials
float lightXshift = 6;
float lightYshift = 10;
int maxMaterials = 10;

//view stuff
float alpha = 0;
float beta = 270;
float speed = .3f;
Vector3f eye;// = Vector3f(0, 0, 5);
Vector3f upVec = Vector3f(0, 1, 0);
Vector3f lookAtPt;// = Vector3f(0, 0, 0);
Vector3f w;
Vector3f u;
Vector3f gaze;

//camera stuff
int clickOn = 0;

//convert to radians
float radConvert = (float)(M_PI / 180.0);

//initial XY coordinates to pull from for objects
float initPos[] = { 18, 10, -15, 16, 3, 7, -4, 3, 3, 13,
12, -1, -11, 19, -20, -17, 3, -18, -19, 15,
-7, 3, 7, 9, -2, 8, 6, -10, 5, 3 };

//sound things
FMOD::System *soundSystem;
FMOD::Sound *analyzeSong;
FMOD::ChannelGroup *channelMusic;
FMOD::Channel *songChannel;

int sampleSize = 128;  //has to be power of 2, 64 vs 128
float *leftSpec;
float *rightSpec;
float *avgSpec;
bool pause = false;
float hzRange;
float tickCount = 0;

//diff songs
int maxSongs = 4;

//beat detection djKaty
float beatThresholdVolume = 30;    // The threshold over which to recognize a beat
int beatThresholdBar = 0;            // The bar in the volume distribution to examine
unsigned int beatPostIgnore = 250;   // Number of ms to ignore track for after a beat is recognized
bool beatDetected;
int beatLastTick = 0;                // Time when last beat occurred

//mode stuff
int mode = 0;
int maxModes = 2;

//animation things
float nyoom = 0;
float tilt;
float dir = 1.0;
int bassCount = 0;
int snareCount = 0;

//djkaty error checking code
void FMODErrorCheck(FMOD_RESULT result)
{
   if (result != FMOD_OK)
   {
      std::cout << "FMOD error! (" << result << ") " << FMOD_ErrorString(result) << std::endl;
      exit(-1);
   }
}

static Vector3f normalize(Vector3f vec) {
   Vector3f ret;
   float length;

   length = sqrtf((vec[0] * vec[0]) + (vec[1] * vec[1]) + (vec[2] * vec[2]));
   ret[0] = vec[0] / length;
   ret[1] = vec[1] / length;
   ret[2] = vec[2] / length;

   return ret;
}

static void error_callback(int error, const char *description)
{
   cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
   if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      analyzeSong->release();
      glfwSetWindowShouldClose(window, GL_TRUE);
   }
   else if (key == GLFW_KEY_A) {
      //pan left
      Vector3f cross;
      Vector3f vec1 = upVec;
      Vector3f vec2 = w;

      cross[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
      cross[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
      cross[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];

      u = normalize(cross);

      eye -= speed * u;
      lookAtPt -= speed * u;

   }
   else if (key == GLFW_KEY_D) {
      //pan right
      Vector3f cross;
      Vector3f vec1 = upVec;
      Vector3f vec2 = w;

      cross[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
      cross[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
      cross[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];

      u = normalize(cross);

      eye += speed * u;
      lookAtPt += speed * u;

   }
   else if (key == GLFW_KEY_W) {
      //zoom in
      gaze = lookAtPt - eye;
      w = -normalize(gaze);

      eye -= speed * w;
      lookAtPt -= speed * w;

   }
   else if (key == GLFW_KEY_S) {
      //zoom out
      gaze = lookAtPt - eye;
      w = -normalize(gaze);

      eye += speed * w;
      lookAtPt += speed * w;

   }
   else if (key == GLFW_KEY_Q) {
      //move light source left
      lightXshift -= .3f;
   }
   else if (key == GLFW_KEY_E) {
      //move light source right
      lightXshift += .3f;
   }
   else if (key == GLFW_KEY_LEFT) {
      //move light source left
      lightXshift -= .3f;
   }
   else if (key == GLFW_KEY_RIGHT) {
      //move light source right
      lightXshift += .3f;
   }
   else if (key == GLFW_KEY_DOWN) {
      //move light source down
      lightYshift -= .3f;
   }
   else if (key == GLFW_KEY_UP) {
      //move light source up
      lightYshift += .3f;
   }
   else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
      printf("snap back to 'origin'\n");

      if (mode == 0)
         eye = Vector3f(0, 0, 5);
      else if (mode == 1)
         eye = Vector3f(0, 0, 6);

      //update back to looking down -z
      alpha = 0;
      beta = 270;
      //eye = Vector3f(0, 0, 0);
      lookAtPt = Vector3f(cos(alpha * radConvert) * cos(beta * radConvert), sin(alpha * radConvert), cos(alpha * radConvert) * cos((90 - beta) * radConvert));
      gaze = lookAtPt - eye;
      w = -normalize(gaze);
   }
   else if (key == GLFW_KEY_P && action == GLFW_PRESS) {
      songChannel->setPaused(pause);

      if (pause == true)
         pause = false;
      else
         pause = true;
   }
   else if (key == GLFW_KEY_M && action == GLFW_PRESS) {
      mode = (mode + 1) % maxModes;

      if (mode == 0)
         eye = Vector3f(0, 0, 5);
      else if (mode == 1)
         eye = Vector3f(0, 0, 6);

   }
   //printf("Eye = %f, %f, %f\nLookAt = %f, %f, %f\n", eye[0], eye[1], eye[2], lookAtPt[0], lookAtPt[1], lookAtPt[2]);
   Vector3f vec = lookAtPt - eye;
   float length = sqrtf((vec[0] * vec[0]) + (vec[1] * vec[1]) + (vec[2] * vec[2]));
   //printf("Length should be ~1 = %f\n\n", length);

}

static void mouse_callback(GLFWwindow *window, int button, int action, int mods) {  //clicking
   double posX, posY;
   if (action == GLFW_PRESS) {
      glfwGetCursorPos(window, &posX, &posY);
      cout << "Pos X " << posX << " Pos Y " << posY << endl;

      cursorX = (float)posX;
      cursorY = (float)posY;

      clickOn = 1;
   }
   else
      clickOn = 0;
}

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
   if (clickOn) {
      curShiftX = cursorX - (float)xpos;
      curShiftY = (float)ypos - cursorY;
      cursorX = (float)xpos;
      cursorY = (float)ypos;
      //printf("cursor: Xshift = %f, yShift = %f\n", curShiftX, curShiftY);

      //update 
      alpha += curShiftY * (180.0f / (float)g_height);
      beta += curShiftX * (360.0f / (float)g_width);


      //check vertical constraints
      if (alpha > 80)
         alpha -= curShiftY * (180.0f / (float)g_height);
      if (alpha < -80)
         alpha -= curShiftY * (180.0f / (float)g_height);

      lookAtPt = Vector3f(eye[0] + (cos(alpha * radConvert) * cos(beta * radConvert)), eye[1] + (sin(alpha * radConvert)), eye[2] + (cos(alpha * radConvert) * cos((90 - beta) * radConvert)));
      gaze = lookAtPt - eye;
      w = -normalize(gaze);
      //printf("LookAT: %f, %f, %f\n", lookAtPt[0], lookAtPt[1], lookAtPt[2]);
   }
}

static void resize_callback(GLFWwindow *window, int width, int height) {
   g_width = width;
   g_height = height;
   glViewport(0, 0, width, height);
}

//helper function to set materials
void SetMaterial(int i, int shape) {
   //note: shape argument useless, too lazy to fix atm

   switch (i) {
   case 0: //shiny blue plastic
      glUniform3f(phong->getUniform("MatAmb"), 0.02f, 0.04f, 0.2f);
      glUniform3f(phong->getUniform("MatDif"), 0.0f, 0.16f, 0.9f);
      glUniform3f(phong->getUniform("MatSpec"), 0.14f, 0.2f, 0.8f);
      glUniform1f(phong->getUniform("shini"), 120.0f);
      break;
   case 1: // flat grey
      glUniform3f(phong->getUniform("MatAmb"), 0.13f, 0.13f, 0.14f);
      glUniform3f(phong->getUniform("MatDif"), 0.3f, 0.3f, 0.4f);
      glUniform3f(phong->getUniform("MatSpec"), 0.3f, 0.3f, 0.4f);
      glUniform1f(phong->getUniform("shini"), 4.0f);
      break;
   case 2: //brass
      glUniform3f(phong->getUniform("MatAmb"), 0.3294f, 0.2235f, 0.02745f);
      glUniform3f(phong->getUniform("MatDif"), 0.7804f, 0.5686f, 0.11373f);
      glUniform3f(phong->getUniform("MatSpec"), 0.9922f, 0.941176f, 0.80784f);
      glUniform1f(phong->getUniform("shini"), 27.9f);
      break;
   case 3: //copper
      glUniform3f(phong->getUniform("MatAmb"), 0.1913f, 0.0735f, 0.0225f);
      glUniform3f(phong->getUniform("MatDif"), 0.7038f, 0.27048f, 0.0828f);
      glUniform3f(phong->getUniform("MatSpec"), 0.257f, 0.1376f, 0.08601f);
      glUniform1f(phong->getUniform("shini"), 12.8f);
      break;

   case 4: //perl
      glUniform3f(phong->getUniform("MatAmb"), 0.25f, 0.20725f, 0.20725f);
      glUniform3f(phong->getUniform("MatDif"), 1.0f, 0.829f, 0.829f);
      glUniform3f(phong->getUniform("MatSpec"), 0.296648f, 0.296648f, 0.296648f);
      glUniform1f(phong->getUniform("shini"), 11.264f);
      break;

   case 5: //tin
      glUniform3f(phong->getUniform("MatAmb"), 0.105882f, 0.058824f, 0.113725f);
      glUniform3f(phong->getUniform("MatDif"), 0.427451f, 0.470588f, 0.541176f);
      glUniform3f(phong->getUniform("MatSpec"), 0.333333f, 0.333333f, 0.521569f);
      glUniform1f(phong->getUniform("shini"), 9.84615f);
      break;

   case 6: //Polished gold
      glUniform3f(phong->getUniform("MatAmb"), 0.24725f, 0.2245f, 0.0645f);
      glUniform3f(phong->getUniform("MatDif"), 0.34615f, 0.3143f, 0.0903f);
      glUniform3f(phong->getUniform("MatSpec"), 0.797357f, 0.723991f, 0.208006f);
      glUniform1f(phong->getUniform("shini"), 83.2f);
      break;

   case 7: //cyan rubber
      glUniform3f(phong->getUniform("MatAmb"), 0.0f, 0.05f, 0.05f);
      glUniform3f(phong->getUniform("MatDif"), 0.4f, 0.5f, 0.5f);
      glUniform3f(phong->getUniform("MatSpec"), 0.04f, 0.7f, 0.7f);
      glUniform1f(phong->getUniform("shini"), 10.0f);
      break;

   case 8: //bronze
      glUniform3f(phong->getUniform("MatAmb"), 0.2125f, 0.1275f, 0.054f);
      glUniform3f(phong->getUniform("MatDif"), 0.714f, 0.4284f, 0.18144f);
      glUniform3f(phong->getUniform("MatSpec"), 0.393548f, 0.271906f, 0.166721f);
      glUniform1f(phong->getUniform("shini"), 25.6f);
      break;

   case 9: //Silver
      glUniform3f(phong->getUniform("MatAmb"), 0.19225f, 0.19225f, 0.19225f);
      glUniform3f(phong->getUniform("MatDif"), 0.50754f, 0.50754f, 0.50754f);
      glUniform3f(phong->getUniform("MatSpec"), 0.508273f, 0.508273f, 0.508273f);
      glUniform1f(phong->getUniform("shini"), 51.2f);
      break;

   }

}

void initFMOD() {
   /*try this???  soundSystem == instance of fmod */
   //init fmod instance
   if (FMOD::System_Create(&soundSystem) != FMOD_OK)      {
      // Report Error
      return;
   }
   int driverCount = 0;
   soundSystem->getNumDrivers(&driverCount);
   if (driverCount == 0)      {
      // Report Error
      //no speaker driver
      return;
   }
   // Initialize our Instance with sampleSize Channels
   soundSystem->init(sampleSize, FMOD_INIT_NORMAL, NULL);

   // create the sound, with loop mode
   //soundSystem->createSound("../resources/music/conqOfSpace.mp3", FMOD_HARDWARE, 0, &song);

   soundSystem->createChannelGroup(0, &channelMusic);

   //setting things to analyze
   soundSystem->createSound("../resources/music/iron.mp3", FMOD_SOFTWARE, 0, &analyzeSong);
   analyzeSong->setMode(FMOD_LOOP_NORMAL);
   analyzeSong->setLoopCount(-1);  //infinite looping
   soundSystem->playSound(FMOD_CHANNEL_FREE, analyzeSong, true, &songChannel);  //true means starts paused

   songChannel->setChannelGroup(channelMusic);

   hzRange = (44100.0f / 2) / (float)sampleSize; //44.1k is rec sampling rate for audible frequencies
}

static void init() {
   GLSL::checkVersion();

   //initFmod
   initFMOD();

   //camera Stuff
   lookAtPt = Vector3f(cos(alpha * radConvert) * cos(beta * radConvert), sin(alpha * radConvert), cos(alpha * radConvert) * cos((90 - beta) * radConvert));
   eye = Vector3f(0, 0, 5);
   gaze = lookAtPt - eye;
   w = -normalize(gaze);

   // Set background color.
   glClearColor(.15f, 0, .2f, 1.0f);
   // Enable z-buffer test.
   glEnable(GL_DEPTH_TEST);
   //alpha blending
   //glEnable(GL_BLEND);
   //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   //from class
   cube = make_shared<Shape>();
   cube->loadMesh(RESOURCE_DIR + "cube.obj");
   cube->resize();
   cube->init();

   //BB8 model by Willy Decarpentrie
   //sketchfab.com/models/6aff787c459a4e00a26ed11ac8f148a1#
   //model split by Anna Velasquez
   sphere = make_shared<Shape>();
   sphere->loadMesh(RESOURCE_DIR + "BB8Bod.obj");
   sphere->resize();
   sphere->init();

   //ring model by virtualBG
   //sketchfab.com/models/247159eef183450a81887b569ba838c9
   ring = make_shared<Shape>();
   ring->loadMesh(RESOURCE_DIR + "ring.obj");
   ring->resize();
   ring->init();

   // Initialize the GLSL phongram.
   phong = make_shared<Program>();
   phong->setVerbose(true);
   phong->setShaderNames(RESOURCE_DIR + "phong_vert.glsl", RESOURCE_DIR + "phong_frag.glsl");
   phong->init();
   phong->addUniform("P");
   phong->addUniform("M");
   phong->addUniform("V");
   phong->addUniform("MatAmb");
   phong->addUniform("MatDif");
   phong->addAttribute("vertPos");
   phong->addAttribute("vertNor");
   phong->addUniform("lightXpos");
   phong->addUniform("lightYpos");
   phong->addUniform("MatSpec");
   phong->addUniform("shini");
   phong->addUniform("viewNormals");

}

void updateFMOD() {
   //update FMOD
   soundSystem->update();

   rightSpec = new float[sampleSize];
   leftSpec = new float[sampleSize];
   avgSpec = new float[sampleSize];

   //for diff stereo channels
   songChannel->getSpectrum(leftSpec, sampleSize, 0, FMOD_DSP_FFT_WINDOW_RECT);
   songChannel->getSpectrum(rightSpec, sampleSize, 1, FMOD_DSP_FFT_WINDOW_RECT);

   //calc avg spectrum - note: maybe visualize separate spectrums too???
   for (int i = 0; i < sampleSize; i++)
      avgSpec[i] = (leftSpec[i] + rightSpec[i]) / 2;

   //just scale it up
   for (int i = 0; i < sampleSize; i++)
      avgSpec[i] *= 100;


   //floats in these spectrums are the dB range from 0 - 100
   //if (pause) {
   //   printf("channel: ");
   //   for (int i = 0; i < 10; i++)
   //      printf("%3.2f ", avgSpec[i]);
   //   printf("\n");
   //}
}

//maybe need this, maybe not??
static void cleanUp() {
   //clean up
   delete[] avgSpec;
   delete[] leftSpec;
   delete[] rightSpec;
}


static void render() {
   // Get current frame buffer size.
   int width, height;
   glfwGetFramebufferSize(window, &width, &height);
   glViewport(0, 0, width, height);

   // Clear framebuffer.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   //Use the matrix stack for Lab 6
   float aspect = width / (float)height;

   // Create the matrix stacks - please leave these alone for now
   auto P = make_shared<MatrixStack>();
   auto M = make_shared<MatrixStack>();
   auto V = make_shared<MatrixStack>();

   //calculate view matrix
   V->pushMatrix();
   V->loadIdentity();
   V->lookAt(eye, lookAtPt, upVec);

   // Apply perspective projection.
   P->pushMatrix();
   P->perspective(45.0f, aspect, 0.01f, 100.0f);

   phong->bind();
   
   //set perspective
   glUniformMatrix4fv(phong->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
   //set lighting
   glUniform1f(phong->getUniform("lightXpos"), lightXshift);
   glUniform1f(phong->getUniform("lightYpos"), lightYshift);
   //set view
   glUniformMatrix4fv(phong->getUniform("V"), 1, GL_FALSE, V->topMatrix().data());

  //update FMOD
   updateFMOD();
   
   if (mode == 0){ //AUDIO BARS
      float startX = -1.55f;
      //eye = Vector3f(0, 0, 5);
      //set view
      //glUniformMatrix4fv(phong->getUniform("V"), 1, GL_FALSE, V->topMatrix().data());

      for (int i = 0; i < sampleSize / 2 - 1; i++) {
         //BEAT DETECTION
         if (avgSpec[2] > 40) //snare??? the fuck is going onnnn
            SetMaterial(tin, 0);
         else if (avgSpec[0] > 85)  //bass???  all good, minor tweaks mayber
            SetMaterial(gold, 0);
         else
            SetMaterial(cyan, 0);

         M->pushMatrix();
         //draw scene here
         float y = avgSpec[i] / 50;
         if (pause == false) {
            y = 0.01f;
         }
         M->translate(Vector3f(startX, 0, 0));
         M->scale(Vector3f(.01f, y, .01f));
         glUniformMatrix4fv(phong->getUniform("M"), 1, GL_FALSE, M->topMatrix().data());
         cube->draw(phong);
         M->popMatrix();

         startX += .05f;
      }
   }   //AUDIO BAR END
   else if (mode == 1) { //planets or smething???
      //eye = Vector3f(0, 0, 6);
      //set view
      //glUniformMatrix4fv(phong->getUniform("V"), 1, GL_FALSE, V->topMatrix().data());
      int count = 0;

      for (int i = 0; i < 13; i++) {
         //set base radii
         float bigRad = .6f;
         float tinyRad = .2f;

         //animation shtuff
         float beatAdd = 0;
         float channelAdd = avgSpec[14 - i] / 50;
         nyoom += .03f;
         tilt = (i * 10.0f);

         if ((i % 2) == 0)
            dir = 1.0f;
         else
            dir = -1.0f;

         if (pause == false)
            channelAdd = 0;

         //BEAT DETECTION
         if (avgSpec[8] > 16 || snareCount > 0) { //snare 15 - 20
            SetMaterial(tin, 0);
            beatAdd = -.2f;
            snareCount++;
         }
         else if (avgSpec[0] > 88 && avgSpec[0] < 120 || bassCount > 0) { //bass all good, minor tweaks mayber
            SetMaterial(gold, 0);
            beatAdd = .2f;//fmin(30 / avgSpec[0], 30/900);
            bassCount++;
         }
         else
            SetMaterial(cyan, 0);
         //end beat detection

         M->pushMatrix();  //draw scene 
         if (i == 0) {  //bass orb
            M->scale(Vector3f(bigRad + beatAdd, bigRad + beatAdd, bigRad + beatAdd));
            glUniformMatrix4fv(phong->getUniform("M"), 1, GL_FALSE, M->topMatrix().data());
            sphere->draw(phong);
         }
         else {  //other frequencies
            SetMaterial(perl, 0);

            if (i % 3 == 0) {
               M->rotate((nyoom + count * 50) * dir, Vector3f(0, 1, 0));  //animation orbit
               M->translate(Vector3f((.9f + (count * .4f)) * dir, 0, 0));
               count++;
            }
            else if (i % 3 == 1) {
               M->rotate((nyoom + count * 50) * dir, Vector3f(0, 0, 1));  //animation orbit
               M->translate(Vector3f(0, (.9f + (count * .4f)) * dir, 0));
            }
            else {
               M->rotate((nyoom + count * 50) * dir, Vector3f(1, 0, 0));  //animation orbit
               M->translate(Vector3f(0, 0, (.9f + (count * .4f)) * dir));
            }

            M->scale(Vector3f(.5f, .5f, .5f));  //another bc too big???
            M->pushMatrix();
               //ring
               M->rotate(nyoom * dir, Vector3f(1,0,0));
               M->scale(Vector3f(.3f + channelAdd, .05f, .3f + channelAdd));
               glUniformMatrix4fv(phong->getUniform("M"), 1, GL_FALSE, M->topMatrix().data());
               ring->draw(phong);
            M->popMatrix();

            M->scale(Vector3f(tinyRad, tinyRad, tinyRad));
            glUniformMatrix4fv(phong->getUniform("M"), 1, GL_FALSE, M->topMatrix().data());
            sphere->draw(phong);
         }
         M->popMatrix();  //end scene

         if (bassCount > 55)  //how long to keep gold beat sphere
            bassCount = 0;

         if (snareCount > 55)  //how long to keep silver beat sphere
            snareCount = 0;
      } 
   } //planets or something end
   else if (mode == 2) {

   }
   //set view
   glUniformMatrix4fv(phong->getUniform("V"), 1, GL_FALSE, V->topMatrix().data());

   cleanUp();
   phong->unbind();

   // Pop matrix stacks.
   P->popMatrix();
   V->popMatrix();

}

int main(int argc, char **argv) {

   // Set error callback.
   glfwSetErrorCallback(error_callback);
   // Initialize the library.
   if (!glfwInit()) {
      return -1;
   }
   //request the highest possible version of OGL - important for mac
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

   // Create a windowed mode window and its OpenGL context.
   g_width = 1280;
   g_height = 960;
   window = glfwCreateWindow(1280, 960, "It's BananaAnna! (Anna Velasquez)", NULL, NULL);
   if (!window) {
      glfwTerminate();
      return -1;
   }
   // Make the window's context current.
   glfwMakeContextCurrent(window);
   // Initialize GLEW.
   glewExperimental = true;
   if (glewInit() != GLEW_OK) {
      cerr << "Failed to initialize GLEW" << endl;
      return -1;
   }
   //weird bootstrap of glGetError
   glGetError();
   cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
   cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

   // Set vsync.
   glfwSwapInterval(1);
   // Set keyboard callback.
   glfwSetKeyCallback(window, key_callback);
   //set the mouse call back
   glfwSetMouseButtonCallback(window, mouse_callback);
   //set the window resize call backs
   glfwSetFramebufferSizeCallback(window, resize_callback);
   //set cursor call back
   glfwSetCursorPosCallback(window, cursor_position_callback);
   //make cursor disapear
   //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

   // Initialize scene. Note geometry initialized in init now
   init();

   // Loop until the user closes the window.
   while (!glfwWindowShouldClose(window)) {
      // Render scene.
      render();
      // Swap front and back buffers.
      glfwSwapBuffers(window);
      // Poll for and process events.
      glfwPollEvents();
   }
   // Quit program.
   glfwDestroyWindow(window);
   glfwTerminate();
   return 0;
}
