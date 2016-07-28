#include "..\includes\stdafx.h"
#include "..\includes\CLODLightManager.h"

using namespace injector;

#define NewLimitExponent 14

char* CLODLightManager::IV::CurrentTimeHours;
char* CLODLightManager::IV::CurrentTimeMinutes;
int(__cdecl *CLODLightManager::IV::DrawCorona)(float x, float y, float z, float radius, unsigned int unk, float unk2, unsigned char r, unsigned char g, unsigned char b);
int(__cdecl *CLODLightManager::IV::DrawCorona2)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14);
void(__stdcall *CLODLightManager::IV::GetRootCam)(int *camera);
void(__stdcall *CLODLightManager::IV::GetGameCam)(int *camera);
bool(__cdecl *CLODLightManager::IV::CamIsSphereVisible)(int camera, float pX, float pY, float pZ, float radius);
void(__cdecl *CLODLightManager::IV::GetCamPos)(int camera, float *pX, float *pY, float *pZ);
int& CTimer::m_snTimeInMillisecondsPauseMode = *(int*)0xBADDEAD;

void CLODLightManager::IV::Init()
{
	CIniReader iniReader("");
	bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1) != 0;
	fCoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 1.0f);
	bSlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
	fCoronaFarClip = iniReader.ReadFloat("LodLights", "CoronaFarClip", 0.0f);
	bool DisableDefaultLodLights = iniReader.ReadInteger("LodLights", "DisableDefaultLodLights", 0) == 1;
	bool DisableCoronasWaterReflection = iniReader.ReadInteger("LodLights", "DisableCoronasWaterReflection", 0) == 1;
	fGenericObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "LamppostsDrawDistance", 0.0f);
	bool SkipIntro = iniReader.ReadInteger("Misc", "SkipIntro", 0) == 1;
	bool DoNotPauseOnMinimize = iniReader.ReadInteger("Misc", "DoNotPauseOnMinimize", 0) == 1;

	struct LoadObjectInstanceHook
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ebx = *(uintptr_t*)(regs.ebp + 0x8);
			regs.ecx = *(uintptr_t*)(regs.ebx + 0x1C);

			PossiblyAddThisEntity((WplInstance*)regs.ebx);
		}
	};

	struct ParseIdeObjsHook
	{
		void operator()(injector::reg_pack& regs)
		{
			auto xmmZero = *(float*)(regs.esp + 0x1C);
			if (xmmZero == 50.0f || xmmZero == 100.0f || xmmZero == 150.0f || xmmZero == 80.0f) //lampposts draw distances
			{
				xmmZero = fGenericObjectsDrawDistance;
			}
			_asm movss   xmm0, xmmZero
		}
	};

	if (bRenderLodLights)
	{
		GetMemoryAddresses();
		IncreaseCoronaLimit();
		LoadDatFile();
		RegisterCustomCoronas();

		if (injector::address_manager::singleton().IsIV())
		{
			injector::MakeCALL(aslr_ptr(0x00402D6C), RegisterLODLights, true);
			injector::MakeInline<LoadObjectInstanceHook>(aslr_ptr(0x008D63A1), aslr_ptr(0x008D63A7));
			injector::MakeInline<ParseIdeObjsHook>(aslr_ptr(0x008D2311), aslr_ptr(0x8D2317));
		}
		else
		{
			injector::MakeCALL(aslr_ptr(0x0047361F), RegisterLODLights, true);
			injector::MakeInline<LoadObjectInstanceHook>(aslr_ptr(0x0091C851), aslr_ptr(0x0091C857));
			injector::MakeInline<ParseIdeObjsHook>(aslr_ptr(0x00918891), aslr_ptr(0x918897));
		}
	}

	if (injector::address_manager::singleton().IsIV())
	{
		if (SkipIntro) { injector::WriteMemory<uint8_t>(aslr_ptr(0x402B49), 0xEB, true); }
		if (DoNotPauseOnMinimize) { injector::MakeNOP(aslr_ptr(0x61D06A), 2, true); }

		if (DisableDefaultLodLights) { injector::WriteMemory<unsigned char>(aslr_ptr(0x00903300), 0xC3u, true); }
		if (DisableCoronasWaterReflection) { injector::MakeNOP(aslr_ptr(0xB54373), 5, true); }
	}
	else
	{
		injector::WriteMemory<uint8_t>(aslr_ptr(0x7FE12C), 0xEB, true);
		struct RegPatch
		{
			void operator()(injector::reg_pack& regs)
			{
				HMODULE hModule = GetModuleHandle(NULL);
				if (hModule != NULL)
				{
					GetModuleFileName(hModule, (char*)regs.esi, 260);
					auto ptr = strrchr((char*)regs.esi, '\\');
					*(ptr+1) = '\0';
				}
			}
		}; injector::MakeInline<RegPatch>(aslr_ptr(0x7FE1B8), aslr_ptr(0x7FE1B8 + 6));

		injector::WriteMemory<uint8_t>(aslr_ptr(0x8B329C), 0xEB, true);
		struct RegPatch2
		{
			void operator()(injector::reg_pack& regs)
			{
				regs.ecx = *(uintptr_t*)(regs.esp + 0x4);
				HMODULE hModule = GetModuleHandle(NULL);
				if (hModule != NULL)
				{
					GetModuleFileName(hModule, (char*)regs.esi, 260);
					auto ptr = strrchr((char*)regs.esi, '\\');
					*(ptr + 1) = '\0';
				}
			}
		}; injector::MakeInline<RegPatch2>(aslr_ptr(0x8B3315), aslr_ptr(0x8B331B));


		if (SkipIntro) { injector::WriteMemory<uint8_t>(aslr_ptr(0x473439), 0xEB, true); }
		if (DoNotPauseOnMinimize) { injector::MakeNOP(aslr_ptr(0x402D5A), 2, true); }

		if (DisableDefaultLodLights) { injector::WriteMemory<unsigned char>(aslr_ptr(0x00975860), 0xC3u, true); }
		if (DisableCoronasWaterReflection) { injector::MakeNOP(aslr_ptr(0xC8E183), 5, true); }
	}
}

