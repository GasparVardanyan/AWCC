# include "AWCCFanBoostManagerInternal.h"

# include <string.h>

# include "AWCC.h"
# include "AWCCConfig.h"
# include "AWCCFanBoostPhaseManager.h"

static _Bool Pending (enum AWCCFan_t, enum AWCCFanBoostPhase_t);
static void SetPhase (enum AWCCFan_t, enum AWCCFanBoostPhase_t);
static void SetBoost (enum AWCCFan_t, AWCCBoost_t);
static void SetBoostByInterval (enum AWCCFan_t, int);
static void RegBoostIntervalByTemperature (enum AWCCFan_t, int);
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

	.Pending = & Pending,
	.SetPhase = & SetPhase,
	.SetBoost = & SetBoost,
	.SetBoostByInterval = & SetBoostByInterval,
	.RegBoostIntervalByTemperature = & RegBoostIntervalByTemperature,
	.LogTime = & LogTime,
};

_Bool Pending (enum AWCCFan_t fan, enum AWCCFanBoostPhase_t phase)
{
	_Bool pending = 1;

	switch (Internal.BoostInfos [fan].Phase) {
	case AWCCFanBoostPhaseUpShift :
		if (AWCCFanBoostPhaseNormal == phase) {
			pending = 0;
		}
		break;
	case AWCCFanBoostPhaseDisabled :
	case AWCCFanBoostPhaseInitial :
			pending = 0;
		break;
	}

	return pending;
}

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
	Internal.BoostInfos [fan].BoostSetTime = Internal.CurrentTime;
	// AWCC.SetFanBoost (fan, boost);
}

void SetBoostByInterval (enum AWCCFan_t fan, int interval)
{
	Internal.LogTime ();
	printf ("%s fan boost interval set to %d\n", AWCC.GetFanName (fan), interval);

	Internal.BoostInfos [fan].BoostIntervalCurrent = interval;

	AWCCBoost_t boost = Internal.Config->FanConfigs [fan].BoostIntervals [interval].Boost;
	if (AWCCFanBoostPhaseUpShift == Internal.BoostInfos [fan].Phase) {
		boost += Internal.Config->FanConfigs [fan].UpBoostShift;
	}

	Internal.SetBoost (fan, boost);
}

void RegBoostIntervalByTemperature (enum AWCCFan_t fan, int interval)
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
