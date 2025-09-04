# ifndef __AWCC_FANBOOSTPHASEMANAGER_H
# define __AWCC_FANBOOSTPHASEMANAGER_H

# include "AWCCFanBoostManagerInternal.h"

struct AWCCFanBoostPhaseManager_t {
	enum AWCCFanBoostPhase_t NextPhasePriority [AWCCFanBoostPhaseCount];
	_Bool (* CanChangeTo) (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
	_Bool (* CanChangeFrom) (enum AWCCFanBoostPhase_t, enum AWCCFan_t);
	void (* InitializePhase) (enum AWCCFan_t);
	void (* ManagePhase) (enum AWCCFan_t);
} extern const AWCCFanBoostPhaseManager [AWCCFanBoostPhaseCount];


# endif // __AWCC_FANBOOSTPHASEMANAGER_H
