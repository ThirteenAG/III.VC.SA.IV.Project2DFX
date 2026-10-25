module;

#include <stdafx.h>
#include <functional>
#include <string>
#include <limits>
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
    float fPoolBaseZ{ std::numeric_limits<float>::quiet_NaN() };
    CRGBA colour;
    float fCustomSizeMult;
    int nCoronaShowMode;
    int nNoDistance;
    int nDrawSearchlight;
    float fHeading;
    float fObjectDrawDistance;
    uint8_t nTrafficLightType = 2, nTrafficLightState = 3;
    CoronaPredicate pPredicate;

    // The game's own corona range of this light, 0 when no entity carries it
    float fGameCoronaRange = 0.0f;
    // Index into the entities the game processes, -1 when the light has no entity
    int32_t nEntity = -1;
    // How far the game's own corona has faded in, eased like the game eases it
    float fGameFade = 0.0f;
    uint16_t nPoolModel = 0;
    CModelInfo::LightPoolData pool;
    std::string poolTexture;

    CLamppostInfo(const CVector& pos, const CVector& localpos, const CRGBA& col, float fCustomMult, int CoronaShowMode, int nNoDistance, int nDrawSearchlight, float heading, float ObjectDrawDistance = 0.0f, CoronaPredicate pPred = nullptr)
        : vecPos(pos), vecLocalPos(localpos), colour(col), fCustomSizeMult(fCustomMult), nCoronaShowMode(CoronaShowMode), nNoDistance(nNoDistance), nDrawSearchlight(nDrawSearchlight), fHeading(heading), fObjectDrawDistance(ObjectDrawDistance), pPredicate(pPred)
    {
        if (col.r >= 250 && col.g >= 100 && col.b <= 150) nTrafficLightState = 1;
        else if (col.r >= 250 && col.g < 100 && col.b == 0) nTrafficLightState = 2;
        else if (col.r == 0 && col.g >= 250 && col.b == 0) nTrafficLightState = 0;
    }
};

export bool m_bCatchLamppostsNow;
export std::vector<CLamppostInfo> m_Lampposts;

// Lights are matched to the entities the game processes by the entity's position: a dummy and the object the game
// makes from it share it.
export namespace LampEntities
{
    std::unordered_map<uint64_t, int32_t> Keys;
    std::vector<uint32_t> LastProcessed;
    uint32_t Frame = 16;

    uint64_t Key(const CVector& p)
    {
        auto q = [](float v) { return static_cast<uint64_t>(static_cast<int64_t>(std::lround(v * 16.0f)) & 0x1FFFFF); };
        return q(p.x) | q(p.y) << 21 | q(p.z) << 42;
    }

    std::vector<RwTexture*> PoolTextures;

    void RetainPoolTexture(RwTexture* texture)
    {
        if (texture && std::find(PoolTextures.begin(), PoolTextures.end(), texture) == PoolTextures.end())
        {
            ++texture->refCount;
            PoolTextures.push_back(texture);
        }
    }

    // Gives the lights from firstLamp on to the entity at entityPos
    void Attach(size_t firstLamp, const CVector& entityPos, float gameCoronaRange, uint16_t model)
    {
        if (firstLamp >= m_Lampposts.size())
            return;
        auto [it, inserted] = Keys.try_emplace(Key(entityPos), static_cast<int32_t>(LastProcessed.size()));
        if (inserted)
            LastProcessed.push_back(0);
        for (size_t i = firstLamp; i < m_Lampposts.size(); ++i)
        {
            auto& lamp = m_Lampposts[i];
            lamp.nEntity = it->second;
            lamp.fGameCoronaRange = gameCoronaRange > 0.0f ? gameCoronaRange : lamp.fGameCoronaRange;
            lamp.nPoolModel = model;
        }
    }

    // Called when the game processes the lights of an entity
    void MarkProcessed(const CVector& entityPos)
    {
        if (Keys.empty())
            return;
        auto it = Keys.find(Key(entityPos));
        if (it != Keys.end())
            LastProcessed[it->second] = Frame;
    }

