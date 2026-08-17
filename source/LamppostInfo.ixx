module;

#include <stdafx.h>
#include <functional>
#include <string>
#include <unordered_map>

export module LamppostInfo;

import Timer;
import FileMgr;
import ModelInfo;

export enum BlinkTypes
{
    DEFAULT,
    RANDOM_FLASHING,
    T_1S_ON_1S_OFF,
    T_2S_ON_2S_OFF,
    T_3S_ON_3S_OFF,
    T_4S_ON_4S_OFF,
    T_5S_ON_5S_OFF,
    T_6S_ON_4S_OFF
};

// ---------------------------------------------------------------------------
// Per-object corona visibility predicates.
//
// Game code installs a predicate per model name before LoadDatFile() is
// called. The name must match the dat-file section name exactly, including
// the leading '%' (e.g. "%nbbridgfk2"). Every corona formed for that model
// carries the predicate for its lifetime, and the render loop checks it each
// frame. Models without a predicate cost a single null check.
// ---------------------------------------------------------------------------
export using CoronaPredicate = std::function<bool()>;

export class CCoronaVisibility
{
public:
    static inline std::unordered_map<std::string, CoronaPredicate> ModelPredicates;

    // p == nullptr removes a previously installed predicate.
    static void SetModelPredicate(const char* szModelName, CoronaPredicate p)
    {
        if (p)
            ModelPredicates.emplace(szModelName, p);
        else
            ModelPredicates.erase(szModelName);
    }

    // Predicate installed for a model, or nullptr if none.
    static CoronaPredicate GetModelPredicate(const char* szModelName)
    {
        auto it = ModelPredicates.find(szModelName);
        return it != ModelPredicates.end() ? it->second : nullptr;
    }
};

export class CLamppostInfo
{
public:
    CVector vecPos;
    CVector vecLocalPos;
    CRGBA colour;
    float fCustomSizeMult;
    int nCoronaShowMode;
    int nNoDistance;
    int nDrawSearchlight;
    float fHeading;
    float fObjectDrawDistance;
    CoronaPredicate pPredicate;

    CLamppostInfo(const CVector& pos, const CVector& localpos, const CRGBA& col, float fCustomMult, int CoronaShowMode, int nNoDistance, int nDrawSearchlight, float heading, float ObjectDrawDistance = 0.0f, CoronaPredicate pPred = nullptr)
        : vecPos(pos), vecLocalPos(localpos), colour(col), fCustomSizeMult(fCustomMult), nCoronaShowMode(CoronaShowMode), nNoDistance(nNoDistance), nDrawSearchlight(nDrawSearchlight), fHeading(heading), fObjectDrawDistance(ObjectDrawDistance), pPredicate(pPred)
    {
    }
};

export bool m_bCatchLamppostsNow;
export std::vector<CLamppostInfo> m_Lampposts;

export float GetDistance(RwV3d* v1, RwV3d* v2)
{
    RwV3d v3;
    v3.x = v2->x - v1->x;
    v3.y = v2->y - v1->y;
    v3.z = v2->z - v1->z;
    return sqrt(v3.x * v3.x + v3.y * v3.y + v3.z * v3.z);
}

export bool IsBlinkingNeeded(int BlinkType)
{
    signed int nOnDuration = 0;
    signed int nOffDuration = 0;

    switch (BlinkType)
    {
        case BlinkTypes::DEFAULT:
            return false;
        case BlinkTypes::RANDOM_FLASHING:
            nOnDuration = 500;
            nOffDuration = 500;
            break;
        case BlinkTypes::T_1S_ON_1S_OFF:
            nOnDuration = 1000;
            nOffDuration = 1000;
            break;
        case BlinkTypes::T_2S_ON_2S_OFF:
            nOnDuration = 2000;
            nOffDuration = 2000;
            break;
        case BlinkTypes::T_3S_ON_3S_OFF:
            nOnDuration = 3000;
            nOffDuration = 3000;
            break;
        case BlinkTypes::T_4S_ON_4S_OFF:
            nOnDuration = 4000;
            nOffDuration = 4000;
            break;
        case BlinkTypes::T_5S_ON_5S_OFF:
            nOnDuration = 5000;
            nOffDuration = 5000;
            break;
        case BlinkTypes::T_6S_ON_4S_OFF:
            nOnDuration = 6000;
            nOffDuration = 4000;
            break;
        default:
            return false;
    }

    return signed(CTimer::m_snTimeInMillisecondsPauseMode % (nOnDuration + nOffDuration)) < nOnDuration;
}

