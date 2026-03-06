# Bugcheck2NES
Windows crashed? Dropping you into a NES Emu!

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

...
Install DDK 7600 and cd this folder and build /cz
...

# Limitations?

* Runs at 640x480 with 16 colors.
* Only works on BIOS systems.
* Is really slow at times.
* Keyboard support is poor. 

# Credits

[smolnes](https://github.com/binji/smolnes/tree/main) for the NES emulator 

[ReactOS Project](https://doxygen.reactos.org/) for providing documentations for bootvid

[OSdev Wiki](https://wiki.osdev.org/%228042%22_PS/2_Controller) for providing documentations for PS/2
