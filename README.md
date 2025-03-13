# cpkg

A simple C/C++ project management tool

## Features

**Work with what you know**

Creating and managing a project, you are working with Lua. cpkg uses Ninja to then build your project.

**Work with what you don't know**

Why deal with the hassle of a CMake file? Just add a Lua script to a third party dependency and make it work.

**Automatically install dependencies**

No need for vcpkg. With the community Lua files, you can automatically download, build, and import third party dependencies with no hassle.

**Configuration**

Make it super simple to build with a specific configuration. Just run `cpkg -b -c <config>`.

## Want to learn more?

`cpkg --help`

### Building

This project is developed using CLion. Linux (GCC) and Windows (MS Build Tools 2022).

Dependencies are vendored such that setting up a development environment is easier.

If you do install dependencies yourself on Windows, good luck.

### TODO

**Last updated 3-13-2025**

- [ ] Finish `NinjaGenerator`
- [x] Add SSL backend for Windows
- [ ] Allow Lua files to read command line arguments
  - Primarily for `-i` (interactive) for configuration
- [x] Allow the user to run a specific script with `--script`
- [ ] Get [getcpkg.net](https://getcpkg.net) up and running
- [ ] Get [community.getcpkg.net](https://community.getcpkg.net) up and running for Lua scripts

&copy; 2025 Ty Qualters. Project licensed MIT.