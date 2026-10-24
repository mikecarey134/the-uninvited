#ifndef INTERACTION_SOUNDS_H
#define INTERACTION_SOUNDS_H

// The interaction system's sound cues, and the file and gain each plays (gain on top of the SFX volume setting).
enum class Cue { Select, Success, Failure, Locked, Target };

struct CueSound
{
	const char* file;
	float gain;
};

inline CueSound cueSound(Cue cue)
{
	switch (cue)
	{
	case Cue::Select:
		return { "assets/sounds/button-20.wav", 0.16f }; // the game's short button click, turned down to a tick under the outcome sounds
	// PLACEHOLDERS (tools/make_placeholder_sounds.py) until real cues are sourced:
	case Cue::Success:
		return { "assets/sounds/placeholder-success.wav", 1.0f }; // two rising notes
	case Cue::Failure:
		return { "assets/sounds/placeholder-failure.wav", 1.0f }; // low falling thud
	case Cue::Locked:
		return { "assets/sounds/placeholder-locked.wav", 1.0f }; // handle rattle
	case Cue::Target:
		return { "assets/sounds/placeholder-target.wav", 1.0f }; // faint ping
	}
	return { "", 0.0f };
}

#endif
