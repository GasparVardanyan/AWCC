# include "AWCCFanBoostManager.h"
# include "AWCC.h"
# include <threads.h>



enum AWCCFanBoostPhase_t {
	AWCCFanBoostPhaseNone = 0,                // Invalid phase used to automatically mark the end of NextPhasePriority
	AWCCFanBoostPhaseDisabled,                // Auto Boost Is Disabled and Control commands are processed if any
	AWCCFanBoostPhaseInitial,                 // Probably not needed
	AWCCFanBoostPhaseUpShift,                 //
	AWCCFanBoostPhaseNormal,                  //
	AWCCFanBoostPhaseShiftToLower,            //
	AWCCFanBoostPhaseHelping,                 //
};

enum {
	AWCCFanBoostPhaseCount = 6
};



struct AWCCFanBoostPhaseManager_t {
	enum AWCCFanBoostPhase_t NextPhasePriority [AWCCFanBoostPhaseCount];
	_Bool (* CanChangeTo) (enum AWCCFanBoostPhase_t);
	_Bool (* CanSetFrom) (enum AWCCFanBoostPhase_t);
	void (* InitializePhase) (enum AWCCFan_t);
	void (* ManagePhase) (enum AWCCFan_t);
} extern const AWCCFanBoostPhaseManager [AWCCFanBoostPhaseCount];



static void Manage (enum AWCCFan_t);
static void Reset (enum AWCCFan_t);
static void SetTemperature (enum AWCCFan_t, AWCCTemperature_t);
static void SetPhase (enum AWCCFan_t, enum AWCCFanBoostPhase_t phase);
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



struct {
	struct {
		AWCCTemperature_t Temperature;
		AWCCBoost_t Boost;
		int BoostInterval;
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

	void (* SetPhase) (enum AWCCFan_t, enum AWCCFanBoostPhase_t);
} static Internal = {
	.BoostInfos = {
		[AWCCFanCPU] = { .Phase = AWCCFanBoostPhaseDisabled, },
		[AWCCFanGPU] = { .Phase = AWCCFanBoostPhaseDisabled, },
	},
	.SetPhase = & SetPhase,
};



void Manage (enum AWCCFan_t fan)
{
	for (int i = 0; i < AWCCFanBoostPhaseCount; i++) {
		enum AWCCFanBoostPhase_t nextPhase = AWCCFanBoostPhaseManager [Internal.BoostInfos [fan].Phase].NextPhasePriority [i];

		if (AWCCFanBoostPhaseNone != nextPhase) {
			if (
				   AWCCFanBoostPhaseManager [Internal.BoostInfos [fan].Phase].CanChangeTo (nextPhase)
				&& AWCCFanBoostPhaseManager [nextPhase].CanSetFrom (Internal.BoostInfos [fan].Phase)
			) {
				Internal.SetPhase (fan, nextPhase);
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
	Internal.SetPhase (fan, AWCCFanBoostPhaseInitial);
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

	AWCCFanBoostManager.Reset (AWCCFanCPU);
	AWCCFanBoostManager.Reset (AWCCFanGPU);
	AWCCFanBoostManager.SetPowerState (AWCC.PowerState ());
	AWCCFanBoostManager.SetConfig (Internal.Configs [Internal.PowerState]);

	while (1) {
		AWCCFanBoostManager.SetTemperature (AWCCFanCPU, AWCC.GetFanTemperature (AWCCFanCPU));
		AWCCFanBoostManager.SetTemperature (AWCCFanGPU, AWCC.GetFanTemperature (AWCCFanGPU));
		AWCCFanBoostManager.SetPowerState (AWCC.PowerState ());
		AWCCFanBoostManager.SetTime (time (NULL));

		AWCCFanBoostManager.Manage (AWCCFanCPU);
		AWCCFanBoostManager.Manage (AWCCFanGPU);

		thrd_sleep (& (struct timespec) { .tv_sec = Internal.Config->TemperatureCheckInterval }, NULL);
	}
}

void SetPhase (enum AWCCFan_t fan, enum AWCCFanBoostPhase_t phase)
{
	Internal.BoostInfos [fan].PhaseSetTime = time (NULL);
	Internal.BoostInfos [fan].Phase = phase;
	AWCCFanBoostPhaseManager [phase].InitializePhase (fan);
}



static _Bool CanChangeTo_Disabled (enum AWCCFanBoostPhase_t);
static _Bool CanSetFrom_Disabled (enum AWCCFanBoostPhase_t);
static void InitializePhase_Disabled (enum AWCCFan_t);
static void ManagePhase_Disabled (enum AWCCFan_t);

static _Bool CanChangeTo_Initial (enum AWCCFanBoostPhase_t);
static _Bool CanSetFrom_Initial (enum AWCCFanBoostPhase_t);
static void InitializePhase_Initial (enum AWCCFan_t);
static void ManagePhase_Initial (enum AWCCFan_t);

const struct AWCCFanBoostPhaseManager_t AWCCFanBoostPhaseManager [] = {
	[AWCCFanBoostPhaseDisabled] = {
		.NextPhasePriority = {
			AWCCFanBoostPhaseInitial,
		},
		.CanChangeTo = & CanChangeTo_Disabled,
		.CanSetFrom = & CanSetFrom_Disabled,
		.InitializePhase = & InitializePhase_Disabled,
		.ManagePhase = & ManagePhase_Disabled,
	},
	[AWCCFanBoostPhaseInitial] = {
		.NextPhasePriority = {
			AWCCFanBoostPhaseUpShift,
			AWCCFanBoostPhaseShiftToLower,
			AWCCFanBoostPhaseHelping,
		},
		.CanChangeTo = & CanChangeTo_Initial,
		.CanSetFrom = & CanSetFrom_Initial,
	},
};

_Bool CanChangeTo_Disabled (enum AWCCFanBoostPhase_t phase)
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

_Bool CanSetFrom_Disabled (enum AWCCFanBoostPhase_t phase)
{
	return 1;
}

void InitializePhase_Disabled (enum AWCCFan_t fan)
{

}

void ManagePhase_Disabled (enum AWCCFan_t fan)
{

}

_Bool CanChangeTo_Initial (enum AWCCFanBoostPhase_t phase)
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

_Bool CanSetFrom_Initial (enum AWCCFanBoostPhase_t phase)
{
	return 0;
}

