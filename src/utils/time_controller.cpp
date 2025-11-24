#include "time_controller.hpp"

TimeController::TimeController()
	: _lastTime(SDL_GetPerformanceCounter())
	, _isPaused(false)
	, _speedCoefficient(1.0f)
	, _rawDeltaTime(0.016f) // Default to ~60fps
{
}

void TimeController::Update() {
	Uint64 currentTime = SDL_GetPerformanceCounter();
	_rawDeltaTime = (float)(currentTime - _lastTime) / (float)SDL_GetPerformanceFrequency();
	_lastTime = currentTime;

	// Cap dt to prevent huge jumps
	if (_rawDeltaTime > 0.1f) {
		_rawDeltaTime = 0.1f;
	}
}

float TimeController::GetDeltaTime() const {
	if (_isPaused) {
		return 0.0f;
	}
	return _rawDeltaTime * _speedCoefficient;
}

void TimeController::SetPaused(bool paused) {
	_isPaused = paused;
}

bool TimeController::IsPaused() const {
	return _isPaused;
}

void TimeController::SetSpeedCoefficient(float coefficient) {
	_speedCoefficient = coefficient;
}

float TimeController::GetSpeedCoefficient() const {
	return _speedCoefficient;
}