export std::map<unsigned int, CLamppostInfo> FileContent;
export inline unsigned int PackKey(unsigned short nModel, unsigned short nIndex)
{
    return nModel << 16 | nIndex;
}

export void LoadDatFile()
{
    CIniReader iniReader("");
    auto DataFilePath = iniReader.GetIniPath();
    DataFilePath.replace_extension(".dat");

    if (FILE* hFile = CFileMgr::OpenFile(DataFilePath.string().c_str(), "r"))
    {
        unsigned short nModel = 0xFFFF, nCurIndexForModel = 0;
        CoronaPredicate pCurPredicate = nullptr;

        while (const char* pLine = CFileMgr::LoadLine(hFile))
        {
            if (pLine[0] && pLine[0] != '#')
            {
                if (pLine[0] == '%')
                {
                    nCurIndexForModel = 0;
                    pCurPredicate = CCoronaVisibility::GetModelPredicate(pLine);
                    if (strcmp(pLine, "%additional_coronas") != 0)
                    {
                        int nID = 0;
                        CModelInfo::GetModelInfo(pLine + 1, &nID);
                        nModel = static_cast<unsigned short>(nID);
                    }
                    else
                    {
                        nModel = 65534;
                    }
                }
                else
                {
                    float			fOffsetX, fOffsetY, fOffsetZ;
                    unsigned int	nRed, nGreen, nBlue, nAlpha;
                    float			fCustomSize = 1.0f;
                    float			fDrawDistance = 0.0f;
                    int				nNoDistance = 0;
                    int				nDrawSearchlight = 0;
                    int				nCoronaShowMode = 0;
                    if (sscanf(pLine, "%3d %3d %3d %3d %f %f %f %f %f %2d %1d %1d", &nRed, &nGreen, &nBlue, &nAlpha, &fOffsetX, &fOffsetY, &fOffsetZ, &fCustomSize, &fDrawDistance, &nCoronaShowMode, &nNoDistance, &nDrawSearchlight) != 12)
                        sscanf(pLine, "%3d %3d %3d %3d %f %f %f %f %2d %1d %1d", &nRed, &nGreen, &nBlue, &nAlpha, &fOffsetX, &fOffsetY, &fOffsetZ, &fCustomSize, &nCoronaShowMode, &nNoDistance, &nDrawSearchlight);
                    CLamppostInfo info(CVector(0.0f, 0.0f, 0.0f), CVector(fOffsetX, fOffsetY, fOffsetZ), CRGBA(static_cast<unsigned char>(nRed), static_cast<unsigned char>(nGreen), static_cast<unsigned char>(nBlue), static_cast<unsigned char>(nAlpha)), fCustomSize, nCoronaShowMode, nNoDistance, nDrawSearchlight, 0.0f, fDrawDistance);
                    info.pPredicate = pCurPredicate;
                    FileContent.insert(std::make_pair(PackKey(nModel, nCurIndexForModel++), info));
                }
            }
        }

        m_bCatchLamppostsNow = true;
        CFileMgr::CloseFile(hFile);
    }
}

export bool IsModelALamppost(unsigned short nModel)
{
    auto it = FileContent.lower_bound(PackKey(nModel, 0));
    return it != FileContent.end() && it->first >> 16 == nModel;
}