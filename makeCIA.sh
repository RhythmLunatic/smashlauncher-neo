#!/bin/bash
bannertool makebanner -i banner.png -a sound.wav -o banner.bin
makerom -f cia -o smashlauncher-neo.cia -rsf cia_workaround.rsf -target t -exefslogo -elf smashlauncher-neo.elf -icon smashlauncher-neo.smdh -banner banner.bin -logo logo.lz11
echo "CIA created."
