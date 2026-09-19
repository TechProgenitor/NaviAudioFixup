//
//  kern_naviaudio.hpp
//  NaviAudioFixup
//
//  Copyright © 2026 TechProgenitor. All rights reserved.
//

#ifndef kern_naviaudio_hpp
#define kern_naviaudio_hpp

#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>

class NAVIAUDIO {
public:
    bool init();
    void deinit();

private:
    /**
     *  Private self instance for callbacks
     */
    static NAVIAUDIO *callbackNAVIAUDIO;
    
    /**
     * Patch kext if needed and prepare other patches
     *
     * @param patcher KernelPatcher instance
     * @param index   kinfo handle
     * @param address kinfo load address
     * @param size    kinfo memory size
     */
    void processKext(KernelPatcher &patcher, size_t index,
                     mach_vm_address_t address, size_t size);

    /**
     * reportCapabilities_LinkInfo wrapper
     */
    static uint32_t wrapReportCapabilitiesLinkInfo(IOService *framebuffer);
    
    /**
     * Original reportCapabilities_LinkInfo
     */
    mach_vm_address_t orgReportCapabilitiesLinkInfo {};

    /**
     * Current progress mask
     */
    struct ProcessingState {
        enum {
            NothingReady = 0,
            NaviAudioPatched = 2,
            EverythingDone = NaviAudioPatched,
        };
    };

    int progressState {ProcessingState::NothingReady};
};

#endif /* kern_naviaudio_hpp */
