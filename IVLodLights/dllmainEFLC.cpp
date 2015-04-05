#include "stdafx.h"
#include "..\includes\CLODLightManager.h"

using namespace injector;
#define NewLimitExponent 14

std::vector<int>					    aCoronas;
std::vector<int>					    aCoronas2;


void CLODLightManager::IV::IncreaseCoronaLimit()
{
    nCoronasLimit = static_cast<DWORD>(3 * pow(2.0, NewLimitExponent)); // 49152, default 3 * pow(2, 8) = 768

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


void CLODLightManager::EFLC::RenderLODLights()
{
    if (*CurrentTimeHours > 19 || *CurrentTimeHours < 7)
    {
        unsigned char	bAlpha;
        unsigned int	nTime = *CurrentTimeHours * 60 + *CurrentTimeMinutes;

        if (nTime >= 20 * 60)
            bAlpha = static_cast<unsigned char>((5.0f / 6.0f)*nTime - 1000.0f);
        else if (nTime < 3 * 60)
            bAlpha = 200;
        else
            bAlpha = static_cast<unsigned char>((-5.0f / 6.0f)*nTime + 350.0f);

        for (auto it = m_pLampposts->cbegin(); it != m_pLampposts->cend(); it++)
        {
            GetRootCam(&currentCamera);
            if (CamIsSphereVisible(currentCamera, it->vecPos.x, it->vecPos.y, it->vecPos.z, 150.0f) || true)
            {
                Vector3	pCamPos;
                GetCamPos(currentCamera, &pCamPos.X, &pCamPos.Y, &pCamPos.Z);
                float		fDistSqr = (pCamPos.X - it->vecPos.x)*(pCamPos.X - it->vecPos.x) + (pCamPos.Y - it->vecPos.y)*(pCamPos.Y - it->vecPos.y) + (pCamPos.Z - it->vecPos.z)*(pCamPos.Z - it->vecPos.z);

                if ((fDistSqr > 280.0f*280.0f && fDistSqr < CoronaFarClip*CoronaFarClip) || it->nNoDistance)
                {
                    if (it->nNoDistance && fDistSqr <= 280.0f*280.0f) { fDistSqr = 290.0f*290.0f; }
                    float	fRadius = min((fDistSqr < 290.0f*290.0f) ? (1.0f / 4.0f)*sqrt(fDistSqr) - 70.0f : (3.0f / 164.0f)*sqrt(fDistSqr) - (5.0f / 41.0f), 20.0f);

                    if (it->fCustomSizeMult != 0.45f)
                    {
                        DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, fRadius * 40.0f, 0, 0.0f, (bAlpha / 500.0f) * it->colour.r, (bAlpha / 500.0f) * it->colour.g, (bAlpha / 500.0f) * it->colour.b);
                    }
                    else
                    {
                        if ((*CurrentTimeMinutes > 0 && *CurrentTimeMinutes < 10) || (*CurrentTimeMinutes > 20 && *CurrentTimeMinutes < 30) || (*CurrentTimeMinutes > 40 && *CurrentTimeMinutes < 50))
                        {
                            it++;
                        }

                        if (it->fCustomSizeMult == 0.45f)
                        {
                            DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, fRadius * 30.0f, 0, 0.0f, (bAlpha / 500.0f) * it->colour.r, (bAlpha / 500.0f) * it->colour.g, (bAlpha / 500.0f) * it->colour.b);
                            it++;
                        }

                        if (it->fCustomSizeMult == 0.45f){
                            DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, fRadius * 30.0f, 0, 0.0f, (bAlpha / 500.0f) * it->colour.g, (bAlpha / 500.0f) * it->colour.r, (bAlpha / 500.0f) * it->colour.b);
                        }
                        else it--;
                    }
                }
            }
        }
    }
}