    bool Processed(int32_t entity)
    {
        return entity >= 0 && Frame - LastProcessed[entity] <= 1;
    }

    void Clear()
    {
        for (auto* texture : PoolTextures)
            CModelInfo::ReleasePoolTexture(texture);
        PoolTextures.clear();
        Keys.clear();
        LastProcessed.clear();
    }
}

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

    return signed(CTimer::GetEffectsTimeInMilliseconds() % (nOnDuration + nOffDuration)) < nOnDuration;
}

export std::map<unsigned int, CLamppostInfo> FileContent;
export inline unsigned int PackKey(unsigned short nModel, unsigned short nIndex)
{
    return nModel << 16 | nIndex;
}

export void LoadDatFile()
{
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&LoadDatFile), &module);
    wchar_t modulePath[MAX_PATH * 4] = {};
    GetModuleFileNameW(module, modulePath, DWORD(std::size(modulePath)));
    std::filesystem::path DataFilePath(modulePath);
    DataFilePath.replace_extension(".dat");

    m_Lampposts.clear();
    LampEntities::Clear();
    FileContent.clear();

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
                    float fOffsetX{}, fOffsetY{}, fOffsetZ{};
                    unsigned int nRed{}, nGreen{}, nBlue{}, nAlpha{};
                    float fCustomSize{}, fDrawDistance{};
                    int nNoDistance{}, nDrawSearchlight{}, nCoronaShowMode{};
                    char texture[25]{};
                    float poolSize{}, poolIntensity{}, poolDepth{};
                    unsigned int poolRed{}, poolGreen{}, poolBlue{};
                    const int fields{ sscanf(pLine, "%u %u %u %u %f %f %f %f %f %d %d %d %24s %f %f %f %u %u %u",
                        &nRed, &nGreen, &nBlue, &nAlpha, &fOffsetX, &fOffsetY, &fOffsetZ, &fCustomSize, &fDrawDistance,
                        &nCoronaShowMode, &nNoDistance, &nDrawSearchlight, texture, &poolSize, &poolIntensity, &poolDepth,
                        &poolRed, &poolGreen, &poolBlue) };
                    if ((fields != 12 && fields != 19) || nRed > 255 || nGreen > 255 || nBlue > 255 || nAlpha > 255 ||
                        !std::isfinite(fOffsetX) || !std::isfinite(fOffsetY) || !std::isfinite(fOffsetZ) ||
                        !std::isfinite(fCustomSize) || !std::isfinite(fDrawDistance))
                        continue;
                    CLamppostInfo info(CVector(0.0f, 0.0f, 0.0f), CVector(fOffsetX, fOffsetY, fOffsetZ), CRGBA(static_cast<unsigned char>(nRed), static_cast<unsigned char>(nGreen), static_cast<unsigned char>(nBlue), static_cast<unsigned char>(nAlpha)), fCustomSize, nCoronaShowMode, nNoDistance, nDrawSearchlight, 0.0f, fDrawDistance);
                    if (fields == 19 && texture[0] != '-' && std::isfinite(poolSize) && poolSize > 0.0f && poolSize <= 30.0f &&
                        std::isfinite(poolIntensity) && poolIntensity > 0.0f && poolIntensity <= 255.0f &&
                        std::isfinite(poolDepth) && poolDepth > 0.0f && poolDepth <= 255.0f &&
                        poolRed <= 255 && poolGreen <= 255 && poolBlue <= 255 && !(poolRed > poolGreen * 2 && poolRed > poolBlue * 2))
                    {
                        info.poolTexture = texture;
                        info.pool.size = poolSize;
                        info.pool.intensity = poolIntensity;
                        info.pool.depth = poolDepth;
                        info.pool.colour = CRGBA(static_cast<unsigned char>(poolRed), static_cast<unsigned char>(poolGreen), static_cast<unsigned char>(poolBlue), 255);
                    }
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