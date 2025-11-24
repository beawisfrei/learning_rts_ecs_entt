#pragma once

#include <SDL3/SDL.h>

class TimeController {
public:
	TimeController();

	// Update internal time tracking (call each frame)
	void Update();

	// Get the modified delta time (0 if paused, otherwise raw_dt * coefficient)
	float GetDeltaTime() const;

	// Pause control
	void SetPaused(bool paused);
	bool IsPaused() const;

	// Speed control
	void SetSpeedCoefficient(float coefficient);
	float GetSpeedCoefficient() const;

private:
	Uint64 _lastTime;
	bool _isPaused;
	float _speedCoefficient;
	float _rawDeltaTime;
};