void CLODLightManager::IV::GetMemoryAddresses()
{
	if (injector::address_manager::singleton().IsIV())
	{
		CLODLightManager::IV::CurrentTimeHours = aslr_ptr(0x11D5300).get();
		CLODLightManager::IV::CurrentTimeMinutes = aslr_ptr(0x11D52FC).get();
		CLODLightManager::IV::DrawCorona = (int(__cdecl *)(float, float, float, float, unsigned int, float, unsigned char, unsigned char, unsigned char))(aslr_ptr(0xA6E240).get());
		CLODLightManager::IV::DrawCorona2 = (int(__cdecl *)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14))(aslr_ptr(0x7E1970).get());
		CLODLightManager::IV::GetRootCam = (void(__stdcall *)(int *camera))(aslr_ptr(0xB006C0).get());
		CLODLightManager::IV::GetGameCam = (void(__stdcall *)(int *camera))(aslr_ptr(0xB006E0).get());
		CLODLightManager::IV::CamIsSphereVisible = (bool(__cdecl *)(int camera, float pX, float pY, float pZ, float radius))(aslr_ptr(0xBB9340).get());
		CLODLightManager::IV::GetCamPos = (void(__cdecl *)(int camera, float *pX, float *pY, float *pZ))(aslr_ptr(0xBB8510).get());
	}
	else
	{
		CLODLightManager::EFLC::CurrentTimeHours = injector::aslr_ptr(0x10C9398).get();
		CLODLightManager::EFLC::CurrentTimeMinutes = injector::aslr_ptr(0x10C9394).get();
		CLODLightManager::EFLC::DrawCorona = (int(__cdecl *)(float, float, float, float, unsigned int, float, unsigned char, unsigned char, unsigned char))(aslr_ptr(0xAA3100).get());
		CLODLightManager::EFLC::DrawCorona2 = (int(__cdecl *)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14))(aslr_ptr(0x89A080).get());
		CLODLightManager::EFLC::GetRootCam = (void(__stdcall *)(int *camera))(aslr_ptr(0xAFA7E0).get());
		CLODLightManager::EFLC::GetGameCam = (void(__stdcall *)(int *camera))(aslr_ptr(0xAFA800).get());
		CLODLightManager::EFLC::CamIsSphereVisible = (bool(__cdecl *)(int camera, float pX, float pY, float pZ, float radius))(aslr_ptr(0xC27DD0).get());
		CLODLightManager::EFLC::GetCamPos = (void(__cdecl *)(int camera, float *pX, float *pY, float *pZ))(aslr_ptr(0xC26FA0).get());
	}
}

