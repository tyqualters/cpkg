-- Quick Reference:
-- > projectDir: The directory of the project file
-- > outputDir: The directory to output the build files to
-- > config: The configuration to build for

-- Project settings
local projectName = "test"
local version = "1.0.0"

-- Project files
local sourceFiles = find_source_files(projectDir .. "/src")
local includeDirs = find_header_files(projectDir .. "/include")
local compilerFlags = ""
local linkerFlags = ""

-- Download an online test file
--if not file_exists("test.lua") then
--    download('https://raw.githubusercontent.com/tyqualters/voidtools/refs/heads/master/scripts/example_file.lua', 'test.lua')
--end

-- Load the test file and run it
--local test, err = load(read_file('test.lua'))
--if test then
--    test()
--else
--    print(err)
--end

-- Debug config
function debug()
    add_project(
        "lib",                        -- Name
        "1.0.0",                      -- Version
        {"lib/src/lib.c"},            -- Source files (Lua array -> std::vector<std::string>)
        {"lib/include"},              -- Include directories (Lua array -> std::vector<std::string>)
        {},                           -- Library directories (Lua array -> std::vector<std::string>)
        {},                           -- Dependencies (Lua array -> std::vector<std::string>)
        "", "", "",                   -- (C,CXX,LD) Flags
        outputDir,                    -- Output directory
        "static",                     -- Build type
        "gcc",                        -- Compiler
        "", "", "-llib"               -- (C,CXX,LD) Import Flags
    )

    add_project(
        projectName,                  -- Name
        "1.0.0",                      -- Version
        sourceFiles,                  -- Source files (Lua array -> std::vector<std::string>)
        includeDirs,                  -- Include directories (Lua array -> std::vector<std::string>)
        {"."},                        -- Library directories (Lua array -> std::vector<std::string>)
        {"lib"},                      -- Dependencies (Lua array -> std::vector<std::string>)
        "", "", "",                   -- (C,CXX,LD) Flags
        outputDir,                    -- Output directory
        "executable",                 -- Build type
        "gcc",                        -- Compiler
        "", "", ""                    -- (C,CXX,LD) Import Flags
    )

    build()
end

-- Release config
-- function release()
--     compilerFlags = compilerFlags .. ' -O2'
--     build(projectName, version, sourceFiles, headerFiles, compilerFlags, linkerFlags, outputDir)
-- end

-- Run the respective config
-- if config == "debug" then
--     debug()
-- elseif config == "release" then
--     release()
-- else
--     error("Unknown config")
-- end

debug()