NaviAudioFixup
===================

NaviAudioFixup is a Lilu plugin for macOS that corrects digital audio routing on AMD Radeon 5000/6000 series GPUs when `AMDRadeonX6000Framebuffer` assigns an incorrect HDA codec NodeID to a display output.

## Background

On my laptop's AMD Radeon RX 5700M, the HDMI video output was working correctly, but no sound was coming out of the GPU. This was because the framebuffer was publishing an incorrect `audio-codec-info` value for the GPU's HDMI port within the IORegistry.

The affected HDMI output was being assigned NodeID `0x07`:

```
00 01 07 00
```

even though the corresponding HDMI audio Pin Complex was NodeID `0x03`.

```
00 01 03 00
```

This pointed to the framebuffer's audio-node assignment rather than a problem with the HDA codec or `AppleGFXHDA`.

## Encoder Object IDs and NodeIDs

Further investigation of the AMD VBIOS `Object_Header` table and the `AMDRadeonX6000Framebuffer` binary showed a hard-coded relationship between Encoder Object IDs and HDA NodeIDs.

The relevant Encoder Object IDs appear together in the framebuffer binary as:

```
1E210000 1E220000 20210000 20220000 21210000 21220000
```

A separate sequence contains the corresponding `audio-codec-info` values:

```
00010300 00010500 00010700 00010900 00010B00 00010D00
```

Putting the two four-byte strings side-by-side produces the following lookup table:

```
encoder_id            audio-codec-info
──────────────────────────────────────
1E210000              00010300
1E220000              00010500
20210000              00010700
20220000              00010900
21210000              00010B00
21220000              00010D00
```

The Encoder Object IDs correspond to the encoder entries found in the VBIOS `Object_Header`.

Through testing different encoder configurations and observing the resulting `audio-codec-info` values, I found that Apple uses this relationship when assigning the HDA NodeID to a framebuffer port.

The issue on the 5700M was that this hard-coded mapping did not correspond to the actual HDA topology of the GPU.

## Design

NaviAudioFixup hooks:

```
AMDRadeonX6000_AmdRadeonFramebuffer::reportCapabilities_LinkInfo()
```

The original Apple implementation is allowed to execute normally. Afterward, the plugin determines the current framebuffer port and, if configured, replaces the resulting `audio-codec-info` property with the requested HDA NodeID.

This allows the framebuffer's audio mapping to be corrected at runtime without modifying the VBIOS or replacing Apple's audio drivers.

## Installation

1) Copy `NaviAudioFixup.kext` to the `EFI/OC/Kexts` folder on your OpenCore EFI.
2) Add `NaviAudioFixup.kext` to the `Kernel -> Add` section of your `config.plist`.

## Configuration

The following device properties are added to your `config.plist` at the device path of your AMD GPU controller, such as `GFX0` or `DGPU`, depending on how the GPU is named in your system's ACPI/IORegistry layout.

Enable the patching mechanism with:

```
naviaudio-patch-enable = True
naviaudio-patch-enable = 1
naviaudio-patch-enable = <01>
naviaudio-patch-enable = <01000000>
```

Each video output on the GPU is assigned a corresponding `port-number`. This value is represented as a device property under `ATY,RadeonFramebuffer@x` and determines which `naviaudio-portx-node` property is used by NaviAudioFixup.

For example, if an HDMI output has a `port-number` of `0x2`, and no sound is coming out of the GPU in macOS, we need to correct the NodeID that macOS is assuming. You can test the six potential NodeIDs one by one:

```
0x03
0x05
0x07
0x09
0x0B
0x0D
```

or retrieve the correct NodeID from Linux.

In Linux, go to `/proc/asound/cardx` and check the `eld#0.x` files. Once your card is connected to an external display, find the corresponding file where `monitor_present` is set to `1`, then make note of the `codec_pin_nid` value.

For example, if the `codec_pin_nid` value is set to `0xb`, you can add that value to the corresponding `naviaudio-portx-node` device property in your `config.plist` by doing the following:

```
naviaudio-port2-node = 11
naviaudio-port2-node = <0B>
naviaudio-port2-node = <0B000000>
```

The same `naviaudio-portx-node` device property can be added to customize the audio routing for ports 0 through 5:

```
naviaudio-port0-node
naviaudio-port1-node
naviaudio-port2-node
naviaudio-port3-node
naviaudio-port4-node
naviaudio-port5-node
```

Only ports with a corresponding `naviaudio-portx-node` property are modified.

### Boot-args
- `-nafoff` disables kext loading
- `-nafdbg` turns on debugging output
- `-nafbeta` enables loading on unsupported os

### Downloads
Available on the [releases](https://github.com/TechProgenitor/NaviAudioFixup/releases) page.

#### Credits
- [Apple](https://www.apple.com) for macOS  
- [vit9696](https://github.com/vit9696) for [Lilu](https://github.com/acidanthera/Lilu)
- [TechProgenitor](https://github.com/TechProgenitor/) for writing the software and maintaining it