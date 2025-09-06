# include "AWCCFanBoostManagerInternal.h"

# include <string.h>

# include "AWCC.h"
# include "AWCCConfig.h"
# include "AWCCFanBoostPhaseManager.h"

static void SetPhase (enum AWCCFan_t, enum AWCCFanBoostPhase_t);
static void SetBoost (enum AWCCFan_t, AWCCBoost_t);
static void SetBoostIntervalByTemperature (enum AWCCFan_t, int);
static _Bool PendingStateSatisfied (enum AWCCFan_t);
static _Bool UpShiftTimePassed (enum AWCCFan_t);
static _Bool BoostDownTimeSatisfied (enum AWCCFan_t);

static void LogTime (void);


struct AWCCFanBoostManagerInternal_t Internal = {
	.BoostInfos = {
		[AWCCFanCPU] = {
			.Phase = AWCCFanBoostPhaseDisabled,
			.BoostPendingState = AWCCBoostPendingNone
		},
		[AWCCFanGPU] = {
			.Phase = AWCCFanBoostPhaseDisabled,
			.BoostPendingState = AWCCBoostPendingNone
		},
	},
	.ConfigUtils = {
		.PendingStateSatisfied = & PendingStateSatisfied,
		.UpShiftTimePassed = & UpShiftTimePassed,
	},
	.SystemLogger = NULL,

	.FanPairs = {
		[AWCCFanCPU] = AWCCFanGPU,
		[AWCCFanGPU] = AWCCFanCPU,
	},

	.SetPhase = & SetPhase,
	.SetBoost = & SetBoost,
	.SetBoostIntervalByTemperature = & SetBoostIntervalByTemperature,
	.LogTime = & LogTime,
};

void SetPhase (enum AWCCFan_t fan, enum AWCCFanBoostPhase_t phase)
{
	Internal.LogTime ();
	printf ("Fan %s entered phase %s\n", AWCC.GetFanName (fan), AWCCFanBoostPhaseNames [phase]);

	Internal.BoostInfos [fan].PhaseSetTime = Internal.CurrentTime;
	Internal.BoostInfos [fan].Phase = phase;
	AWCCFanBoostPhaseManager [phase].InitializePhase (fan);
}

void SetBoost (enum AWCCFan_t fan, AWCCBoost_t boost)
{
	Internal.LogTime ();
	printf ("%s fan boost set to %d\n", AWCC.GetFanName (fan), boost);

	Internal.BoostInfos [fan].Boost = boost;
	// FIXME:
	// AWCC.SetFanBoost (fan, boost);
}

void SetBoostIntervalByTemperature (enum AWCCFan_t fan, int interval)
{
	if (Internal.BoostInfos [fan].BoostIntervalByTemperature < interval) {
		if (AWCCBoostPendingUp != Internal.BoostInfos [fan].BoostPendingState) {
			Internal.BoostInfos [fan].BoostPendingState = AWCCBoostPendingUp;
			Internal.BoostInfos [fan].BoostPendingTime = Internal.CurrentTime;
		}
	}
	else if (Internal.BoostInfos [fan].BoostIntervalByTemperature > interval) {
		if (AWCCBoostPendingDown != Internal.BoostInfos [fan].BoostPendingState) {
			Internal.BoostInfos [fan].BoostPendingState = AWCCBoostPendingDown;
			Internal.BoostInfos [fan].BoostPendingTime = Internal.CurrentTime;
		}
	}
	else {
		if (AWCCBoostPendingNone != Internal.BoostInfos [fan].BoostPendingState) {
			Internal.BoostInfos [fan].BoostPendingState = AWCCBoostPendingNone;
			Internal.BoostInfos [fan].BoostPendingTime = Internal.CurrentTime;
		}
		goto skip_boost_registration;
	}

	Internal.BoostInfos [fan].BoostIntervalByTemperature = interval;
	Internal.BoostInfos [fan].BoostIntervalByTemperatureSetTime = Internal.CurrentTime;

skip_boost_registration:;
}

_Bool PendingStateSatisfied (enum AWCCFan_t fan)
{
	_Bool satisfied = 0;

	if (difftime (Internal.CurrentTime, Internal.BoostInfos [fan].BoostPendingTime) >= Internal.Config->FanConfigs [fan].PendingTime) {
		satisfied = 1;
	}

	return satisfied;
}

_Bool UpShiftTimePassed (enum AWCCFan_t fan)
{
	_Bool satisfied = 0;

	if (difftime (Internal.CurrentTime, Internal.BoostInfos [fan].PhaseSetTime) >= Internal.Config->FanConfigs [fan].UpBoostShiftTime) {
		satisfied = 1;
	}

	return satisfied;
}

void LogTime (void) {
	const char * time_str = ctime (& Internal.CurrentTime);
	printf ("[%.*s] ", (int) strlen (time_str) - 1, time_str);
}