void CLODLightManager::IV::IncreaseCoronaLimit()
{
	auto nCoronasLimit = static_cast<DWORD>(3 * pow(2.0, NewLimitExponent)); // 49152, default 3 * pow(2, 8) = 768

	static std::vector<int> aCoronas;
	static std::vector<int> aCoronas2;
	aCoronas.resize(nCoronasLimit * 0x3C * 4);
	aCoronas2.resize(nCoronasLimit * 0x3C * 4);

	if (injector::address_manager::singleton().IsIV())
	{
		AdjustPointer(aslr_ptr(0x7E19F4), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E19EE), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E0F95), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A02), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A10), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A74), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A7C), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A84), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A8D), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1182), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E19A1), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A39), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A4C), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A5A), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E19C1), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E19DA), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E19B5), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E19AB), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E19CD), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E199A), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A22), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A60), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1A93), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));
		AdjustPointer(aslr_ptr(0x7E1AAB), &aCoronas[0], aslr_ptr(0x116AB00), aslr_ptr(0x116AB00 + 0x3C));

		AdjustPointer(aslr_ptr(0x7E10AB), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E14BA), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E10B9), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E10C7), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E10CF), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E14C2), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E10FD), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E14CA), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E10D5), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E1671), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E10DB), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E14A5), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E10E5), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E14AC), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E10F5), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E14B3), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E1103), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));
		AdjustPointer(aslr_ptr(0x7E1318), &aCoronas2[0], aslr_ptr(0x115EB00), aslr_ptr(0x115EB00 + 0x1B));

		WriteMemory<unsigned char>(aslr_ptr(0x7E109F + 0x2), NewLimitExponent);
		WriteMemory<unsigned char>(aslr_ptr(0x7E149A + 0x2), NewLimitExponent);
		WriteMemory<unsigned char>(aslr_ptr(0x7E130E + 0x2), NewLimitExponent);

		WriteMemory<unsigned int>(aslr_ptr(0x7E1979 + 0x2), nCoronasLimit);
		WriteMemory<unsigned int>(aslr_ptr(0x7E1072 + 0x2), nCoronasLimit);
		WriteMemory<unsigned int>(aslr_ptr(0x7E1189 + 0x1), nCoronasLimit * 64);
	}
	else
	{
		AdjustPointer(aslr_ptr(0x89A104), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A0FE), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x8995C5), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A112), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A120), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A184), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A18C), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A194), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A19D), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x8997B2), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A0B1), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A149), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A15C), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A16A), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A0D1), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A0EA), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A0C5), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A0BB), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A0DD), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A0AA), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A132), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A170), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A1A3), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));
		AdjustPointer(aslr_ptr(0x89A1BB), &aCoronas[0], aslr_ptr(0x111A040), aslr_ptr(0x111A040 + 0x3C));

		AdjustPointer(aslr_ptr(0x8996DB), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899BCA), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x8996E9), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x8996F7), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x8996FF), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899BD2), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x89972D), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899BDA), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899705), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899D81), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x89970B), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899BB5), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899715), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899BBC), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899725), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899BC3), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899733), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));
		AdjustPointer(aslr_ptr(0x899A28), &aCoronas2[0], aslr_ptr(0x110E040), aslr_ptr(0x110E040 + 0x1B));

		WriteMemory<unsigned char>(aslr_ptr(0x8996CF + 0x2), NewLimitExponent);
		WriteMemory<unsigned char>(aslr_ptr(0x899A1E + 0x2), NewLimitExponent);
		WriteMemory<unsigned char>(aslr_ptr(0x899BAA + 0x2), NewLimitExponent);

		WriteMemory<unsigned int>(aslr_ptr(0x89A089 + 0x2), nCoronasLimit);
		WriteMemory<unsigned int>(aslr_ptr(0x8996A2 + 0x2), nCoronasLimit);
		WriteMemory<unsigned int>(aslr_ptr(0x8997B9 + 0x1), nCoronasLimit * 64);
	}
}


