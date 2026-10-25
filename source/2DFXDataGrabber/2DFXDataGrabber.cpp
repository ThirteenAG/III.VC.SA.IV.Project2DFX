#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Portable.hpp"

namespace fs = std::filesystem;

namespace
{
    using Bytes = std::vector<uint8_t>;

    struct Light
    {
        std::string model;
        std::array<float, 3> position{};
        std::array<unsigned, 4> colour{ 0, 0, 0, 255 };
        std::array<unsigned, 3> poolColour{};
        float size{}, distance{}, poolSize{}, intensity{}, depth{ 15.0f };
        unsigned blink{}, noDistance{}, searchlight{}, nativeMode{};
        std::string texture{ "-" };
    };

    std::string Lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    std::string Trim(const std::string& s)
    {
        const auto first{ s.find_first_not_of(" \t\r\n") };
        return first == s.npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
    }

    std::vector<std::string> Tokens(std::string line)
    {
        const auto comment{ line.find('#') };
        if (comment != line.npos)
            line.resize(comment);
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream input(line);
        std::vector<std::string> result;
        std::string token;
        while (input >> std::quoted(token))
            result.push_back(token);
        return result;
    }

    float Number(const std::string& token)
    {
        size_t used{};
        const float value{ std::stof(token, &used) };
        if (used != token.size() || !std::isfinite(value))
            throw std::runtime_error("Invalid number: " + token);
        return value;
    }

    unsigned Integer(const std::string& token)
    {
        size_t used{};
        const auto value{ std::stoul(token, &used) };
        if (used != token.size() || token.front() == '-' || value > UINT32_MAX)
            throw std::runtime_error("Invalid unsigned integer: " + token);
        return static_cast<unsigned>(value);
    }

    template<typename T> T Read(const Bytes& data, size_t offset)
    {
        if (offset > data.size() || sizeof(T) > data.size() - offset)
            throw std::runtime_error("Truncated binary record");
        T value{};
        std::memcpy(&value, data.data() + offset, sizeof(value));
        return value;
    }

    std::string Name(const Bytes& data, size_t offset, size_t length)
    {
        if (offset > data.size() || length > data.size() - offset)
            throw std::runtime_error("Truncated binary name");
        std::string result;
        for (size_t i{}; i < length && data[offset + i]; ++i)
        {
            const auto c{ data[offset + i] };
            if (c <= 32 || c > 126 || c == '"')
                throw std::runtime_error("Invalid binary name");
            result.push_back(static_cast<char>(c));
        }
        return Lower(result);
    }

    Bytes Load(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            throw std::runtime_error("Cannot open " + path.string());
        const auto size{ file.tellg() };
        if (size < 0 || size > 256 * 1024 * 1024)
            throw std::runtime_error("Invalid file size: " + path.string());
        Bytes data(static_cast<size_t>(size));
        file.seekg(0);
        if (!file.read(reinterpret_cast<char*>(data.data()), data.size()))
            throw std::runtime_error("Cannot read " + path.string());
        return data;
    }

    unsigned Blink(unsigned type, bool sa)
    {
        if (sa)
        {
            switch (type)
            {
            case 3: return 2;
            case 4: return 3;
            case 5: return 5;
            case 11: return 6;
            case 12: case 13: return 7;
            default: return 0;
            }
        }
        switch (type)
        {
        case 2: case 3: case 10: return 1;
        case 4: case 5: case 13: return 2;
        case 6: case 7: return 3;
        case 8: case 9: return 5;
        case 14: return 7;
        default: return 0;
        }
    }

