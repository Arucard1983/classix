# Darling Super-Rosetta (WIP for a future merge to Darling)

This fork is intended to refactor the original ClassiX as a PowerPC version of Rosetta to Darling handle PowerPC applications.

 The refactored Goals are the following ones:

- Focus the PowerPC core emulator, and potential open the ceiling to G4 emulation as long term objective.
- Drop 68k, XCOFF, obsolete executable formats or the ToolBox itself since is not realistic possible to create a compatibility layer to modern operating systems, when the older Macintosh programs simply bypass the operating system and uses directly the hardware.
- The PEF handler should not only works on OS 9 symbols, but also OS X programs (Earlier Mac OS X PPC apps uses the older PEF format, but uses modern Mac OS X frameworks)
- Add Mach-O PowerPC support to the Darling-ClassiX works as the open source replacement of Rosetta.
- The target OS 9 support should be focused on Carbon application. The implementation should be robust, to either:
 1.  The CFM scan the virtual CarbonLib API list to the Carbon and other Darling Frameworks to match a function from the ancient ToolBox inside the Carbon framework. This strategy enables non-Carbon programs that could call Carbon from OS X indirectly and run fine.
 2.  The PEF program call CarbonLib and it is dispatched to Carbon framework. This is the golden rule for good behaved programs.
 3.  The PEF program calls a ToolBox function that is not part of Carbon, but inside the Classix own ToolBox. The code will use Mac OS X frameworks from Darling to handle the request. This is meant for specific cases.
 4.  And finally, the default answer when the previous ones not match, it will be the default Not Supported or OK stub. Later, any attempts to bypass the API will be terminated.

- When The PEF targets OS X (like Mach-O) then the ClassiX.framework (which handles the OS 9 ToolBox code) is bypassed and calls any Darling framework like any OS X program. The ClassiXCore will act as a Rosetta.

  ## Why the changes ?
  - The original ClassiX project, that was focused on OS 9 was focused to run the major number possible of Mac OS Classic programs, but the sheer scale and the bad ToolBox documentation prevent any real progress.
  - Many older Mac OS 9 and earlier used the underlying 68k emulator to handle low level access operating system functionality that is not feasible to a Wine or a NTVDM/WOW implementation. It was like giving vxd driver support for NT or Linux kernel.
  - Focusing on Carbon or CarbonLib, even on runtime, is more feasible and catches the critical transition to Mac OS X era. It contains over 70% of older ToolBox with curated safety handlers. This is like the Win16 support on Wine, or the WowExec on Windows NT. Only well behaved programs works, if it calls DOS (NTVDM) it will be catch and emulated, or simply killed due to fundamental incompatibilities.
  - Like NTVDM (DosBox is more like an Executor for a MacBox), some ToolBox functions could be implemented in the ClassiX.framework as the same functionality exists on some Darling framework. Otherwise, dispatch the Fatal Error.
  - The original tentative ToolBox will be maintained for now, but more to serve a blueprint to implement a rare extention that some Carbon of OS 9 needed to work properly.
  - Extending CFM handling of Mac OS X programs is currently more important, as this project becomes a Rosetta. With Mach-O pass-through the Rosetta mode is self-explanatiory.
  - This repository is planned one day to become part of Darling, since it is the fastest way to provide PowerPC support wthout diverting other tasks.

  ## How this framework is intended to Work ?
  - The ClassiXCore deploys an anchor program (classix) to handle the PowerPC binary and expose the following chain.
   1. If the Program is a Mach-O PPC program, it works like Rosetta, then will call any Darling framework.
   2. Now if it is a PEF binary, then it will take the OS X route if calls native OS X Frameworks. Now when calls a table of symbols form OS 9, then...
   3. The OS 9 route (the original one from the project) makes the ClassiX Frmework and ClassiXCore to check the API table, using the Carbon imports by default. When the calls the CarbonLib, it only save the trouble to search, otherwise will search inside the CarbonLib tree.
   4. When no Carbon matches found, it will scan the ClassiX ToolBox for the function, and luckily it is implemented.
   5. Finally, it reaches the Not Supported default call. It is not supported by this project.

  ## The removal of any plan to support 68k will not cause issues ?
  - On original OS 9, even using PowerPC programs, any system call will soon or later call several times by a second subroutines written in 68k assembly, using the Mixed Code Manager. This project is intended for pure PPC code.
  - And if you dare to ask, VISE installers, Aladdin Stuff-It Installers are a nightmare of compatibility with illegal system extensions, and 68k code mixed with earlier PowerPC assembly. A real solution is a Darling Installer with opens source tools to unpack this stuff and rebuild the program folder without dangerous and unneeded stuff.
  - Running 68k on a Wine like environment is like giving real memory access of a MS-DOS program to modern Linux. This requires a full system emulator like Executor 2000 or M.A.C.E. if you do not want real Apple System Software.

 ## How the ToolBox will work ?
  - As a last line of compatibility when CarbonLib and Carbon don not match a function.
  - To make ClassiX agnostic, even with Darling in mid, map any ToolBox to a Darling Framework.
  - Examples: If a game needs Open Transport, uses the Network.framework with a passive principle. The game will read the active network settings and adapter, send and receive data, but are forbidden to configure network adapters and install drivers!
