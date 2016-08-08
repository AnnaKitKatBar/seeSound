#include "fmod.hpp"
#include "fmod_errors.h"
#include <iostream>
#include <Windows.h>
#include <list>
#include <algorithm> // for find
#include <memory> // for unique_ptr in VS2012

#define _USE_MATH_DEFINES

#include <math.h>

namespace SFMOD
{
	class SimpleFMODResource;
	class Song;
	class SoundEffect;

	// Main API. Create a single instance of SimpleFMOD in your application
	class SimpleFMOD
	{
		// Allow resources to access registerResource() and unregisterResource()
		friend class SimpleFMODResource;

	public:
		SimpleFMOD();
		~SimpleFMOD();

		// Return pointer to FMOD API
		FMOD::System *FMOD() { return system; }

		// Per frame update
		void Update();

		// Load and register resources
		Song LoadSong(const char *data, FMOD::ChannelGroup *channelGroup, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO info);
		Song LoadSong(const char *filename, FMOD_MODE mode = FMOD_DEFAULT);
		Song LoadSong(int resourceId, LPCTSTR resourceType, FMOD_MODE mode = 0);
		SoundEffect LoadSoundEffect(const char *filename, FMOD_MODE mode = FMOD_DEFAULT);
		SoundEffect LoadSoundEffect(int resourceId, LPCTSTR resourceType, FMOD_MODE mode = 0);

		// Volume controls
		void SetMasterVolumeMusic(float vol);
		void SetMasterVolumeEffects(float vol);
		float GetMasterVolumeMusic();
		float GetMasterVolumeEffects();

	private:
		// FMOD API
		FMOD::System *system;

		// List of managed FMOD sounds
		std::list<SimpleFMODResource *> updateableResources;

		// Register/unregister a sound
		void registerResource(SimpleFMODResource *);
		void unregisterResource(SimpleFMODResource *);

		// Channel groups
		FMOD::ChannelGroup *channelMusic;
		FMOD::ChannelGroup *channelEffects;
	};

	// Function object for std::unique_ptr to automatically release FMOD resources
	struct ReleaseFMODResource
	{
		void operator()(FMOD::Sound *r) const
		{
			r->release();
		}
	};

	// Managed FMOD sound resource
	typedef std::unique_ptr<FMOD::Sound, ReleaseFMODResource> ResourceType;

	class SimpleFMODResource
	{
	protected:
		// Pointer to SimpleFMOD API
		SimpleFMOD *engine;

		// The sound being managed by this object
		ResourceType resource;

	private:
		// No copying allowed of this class or any derived class
		SimpleFMODResource(SimpleFMODResource const &o);
		SimpleFMODResource &operator=(SimpleFMODResource const &);

	protected:
		// Default constructors for when no resource has been assigned yet
		SimpleFMODResource() : engine(NULL) {}
		SimpleFMODResource(SimpleFMOD *fmod) : engine(fmod) { fmod->registerResource(this); }

		// Move constructor
		SimpleFMODResource(SimpleFMODResource &&o) : engine(o.engine), resource(std::move(o.resource))
		{
			o.engine->unregisterResource(&o);
			engine->registerResource(this);
		}

		// Move assignment operator
		SimpleFMODResource &operator=(SimpleFMODResource &&o)
		{
			if (this != &o)
			{
				engine = o.engine;
				resource = std::move(o.resource);
				o.engine->unregisterResource(&o);
				engine->registerResource(this);
			}
			return *this;
		}

	public:
		// Get raw pointer to resource
		FMOD::Sound *Get() const { return resource.get(); }

		// User-definable per-frame update function
		virtual void Update() {}
	};

	// Song: Example SimpleFMOD resource. Played as a stream. Uses 'channelMusic' channel group. Stores channel. Plays in a loop.
	class Song : public SimpleFMODResource
	{
	private:
		// Fade parameters
		int fadeStartTick;
		int fadeLength;
		float fadeStartVol;
		float fadeTargetVol;
		bool fade;
		bool fadePauseAfter;

		// Channel and channel group
		FMOD::Channel *channel;
		FMOD::ChannelGroup *channelGroup;

	public:
		// Constructor
		Song() {}
		Song(SimpleFMOD *fmod, const char *data, FMOD::ChannelGroup *channelGroup, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO info);
		Song(SimpleFMOD *fmod, const char *filename, FMOD::ChannelGroup *channelGroup = NULL, FMOD_MODE mode = FMOD_DEFAULT);
		Song(SimpleFMOD *fmod, int resource, LPCTSTR resourceType, FMOD::ChannelGroup *channelGroup = NULL, FMOD_MODE mode = 0);

		// Move constructor
		Song(Song &&o) : SimpleFMODResource(std::move(o)), channel(o.channel), fade(false) {}
		Song &operator=(Song &&o) { if (this != &o) { this->SimpleFMODResource::operator=(std::move(o)); channel = o.channel; channelGroup = o.channelGroup; fade = false; } return *this; }

		// Sound controls
		FMOD::Channel *Start(bool paused = false);
		void Stop();
		bool TogglePause();
		bool GetPaused();
		void SetPaused(bool pause);
		void SetVolume(float volume);
		void Fade(int ms, float target = 0.0f, bool pauseWhenDone = true);

		// Retrieve the sound's FMOD channel
		FMOD::Channel *GetChannel();

		// Per-frame update
		virtual void Update();
	};

	// SoundEffect: Example SimpleFMOD resource. Played directly (not a stream). Uses 'channelEffects' channel group. Does not store channel. Plays one-shot.
	class SoundEffect : public SimpleFMODResource
	{
	private:
		FMOD::ChannelGroup *channelGroup;

	public:
		// Constructor
		SoundEffect() {}
		SoundEffect(SimpleFMOD *fmod, const char *filename, FMOD::ChannelGroup *channelGroup = NULL, FMOD_MODE mode = FMOD_DEFAULT);
		SoundEffect(SimpleFMOD *fmod, int resource, LPCTSTR resourceType, FMOD::ChannelGroup *channelGroup = NULL, FMOD_MODE mode = 0);

		// Move constructor
		SoundEffect(SoundEffect &&o) : SimpleFMODResource(std::move(o)) {}
		SoundEffect &operator=(SoundEffect &&o) { if (this != &o) { this->SimpleFMODResource::operator=(std::move(o)); channelGroup = o.channelGroup; } return *this; }

		void Play();
	};
}
