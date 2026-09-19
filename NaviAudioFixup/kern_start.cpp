//
//  kern_start.cpp
//  NaviAudioFixup
//
//  Copyright © 2026 TechProgenitor. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_naviaudio.hpp"

static NAVIAUDIO naviaudio;

static const char *bootargOff[] {
	"-nafoff"
};

static const char *bootargDebug[] {
	"-nafdbg"
};

static const char *bootargBeta[] {
	"-nafbeta"
};

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::Catalina,
	KernelVersion::Tahoe,
	[]() {
        naviaudio.init();
	}
};
