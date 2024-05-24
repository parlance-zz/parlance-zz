Listed below is a collection of solo projects undertaken purely for my personal interest. I will be uploading some of these projects as a demonstration of my abilities to potential employers, however, I did not have the confidence that any of these projects were worth preserving at the time. There may be other versions or unfinished breaking changes in what is published to my personal repository. If there is interest I will look into my old files to try to find other versions to publish.

Pre-2004:
* Simple real-time fluid simulation and software renderer (BlitzPlus)
* Partially functional NES emulator; CPU, PPU, debugger and basic memory mapper. Able to display Zelda title screen and run some homebrew demos without sound (BlitzPlus, 6502 ASM) (https://github.com/parlance-zz/parlance-zz/tree/main/pnes)
* 8kb "demo" real-time music synth with DSP, offline MIDI conversion tools (C++, OpenGL)
* Multiplayer Xbox homebrew game (C++ with Xbox SDK / DirectX) (not my video, a user managed to get it running on the Xbox 360 through emulation https://www.youtube.com/watch?v=YwC_8DD_GJg)
* Unfinished clone of Quake 3 game engine (before the source was public) on Xbox with BSP and MD3 rendering, native Xbox shader compiler from Q3 materials. (C++, XDK  / DirectX and shader assembly). At the end of development it was capable of loading and rendering any Q3 map with all Q3 shader features supported; rendering maps in 4x splitscreen with dozens of animated characters at 60 fps on original Xbox. (https://github.com/parlance-zz/parlance-zz/tree/main/projectx)

Post 2004:
* Bitmap -> Level conversion tool for Worms Armageddon (Win32 Game) (C++)
* 3-player patch / ROM hack for Seiken Densetsu 3 (SNES Game) (65C816 ASM) (https://www.romhacking.net/hacks/179/)
* Small bug fix / feature patch to libopenmpt project for their interactive API (C++)
* Compact contiguous heap allocators for LuaJIT (C++)
* Simple lossy audio codec (basic MDCT quantization and entropy coding. Written in BlitzPlus)
* Content pre-processing / resource compilation tool for simple 2D game engine (Python)
* Experimental software GPU renderer for point clouds with post-processing (C++, CUDA, Nvidia CG) (https://github.com/parlance-zz/parlance-zz/tree/main/vtrace)
* Proof-of-concept keylogger with remote control and optional code injection (C++, x86 ASM, PHP and SQL server backend) (https://github.com/parlance-zz/parlance-zz/tree/main/keylogger)
* x64 protected mode boot loader with basic IO and memory management (x64 ASM, C) (https://github.com/parlance-zz/parlance-zz/tree/main/snsos)
* Random tilemap generator for Unity from example maps with enforced constraints - variation on wave-function-collapse, basically a CSP solver with bitfields and fast intrinsics. (C++ and C#) (https://github.com/parlance-zz/parlance-zz/tree/main/rmx)
* DSP pre-processing tool for machine learning to convert raw audio to and from quantized power-spectral-density log-spectrograms (C++, AVX2) (https://github.com/parlance-zz/Pulse)

While working for a past employer:
* Conversion tool for Windows Server 2003 scheduled tasks exported binary format to Server 2008 XML format (C++)
* Windows service for configurable wake-on-LAN proxy / broadcast (C++, WinPCAP)
* CGI <-> Powershell interface for Microsoft IIS (Powershell)
* Active Directory, Microsoft Exchange, SCCM, and OOB server management tools with web interface (Powershell)
* Automated user / student change management tools (Powershell, XML, REST APIs)
* Dynamically generated web phonebook, integrating Active Directory and Cisco Unified Communications (PHP, SQL, SOAP, Cisco AXL)
* Key Module programming web app for Cisco Unified Communication environments (PHP, SOAP, AXL)
* Powershell API and tools for Xerox Docushare (Powershell, SOAP)
* 
2022-2024:
* G-Diffuser - A stable diffusion interface focusing on automated workflows, included a discord bot as well. The [project](https://github.com/parlance-zz/g-diffuser-bot) is now significantly out-dated.
* Dual Diffusion - A generative diffusion model for music. This is my current [project](https://github.com/parlance-zz/dualdiffusion) and is in active development.
