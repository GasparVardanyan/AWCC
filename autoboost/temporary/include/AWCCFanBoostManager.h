# ifndef __AWCC_FANBOOSTMANAGER_H
# define __AWCC_FANBOOSTMANAGER_H

# include <time.h>

# include "AWCC.h"
# include "AWCCConfig.h"

struct AWCCFanBoostManager_t {
	void (* Manage) (enum AWCCFan_t);
	void (* Reset) (enum AWCCFan_t);
	void (* SetTemperature) (enum AWCCFan_t, AWCCTemperature_t);
	void (* SetTime) (time_t);
	void (* SetConfig) (const struct AWCCConfig_t *);
	void (* Loop) (void);
	void (* SetPowerState) (enum AWCCPowerState_t);
} extern const AWCCFanBoostManager;

# endif // __AWCC_FANBOOSTMANAGER_H
