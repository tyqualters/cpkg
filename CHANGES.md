# CarePackage Changelogs

## Version 0.0.1.2 (STABLE)

Date: 20221001

This is the third official release of CarePackage.

New commands:

    + run
    + .

The "." command is just an alias for the "run" command.

The new command will assume the Current Working Directory (CWD) and run the existing executable.

If the executable is not found, it will call for a compile before running.

Building now displays the time elapsed for compiling. (CarePackage took 9s!)

I drank a full large Dr. Pepper from McDonalds, so I'm about 3% fatter than I was before I started with these changes. Now I'm going to have to hit Jim tonight.

Fixed the prompt for build directory deletion from the previous version. Sorry, not sorry, this is a new project it won't be immediately perfect.

Immediate TODO for next version:

    - Multi-threaded compiling option. (Have separate threads each run an instance of GCC to compile concurrently **if possible**.)

Postponed TODOs for future versions:

    - Complete the script parsing.
    - Add in static library building for Windows.
    - Add in dynamic library building.
    - Actually compile for Windows LOL!
    - Rewrite the whole program in Rust on a separate dev branch so that the cursed Crustaceans can have some inner peace.
    - Prettify the care.pkg file so that it's not so freakin' ugly with multiple dependencies.
    - Maybe solve world hunger. I don't know.

Hopefully someday GCC will add Text Formatting to C++20.

Probably forgot to do an online quiz or something for school.

Now that I can finally run with this new command, maybe I will be able to pass that upcoming PT test.

## Version 0.0.1.1 (STABLE)

Date: 20220927

This is the second official release of CarePackage.

Logs are a little more verbose now!

+ [ERR] -> Error
+ [VFY] -> Verify
+ [TCS] -> Task Completed Successfully
+ [LOG] -> Standard program log

Additionally, Linux support for building static libraries added.

To configure, under `[configuration]` add `type="slib"`.

Dynamic libraries support and Windows static libraries support to come in the future.

Just for informative reasons, Linux uses "ar" to create static libraries whereas Windows uses something like "lib.exe" to create these libraries, which is why Linux support was added now and Windows will come later.

## Version 0.0.1.0 (STABLE)

Date: 20220923

This is the first official release of CarePackage.

Commands:

    + cpkg init [path]
    + cpkg clean [path]
    + cpkg build [path]

### (NEW) Initialize Projects

`cpkg init .`

Initialize a new cpkg project.

This will generate a packages directory and a care.pkg file.

### (NEW) Build Projects

`cpkg build .`

Build an existing project.

This will automatically run through a thorough build process.

Will generate object files, link them, and produce a binary file in the output directory.

### (NEW) Clean Projects

`cpkg clean .`

Remove all the object files and the build files from a project.

Please ensure your build directory is a subdirectory of the project directory. Otherwise, undefined behavior will occur.

### (NEW) care.pkg File

Example file:

```TOML
[CarePackage]
name="cpkg"
author="Ty Qualters"
src="./src/"
include="./src/"
out="./bin/"
version="0.0.1.0"
license="MIT"

[configuration]
C="/usr/bin/gcc"
CC="/usr/bin/g++"
cflags="-Wall -std=c17"
cppflags="-Wall -std=c++20"
linkflags=""

[packages]
net="getcpkg.net"
```

The care.pkg file must be formatted the same as the preceding example. Otherwise, you risk undefined behavior.

Currently, the care.pkg file is only used for compiler flags. However, in the future, it will have way more interaction within the project.
