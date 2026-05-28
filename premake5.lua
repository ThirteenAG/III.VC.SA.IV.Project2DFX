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
project "VCLodLights"
project "SALodLights"
project "IVLodLights"
   removefiles { "source/*.hpp", "source/*.ixx" }
   files { "source/FileMgr.ixx", "source/LamppostInfo.ixx", "source/Timer.ixx" }