    bool Searchlight(const std::string& model, bool sa)
    {
        static const std::vector<std::string> common{ "lamppost1", "lamppost2", "lamppost3", "sub_floodlite", "mlamppost", "bollardlight" };
        static const std::vector<std::string> classic{ "doublestreetlght1", "lampost_coast", "doc_floodlite" };
        static const std::vector<std::string> sanAndreas{ "vegaslampost", "streetlamp1", "streetlamp2", "ce_radarmast3", "radarmast1_lawn", "cj_traffic_light5", "lamppost_coast", "airuntest_las", "vegasairportlight", "gay_lamppost", "gay_traffic_light", "cj_traffic_light4", "lodnsmitter_sfs01", "lodrailbge01_sfse", "lodrailbge02_sfse", "lodrailbge04_sfse", "lodrailbge09_sfse", "lodtainercrane_04", "lod_refchimny01", "mtraffic1", "mtraffic2", "mtraffic3", "mtraffic4", "trafficlight1", "vgsstriptlights1", "circuslampost03" };
        const auto contains{ [&](const auto& names)
        {
            return std::any_of(names.begin(), names.end(), [&](const auto& name) { return model.find(name) != model.npos; });
        } };
        return contains(common) || contains(sa ? sanAndreas : classic);
    }

    class Extractor
    {
        fs::path root;
        bool sa;
        std::map<unsigned, std::string> models;
        std::vector<fs::path> archives, placements;
        std::set<fs::path> manifests;
        std::set<std::string> extracted;

        fs::path Resolve(const std::string& name, bool optional = false) const
        {
            fs::path path{ name };
            if (fs::exists(root / path))
                return root / path;
            // Also accept the original copied-data directory layout.
            auto it{ path.begin() };
            if (it != path.end() && Lower(it->string()) == "data")
            {
                fs::path relative;
                for (++it; it != path.end(); ++it)
                    relative /= *it;
                if (fs::exists(root / relative))
                    return root / relative;
            }
            if (optional)
                return {};
            throw std::runtime_error("Missing input: " + (root / path).string());
        }

        void ReadIDE(const fs::path& path)
        {
            std::ifstream file(path);
            if (!file)
                throw std::runtime_error("Cannot open " + path.string());
            std::string line, section;
            size_t lineNumber{};
            while (std::getline(file, line))
            {
                ++lineNumber;
                const auto t{ Tokens(line) };
                if (t.empty())
                    continue;
                if (t.size() == 1)
                {
                    section = Lower(t[0]);
                    continue;
                }
                try
                {
                    if (section == "objs" || section == "tobj" || section == "anim" || section == "hier")
                        models[Integer(t[0])] = Lower(t.at(1));
                    else if (section == "2dfx" && !sa)
                    {
                        if (t.size() < 9 || Integer(t[8]) != 0)
                            continue;
                        if (t.size() != 20)
                            throw std::runtime_error("Expected 20 light fields");
                        Light light;
                        const auto id{ Integer(t[0]) };
                        light.model = std::to_string(id);
                        for (size_t i{}; i < 3; ++i)
                        {
                            light.position[i] = Number(t[i + 1]);
                            light.colour[i] = Integer(t[i + 4]);
                            light.poolColour[i] = light.colour[i];
                            if (light.colour[i] > 255)
                                throw std::runtime_error("Colour out of range");
                        }
                        light.texture = Lower(t[10]);
                        light.distance = Number(t[11]);
                        light.size = Number(t[13]);
                        light.poolSize = Number(t[14]);
                        light.intensity = static_cast<float>(Integer(t[15]));
                        light.blink = Blink(Integer(t[16]), false);
                        // Keep light records independently of corona filtering for baseline matching.
                        lights.push_back(light);
                    }
                }
                catch (const std::exception& e)
                {
                    throw std::runtime_error(path.string() + ":" + std::to_string(lineNumber) + ": " + e.what());
                }
            }
        }

        void ReadManifest(const fs::path& path)
        {
            if (!manifests.insert(fs::weakly_canonical(path)).second)
                return;
            std::ifstream file(path);
            if (!file)
                throw std::runtime_error("Cannot open " + path.string());
            std::string line;
            while (std::getline(file, line))
            {
                const auto t{ Tokens(line) };
                if (t.size() < 2)
                    continue;
                const auto type{ Lower(t[0]) };
                if (type == "ide")
                    ReadIDE(Resolve(t[1]));
                else if (type == "ipl" && Lower(fs::path(t[1]).extension().string()) == ".ipl")
                {
                    // Portable bundles omit zone and occlusion placement files.
                    const auto placement{ Resolve(t[1], path.parent_path() == root) };
                    if (!placement.empty())
                        placements.push_back(placement);
                }
                else if (type == "img" && sa)
                    archives.push_back(Resolve(t[1]));
            }
        }

