#include "SimpleFMOD.h"

/*
SimpleFMOD - Library to enable simple use of basic FMOD features
Written by Katy Coe
(c) Katy Coe 2012, 2013 - No unauthorized copying or re-distribution
www.djkaty.com
*/

namespace SFMOD
{
	// Check for errors in FMOD commands
	void ErrorCheck(FMOD_RESULT result)
	{
		if (result != FMOD_OK)
		{
			std::cout << "FMOD error! (" << result << ") " << FMOD_ErrorString(result) << std::endl;
			exit(-1);
		}
	}

	// Initialize FMOD sound system
	SimpleFMOD::SimpleFMOD()
	{
		unsigned int version;
		int numDrivers;
		FMOD_SPEAKERMODE speakerMode;
		FMOD_CAPS caps;
		char name[256];

		// Create FMOD interface object
		ErrorCheck(FMOD::System_Create(&system));

		// Check version
		ErrorCheck(system->getVersion(&version));

		if (version < FMOD_VERSION)
		{
			std::cout << "Error! You are using an old version of FMOD " << version << ". This program requires " << FMOD_VERSION << std::endl;
			exit(-1);
		}
	
		// Get number of sound cards
		ErrorCheck(system->getNumDrivers(&numDrivers));
	
		// No sound cards (disable sound)
		if (numDrivers == 0)
			ErrorCheck(system->setOutput(FMOD_OUTPUTTYPE_NOSOUND));

		// At least one sound card
		else
		{
			// Get the capabilities of the default (0) sound card
			ErrorCheck(system->getDriverCaps(0, &caps, 0, &speakerMode));
	
			// Set the speaker mode to match that in Control Panel
			ErrorCheck(system->setSpeakerMode(speakerMode));
	
			// Increase buffer size if user has Acceleration slider set to off
			if (caps & FMOD_CAPS_HARDWARE_EMULATED)
			{
				ErrorCheck(system->setDSPBufferSize(1024, 10));
			}
	
			// Get name of driver
			ErrorCheck(system->getDriverInfo(0, name, 256, 0));
	
			// SigmaTel sound devices crackle for some reason if the format is PCM 16-bit.
			// PCM floating point output seems to solve it.
			if (strstr(name, "SigmaTel"))
				ErrorCheck(system->setSoftwareFormat(48000, FMOD_SOUND_FORMAT_PCMFLOAT, 0, 0, FMOD_DSP_RESAMPLER_LINEAR));
		}

		// Initialise FMOD
		FMOD_RESULT result = system->init(100, FMOD_INIT_NORMAL, 0);

		// If the selected speaker mode isn't supported by this sound card, swtich it back to stereo
		if (result == FMOD_ERR_OUTPUT_CREATEBUFFER)
		{
			ErrorCheck(system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO));
			result = system->init(100, FMOD_INIT_NORMAL, 0);
		}
		ErrorCheck(result);

