#include "fmod.hpp"
#include "fmod_errors.h" // Only if you want error checking

typedef FMOD::Sound* SoundClass;

class SoundSystemClass
{
   // Pointer to the FMOD instance
   FMOD::System *m_pSystem;

   SoundSystemClass()
   {
      if (FMOD::System_Create(&m_pSystem) != FMOD_OK)
      {
         // Report Error
         return;
      }

      int driverCount = 0;
      m_pSystem->getNumDrivers(&driverCount);

      if (driverCount == 0)
      {
         // Report Error
         return;
      }

      // Initialize our Instance with 36 Channels
      m_pSystem->init(36, FMOD_INIT_NORMAL, 0);
   }

   void createSound(SoundClass *pSound, const char* pFile)
   {
      m_pSystem->createSound(pFile, FMOD_HARDWARE, 0, pSound);
   }

   void playSound(SoundClass pSound, bool bLoop = false)
   {
      if (!bLoop)
         pSound->setMode(FMOD_LOOP_OFF);
      else
      {
         pSound->setMode(FMOD_LOOP_NORMAL);
         pSound->setLoopCount(-1);
      }

      m_pSystem->playSound(FMOD_CHANNEL_FREE, pSound, false, 0);
   }

   void releaseSound(SoundClass pSound)
   {
      pSound->release();
   }
};