# include "AWCCFanBoostPhaseManager.h"

# include "AWCCConfig.h"
#include "AWCCFanBoostManagerInternal.h"

// TODO: read controls
static _Bool CanChangeFromDisabledTo (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
// TODO: read controls
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

static _Bool CanChangeFromShiftToLowerTo (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static _Bool CanChangeToShiftToLowerFrom (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static void InitializePhase_ShiftToLower (enum AWCCFan_t);
static void ManagePhase_ShiftToLower (enum AWCCFan_t);

static _Bool CanChangeFromHelpingTo (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static _Bool CanChangeToHelpingFrom (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
static void InitializePhase_Helping (enum AWCCFan_t);
static void ManagePhase_Helping (enum AWCCFan_t);

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
		.CanChangeTo = & CanChangeFromShiftToLowerTo,
		.CanChangeFrom = & CanChangeToShiftToLowerFrom,
		.InitializePhase = & InitializePhase_ShiftToLower,
		.ManagePhase = & ManagePhase_ShiftToLower,
	},
	[AWCCFanBoostPhaseHelping] = {
		.NextPhasePriority = {
			AWCCFanBoostPhaseDisabled,
			AWCCFanBoostPhaseUpShift,
		},
		.CanChangeTo = & CanChangeFromHelpingTo,
		.CanChangeFrom = & CanChangeToHelpingFrom,
		.InitializePhase = & InitializePhase_Helping,
		.ManagePhase = & ManagePhase_Helping,
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
	return 0;
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
	_Bool can = 0;

	switch (phase) {
	case AWCCFanBoostPhaseDisabled :
		can = 1;
		break;
	case AWCCFanBoostPhaseUpShift :
		if (AWCCBoostPendingUp == Internal.BoostInfos [fan].BoostPendingState) {
			if (1 == Internal.ConfigUtils.PendingStateSatisfied (fan)) {
				can = 1;
			}
		}
		break;
	case AWCCFanBoostPhaseNormal :
		if (AWCCBoostPendingUp != Internal.BoostInfos [fan].BoostPendingState) {
			if (1 == Internal.ConfigUtils.UpShiftTimePassed (fan)) {
				can = 1;
			}
		}
		break;
	};

	return can;
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
	Internal.BoostInfos [fan].UpShiftInfo.ShiftActive = 0;
}

void ManagePhase_UpShift (enum AWCCFan_t fan)
{
	if (Internal.BoostInfos [fan].UpShiftInfo.ShiftActive == 0) {
	}
}

_Bool CanChangeFromNormalTo (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	return 0;
}

_Bool CanChangeToNormalFrom (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	_Bool can = 0;

	switch (phase) {
	case AWCCFanBoostPhaseUpShift :
		can = 1;
		break;
	default:
		can = 0;
		break;
	}

	return can;
}

void InitializePhase_Normal (enum AWCCFan_t fan)
{
}

void ManagePhase_Normal (enum AWCCFan_t fan)
{
}

_Bool CanChangeFromShiftToLowerTo (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	return 0;
}

_Bool CanChangeToShiftToLowerFrom (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	return 0;
}

void InitializePhase_ShiftToLower (enum AWCCFan_t fan)
{
}

void ManagePhase_ShiftToLower (enum AWCCFan_t fan)
{
}

_Bool CanChangeFromHelpingTo (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	return 0;
}

_Bool CanChangeToHelpingFrom (enum AWCCFanBoostPhase_t phase, enum AWCCFan_t fan)
{
	return 0;
}

void InitializePhase_Helping (enum AWCCFan_t fan)
{
}

void ManagePhase_Helping (enum AWCCFan_t fan)
{
}