void CLODLightManager::IV::RegisterCustomCoronas()
{
	unsigned short		nModelID = 65534;

	auto	itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
	for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
		m_pLampposts->push_back(CLamppostInfo(it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, 0.0f));
}

WplInstance* CLODLightManager::IV::PossiblyAddThisEntity(WplInstance* pInstance)
{
	if (m_bCatchLamppostsNow && IsModelALamppost(pInstance->ModelNameHash))
		RegisterLamppost(pInstance);

	return pInstance;
}

void CLODLightManager::IV::RegisterLamppost(WplInstance* pObj)
{
	DWORD       		nModelID = pObj->ModelNameHash;
	CMatrix				dummyMatrix;

	float qw = pObj->RotationW;
	float qx = pObj->RotationX;
	float qy = pObj->RotationY;
	float qz = pObj->RotationZ;

	float n = 1.0f / sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
	qx *= n;
	qy *= n;
	qz *= n;
	qw *= n;

	dummyMatrix.matrix.right.x = 1.0f - 2.0f*qy*qy - 2.0f*qz*qz;
	dummyMatrix.matrix.right.y = 2.0f*qx*qy - 2.0f*qz*qw;
	dummyMatrix.matrix.right.z = 2.0f*qx*qz + 2.0f*qy*qw;

	dummyMatrix.matrix.up.x = 2.0f*qx*qy + 2.0f*qz*qw;
	dummyMatrix.matrix.up.y = 1.0f - 2.0f*qx*qx - 2.0f*qz*qz;
	dummyMatrix.matrix.up.z = 2.0f*qy*qz - 2.0f*qx*qw;

	dummyMatrix.matrix.at.x = 2.0f*qx*qz - 2.0f*qy*qw;
	dummyMatrix.matrix.at.y = 2.0f*qy*qz + 2.0f*qx*qw;
	dummyMatrix.matrix.at.z = 1.0f - 2.0f*qx*qx - 2.0f*qy*qy;

	dummyMatrix.matrix.pos.x = pObj->PositionX;
	dummyMatrix.matrix.pos.y = pObj->PositionY;
	dummyMatrix.matrix.pos.z = pObj->PositionZ;
	
	if (GetDistance((RwV3d*)&CVector(pObj->PositionX, pObj->PositionY, pObj->PositionZ), (RwV3d*)&CVector(-278.37f, -1377.48f, 90.98f)) <= 300.0f)
		return;

	auto	itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
	for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
		m_pLampposts->push_back(CLamppostInfo(dummyMatrix * it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, atan2(dummyMatrix.GetUp()->y, -dummyMatrix.GetUp()->x)));
}