void CLODLightManager::IV::RenderLODLights()
{
    if (*CurrentTimeHours > 19 || *CurrentTimeHours < 7)
    {
        unsigned char	bAlpha;
        unsigned int	nTime = *CurrentTimeHours * 60 + *CurrentTimeMinutes;

        if (nTime >= 20 * 60)
            bAlpha = static_cast<unsigned char>((5.0f / 6.0f)*nTime - 1000.0f);
        else if (nTime < 3 * 60)
            bAlpha = 200;
        else
            bAlpha = static_cast<unsigned char>((-5.0f / 6.0f)*nTime + 350.0f);

        for (auto it = m_pLampposts->cbegin(); it != m_pLampposts->cend(); it++)
        {
            GetRootCam(&currentCamera);
            //if (CamIsSphereVisible(currentCamera, it->vecPos.x, it->vecPos.y, it->vecPos.z, 150.0f) || true)
            //{
                Vector3	pCamPos;
                GetCamPos(currentCamera, &pCamPos.X, &pCamPos.Y, &pCamPos.Z);
                float		fDistSqr = (pCamPos.X - it->vecPos.x)*(pCamPos.X - it->vecPos.x) + (pCamPos.Y - it->vecPos.y)*(pCamPos.Y - it->vecPos.y) + (pCamPos.Z - it->vecPos.z)*(pCamPos.Z - it->vecPos.z);

                if ((fDistSqr > 280.0f*280.0f && fDistSqr < CoronaFarClip*CoronaFarClip) || it->nNoDistance)
                {
                    if (it->nNoDistance && fDistSqr <= 280.0f*280.0f) { fDistSqr = 290.0f*290.0f; }
                    float	fRadius = min((fDistSqr < 290.0f*290.0f) ? (1.0f / 4.0f)*sqrt(fDistSqr) - 70.0f : (3.0f / 164.0f)*sqrt(fDistSqr) - (5.0f / 41.0f), 20.0f);

                    if (it->fCustomSizeMult != 0.45f)
                    {
                        DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, fRadius * 40.0f, 0, 0.0f, (bAlpha / 500.0f) * it->colour.r, (bAlpha / 500.0f) * it->colour.g, (bAlpha / 500.0f) * it->colour.b);
                    }
                }
            //}
        }
    }
}

void __declspec(naked) CLODLightManager::IV::LoadObjectInstanceHook()
{
    _asm
    {
        mov     ebx, [ebp + 8]
        mov pInstance, ebx
    }
    PossiblyAddThisEntity(pInstance);
    _asm
    {
        mov     ebx, [ebp + 8]
        mov     ecx, [ebx + 1Ch]
        jmp hookJmpAddr
    }
}

void CLODLightManager::IV::GetMemoryAddresses()
{
    if (injector::address_manager::singleton().IsIV())
    {
        CLODLightManager::IV::CurrentTimeHours = aslr_ptr(0x11D5300).get();
        CLODLightManager::IV::CurrentTimeMinutes = aslr_ptr(0x11D52FC).get();
        CLODLightManager::IV::DrawCorona = (int(__cdecl *)(float, float, float, float, unsigned int, float, unsigned char, unsigned char, unsigned char))(aslr_ptr(0xA6E240).get());
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
        CLODLightManager::EFLC::GetRootCam = (void(__stdcall *)(int *camera))(aslr_ptr(0xAFA7E0).get());
        CLODLightManager::EFLC::CamIsSphereVisible = (bool(__cdecl *)(int camera, float pX, float pY, float pZ, float radius))(aslr_ptr(0xC27DD0).get());
        CLODLightManager::EFLC::GetCamPos = (void(__cdecl *)(int camera, float *pX, float *pY, float *pZ))(aslr_ptr(0xC26FA0).get());
    }
}

void CLODLightManager::IV::Init()
{
    GetMemoryAddresses();
    
    IncreaseCoronaLimit();
    LoadDatFile();
    RegisterCustomCoronas();
    CoronaFarClip = 9000.0f;

    if (injector::address_manager::singleton().IsIV())
    {
        injector::WriteMemory<uint8_t>(aslr_ptr(0x402B49), 0xEB, true); // skip intro

        //injector::WriteMemory<unsigned char>(aslr_ptr(0x00903300), 0xC3u, true); // Disable default LOD lights
        injector::MakeNOP(aslr_ptr(0xB54373), 5, true); // Disable all coronas water reflections

        injector::MakeJMP(aslr_ptr(0x008D63A1), LoadObjectInstanceHook, true);
        hookJmpAddr = (uintptr_t)injector::aslr_ptr(0x8D63A7).get<void>();

        //injector::MakeCALL(aslr_ptr(0x00402D6C), RenderLODLights, true);
        injector::MakeCALL(aslr_ptr(0x009056C9), RenderLODLights, true);
    }
    else 
    {
        injector::WriteMemory<uint8_t>(aslr_ptr(0x473439), 0xEB, true); // skip intro
        injector::MakeNOP(aslr_ptr(0x402D5A), 2, true); // ALLOW ALT+TABBING WITHOUT PAUSING

        injector::WriteMemory<unsigned char>(aslr_ptr(0x00975860), 0xC3u, true); // Disable default LOD lights
        injector::MakeNOP(aslr_ptr(0xC8E183), 5, true); // Disable all coronas water reflections

        injector::MakeJMP(aslr_ptr(0x0091C851), LoadObjectInstanceHook, true);
        hookJmpAddr = (uintptr_t)injector::aslr_ptr(0x91C857).get<void>();

        injector::MakeCALL(aslr_ptr(0x0047361F), CLODLightManager::EFLC::RenderLODLights);
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