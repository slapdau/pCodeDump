# pCode Dump

An application to decode and display the contents of UCSD pCode files,
specifically those from Apple Pascal. It can optionally:

 * List procedures.
 * List symbolic pCode.
 * List disassembled 6502 code.
 * Display interface text.

## Building

### Windows

pCode Dump builds with Visual Studio. The project is current configured
for the v14.3 toolset and requires Visual Studio 2022. Visual Studio
Community can be used.

pCode Dump requires the Boost C++ library. The build requires a version of
Boost that puts 32-bit and 64-bit version libraries in the same directory, and
is known to work with version 1.82.

The pCode Dump build requires the following environment variables to be set:

 * **`BOOST_INCLUDE`** Location of the folder containing the Boost header
   files. The include folder should have the `boost` folder.
 * **`BOOST_LIB`** Location of the folder containing the Boost libraries.
   This folder contains all libraries: 32-bit, 64-bit, static, dynamic,
   debug, optimised.
 
 ### Linux

 See `Makefile`. However, this build has yet to be revalidated with recent
 builds of Linux and Boost.