void CLODLightManager::IV::RegisterLODLights()
{
	if (m_bCatchLamppostsNow)
		m_bCatchLamppostsNow = false;

	if (*CurrentTimeHours > 19 || *CurrentTimeHours < 7)
	{
		unsigned char	bAlpha = 0;
		float	        fRadius = 0.0f;
		unsigned int	nTime = *CurrentTimeHours * 60 + *CurrentTimeMinutes;
		unsigned int    curMin = *CurrentTimeMinutes;

		if (nTime >= 19 * 60)
			bAlpha = static_cast<unsigned char>((3.0f / 4.0f)*nTime - 825.0f); // http://goo.gl/O03RpE {(19*60)a + y = 30,  (24*60)a + y = 255}
		else if (nTime < 3 * 60)
			bAlpha = 255;
		else
			bAlpha = static_cast<unsigned char>((-15.0f / 16.0f)*nTime + 424.0f); // http://goo.gl/M8Dev9 {(7*60)a + y = 30,  (3*60)a + y = 255}

		for (auto it = m_pLampposts->cbegin(); it != m_pLampposts->cend(); it++)
		{
			static int currentCamera;
			GetRootCam(&currentCamera);
			if ((it->vecPos.z >= -15.0f) && (it->vecPos.z <= 1030.0f) /*&& CamIsSphereVisible(currentCamera, it->vecPos.x, it->vecPos.y, it->vecPos.z, 3.0f)*/)
			{
				CVector	CamPos = CVector();
				GetCamPos(currentCamera, &CamPos.x, &CamPos.y, &CamPos.z);
				CVector*	pCamPos = &CamPos;
				float		fDistSqr = (pCamPos->x - it->vecPos.x)*(pCamPos->x - it->vecPos.x) + (pCamPos->y - it->vecPos.y)*(pCamPos->y - it->vecPos.y) + (pCamPos->z - it->vecPos.z)*(pCamPos->z - it->vecPos.z);

				if ((fDistSqr > 250.0f*250.0f && fDistSqr < fCoronaFarClip*fCoronaFarClip) || it->nNoDistance)
				{
					if (it->nNoDistance)
						fRadius = 3.5f;
					else
						fRadius = (fDistSqr < 300.0f*300.0f) ? (0.07f)*sqrt(fDistSqr) - 17.5f : 3.5f; // http://goo.gl/vhAZSx

					if (bSlightlyIncreaseRadiusWithDistance)
						fRadius *= min((0.00136364f)*sqrt(fDistSqr) + 0.590909f, 3.0f); // http://goo.gl/3kDpnC {(300)a + y = 1.0,  (2500)a + y = 4}

					if (it->fCustomSizeMult != 0.45f)
					{
						if (!it->nCoronaShowMode)
						{
							//DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
							DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
						}
						//else
						//{
						//	static float blinking = 1.0f;
						//	if (IsBlinkingNeeded(it->nCoronaShowMode))
						//		blinking -= CTimer::ms_fTimeStep / 1000.0f;
						//	else
						//		blinking += CTimer::ms_fTimeStep / 1000.0f;
						//
						//	(blinking > 1.0f) ? blinking = 1.0f : (blinking < 0.0f) ? blinking = 0.0f : 0.0f;
						//
						//	CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, blinking * (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0xFF, 255.0f, false, false);
						//}
					}
					else
					{
						fRadius *= 1.3f;
						if ((it->colour.r >= 250 && it->colour.g >= 100 && it->colour.b <= 100) && ((curMin == 9 || curMin == 19 || curMin == 29 || curMin == 39 || curMin == 49 || curMin == 59))) //yellow
						{
							//DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
							DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
						}
						else
						{
							if ((abs(it->fHeading) >= (3.1415f / 6.0f) && abs(it->fHeading) <= (5.0f * 3.1415f / 6.0f)))
							{
								if ((it->colour.r >= 250 && it->colour.g < 100 && it->colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
								{
									//DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
									DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
								}
								else
								{
									if ((it->colour.r == 0 && it->colour.g >= 100 && it->colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
									{
										//DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
										DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
									}
								}
							}
							else
							{
								if ((it->colour.r == 0 && it->colour.g >= 250 && it->colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
								{
									//DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
									DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
								}
								else
								{
									if ((it->colour.r >= 250 && it->colour.g < 100 && it->colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
									{
										//DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
										DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		auto& gvm = injector::address_manager::singleton();

		if ((gvm.IsIV() && gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0 && gvm.GetMajorRevisionVersion() == 0 && gvm.GetMinorRevisionVersion() == 7)
		|| gvm.IsEFLC() && gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 1 && gvm.GetMajorRevisionVersion() == 2 && gvm.GetMinorRevisionVersion() == 0)
		{
			if (gvm.IsUS())
			{
				CLODLightManager::IV::Init();
			}
		}
	}
	return TRUE;
}