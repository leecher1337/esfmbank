About ESFM bank editor
======================

[ESS](https://en.wikipedia.org/wiki/ESS_Technology) PCI Soundcards like 
the ES1969 (Solo1), ES1946, Allegro PCI etc. have their own 4-channel 
synthesizer which is more capable than [OPL3](https://moddingwiki.shikadi.net/wiki/OPL_chip),
which these card also are capable of emulating.
Their synthesizer is called ESFM and due to its enhanced capabilities, the 
driver has its own Patches-table which takes into account the 4 oeprators with
individual frequency offsets, something the basic OPL is not capable of.
The Patches-table is embedded within the driver, but I reverse-engineeres the
ES1969 driver and ported it to also support 64bit, which is available 
[here](https://github.com/leecher1337/es1969).
This copy of the driver also allows you to load your custom patch set from a file
so that you can adjust the instrument tables according to your needs and maybe
implement some additional effects that can again then be ported back into 
the driver.

As it is not convenient to edit the table with a hex-editor to experiment with
it, ESFM bank editor has been created. It runs on 32bit and 64bit Windows and
allows you to load and save the patches table and also test you settings by 
playing an instrument note on the ESFM card (supported ESFM card required).

Installation
============
Depending on the Windows version you have, either download the 32bit or 64bit 
package from the "Releases" link on the right side.
Unpack all files to a directory and run esfmbank.exe

How to use
==========
The release package ships with the default patch set that gets used by all 
drivers (including i.e. Allegro DOS sound system) except the NT4 driver
which has a slightly different patch set.
Both patch sets can be obtained from 
[ESSplaymid repository](https://github.com/pachuco/ESSPlayMid/tree/master/bin).

When opening the application, the default patch set is loaded when available.
The user interface is pretty similar to that of 
[OPL3BankEditor](https://github.com/Wohlstand/OPL3BankEditor).
To the left, you can choose between General Midi Melodic (0-127) and 
Percussion (128-255) instruments.
When clicking on an instrument, its parameters are shown to the right side.
If an instrument has a second voice, the check box `[x] Second voice`
needs to be checked, otherwise the Tab with the second voice will be readonly.
Per voice, there are the 4 Operators that can be tuned accordingly. 
For an explanation on all the settings and values, please see the 
[ESFM documentation](https://github.com/jwt27/esfm).
When you change a parameter for an instrument, the change will not be applied,
unless you click on the "Apply" button.

An exception to that rule are the "Play note" etc. buttons on the right which
temporarily apply your settings when playing the note so that you can 
experiment with the parameters. 
Use "Reset" button to reset the parameters to the state of the last click 
on "Apply" (or to initial state if there was no "Apply" click yet).
Apply/Reset are per operator.
For testing the chord, as mentioned, use the buttons under "Testing" tab.
For them to be enabled and to work properly, you need to connect to your 
ESS soundcard and for this, you need to know the correct base address of the
SB base port. The application tries to already fill in the correct port,
if you have the ESS soundcard driver installed.
Look under "Soundcard connection" frame for it. In the dropdown menu, you
can select your ESFM soundcard and the correct port should be filled in,
so you just have to click onto "Connect" button to connect to your soundcard.
Be aware that chosing a wrong port can have unforseen consequences for the 
stability of you machine, so ensure that you are not using a random port there!

The "MIDI input" tab acts as a possibility to play a MIDI file with the current
patchset from an external MIDI player. In order to make use of it, you need 
the [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) driver 
and create a virtual MIDI cable. 
Than connect to the loopMIDI MIDI input in the "MIDI input" tab and press
"Connect". On the other end of the "loopMIDI" cable, attach your MIDI player.
I recommend 
[TMIDI: (Tom's MIDI Player)](https://www.grandgent.com/tom/projects/tmidi/)
for that purpose.
ESFM bank editor takes the MIDI data from the player, uses the current 
bank settings (which need to be applied) for MIDI playback and sends the 
signals to the Soundcard.

Patch table format
==================
The format of the patch table is pretty straightforward:
Check out [ESFM docs](https://github.com/jwt27/esfm) for register definitions.

The patch file starts with 128 USHORTs that specify the offset of the 128
programs defined by [General Midi](https://de.wikipedia.org/wiki/General_MIDI).

Then follows the table with 128 USHORTs that specify the offset of the 
General midi Percussion instruments also defined by general midi.

Afterwards the actual patch definitions start, consisting of:

Byte 0
------
```
    ╔═══════╦═══════╤═══════╤═══════╤═══════╤═══════╤═══════╤═══════╤═══════╗
    ║ R↓ B→ ║   7   │   6   │   5   │   4   │   3   │   2   │   1   │   0   ║
    ╠═══════╬═══════╪═══════╪═══════╪═══════╪═══════╪═══════╧═══════╪═══════╣
    ║       ║  FP4  │  FP3  │  FP2  │  FP1  │   ?   │   Operation   │ PAT16 ║
    ╚═══════╩═══════╧═══════╧═══════╧═══════╧═══════╧═══════════════╧═══════╝
```

PAT16: Use Patch16?
Operation:
	0 - Only use voice 1
	1 - Also use second voice, if set
	2 - Also use second voice, if not set, steal voice
FP1..FP4: Set fixed pitch for operator 1-4

Byte 3
------
Relative velocity for operator 1-4
```
    ╔═══════╦═══════╤═══════╤═══════╤═══════╤═══════╤═══════╤═══════╤═══════╗
    ║ R↓ B→ ║   7   │   6   │   5   │   4   │   3   │   2   │   1   │   0   ║
    ╠═══════╬═══════╧═══════╪═══════╧═══════╪═══════╧═══════╪═══════╧═══════╣
    ║       ║ Rel.veloc. 4  │ Rel.veloc. 3  │ Rel.veloc. 2  │ Rel.veloc. 1  ║
    ╚═══════╩═══════════════╧═══════════════╧═══════════════╧═══════════════╝
```

Byte 4-11,12-19,20-27,28-35
---------------------------
Registers to set for each operator as documented under "Operator registers" 
in ESFM documentation.

If `Operation` in Byte 0 is > 0 then data for the second voice follows 
(Starting with byte 0 from above again).

Compiling
=========
The application has been developed and compile with Microsoft Visual 
Studio 6 and it contains a .dsp file for it, but you should also be able 
to compile it with newer versions of Visual Studio.

References
==========
[ES1969 Windows driver](https://github.com/leecher1337/es1969)

[ESFM documentation](https://github.com/jwt27/esfm)

Author
======
(c)oded by leecher@dose.0wnz.at
For support use the issue tracker on the project's page:
https://github.com/leecher1337/esfmbank
