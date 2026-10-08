require "vstudio"

-- PS2 and PSP as Visual Studio platforms of their own (as premake-consoles does for
-- consoles); their projects are Makefile ones, so no MSBuild platform files are needed.
premake.vstudio.vs2010_architectures.ps2 = "PS2"
premake.vstudio.vs2010_architectures.psp = "PSP"
premake.api.addAllowed("system", { "ps2", "psp" })

-- The folder a project is deployed to, and the game it is started from when debugging,
-- is the path of one machine and does not belong in the repository. It is read from a
-- `.env` file next to this script, which is not tracked by git and holds one
-- `<KEY>=<folder>` line per game (quotes and a trailing slash are optional). A project
-- whose key is missing is not deployed at all.
local envkeys = nil
function envdir(key)
   if not envkeys then
      envkeys = {}
      local text = io.readfile(path.join(_SCRIPT_DIR, ".env")) or ""
      for line in text:gmatch("[^\r\n]+") do
         local k, v = line:match("^%s*([%w_]+)%s*=%s*(.-)%s*$")
         if k and v ~= "" then
            v = v:gsub('^"', ""):gsub('"$', ""):gsub("^'", ""):gsub("'$", "")
            envkeys[k] = v
         end
      end
   end

   local value = envkeys[key]
   if not value then return nil end

   value = value:gsub("[%s\\/]+$", "")
   if value == "" then return nil end

   return path.translate(value)
end

-- Deploys the built .asi into the script folder of the game that `key` names in the .env
-- file, and starts the game from there when debugging. Only a plugin that is already
-- installed in the game folder is replaced, a folder without one is left alone.
function setpaths(key, exepath, scriptspath)
   scriptspath = scriptspath or "scripts/"
   local gamepath = envdir(key)
   if gamepath then
      local target = gamepath .. "\\" .. path.translate(scriptspath)
      postbuildcommands {
         "if exist \"" .. target .. "$(TargetFileName)\" copy /y \"$(TargetPath)\" \"" .. target .. "\"",
      }
      debugdir (gamepath)
      if (exepath) then
         debugcommand (gamepath .. "\\" .. path.translate(exepath))
      end
   end
end

workspace "III.VC.SA.IV.Project2DFX"
   configurations { "Release", "Debug" }
   platforms { "Win32" }
   architecture "x86"
   location "build"
   objdir ("build/obj")
   buildlog ("build/log/%{prj.name}.log")
   cppdialect "C++latest"
   
   kind "SharedLib"
   language "C++"
   targetdir "data/%{prj.name}/"
   targetextension ".asi"
   characterset ("Unicode")
   staticruntime "On"

   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{cfg.buildtarget.name}\"" }
   defines { "rsc_FileDescription=\"https://thirteenag.github.io/p2dfx\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX\"" }

   local major = os.date("%d")
   local minor = os.date("%m")
   local build = os.date("%Y")
   local revision = os.date("%H") .. os.date("%M")

   local githash = ""
   local f = io.popen("git rev-parse --short HEAD")
   if f then
      githash = f:read("*a"):gsub("%s+", "")
      f:close()
   end

   local productVersion = major .. "." .. minor .. "." .. build .. "." .. revision
   if githash ~= "" then
      productVersion = productVersion .. "-" .. githash
   end

   defines { "rsc_FileVersion_MAJOR=" .. major }
   defines { "rsc_FileVersion_MINOR=" .. minor }
   defines { "rsc_FileVersion_BUILD=" .. build }
   defines { "rsc_FileVersion_REVISION=" .. revision }
   defines { "rsc_FileVersion=\"" .. major .. "." .. minor .. "." .. build .. "\"" }
   defines { "rsc_ProductVersion=\"" .. productVersion .. "\"" }
   defines { "rsc_GitSHA1=\"" .. githash .. "\"" }
   defines { "rsc_GitSHA1W=L\"" .. githash .. "\"" }

   defines { "_CRT_SECURE_NO_WARNINGS" }

   files { "source/%{prj.name}/*.h", "source/%{prj.name}/*.cpp" }
   files { "source/%{prj.name}/*.hpp", "source/%{prj.name}/*.ixx" }
   files { "source/*.hpp", "source/*.ixx" }
   files { "resources/*.rc" }
   files { "external/hooking/Hooking.Patterns.h", "external/hooking/Hooking.Patterns.cpp" }
   files { "external/injector/safetyhook/include/**.hpp", "external/injector/safetyhook/src/**.cpp" }
   files { "external/injector/zydis/**.h", "external/injector/zydis/**.c" }
   files { "data/**/*.ini" }
   includedirs { "includes" }
   includedirs { "external/hooking" }
   includedirs { "external/injector/safetyhook/include" }
   includedirs { "external/injector/zydis" }
   includedirs { "external/injector/include" }
   includedirs { "external/inireader" }
   includedirs { "external/filewatch" }
   includedirs { "external/ModUtils" }

   vpaths {
      ["source"] = { "source/**.*" },
      ["shaders"] = { "source/**.fx", "source/**.vs", "source/**.ps", "source/**.hlsl" },
      ["ini"] = { "data/**.ini" },
      ["data"] = { "data/**.cfg", "data/**.dat" },
      ["resources/*"] = { "resources/*" },
      ["includes/*"] = { "includes/**" },
      ["external/*"] = { "external/**" },
   }

   filter "configurations:Debug"
      defines "DEBUG"
      symbols "On"

   filter "configurations:Release"
      defines "NDEBUG"
      optimize "On"

