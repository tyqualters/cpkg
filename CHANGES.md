# CarePackage Changelogs

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
