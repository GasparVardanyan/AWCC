# include "AWCCFanBoostManager.h"

# include <string.h>
# include <threads.h>

# include "AWCC.h"
# include "AWCCSystemLogger.h"
# include "AWCCFanBoostManagerInternal.h"
# include "AWCCFanBoostPhaseManager.h"

static void Manage (enum AWCCFan_t);
static void Reset (enum AWCCFan_t);
static void SetTemperature (enum AWCCFan_t, AWCCTemperature_t);
static void SetTime (time_t);
static void SetConfig (const struct AWCCConfig_t *);
static void SetPowerState (enum AWCCPowerState_t);
static void Loop (void);

const struct AWCCFanBoostManager_t AWCCFanBoostManager = {
	.Manage = & Manage,
	.Reset = & Reset,
	.SetTemperature = & SetTemperature,
	.SetTime = & SetTime,
	.SetConfig = & SetConfig,
	.SetPowerState = & SetPowerState,
	.Loop = & Loop,
};

static _Bool CanChangeFromDisabledToInitial (enum AWCCFan_t);
static _Bool CanChangeFromInitialToUpshift (enum AWCCFan_t);
static _Bool CanChangeFromUpShiftToNormal (enum AWCCFan_t);
static _Bool CanChangeFromUpShiftToUpShift (enum AWCCFan_t);

static _Bool (*
	CanChangeFromTo [AWCCFanBoostPhaseCount] [AWCCFanBoostPhaseCount]
) (enum AWCCFan_t) = {
	[AWCCFanBoostPhaseDisabled] = {
		[AWCCFanBoostPhaseInitial] = & CanChangeFromDisabledToInitial,
	},
	[AWCCFanBoostPhaseInitial] = {
		[AWCCFanBoostPhaseUpShift] = & CanChangeFromInitialToUpshift,
	},
	[AWCCFanBoostPhaseUpShift] = {
		[AWCCFanBoostPhaseNormal] = & CanChangeFromUpShiftToNormal,
		[AWCCFanBoostPhaseUpShift] = & CanChangeFromUpShiftToUpShift,
	}
};

_Bool CanChangeFromDisabledToInitial (enum AWCCFan_t fan) {
	return 1;
}

_Bool CanChangeFromInitialToUpshift (enum AWCCFan_t fan) {
	return 1;
}

_Bool CanChangeFromUpShiftToNormal (enum AWCCFan_t fan) {
	_Bool can = 0;
	if (AWCCBoostPendingUp != Internal.BoostInfos [fan].BoostPendingState) {
		if (1 == Internal.ConfigUtils.UpShiftTimePassed (fan)) {
			can = 1;
		}
	}
	return can;
}

_Bool CanChangeFromUpShiftToUpShift (enum AWCCFan_t fan) {
	return 0;
}

void Manage (enum AWCCFan_t fan)
{
	if (NULL != CanChangeFromTo [Internal.BoostInfos [fan].Phase]) {
		for (int i = 0; i < AWCCFanBoostPhaseCount; i++) {
			enum AWCCFanBoostPhase_t nextPhase = AWCCFanBoostPhaseManager [Internal.BoostInfos [fan].Phase].NextPhasePriority [i];

			if (AWCCFanBoostPhaseNone != CanChangeFromTo [Internal.BoostInfos [fan].Phase] [nextPhase]) {
				if ((* CanChangeFromTo [Internal.BoostInfos [fan].Phase] [nextPhase]) (fan)) {
					Internal.SetPhase (fan, nextPhase);
					// NOTE: AWCCFanBoostPhaseManager [nextPhase].InitializePhase (fan) is called here
					break;
				}
			}
			else {
				break;
			}
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
					Internal.RegBoostIntervalByTemperature (fan, interval);
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