        void ReadEffects(const Bytes& data, size_t begin, size_t end, const std::string& model)
        {
            if (end - begin < 4)
                throw std::runtime_error("Truncated 2DFX header");
            const auto count{ Read<uint32_t>(data, begin) };
            size_t cursor{ begin + 4 };
            if (count > (end - cursor) / 20)
                throw std::runtime_error("Invalid 2DFX count");
            for (uint32_t i{}; i < count; ++i)
            {
                if (end - cursor < 20)
                    throw std::runtime_error("Truncated 2DFX record");
                const auto type{ Read<uint32_t>(data, cursor + 12) };
                const auto size{ Read<uint32_t>(data, cursor + 16) };
                const size_t payload{ cursor + 20 };
                if (size > end - payload)
                    throw std::runtime_error("2DFX payload exceeds chunk");
                if (type == 0)
                {
                    if (size != 76 && size != 80)
                        throw std::runtime_error("Unsupported light payload size " + std::to_string(size));
                    Light light;
                    light.model = model;
                    for (size_t axis{}; axis < 3; ++axis)
                        light.position[axis] = Read<float>(data, cursor + axis * 4);
                    for (size_t channel{}; channel < 3; ++channel)
                    {
                        light.colour[channel] = data[payload + channel];
                        light.poolColour[channel] = light.colour[channel];
                    }
                    light.distance = Read<float>(data, payload + 4);
                    light.size = Read<float>(data, payload + 12);
                    light.poolSize = Read<float>(data, payload + 16);
                    const auto flash{ data[payload + 20] };
                    light.nativeMode = flash;
                    light.blink = Blink(flash, true);
                    light.intensity = data[payload + 23];
                    light.texture = Name(data, payload + 49, 24);
                    const auto z{ data[payload + 73] };
                    light.depth = z ? static_cast<float>(z) : 15.0f;
                    light.searchlight = Searchlight(model, true);
                    if (flash == 7)
                    {
                        light.size = 0.45f;
                        light.searchlight = 0;
                    }
                    if (flash == 8)
                        light.size = 0.1f;
                    lights.push_back(light);
                }
                cursor = payload + size;
            }
            if (cursor != end)
                throw std::runtime_error("Trailing bytes in 2DFX chunk");
        }

        void ReadChunks(const Bytes& data, size_t begin, size_t end, const std::string& model, unsigned depth = 0)
        {
            if (depth > 16)
                throw std::runtime_error("RenderWare nesting too deep");
            for (size_t cursor{ begin }; cursor < end;)
            {
                if (end - cursor < 12)
                    throw std::runtime_error("Truncated RenderWare header");
                const auto type{ Read<uint32_t>(data, cursor) };
                const auto size{ Read<uint32_t>(data, cursor + 4) };
                if (size > end - cursor - 12)
                    throw std::runtime_error("RenderWare chunk exceeds parent");
                const size_t next{ cursor + 12 + size };
                if (type == 0x253F2F8)
                    ReadEffects(data, cursor + 12, next, model);
                else if (type == 3 || type == 0x10 || type == 0xF || type == 0x1A || type == 0x14)
                    ReadChunks(data, cursor + 12, next, model, depth + 1);
                cursor = next;
            }
        }

