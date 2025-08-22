# include "AWCCFanBoostManager.h"

# include <string.h>
# include <threads.h>

# include "AWCC.h"
#include "AWCCSystemLogger.h"

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

struct AWCCFanBoostPhaseManager_t {
	enum AWCCFanBoostPhase_t NextPhasePriority [AWCCFanBoostPhaseCount];
	_Bool (* CanChangeTo) (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
	_Bool (* CanChangeFrom) (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
	void (* InitializePhase) (enum AWCCFan_t);
	void (* ManagePhase) (enum AWCCFan_t);
} extern const AWCCFanBoostPhaseManager [AWCCFanBoostPhaseCount];

static void Manage (enum AWCCFan_t);
static void Reset (enum AWCCFan_t);
static void SetTemperature (enum AWCCFan_t, AWCCTemperature_t);
static void SetPhase (enum AWCCFan_t, enum AWCCFanBoostPhase_t phase);
static void SetBoost (enum AWCCFan_t, AWCCBoost_t);
static void SetTime (time_t);
static void SetConfig (const struct AWCCConfig_t *);
static void SetPowerState (enum AWCCPowerState_t);
static void Loop (void);

static void LogTime (void);

const struct AWCCFanBoostManager_t AWCCFanBoostManager = {
	.Manage = & Manage,
	.Reset = & Reset,
	.SetTemperature = & SetTemperature,
	.SetTime = & SetTime,
	.SetConfig = & SetConfig,
	.SetPowerState = & SetPowerState,
	.Loop = & Loop,
};

struct {
	struct {
		AWCCTemperature_t Temperature;
		AWCCBoost_t Boost;
		int BoostIntervalByTemperature;
		int BoostIntervalCurrent;
		time_t PhaseSetTime;
		enum AWCCFanBoostPhase_t Phase;
		struct {
			_Bool ShiftActive;
		} UpShiftInfo;
	} BoostInfos [2];

	time_t CurrentTime;

	const struct AWCCConfig_t * Config;
	const struct AWCCConfig_t * Configs [2];
	enum AWCCPowerState_t PowerState;
	const struct AWCCSystemLogger_t * SystemLogger;

	void (* SetPhase) (enum AWCCFan_t, enum AWCCFanBoostPhase_t);
	void (* SetBoost) (enum AWCCFan_t, AWCCBoost_t);
	void (* LogTime) (void);
} static Internal = {
	.BoostInfos = {
		[AWCCFanCPU] = { .Phase = AWCCFanBoostPhaseDisabled, },
		[AWCCFanGPU] = { .Phase = AWCCFanBoostPhaseDisabled, },
	},
	.SystemLogger = NULL,

	.SetPhase = & SetPhase,
	.SetBoost = & SetBoost,
	.LogTime = & LogTime,
};

void LogTime (void) {
	const char * time_str = ctime (& Internal.CurrentTime);
	printf ("[%.*s] ", (int) strlen (time_str) - 1, time_str);
}

void Manage (enum AWCCFan_t fan)
{
	for (int i = 0; i < AWCCFanBoostPhaseCount; i++) {
		enum AWCCFanBoostPhase_t nextPhase = AWCCFanBoostPhaseManager [Internal.BoostInfos [fan].Phase].NextPhasePriority [i];

		if (AWCCFanBoostPhaseNone != nextPhase) {
			if (
				   AWCCFanBoostPhaseManager [Internal.BoostInfos [fan].Phase].CanChangeTo (nextPhase, fan)
				&& AWCCFanBoostPhaseManager [nextPhase].CanChangeFrom (Internal.BoostInfos [fan].Phase, fan)
			) {
				Internal.SetPhase (fan, nextPhase);
				// NOTE: AWCCFanBoostPhaseManager [nextPhase].InitializePhase (fan) is called here
				break;
			}
		}
		else {
			break;
		}
	}

	AWCCFanBoostPhaseManager [Internal.BoostInfos [fan].Phase].ManagePhase (fan);
}

void Reset (enum AWCCFan_t fan)
{
	if (AWCCFanBoostPhaseDisabled != Internal.BoostInfos [fan].Phase) {
		Internal.SetPhase (fan, AWCCFanBoostPhaseInitial);
	}
}

void SetTemperature (enum AWCCFan_t fan, AWCCTemperature_t temperature)
{
	Internal.BoostInfos [fan].Temperature = temperature;
}

void SetTime (time_t time)
{
	Internal.CurrentTime = time;
}

void SetConfig (const struct AWCCConfig_t * config)
{
	Internal.Config = config;
}

void SetPowerState (enum AWCCPowerState_t state)
{
	if (Internal.PowerState != state) {
		Internal.PowerState = state;
		AWCCFanBoostManager.SetConfig (Internal.Configs [state]);

		AWCCFanBoostManager.Reset (AWCCFanCPU);
		AWCCFanBoostManager.Reset (AWCCFanGPU);
	}
}

void Loop (void)
{
	struct AWCCConfig_t conf_ac = AWCCDefaultConfigAC ();
	struct AWCCConfig_t conf_bat = AWCCDefaultConfigBAT ();

	Internal.Configs [AWCCPowerStateAC] = & conf_ac; // TODO: Set outside
	Internal.Configs [AWCCPowerStateBAT] = & conf_bat; // TODO: Set outside

	Internal.SystemLogger = & AWCCSystemLoggerDefault,

	AWCCFanBoostManager.Reset (AWCCFanCPU);
	AWCCFanBoostManager.Reset (AWCCFanGPU);
	AWCCFanBoostManager.SetPowerState (AWCC.PowerState ());
	AWCCFanBoostManager.SetConfig (Internal.Configs [Internal.PowerState]);

	enum AWCCFan_t fans [2] = {AWCCFanCPU, AWCCFanGPU};

	while (1) {
		AWCCFanBoostManager.SetPowerState (AWCC.PowerState ());
		AWCCFanBoostManager.SetTime (time (NULL));

		for (int fan_i = 0; fan_i < 2; fan_i++) {
			enum AWCCFan_t fan = fans [fan_i];
			AWCCFanBoostManager.SetTemperature (fan, AWCC.GetFanTemperature (fan));

			for (int interval = 0; interval < Internal.Config->FanConfigs [fan]._BoostIntervalCount; interval++) {
				if (
					Internal.Config->FanConfigs [fan].BoostIntervals [interval].TemperatureRange.Min <= Internal.BoostInfos [fan].Temperature  &&
					Internal.Config->FanConfigs [fan].BoostIntervals [interval].TemperatureRange.Max >= Internal.BoostInfos [fan].Temperature
				) {
					Internal.BoostInfos [fan].BoostIntervalByTemperature = interval;
					break;
				}
			}
		}

		AWCCFanBoostManager.Manage (AWCCFanCPU);
		AWCCFanBoostManager.Manage (AWCCFanGPU);

		if (NULL != Internal.SystemLogger) { // TODO: Separate system logger data in Internal
			Internal.SystemLogger->LogCpuTemp (Internal.BoostInfos [AWCCFanCPU].Temperature);
			Internal.SystemLogger->LogGpuTemp (Internal.BoostInfos [AWCCFanGPU].Temperature);
			Internal.SystemLogger->LogCpuBoost (Internal.BoostInfos [AWCCFanCPU].Boost);
			Internal.SystemLogger->LogGpuBoost (Internal.BoostInfos [AWCCFanGPU].Boost);
			// if (AWCCModeG != Internal.ModeInfo.Mode) {
			// 	Internal.SystemLogger->LogCpuBoost (Internal.BoostInfos [AWCCFanCPU].Boost);
			// 	Internal.SystemLogger->LogGpuBoost (Internal.BoostInfos [AWCCFanGPU].Boost);
			// }
			// else {
			// 	Internal.SystemLogger->LogCpuBoost (AWCC.GetCpuBoost ());
			// 	Internal.SystemLogger->LogGpuBoost (AWCC.GetGpuBoost ());
			// }
			// Internal.SystemLogger->LogMode (Internal.ModeInfo.Mode);
			Internal.SystemLogger->LogCpuRpm (AWCC.GetCpuRpm ());
			Internal.SystemLogger->LogGpuRpm (AWCC.GetGpuRpm ());
		}

		thrd_sleep (& (struct timespec) {
			.tv_sec = Internal.Config->TemperatureCheckInterval
		}, NULL);
	}
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
	AWCC.SetFanBoost (fan, boost);
}

static _Bool CanChangeFromDisabledTo (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static _Bool CanChangeToDisabledFrom (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static void InitializePhase_Disabled (enum AWCCFan_t);
static void ManagePhase_Disabled (enum AWCCFan_t);

static _Bool CanChangeFromInitialTo (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static _Bool CanChangeToInitialFrom (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static void InitializePhase_Initial (enum AWCCFan_t);
static void ManagePhase_Initial (enum AWCCFan_t);

static _Bool CanChangeFromUpShiftTo (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static _Bool CanChangeToUpShiftFrom (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static void InitializePhase_UpShift (enum AWCCFan_t);
static void ManagePhase_UpShift (enum AWCCFan_t);

static _Bool CanChangeFromNormalTo (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static _Bool CanChangeToNormalFrom (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static void InitializePhase_Normal (enum AWCCFan_t);
static void ManagePhase_Normal (enum AWCCFan_t);

const struct AWCCFanBoostPhaseManager_t AWCCFanBoostPhaseManager [] = {
	[AWCCFanBoostPhaseDisabled] = {
		.NextPhasePriority = {
			AWCCFanBoostPhaseDisabled,
			AWCCFanBoostPhaseInitial,
		},
		.CanChangeTo = & CanChangeFromDisabledTo,
		.CanChangeFrom = & CanChangeToDisabledFrom,
		.InitializePhase = & InitializePhase_Disabled,
		.ManagePhase = & ManagePhase_Disabled,
	},
	[AWCCFanBoostPhaseInitial] = {
		.NextPhasePriority = {
			AWCCFanBoostPhaseUpShift,
			AWCCFanBoostPhaseHelping,
		},
		.CanChangeTo = & CanChangeFromInitialTo,
		.CanChangeFrom = & CanChangeToInitialFrom,
		.InitializePhase = & InitializePhase_Initial,
		.ManagePhase = & ManagePhase_Initial,
	},
	[AWCCFanBoostPhaseUpShift] = {
		.NextPhasePriority = {
			AWCCFanBoostPhaseDisabled,
			AWCCFanBoostPhaseUpShift,
			AWCCFanBoostPhaseNormal,
			AWCCFanBoostPhaseHelping,
		},
		.CanChangeTo = & CanChangeFromUpShiftTo,
		.CanChangeFrom = & CanChangeToUpShiftFrom,
		.InitializePhase = & InitializePhase_UpShift,
		.ManagePhase = & ManagePhase_UpShift,
	},
	[AWCCFanBoostPhaseNormal] = {
		.NextPhasePriority = {
			AWCCFanBoostPhaseDisabled,
			AWCCFanBoostPhaseUpShift,
			AWCCFanBoostPhaseNormal,
			AWCCFanBoostPhaseShiftToLower,
			AWCCFanBoostPhaseHelping,
		},
		.CanChangeTo = & CanChangeFromNormalTo,
		.CanChangeFrom = & CanChangeToNormalFrom,
		.InitializePhase = & InitializePhase_Normal,
		.ManagePhase = & ManagePhase_Normal,
	},
	[AWCCFanBoostPhaseShiftToLower] = {
		.NextPhasePriority = {
			AWCCFanBoostPhaseDisabled,
			AWCCFanBoostPhaseUpShift,
			AWCCFanBoostPhaseShiftToLower,
			AWCCFanBoostPhaseHelping,
		},
		.CanChangeTo = & CanChangeFromNormalTo,
		.CanChangeFrom = & CanChangeToNormalFrom,
		.InitializePhase = & InitializePhase_Normal,
		.ManagePhase = & ManagePhase_Normal,
	},
	[AWCCFanBoostPhaseHelping] = {
		.NextPhasePriority = {
			AWCCFanBoostPhaseDisabled,
			AWCCFanBoostPhaseUpShift,
		},
		.CanChangeTo = & CanChangeFromNormalTo,
		.CanChangeFrom = & CanChangeToNormalFrom,
		.InitializePhase = & InitializePhase_Normal,
		.ManagePhase = & ManagePhase_Normal,
	},
};

_Bool CanChangeFromDisabledTo (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	_Bool can = 0;

	switch (phase) {
		case AWCCFanBoostPhaseInitial :
			can = 1;
			break;
		default :
			can = 0;
			break;
	}

	return can;
}

_Bool CanChangeToDisabledFrom (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	return 1;
}

void InitializePhase_Disabled (enum AWCCFan_t fan)
{
	if (AWCCModeG != AWCC.GetMode ()) {
		Internal.SetBoost (AWCCFanCPU, Internal.Config->NoBoostConf.FanBoosts [AWCCFanCPU]);
		Internal.SetBoost (AWCCFanGPU, Internal.Config->NoBoostConf.FanBoosts [AWCCFanGPU]);
	}
}

void ManagePhase_Disabled (enum AWCCFan_t fan)
{

}

_Bool CanChangeFromInitialTo (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	_Bool can = 0;

	if (AWCCFanBoostPhaseUpShift == phase) {
		can = 1;
	}

	return can;
}

_Bool CanChangeToInitialFrom (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	_Bool can = 0;

	switch (phase) {
		case AWCCFanBoostPhaseDisabled :
			can = 1;
			break;
		default :
			can = 0;
			break;
	}

	return can;
}

void InitializePhase_Initial (enum AWCCFan_t fan)
{

}

void ManagePhase_Initial (enum AWCCFan_t fan)
{

}

_Bool CanChangeFromUpShiftTo (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	return 0;
}

_Bool CanChangeToUpShiftFrom (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	_Bool can = 0;

	if (
		   AWCCFanBoostPhaseUpShift != phase
		|| Internal.BoostInfos [fan].BoostIntervalByTemperature != Internal.BoostInfos [fan].BoostIntervalCurrent
	) {
		can = 1;
	}

	return can;
}

void InitializePhase_UpShift (enum AWCCFan_t fan)
{
}

void ManagePhase_UpShift (enum AWCCFan_t fan)
{
}

_Bool CanChangeFromNormalTo (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	return 0;
}

_Bool CanChangeToNormalFrom (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	return 0;
}

void InitializePhase_Normal (enum AWCCFan_t fan)
{
}

void ManagePhase_Normal (enum AWCCFan_t fan)
{
}
