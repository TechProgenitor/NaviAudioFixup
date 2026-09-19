KEXT=NaviAudioFixup.kext
DIST=NaviAudioFixup
BUILDDIR=./build

.PHONY: all
all:
	xcodebuild build $(OPTIONS) -configuration Debug
	xcodebuild build $(OPTIONS) -configuration Release

.PHONY: clean
clean:
	xcodebuild clean $(OPTIONS) -configuration Debug
	xcodebuild clean $(OPTIONS) -configuration Release
	rm -rf ./Distribute

.PHONY: distribute
distribute:
	rm -rf ./Distribute
	mkdir -p ./Distribute
	cp -R $(BUILDDIR)/Release/$(KEXT) ./Distribute/
	ditto -c -k --sequesterRsrc --zlibCompressionLevel 9 ./Distribute ./NaviAudioFixup.zip