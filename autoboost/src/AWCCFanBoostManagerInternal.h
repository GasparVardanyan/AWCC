# ifndef __AWCC_FANBOOSTMANAGER_INTERNAL_H
# define __AWCC_FANBOOSTMANAGER_INTERNAL_H

# include <time.h>

# include "AWCC.h"

enum AWCCFanBoostPhase_t {
	AWCCFanBoostPhaseNone = 0,                // Invalid phase used to automatically mark the end of NextPhasePriority
	AWCCFanBoostPhaseDisabled,                // Auto Boost Is Disabled and Control commands are processed if any
	AWCCFanBoostPhaseInitial,                 // Probably not needed
	AWCCFanBoostPhaseUpShift,                 //
	AWCCFanBoostPhaseNormal,                  //
	AWCCFanBoostPhaseShiftToLower,            //
	AWCCFanBoostPhaseHelping,                 //
};

static const char * AWCCFanBoostPhaseNames [] = {
	[AWCCFanBoostPhaseDisabled] = "Disabled",
	[AWCCFanBoostPhaseInitial] = "Initial",
	[AWCCFanBoostPhaseUpShift] = "UpShift",
	[AWCCFanBoostPhaseNormal] = "Normal",
	[AWCCFanBoostPhaseShiftToLower] = "ShiftToLower",
	[AWCCFanBoostPhaseHelping] = "Helping",
};

enum {
	AWCCFanBoostPhaseCount = 7
};

struct AWCCFanBoostManagerInternal_t {
	struct {
		AWCCTemperature_t Temperature;
		AWCCBoost_t Boost;
		int BoostIntervalByTemperature;
		int BoostIntervalCurrent;
		enum {
			AWCCBoostPendingNone,
			AWCCBoostPendingUp,
			AWCCBoostPendingDown,
		} BoostPendingState;
		time_t BoostPendingTime;
		time_t BoostIntervalByTemperatureSetTime;
		time_t PhaseSetTime;
		time_t BoostSetTime;
		enum AWCCFanBoostPhase_t Phase;
	} BoostInfos [2];

	struct {
		_Bool (* PendingStateSatisfied) (enum AWCCFan_t);
		_Bool (* UpShiftTimePassed) (enum AWCCFan_t);
	} ConfigUtils;

	time_t CurrentTime;

	const struct AWCCConfig_t * Config;
	const struct AWCCConfig_t * Configs [2];
	enum AWCCPowerState_t PowerState;
	const struct AWCCSystemLogger_t * SystemLogger;

	enum AWCCFan_t FanPairs [2];

	_Bool (* Pending) (enum AWCCFan_t, enum AWCCFanBoostPhase_t);
	void (* SetPhase) (enum AWCCFan_t, enum AWCCFanBoostPhase_t);
	void (* SetBoost) (enum AWCCFan_t, AWCCBoost_t);
	void (* SetBoostByInterval) (enum AWCCFan_t, int);
	void (* RegBoostIntervalByTemperature) (enum AWCCFan_t, AWCCBoost_t);
	void (* LogTime) (void);
} extern Internal;

# endif // __AWCC_FANBOOSTMANAGER_INTERNAL_H