        void ReadTextEffects(const fs::path& path)
        {
            std::ifstream file(path);
            if (!file)
                throw std::runtime_error("Cannot open " + path.string());
            Light light;
            bool active{};
            std::string line;
            const auto finish{ [&]()
            {
                if (active)
                    lights.push_back(light);
            } };
            while (std::getline(file, line))
            {
                const auto t{ Tokens(line) };
                if (t.size() < 2)
                    continue;
                if (t[0] == "2dfxType")
                {
                    finish();
                    light = Light{};
                    light.model = Lower(path.stem().string());
                    active = t[1] == "LIGHT";
                }
                else if (active)
                {
                    if (t[0] == "Position")
                        for (size_t i{}; i < 3; ++i)
                            light.position[i] = Number(t.at(i + 1));
                    else if (t[0] == "Color")
                        for (size_t i{}; i < 3; ++i)
                            light.poolColour[i] = light.colour[i] = Integer(t.at(i + 1));
                    else if (t[0] == "CoronaSize") light.size = Number(t[1]);
                    else if (t[0] == "CoronaFarClip") light.distance = Number(t[1]);
                    else if (t[0] == "ShadowSize") light.poolSize = Number(t[1]);
                    else if (t[0] == "ShadowColorMP") light.intensity = Number(t[1]);
                    else if (t[0] == "ShadowZDistance")
                    {
                        light.depth = Number(t[1]);
                        if (light.depth == 0.0f)
                            light.depth = 15.0f;
                    }
                    else if (t[0] == "ShadowTexName") light.texture = Lower(t[1]);
                }
            }
            finish();
        }

        void ReadDFF(const Bytes& data, const std::string& model)
        {
            if (!extracted.insert(model).second)
                return;
            if (data.size() < 12)
                throw std::runtime_error("Truncated DFF: " + model);
            const size_t end{ 12ull + Read<uint32_t>(data, 4) };
            if (end > data.size())
                throw std::runtime_error("Truncated DFF root: " + model);
            ReadChunks(data, 0, end, model);
        }

