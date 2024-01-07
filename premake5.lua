newoption {
    trigger     = "with-version",
    value       = "STRING",
    description = "Current version",
    default     = "5.0",
}

workspace "III.VC.SA.IV.Project2DFX"
   configurations { "Release", "Debug" }
   platforms { "Win32" }
   architecture "x32"
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
   
   defines { "rsc_CompanyName=\"III.VC.SA.IV.LCS.VCS.Project2DFX\"" }
   defines { "rsc_LegalCopyright=\"III.VC.SA.IV.LCS.VCS.Project2DFX\""} 
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.dll\"" }
   defines { "rsc_FileDescription=\"III.VC.SA.IV.LCS.VCS.Project2DFX\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX\"" }

   local major = 1
   local minor = 0
   local build = 0
   local revision = 0
   if(_OPTIONS["with-version"]) then
     local t = {}
     for i in _OPTIONS["with-version"]:gmatch("([^.]+)") do
       t[#t + 1], _ = i:gsub("%D+", "")
     end
     while #t < 4 do t[#t + 1] = 0 end
     major = math.min(tonumber(t[1]), 255)
     minor = math.min(tonumber(t[2]), 255)
     build = math.min(tonumber(t[3]), 65535)
     revision = math.min(tonumber(t[4]), 65535)
   end
   defines { "rsc_FileVersion_MAJOR=" .. major }
   defines { "rsc_FileVersion_MINOR=" .. minor }
   defines { "rsc_FileVersion_BUILD=" .. build }
   defines { "rsc_FileVersion_REVISION=" .. revision }
   defines { "rsc_FileVersion=\"" .. major .. "." .. minor .. "." .. build .. "\"" }
   defines { "rsc_ProductVersion=\"" .. major .. "." .. minor .. "." .. build .. "\"" }

   defines { "_CRT_SECURE_NO_WARNINGS" }
   
   files { "source/%{prj.name}/*.h", "source/%{prj.name}/*.cpp" }
   files { "resources/*.rc" }
   files { "external/hooking/Hooking.Patterns.h", "external/hooking/Hooking.Patterns.cpp" }
   includedirs { "includes" }
   includedirs { "external/hooking" }
   includedirs { "external/injector/include" }
   includedirs { "external/inireader" }
      
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
project "IIILodLights"
	files { "includes/*.h", "includes/*.cpp" }
project "VCLodLights"
	files { "includes/*.h", "includes/*.cpp" }
project "SALodLights"
	files { "includes/*.h", "includes/*.cpp" }
project "IVLodLights"
	files { "includes/CLODLightManager.h", "includes/CLODLightManager.cpp" }
