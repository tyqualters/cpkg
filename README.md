# Status

Due to time constraints, this project is not actively maintained. Hopefully in the future, more time may be allocated.

# CarePackage

A simple C/C++ project management tool

## Features

**Work with what you know**

To create and manage a project, you use a simple Lua script. CarePackage runs everything else behind the scenes.

**Work with what you don't know**

Automate the whole dependency management process with a Lua script that can automatically build and (**TODO**) include your project.

**Automatically install dependencies**

(**TODO**) No need for vcpkg. With the community Lua files, you can automatically download, build, and import third party dependencies with no hassle.

**Configuration**

Make it super simple to build with a specific configuration. Just run `cpkg -b -c <config>`.

## Want to learn more?

`cpkg --help`

### Building

This project is developed using CLion. Linux (GCC) and Windows (MS Build Tools 2022).

Dependencies are vendored so that setting up a development environment is easier.

&copy; 2025 Ty Qualters. Project licensed MIT.