        void ReadArchive(const fs::path& path)
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file)
                throw std::runtime_error("Cannot open " + path.string());
            const uint64_t fileSize{ static_cast<uint64_t>(file.tellg()) };
            file.seekg(0);
            Bytes header(8);
            if (!file.read(reinterpret_cast<char*>(header.data()), 8))
                throw std::runtime_error("Truncated IMG header");
            Bytes directory;
            bool version2{ std::memcmp(header.data(), "VER2", 4) == 0 };
            uint32_t count{};
            if (version2)
            {
                count = Read<uint32_t>(header, 4);
                if (count > (fileSize - 8) / 32)
                    throw std::runtime_error("Invalid IMG directory count");
                directory.resize(static_cast<size_t>(count) * 32);
                if (!file.read(reinterpret_cast<char*>(directory.data()), directory.size()))
                    throw std::runtime_error("Truncated IMG directory");
            }
            else
            {
                auto dir{ path };
                dir.replace_extension(".dir");
                directory = Load(dir);
                if (directory.size() % 32)
                    throw std::runtime_error("Invalid IMG v1 directory");
                count = static_cast<uint32_t>(directory.size() / 32);
            }
            for (uint32_t i{}; i < count; ++i)
            {
                const size_t entry{ static_cast<size_t>(i) * 32 };
                const auto name{ Name(directory, entry + 8, 24) };
                if (fs::path(name).extension() != ".dff")
                    continue;
                const uint64_t offset{ 2048ull * Read<uint32_t>(directory, entry) };
                const uint64_t sectors{ version2 ? Read<uint16_t>(directory, entry + 4) : Read<uint32_t>(directory, entry + 4) };
                const uint64_t length{ sectors * 2048 };
                if (!length || length > 128 * 1024 * 1024 || offset > fileSize || length > fileSize - offset)
                    throw std::runtime_error("Invalid IMG extent: " + name);
                Bytes data(static_cast<size_t>(length));
                file.seekg(offset);
                if (!file.read(reinterpret_cast<char*>(data.data()), data.size()))
                    throw std::runtime_error("Cannot read IMG entry: " + name);
                ReadDFF(data, fs::path(name).stem().string());
            }
        }

    public:
        std::vector<Light> lights;
        size_t instances{};

        Extractor(fs::path path, bool sanAndreas) : root(std::move(path)), sa(sanAndreas) {}

        void Run(const std::string& manifest)
        {
            if (fs::exists(root / "data" / "default.dat"))
                ReadManifest(root / "data" / "default.dat");
            if (fs::exists(root / "data" / manifest))
                ReadManifest(root / "data" / manifest);
            else if (fs::exists(root / manifest))
                ReadManifest(root / manifest);
            else if (!sa || !fs::exists(root / "2dfx"))
                throw std::runtime_error("Game manifest not found: " + manifest);

            for (const auto& path : placements)
            {
                const auto data{ Load(path) };
                if (data.size() >= 4 && std::memcmp(data.data(), "bnry", 4) == 0)
                {
                    const auto count{ Read<uint32_t>(data, 4) };
                    const auto offset{ Read<uint32_t>(data, 28) };
                    if (offset > data.size() || count > (data.size() - offset) / 40)
                        throw std::runtime_error("Truncated binary IPL: " + path.string());
                    for (uint32_t i{}; i < count; ++i)
                    {
                        for (size_t field{}; field < 7; ++field)
                            if (!std::isfinite(Read<float>(data, static_cast<size_t>(offset) + i * 40ull + field * 4)))
                                throw std::runtime_error("Non-finite binary IPL transform: " + path.string());
                    }
                    instances += count;
                }
                else
                {
                    std::istringstream file(std::string(data.begin(), data.end()));
                    std::string line, section;
                    while (std::getline(file, line))
                    {
                        const auto t{ Tokens(line) };
                        if (t.size() == 1)
                            section = Lower(t[0]);
                        else if (!t.empty() && section == "inst")
                        {
                            if (t.size() < 11)
                                throw std::runtime_error("Invalid IPL instance: " + path.string());
                            Integer(t[0]);
                            for (size_t field{ 2 }; field < t.size(); ++field)
                                Number(t[field]);
                            ++instances;
                        }
                    }
                }
            }

            if (sa)
            {
                // Loose models override archives, just as a modded extraction directory does.
                const auto loose{ root / "2dfx" };
                if (fs::exists(loose))
                {
                    std::vector<fs::path> paths;
                    for (const auto& entry : fs::directory_iterator(loose))
                        if (Lower(entry.path().extension().string()) == ".dff")
                            paths.push_back(entry.path());
                    std::sort(paths.begin(), paths.end());
                    for (const auto& path : paths)
                        ReadDFF(Load(path), Lower(path.stem().string()));
                    paths.clear();
                    for (const auto& entry : fs::directory_iterator(loose))
                        if (Lower(entry.path().extension().string()) == ".txt" && !extracted.count(Lower(entry.path().stem().string())))
                            paths.push_back(entry.path());
                    std::sort(paths.begin(), paths.end());
                    for (const auto& path : paths)
                        ReadTextEffects(path);
                }
                const auto main{ root / "models" / "gta3.img" };
                if (fs::exists(main))
                    archives.insert(archives.begin(), main);
                std::set<fs::path> seen;
                for (const auto& path : archives)
                    if (seen.insert(fs::weakly_canonical(path)).second)
                        ReadArchive(path);
            }
            else
            {
                std::map<std::string, unsigned> trafficIndices;
                for (auto& light : lights)
                {
                    const auto id{ Integer(light.model) };
                    const auto it{ models.find(id) };
                    if (it == models.end())
                        throw std::runtime_error("Unknown 2DFX model ID " + light.model);
                    light.model = it->second;
                    light.searchlight = Searchlight(light.model, false);
                    if (light.size > 1.0f)
                        light.size = 1.2f;
                    if (light.model == "mtraffic1" || light.model == "mtraffic2" || light.model == "mtraffic4" || light.model == "trafficlight1")
                    {
                        light.size = 0.45f;
                        light.searchlight = 0;
                        const unsigned phase{ trafficIndices[light.model]++ % 3 };
                        light.colour = phase == 0 ? std::array<unsigned, 4>{ 255, 0, 0, 255 } :
                            phase == 1 ? std::array<unsigned, 4>{ 255, 128, 0, 255 } : std::array<unsigned, 4>{ 0, 255, 0, 255 };
                    }
                }
            }
            for (const auto& light : lights)
            {
                for (const float value : { light.position[0], light.position[1], light.position[2], light.size, light.distance, light.poolSize, light.intensity, light.depth })
                    if (!std::isfinite(value))
                        throw std::runtime_error("Non-finite light field: " + light.model);
            }
        }
    };

    std::string Pool(const Light& light)
    {
        std::ostringstream out;
        out << std::setprecision(9) << ' ' << (light.texture.empty() ? "-" : light.texture)
            << ' ' << light.poolSize << ' ' << light.intensity << ' ' << light.depth;
        for (size_t i{}; i < 3; ++i)
            out << ' ' << light.poolColour[i];
        return out.str();
    }

    void Write(const fs::path& output, const std::string& text)
    {
        auto temp{ output };
        temp += ".tmp";
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        if (!file || !file.write(text.data(), text.size()) || !file.flush())
            throw std::runtime_error("Cannot write " + temp.string());
        file.close();
        fs::copy_file(temp, output, fs::copy_options::overwrite_existing);
        fs::remove(temp);
    }

    std::string Merge(const std::string& original, const std::vector<Light>& lights, bool keepPools = false)
    {
        std::string result, model;
        size_t matched{}, preserved{}, cursor{};
        while (cursor < original.size())
        {
            auto end{ original.find('\n', cursor) };
            if (end == original.npos)
                end = original.size();
            auto line{ original.substr(cursor, end - cursor) };
            const std::string ending{ !line.empty() && line.back() == '\r' ? "\r\n" : "\n" };
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            const auto clean{ Trim(line) };
            if (!clean.empty() && clean[0] == '%')
                model = Lower(clean.substr(1));
            else if (!clean.empty() && clean[0] != '#')
            {
                const auto t{ Tokens(line) };
                // Legacy custom rows are preserved verbatim when they have no native source.
                if (t.size() != 12 && t.size() != 19)
                {
                    result += line;
                    if (end < original.size())
                        result += ending;
                    cursor = end + 1;
                    ++preserved;
                    continue;
                }
                std::array<float, 3> position{ Number(t[4]), Number(t[5]), Number(t[6]) };
                const Light* match{};
                float closest{ 0.0001f };
                for (const auto& light : lights)
                {
                    if (light.model != model)
                        continue;
                    float distance{};
                    for (size_t i{}; i < 3; ++i)
                        distance += (position[i] - light.position[i]) * (position[i] - light.position[i]);
                    if (distance < closest)
                    {
                        closest = distance;
                        match = &light;
                    }
                }
                if (match && !(keepPools && t.size() == 19))
                {
                    if (t.size() == 19)
                    {
                        std::istringstream fields(line);
                        std::string field;
                        for (int i{}; i < 12; ++i)
                            fields >> field;
                        line.resize(static_cast<size_t>(fields.tellg()));
                    }
                    line += Pool(*match);
                    ++matched;
                }
                else
                    ++preserved;
            }
            result += line;
            if (end < original.size())
                result += ending;
            cursor = end + 1;
        }
        std::cout << "Native pool matches: " << matched << "; existing rows preserved verbatim: " << preserved << '\n';
        return result;
    }

    std::map<std::string, std::string> TextModels(const fs::path& root, const std::vector<Light>& lights)
    {
        std::map<std::string, std::string> inputs;
        if (!fs::exists(root / "2dfx"))
            return inputs;
        for (const auto& entry : fs::directory_iterator(root / "2dfx"))
        {
            if (Lower(entry.path().extension().string()) == ".txt")
            {
                const auto bytes{ Load(entry.path()) };
                inputs[entry.path().stem().string()] = std::string(bytes.begin(), bytes.end());
            }
        }
        std::map<std::string, std::ostringstream> converted;
        for (const auto& light : lights)
        {
            if (inputs.count(light.model))
                continue;
            auto& text{ converted[light.model] };
            text << "2dfxType LIGHT\nPosition" << std::fixed << std::setprecision(6);
            for (const auto value : light.position)
                text << ' ' << value;
            text << "\nColor";
            for (const auto value : light.colour)
                text << ' ' << value;
            static const std::array<const char*, 14> modes{
                "DEFAULT", "RANDOM_FLASHING", "RANDOM_FLASHIN_ALWAYS_AT_WET_WEATHER",
                "LIGHTS_ANIM_SPEED_4X", "LIGHTS_ANIM_SPEED_2X", "LIGHTS_ANIM_SPEED_1X",
                "ALWAYS_AT_WET_WEATHER", "TRAFFICLIGHT", "TRAINCROSSLIGHT", "AT_RAIN_ONLY",
                "DEFAULT", "5S_ON_5S_OFF", "6S_ON_4S_OFF", "6S_ON_4S_OFF_2"
            };
            text << "\nCoronaFarClip " << light.distance << "\nCoronaSize " << light.size
                << "\nCoronaShowMode " << (light.nativeMode < modes.size() ? modes[light.nativeMode] : "DEFAULT") << '\n';
        }
        for (const auto& [model, text] : converted)
            inputs[model] = text.str();
        return inputs;
    }

    std::string Generate(const std::vector<Light>& lights)
    {
        std::map<std::string, std::vector<const Light*>> groups;
        for (const auto& light : lights)
            if (light.size > 0.0f && light.model != "destruct2" && light.model != "destruct03")
                groups[light.model].push_back(&light);
        std::ostringstream out;
        out << "#%modelname\r\n#Red Green Blue Alpha OffsetX OffsetY OffsetZ CustomSize DrawDistance BlinkType NoDistance DrawSearchlight PoolTexture PoolSize PoolIntensity PoolDepth PoolRed PoolGreen PoolBlue\r\n";
        for (const auto& [model, group] : groups)
        {
            out << '%' << model << "\r\n";
            for (const auto* light : group)
            {
                for (const auto value : light->colour)
                    out << value << ' ';
                out << std::setprecision(9);
                for (const auto value : light->position)
                    out << (value == 0.0f ? 0.0f : value) << ' ';
                out << light->size << ' ' << light->distance << ' ' << light->blink << ' ' << light->noDistance << ' ' << light->searchlight << Pool(*light) << "\r\n";
            }
        }
        return out.str();
    }
}

