# Bugcheck2NES
Windows crashed? Dropping you into a NES Simulator!
# Snapshot
![img](https://raw.githubusercontent.com/1234567890zzppap/Bugcheck2NES/refs/heads/main/img/1.png)
![img](https://raw.githubusercontent.com/1234567890zzppap/Bugcheck2NES/refs/heads/main/img/2.png)

# How to run it?

1. Enable test signing and reboot

```
bcdedit /set testsigning on
shutdown /r /t 0
```

2. Create a service using SC

```
sc create nesDriver binPath=C:\where\ever\the\driver\is\nesDriver.sys type=kernel start=demand
```

3. Run it!

```
sc start nesDriver
```

# How to build?

Install DDK 7600 and cd this folder and

```
build /cz
```

# Limitations?

* Runs at 640x480 with 16 colors.
* Only works on BIOS systems.
* Is really slow at times.
* Keyboard support is poor.
* DO NOT MOVE YOUR MOUSE WHEN YOU PLAY.It will broken input.

# Input

| Action | Key |
| --- | --- |
| DPAD-UP | <kbd>W</kbd> |
| DPAD-DOWN | <kbd>S</kbd> |
| DPAD-LEFT | <kbd>A</kbd> |
| DPAD-RIGHT | <kbd>D</kbd> |
| B | <kbd>Z</kbd> |
| A | <kbd>X</kbd> |
| START | <kbd>Enter</kbd> |
| SELECT | <kbd>T</kbd> |
| QUIT | <kbd>Q</kbd> |

# Credits

[Bugcheck2Linux](https://github.com/NSG650/BugCheck2Linux) Refer to the BugcheckCallback part of the source code

[smolnes](https://github.com/binji/smolnes/tree/main) for the NES emulator 

[ReactOS Project](https://doxygen.reactos.org/) for providing documentations for bootvid

[OSdev Wiki](https://wiki.osdev.org/%228042%22_PS/2_Controller) for providing documentations for PS/2