project "2DFXDataGrabber"
   kind "ConsoleApp"
   targetdir "data/%{prj.name}/"
   targetextension ".exe"
   characterset ("MBCS")
   removefiles { "source/*.hpp", "source/*.ixx" }
project "IIILodLights"
   setpaths("GTAIII_DIR", "gta3.exe")
project "VCLodLights"
   setpaths("GRAND_THEFT_AUTO_VICE_CITY_DIR", "gta-vc.exe")
project "SALodLights"
   setpaths("GTA_SAN_ANDREAS_DIR", "gta_sa.exe")
project "IVLodLights"
   removefiles { "source/*.hpp", "source/*.ixx" }
   files { "source/FileMgr.ixx", "source/LamppostInfo.ixx", "source/Timer.ixx" }
   -- the plugins of GTA IV are loaded from the plugins folder
   setpaths("GTA_IV_DIR", "GTAIV.exe", "plugins/")

-- ====================== CONSOLE SOLUTIONS ======================
-- Vice City Stories and Liberty City Stories on PCSX2F (PS2) and PPSSPP (PSP). These are
-- MIPS modules built by the SDK submodules (external/ps2sdk, external/pspsdk) from
-- source/console/<name>/module.json into data/<name>/, laid out the way they are unpacked
-- into the folder of the emulator. source/console/shared holds what they have in common.
function ConsoleSetup(name, platform)
   workspace (name)
      configurations { "Release", "Debug" }
      platforms { platform }
      system (platform:lower())
      bindirs { "$(PATH)" } -- unknown VS platform: keep the system PATH for the build commands
      location "build"
      kind "Makefile"
      language "C++"
      includedirs { "external/injector/include" }
      files { "source/console/%{prj.name}/*.h", "source/console/%{prj.name}/*.hpp", "source/console/%{prj.name}/*.c",
              "source/console/%{prj.name}/*.cpp", "source/console/%{prj.name}/module.json", "source/console/%{prj.name}/*.exp",
              "source/console/shared/**.hpp", "data/%{prj.name}/**.ini" }
      filter "configurations:Debug"
         defines { "DEBUG" }
      filter "configurations:Release"
         defines { "NDEBUG" }
      filter {}
end

-- Builds the module with the SDK of `sdk` into data/<name>/<output>, and copies it into the
-- folder of the emulator that `key` names in the .env file.
function consolepaths(sdk, key, output, exepath)
   local manifest = '"%{wks.location}/../source/console/%{prj.name}/module.json"'
   local command = 'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}/../external/' .. sdk .. '/plugins/build-module.ps1" -Project ' .. manifest
   if sdk == "pspsdk" then
      command = command .. ' -Configuration "%{cfg.buildcfg}"'
   end
   local gamepath = envdir(key)
   local deploy = {}
   if gamepath then
      local target = gamepath .. "\\" .. path.translate(path.getdirectory(output))
      deploy = { 'if not exist "' .. target .. '" mkdir "' .. target .. '"',
         'copy /y "$(NMakeOutput)" "' .. target .. '"' }
      debugdir (gamepath)
      debugcommand (gamepath .. "\\" .. exepath)
   end
   buildcommands { command, 'if errorlevel 1 exit /b %errorlevel%', deploy }
   rebuildcommands { command .. ' -Clean', 'if errorlevel 1 exit /b %errorlevel%', command,
      'if errorlevel 1 exit /b %errorlevel%', deploy }
   cleancommands { command .. ' -Clean' }
   targetdir ("data/%{prj.name}/" .. path.getdirectory(output))
   targetname (path.getbasename(output))
   targetextension (path.getextension(output))
end

ConsoleSetup("Project2DFX.PS2", "PS2")
   includedirs { "external/ps2sdk/ps2sdk/ee" }

project "GTAVCS.PCSX2F.Project2DFX"
   consolepaths("ps2sdk", "PCSX2F_DIR", "PLUGINS/GTAVCS.PCSX2F.Project2DFX.elf", "pcsx2-qtx64.exe")
project "GTALCS.PCSX2F.Project2DFX"
   consolepaths("ps2sdk", "PCSX2F_DIR", "PLUGINS/GTALCS.PCSX2F.Project2DFX.elf", "pcsx2-qtx64.exe")

ConsoleSetup("Project2DFX.PSP", "PSP")
   includedirs { "external/pspsdk/usr/local/pspdev/psp/sdk/include" }

project "GTAVCS.PPSSPP.Project2DFX"
   consolepaths("pspsdk", "PPSSPP_DIR", "memstick/PSP/PLUGINS/GTAVCS.PPSSPP.Project2DFX/GTAVCS.PPSSPP.Project2DFX.prx", "PPSSPPWindows64.exe")
project "GTALCS.PPSSPP.Project2DFX"
   consolepaths("pspsdk", "PPSSPP_DIR", "memstick/PSP/PLUGINS/GTALCS.PPSSPP.Project2DFX/GTALCS.PPSSPP.Project2DFX.prx", "PPSSPPWindows64.exe")