int main(int argc, char** argv)
{
    try
    {
        fs::path root{ fs::current_path() }, output, baseline;
        std::string game;
        bool keepPools{};
        for (int i{ 1 }; i < argc; ++i)
        {
            const std::string option{ argv[i] };
            if (option == "--help" || option == "-h")
            {
                std::cout << "2DFXDataGrabber [--game iv|iii|lcs|vcs|vc|sa] [--root GAME_OR_DATA_DIR] [--output FILE] [--baseline FILE] [--keep-pools]\n"
                    "Detects gta4.dat, gta3.dat, gta_lcs.dat, gta_vcs.dat, gta_vc.dat, then SA 2dfx inputs.\n"
                    "Reads IV .light/.skel, IDE/IPL, and SA IMG/DFF or converted 2dfx/*.txt. No converter is required.\n"
                    "--baseline preserves every existing field, section, custom row and comment; appends native pool fields to matching rows.\n"
                    "--keep-pools also preserves existing pool fields when using --baseline.\n";
                return 0;
            }
            if (option == "--keep-pools")
            {
                keepPools = true;
                continue;
            }
            if (++i >= argc)
                throw std::runtime_error("Missing value for " + option);
            if (option == "--root") root = argv[i];
            else if (option == "--output") output = argv[i];
            else if (option == "--baseline") baseline = argv[i];
            else if (option == "--game") game = Lower(argv[i]);
            else throw std::runtime_error("Unknown option: " + option);
        }
        root = fs::absolute(root);
        if (game.empty())
        {
            for (const auto& [mode, manifest] : std::vector<std::pair<std::string, std::string>>{
                { "iv", "gta4.dat" }, { "iii", "gta3.dat" }, { "lcs", "gta_lcs.dat" },
                { "vcs", "gta_vcs.dat" }, { "vc", "gta_vc.dat" } })
            {
                if (fs::exists(root / manifest) || fs::exists(root / "data" / manifest))
                {
                    game = mode;
                    break;
                }
            }
            if (game.empty())
                game = "sa";
        }
        const std::map<std::string, std::pair<std::string, std::string>> modes{
            { "iv", { "gta4.dat", "IVLodLights.dat" } }, { "iii", { "gta3.dat", "IIILodLights.dat" } },
            { "lcs", { "gta_lcs.dat", "LCSLodLights.dat" } }, { "vcs", { "gta_vcs.dat", "VCSLodLights.dat" } },
            { "vc", { "gta_vc.dat", "VCLodLights.dat" } }, { "sa", { "gta.dat", "SALodLights.dat" } }
        };
        const auto mode{ modes.find(game) };
        if (mode == modes.end())
            throw std::runtime_error("Supported games: iv, iii, lcs, vcs, vc, sa");
        if (output.empty())
            output = mode->second.second;
        output = fs::absolute(output);
        if (!baseline.empty())
            baseline = fs::absolute(baseline);
        const bool pools{ game == "iii" || game == "vc" || game == "sa" };
        if (keepPools && baseline.empty())
            throw std::runtime_error("--keep-pools requires --baseline");
        if (!pools && !baseline.empty())
            throw std::runtime_error("--baseline is supported for iii, vc and sa");
        Extractor extractor(root, game == "sa");
        const bool emptyPortable{ argc == 1 && game == "sa" && !fs::exists(root / "2dfx") &&
            !fs::exists(root / "data" / "gta.dat") && !fs::exists(root / "gta.dat") };
        if (pools && !emptyPortable)
        {
            extractor.Run(mode->second.first);
            if (extractor.lights.empty())
                throw std::runtime_error("No native lights found; output was not changed");
        }
        std::string result;
        if (!baseline.empty())
        {
            const auto bytes{ Load(baseline) };
            result = Merge(std::string(bytes.begin(), bytes.end()), extractor.lights, keepPools);
        }
        else if (game != "sa" || emptyPortable || (fs::exists(root / "2dfx") &&
            std::any_of(fs::directory_iterator(root / "2dfx"), fs::directory_iterator{},
                [&](const auto& entry) { return Lower(entry.path().extension().string()) == ".txt" || (argc == 1 && Lower(entry.path().extension().string()) == ".dff"); })))
        {
            const auto previous{ fs::current_path() };
            struct RestoreDirectory
            {
                fs::path path;
                ~RestoreDirectory()
                {
                    fs::current_path(path);
                }
            } restore{ previous };
            // Copied manifests refer to paths relative to their data directory.
            auto portableRoot{ root };
            if (game != "sa" && !fs::exists(root / mode->second.first))
                portableRoot /= "data";
            fs::current_path(portableRoot);
            if (game == "iv") result = Portable::ConvertLightTo2DFX();
            else if (game == "sa") result = Portable::ConvertTextTo2DFX(TextModels(root, extractor.lights));
            else result = Portable::Run(game);
            std::string crlf;
            for (const char c : result)
                crlf += c == '\n' ? "\r\n" : std::string(1, c);
            result = pools ? Merge(crlf, extractor.lights) : crlf;
            if (game == "lcs" || game == "vcs")
            {
                crlf.clear();
                for (const char c : Portable::Additional)
                    crlf += c == '\n' ? "\r\n" : std::string(1, c);
                Write(output.parent_path() / ("ADD" + output.filename().string()), crlf);
            }
        }
        else
            result = Generate(extractor.lights);
        Write(output, result);
        std::cout << "Wrote " << output.string() << '\n';
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "2DFXDataGrabber: " << e.what() << '\n';
        return 1;
    }
}