		// Create two channel groups to allow master volume control
		// One for music, one for effects
		ErrorCheck(system->createChannelGroup(NULL, &channelMusic));
		ErrorCheck(system->createChannelGroup(NULL, &channelEffects));
	}

	// Release FMOD sound system
	SimpleFMOD::~SimpleFMOD()
	{
		system->release();
	}

	// Per-frame sound system update
	void SimpleFMOD::Update()
	{
		ErrorCheck(system->update());

		for (auto &r : updateableResources)
			r->Update();
	}

	// Register a resource for update (interal use only)
	void SimpleFMOD::registerResource(SimpleFMODResource *res)
	{
		updateableResources.push_back(res);
	}

	// Unregister a resource from update (interal use only)
	void SimpleFMOD::unregisterResource(SimpleFMODResource *res)
	{
		updateableResources.remove(res);
	}

	// Get and set master volumes
	float SimpleFMOD::GetMasterVolumeMusic()
	{
		float vol;
		channelMusic->getVolume(&vol);
		return vol;
	}

	float SimpleFMOD::GetMasterVolumeEffects()
	{
		float vol;
		channelEffects->getVolume(&vol);
		return vol;
	}

	void SimpleFMOD::SetMasterVolumeMusic(float vol)
	{
		vol = max(min(vol, 1.0f), 0.0f);

		channelMusic->setVolume(vol);
	}

	void SimpleFMOD::SetMasterVolumeEffects(float vol)
	{
		vol = max(min(vol, 1.0f), 0.0f);

		channelEffects->setVolume(vol);
	}

	// Song factory
	Song SimpleFMOD::LoadSong(const char *data, FMOD::ChannelGroup *channelGroup, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO info)
	{
		return Song(this, data, channelGroup, mode, info);
	}

	Song SimpleFMOD::LoadSong(const char *filename, FMOD_MODE mode)
	{
		return Song(this, filename, channelMusic, mode);
	}

	Song SimpleFMOD::LoadSong(int resourceId, LPCTSTR resourceType, FMOD_MODE mode)
	{
		return Song(this, resourceId, resourceType, channelMusic, mode);
	}

	// Sound effect factory
	SoundEffect SimpleFMOD::LoadSoundEffect(const char *filename, FMOD_MODE mode)
	{
		return SoundEffect(this, filename, channelEffects, mode);
	}

	SoundEffect SimpleFMOD::LoadSoundEffect(int resourceId, LPCTSTR resourceType, FMOD_MODE mode)
	{
		return SoundEffect(this, resourceId, resourceType, channelEffects, mode);
	}

	// Set up a song
	Song::Song(SimpleFMOD *fmod, const char *data, FMOD::ChannelGroup *cg, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO info) : SimpleFMODResource(fmod)
	{
		// Set stream size higher than the default (16384) to try to help reduce stuttering
		engine->FMOD()->setStreamBufferSize(65536, FMOD_TIMEUNIT_RAWBYTES);

		// Open the stream
		FMOD::Sound *s;
		ErrorCheck(engine->FMOD()->createStream(data, mode, &info, &s));
		resource = ResourceType(s);

		// Remember channel group
		channelGroup = cg;

		fade = false;
	}

	Song::Song(SimpleFMOD *fmod, const char *filename, FMOD::ChannelGroup *cg, FMOD_MODE mode) : SimpleFMODResource(fmod)
	{
		// Set stream size higher than the default (16384) to try to help reduce stuttering
		engine->FMOD()->setStreamBufferSize(65536, FMOD_TIMEUNIT_RAWBYTES);

		// Open the stream
		FMOD::Sound *s;
		ErrorCheck(engine->FMOD()->createStream(filename, mode, 0, &s));
		resource = ResourceType(s);

		// Remember channel group
		channelGroup = cg;

		fade = false;
	}

	Song::Song(SimpleFMOD *fmod, int resourceId, LPCTSTR resourceType, FMOD::ChannelGroup *cg, FMOD_MODE mode) : SimpleFMODResource(fmod)
	{
		HRSRC rsrc = FindResource(NULL, MAKEINTRESOURCE(resourceId), resourceType);
		HGLOBAL handle = LoadResource(NULL, rsrc);

		DWORD audioSize = SizeofResource(NULL, rsrc);
		LPVOID audioData = LockResource(handle);

		FMOD_CREATESOUNDEXINFO audioInfo;
		memset(&audioInfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
		audioInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		audioInfo.length = static_cast<unsigned int>(audioSize);

		engine->FMOD()->setStreamBufferSize(65536, FMOD_TIMEUNIT_RAWBYTES);

		FMOD::Sound *s;
		ErrorCheck(engine->FMOD()->createStream(static_cast<const char *>(audioData), FMOD_OPENMEMORY | mode, &audioInfo, &s));
		resource = ResourceType(s);

		// Remember channel group
		channelGroup = cg;

		fade = false;
	}

	// Update a song's fade status
	void Song::Update()
	{
		if (fade)
		{
			// Get fade progression from 0.0f - 1.0f depending on number of milliseconds elapsed since fade started
			float progress = min(static_cast<float>(GetTickCount() - fadeStartTick) / fadeLength, 1.0f);

			// Fade is over if progression is at 1.0f
			if (progress == 1.0f)
				fade = false;

			// Translate linear progress into a smooth sine-squared fade effect
			float volume = static_cast<float>(sin(progress * M_PI / 2));
			volume *= volume;

			// Scale volume between start and target volumes
			volume = volume * (fadeTargetVol - fadeStartVol) + fadeStartVol;

			// Alter song volume
			SetVolume(volume);

			// Post-fade processing
			if (!fade)
				SetPaused(fadePauseAfter);
		}
	}

	// Start playing a song
	FMOD::Channel *Song::Start(bool paused)
	{
		// Channel volume will be set to 1.0f (max) automatically
		ErrorCheck(engine->FMOD()->playSound(FMOD_CHANNEL_FREE, resource.get(), true, &channel));

		// Add to channel group (for master volume)
		if (channelGroup)
			channel->setChannelGroup(channelGroup);

		// Songs repeat forever by default
		channel->setLoopCount(-1);
		channel->setMode(FMOD_LOOP_NORMAL);

		// Flush buffer to ensure loop logic is executed
		channel->setPosition(0, FMOD_TIMEUNIT_MS);

		// Set paused or not as applicable
		if (!paused)
			channel->setPaused(paused);

		// Cancel any fade that was previously applied
		fade = false;

		return channel;
	}

	// Stop a song and free the channel
	void Song::Stop()
	{
		channel->stop();
		channel = NULL;
	}

	// Get the FMOD channel used by a song
	FMOD::Channel *Song::GetChannel()
	{
		return channel;
	}

	// Pause/unpause a song
	bool Song::TogglePause()
	{
		bool isPaused;
		channel->getPaused(&isPaused);
		channel->setPaused(!isPaused);
		return !isPaused;
	}

	bool Song::GetPaused()
	{
		bool paused;
		channel->getPaused(&paused);
		return paused;
	}

	void Song::SetPaused(bool paused)
	{
		channel->setPaused(paused);
	}

	// Set song volume
	void Song::SetVolume(float volume)
	{
		channel->setVolume(volume);
	}

	// Begin fading a song for ms milliseconds from the current volume to a target volume of 'target'
	void Song::Fade(int ms, float target, bool pauseWhenDone)
	{
		float currentVolume;

		fadeLength = ms;
		fadeStartTick = GetTickCount();

		channel->getVolume(&currentVolume);
		fadeStartVol = currentVolume;
		fadeTargetVol = target;
		fade = true;
		fadePauseAfter = pauseWhenDone;
	}

	// Prepare a sound effect
	SoundEffect::SoundEffect(SimpleFMOD *fmod, const char *filename, FMOD::ChannelGroup *cg, FMOD_MODE mode) : SimpleFMODResource(fmod)
	{
		FMOD::Sound *s;
		ErrorCheck(engine->FMOD()->createSound(filename, mode, 0, &s));
		resource = ResourceType(s);

		// Remember channel group
		channelGroup = cg;
	}

	SoundEffect::SoundEffect(SimpleFMOD *fmod, int resourceId, LPCTSTR resourceType, FMOD::ChannelGroup *cg, FMOD_MODE mode) : SimpleFMODResource(fmod)
	{
		HRSRC rsrc = FindResource(NULL, MAKEINTRESOURCE(resourceId), resourceType);
		HGLOBAL handle = LoadResource(NULL, rsrc);

		DWORD audioSize = SizeofResource(NULL, rsrc);
		LPVOID audioData = LockResource(handle);

		FMOD_CREATESOUNDEXINFO audioInfo;
		memset(&audioInfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
		audioInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		audioInfo.length = static_cast<unsigned int>(audioSize);

		FMOD::Sound *s;
		ErrorCheck(engine->FMOD()->createSound(static_cast<const char *>(audioData), FMOD_OPENMEMORY | mode, &audioInfo, &s));
		resource = ResourceType(s);

		// Remember channel group
		channelGroup = cg;
	}

	// Play a sound effect
	void SoundEffect::Play()
	{
		FMOD::Channel *channel;

		// Channel volume will be set to 1.0f (max) automatically
		ErrorCheck(engine->FMOD()->playSound(FMOD_CHANNEL_FREE, resource.get(), true, &channel));

		// Add to channel group (for master volume)
		if (channelGroup)
			channel->setChannelGroup(channelGroup);

		channel->setPaused(false);
	}
}
