//
//  kern_naviaudio.cpp
//  NaviAudioFixup
//
//  Copyright © 2026 TechProgenitor. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include "kern_naviaudio.hpp"
#include <IOKit/IOService.h>

static const char *kextId {
    "com.apple.kext.AMDRadeonX6000Framebuffer"
};

static const char *kextPath[] {
    "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer"
};

static KernelPatcher::KextInfo kext { kextId, kextPath, 1, {true, true}, {}, KernelPatcher::KextInfo::Unloaded };

NAVIAUDIO *NAVIAUDIO::callbackNAVIAUDIO;

bool NAVIAUDIO::init() {
    callbackNAVIAUDIO = this;
    
    LiluAPI::Error error = lilu.onKextLoad(&kext, 1,
        [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
            static_cast<NAVIAUDIO *>(user)->processKext(patcher, index, address, size);
        }, this);

    if (error != LiluAPI::Error::NoError) {
        SYSLOG("naviaudio", "failed to register onPatcherLoad method %d", error);
        return false;
    }

    return true;
}

void NAVIAUDIO::deinit() {}

uint32_t NAVIAUDIO::wrapReportCapabilitiesLinkInfo(IOService *framebuffer) {

    // Let Apple's implementation run completely first.
    uint32_t ret = FunctionCast(wrapReportCapabilitiesLinkInfo, callbackNAVIAUDIO->orgReportCapabilitiesLinkInfo)(framebuffer);
    
    // Point to Device Properties of current GPU Instance
    IORegistryEntry *currentGPU = framebuffer;
    currentGPU = currentGPU->getParentEntry(gIOServicePlane)->getParentEntry(gIOServicePlane);
    
    // Check Patcher Enable Flag
    OSData *enabled = OSDynamicCast(OSData, currentGPU->getProperty("naviaudio-patch-enable"));

    if (!enabled || enabled->getLength() == 0)
        return ret;

    uint8_t value = static_cast<const uint8_t *>(enabled->getBytesNoCopy())[0];

    if (!value)
        return ret;
    
    // Detect Current Port in Use
    OSNumber *portNumber = OSDynamicCast(OSNumber, framebuffer->getProperty("port-number"));
    const char *propertyName;
    
    if (!portNumber)
        return ret;

    switch (portNumber->unsigned32BitValue()) {
        case 0:
            propertyName = "naviaudio-port0-node";
            break;

        case 1:
            propertyName = "naviaudio-port1-node";
            break;

        case 2:
            propertyName = "naviaudio-port2-node";
            break;

        case 3:
            propertyName = "naviaudio-port3-node";
            break;

        case 4:
            propertyName = "naviaudio-port4-node";
            break;

        case 5:
            propertyName = "naviaudio-port5-node";
            break;

        default:
            return ret;
    }
    
    // Detect Requested NodeID
    OSData *nodeID = OSDynamicCast(OSData, currentGPU->getProperty(propertyName));

    if (!nodeID || nodeID->getLength() == 0)
        return ret;

    value = static_cast<const uint8_t *>(nodeID->getBytesNoCopy())[0];

    if (value < 3 || value > 13 || !(value & 1))
        return ret;

    uint8_t codecInfoBytes[4] = {0x00, 0x01, value, 0x00};

    // Replace the audio-codec-info device property
    OSData *codecInfo = OSData::withBytes(codecInfoBytes, sizeof(codecInfoBytes));
    
    if (!codecInfo) {
        SYSLOG("naviaudio", "failed to create audio-codec-info OSData");
        return ret;
    }

    if (framebuffer->setProperty("audio-codec-info", codecInfo)) {
        DBGLOG("naviaudio", "successfully set audio-codec-info");
        DBGLOG("naviaudio", "port %u -> NodeID %u", portNumber->unsigned32BitValue(), value);
    } else {
        SYSLOG("naviaudio", "failed to set audio-codec-info");
    }
    
    codecInfo->release();

    return ret;
}

void NAVIAUDIO::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (progressState != ProcessingState::EverythingDone) {
        if (kext.loadIndex == index) {
            DBGLOG("naviaudio", "found %s", kext.id);

            KernelPatcher::RouteRequest request("__ZN35AMDRadeonX6000_AmdRadeonFramebuffer27reportCapabilities_LinkInfoEv", wrapReportCapabilitiesLinkInfo, orgReportCapabilitiesLinkInfo);
            patcher.routeMultiple(index, &request, 1, address, size);

            if (patcher.getError() == KernelPatcher::Error::NoError) {
                DBGLOG("naviaudio", "reportCapabilities_LinkInfo hook installed");
                progressState |= ProcessingState::NaviAudioPatched;
            } else {
                SYSLOG("naviaudio", "failed to hook reportCapabilities_LinkInfo");
                patcher.clearError();
            }
        }
    }
}
