#include "Parser.h"

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <map>

std::string GetBMSDirectory(const std::string& bms_filepath) {
    size_t last_slash = bms_filepath.find_last_of("/\\");
    if (last_slash == std::string::npos) return "./";
    return bms_filepath.substr(0, last_slash + 1);
}

// ----------------------------------------------------
// 仮想的な外部ロードAPI
// ----------------------------------------------------
int g_resource_counter = 1;

int VirtualLoadWAVFile(const std::string& path) {
    std::cout << "[LOAD] WAV: " << path << std::endl;
    return g_resource_counter++;
}

int VirtualLoadBMPFile(const std::string& path) {
    std::cout << "[LOAD] BMP: " << path << std::endl;
    return g_resource_counter++;
}

// =======================================
// BMS パーサ本体
// =======================================
bool BMSParser::Parse(const std::string& filepath, BMSData& out_data)
{
    std::ifstream file(filepath);
    if (file.fail())
    {
        std::cerr << "[ERROR] Failed to open BMS file: " << filepath << std::endl;
        return false;
    }

    std::string line;

    // ----------------------------------------------------
    // LN開始待ちマップ
    // ----------------------------------------------------
    std::map<int, Note> ln_starts;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] != '#')
            continue;

        // -----------------------------
        // #TITLE
        // -----------------------------
        if (line.find("#TITLE") == 0)
        {
            std::string value = line.substr(6);
            if (!value.empty() && value[0] == ' ') value = value.substr(1);
            out_data.title = value;
            continue;
        }

        // -----------------------------
        // #BPM (初期BPM)
        // -----------------------------
        if (line.find("#BPM ") == 0)
        {
            std::string v = line.substr(5);
            if (!v.empty() && v[0]==' ') v = v.substr(1);
            out_data.initial_bpm = std::stod(v);
            continue;
        }

        // -----------------------------
        // #BPMxx (拡張BPM)
        // -----------------------------
        if (line.find("#BPM") == 0 && line.size() > 6 && line[4] != ' ')
        {
            std::string id = line.substr(4,2);
            std::string v  = line.substr(6);
            if (!v.empty() && v[0]==' ') v = v.substr(1);
            out_data.bpm_table[id] = std::stod(v);
            continue;
        }

        // -----------------------------
        // #WAVxx
        // -----------------------------
        if (line.find("#WAV") == 0 && line.size() > 6)
        {
            std::string id = line.substr(4,2);
            std::string path = line.substr(6);
            if (!path.empty() && path[0]==' ') path = path.substr(1);
            out_data.wav_files[id] = path;
            continue;
        }

        // -----------------------------
        // #STOPxx
        // -----------------------------
        if (line.find("#STOP") == 0 && line.size() > 7)
        {
            std::string id = line.substr(5,2);
            std::string v  = line.substr(7);
            if (!v.empty() && v[0]==' ') v = v.substr(1);
            out_data.stop_table[id] = std::stod(v);
            continue;
        }

        // ----------------------------------------------
        // #mmmcc:DATA 以外は無視
        // ----------------------------------------------
        size_t cpos = line.find(':');
        if (cpos == std::string::npos) continue;

        int measure = 0;
        int channel = 0;
        std::string data_str;

        try
        {
            measure = std::stoi(line.substr(1,3));
            channel = std::stoi(line.substr(4,2), nullptr, 16);
            data_str = line.substr(cpos+1);
        }
        catch(...)
        {
            continue;
        }

        if (data_str.size() % 2 != 0) continue;

        // =====================================================
        // BPM/STOP/小節倍率
        // =====================================================
        if (channel == 0x02 || channel == 0x03 || channel == 0x08)
        {
            if (channel == 0x02)
            {
                out_data.measure_rate_map[measure] = std::stod(data_str);
            }
            else
            {
                int N = data_str.size() / 2;
                for (int i=0; i<N; i++)
                {
                    std::string id = data_str.substr(i*2,2);
                    if (id == "00") continue;

                    double pos = (double)i / (double)N;

                    Note ev{};
                    ev.measure = measure;
                    ev.channel = channel;
                    ev.wav_id = "";
                    ev.def_id = id;
                    ev.end_measure = -1;
                    ev.end_pos = -1.0;
                    ev.end_time_ms = 0.0;

                    ev.pos_raw = pos;
                    ev.time_ms = 0.0;

                    out_data.notes.push_back(ev);
                }
            }
            continue;
        }

        // =====================================================
        // LN終了 (61〜69)
        // =====================================================
        if (channel >= 0x61 && channel <= 0x69)
        {
            int N = data_str.size() / 2;
            int start_ch = channel - 0x10;

            for (int i=0; i<N; i++)
            {
                if (data_str.substr(i*2,2) == "00") continue;

                auto it = ln_starts.find(start_ch);
                if (it != ln_starts.end())
                {
                    Note ln = it->second;
                    ln_starts.erase(it);

                    ln.end_measure = measure;
                    ln.end_pos = (double)i / (double)N;

                    out_data.notes.push_back(ln);
                }
            }
            continue;
        }

        // =====================================================
        // 通常/LN開始ノーツ
        // =====================================================
        if (channel > 0x00)
        {
            int N = data_str.size() / 2;
            for (int i=0; i<N; i++)
            {
                std::string id = data_str.substr(i*2,2);
                if (id == "00") continue;

                double pos = (double)i / (double)N;

                Note note{};
                note.measure = measure;
                note.channel = channel;
                note.wav_id = id;
                note.def_id = "";
                note.end_measure = -1;
                note.end_pos = -1.0;
                note.end_time_ms = 0.0;

                note.pos_raw = pos;
                note.time_ms = 0.0;

                if (channel >= 0x51 && channel <= 0x59)
                {
                    ln_starts[channel] = note;
                }
                else
                {
                    out_data.notes.push_back(note);
                }
            }
            continue;
        }
    }

    // =====================================================
    // ソート（measure → pos_raw）
    // =====================================================
    std::sort(out_data.notes.begin(), out_data.notes.end(),
        [](const Note& a, const Note& b){
            if (a.measure != b.measure)
                return a.measure < b.measure;
            return a.pos_raw < b.pos_raw;
        }
    );

    // =====================================================
    // 1PASS: time_ms 再計算
    // =====================================================
    double cur_time = 0.0;
    double cur_bpm  = out_data.initial_bpm;
    double rate     = 1.0;
    int last_m      = -1;

    for (auto& n : out_data.notes)
    {
        if (n.measure != last_m)
        {
            if (last_m != -1)
            {
                cur_time += (60000.0/cur_bpm)*4.0*rate;
            }

            auto it = out_data.measure_rate_map.find(n.measure);
            rate = (it != out_data.measure_rate_map.end()) ? it->second : 1.0;

            last_m = n.measure;
        }

        double Tm = (60000.0/cur_bpm)*4.0*rate;

        double final_t = cur_time + n.pos_raw * Tm;
        n.time_ms = final_t;

        if (n.channel == 0x03)
        {
            auto itb = out_data.bpm_table.find(n.def_id);
            if (itb != out_data.bpm_table.end())
                cur_bpm = itb->second;
        }
        else if (n.channel == 0x08)
        {
            auto its = out_data.stop_table.find(n.def_id);
            if (its != out_data.stop_table.end())
            {
                double stop_beats = its->second;
                cur_time += stop_beats * (60000.0/cur_bpm);
            }
        }
    }

    // =====================================================
    // 2PASS: calc_time_at による LN end_time_ms 計算
    // =====================================================
    auto calc_time_at = [&](int target_m, double target_pos)
    {
        double t = 0.0;
        double bpm = out_data.initial_bpm;
        double r   = 1.0;
        int last   = -1;

        for (const auto& e : out_data.notes)
        {
            if (e.measure != last)
            {
                if (last != -1)
                {
                    t += (60000.0/bpm)*4.0*r;
                }

                auto rit = out_data.measure_rate_map.find(e.measure);
                r = (rit != out_data.measure_rate_map.end()) ? rit->second : 1.0;

                last = e.measure;
            }

            if (e.measure == target_m)
            {
                double Tm = (60000.0/bpm)*4.0*r;
                t += Tm * target_pos;
                return t;
            }

            if (e.channel == 0x03)
            {
                auto itb = out_data.bpm_table.find(e.def_id);
                if (itb != out_data.bpm_table.end())
                    bpm = itb->second;
            }
            else if (e.channel == 0x08)
            {
                auto its = out_data.stop_table.find(e.def_id);
                if (its != out_data.stop_table.end())
                    t += its->second * (60000.0/bpm);
            }
        }

        double fallback = (60000.0/bpm)*4.0*r;
        t += fallback * target_pos;
        return t;
    };

    for (auto& n : out_data.notes)
    {
        if (n.end_measure != -1)
        {
            n.end_time_ms = calc_time_at(n.end_measure, n.end_pos);
        }
    }

    return true;
}

// ----------------------------------------------------
// リソースロードを管理する関数
// ----------------------------------------------------
void LoadBMSResources(BMSData& data, const std::string& bms_filepath)
{
    // ======================================
    // 1. パスの解決 (ResolveResourcePaths 相当)
    // ======================================
    std::string base_dir = GetBMSDirectory(bms_filepath);

    std::string resolved_stagefile;
    if (!data.stagefile.empty()) {
        resolved_stagefile = base_dir + data.stagefile;
    }

    std::map<std::string, std::string> resolved_wavs;
    for (const auto& kv : data.wav_files) {
        std::string id = kv.first;          // "01" "0A" など
        std::string path = kv.second;       // WAVファイル名
        resolved_wavs[id] = base_dir + path;
    }

    std::map<std::string, std::string> resolved_bmps;
    for (const auto& kv : data.bmp_files) {
        std::string id = kv.first;
        std::string path = kv.second;
        resolved_bmps[id] = base_dir + path;
    }

    // ======================================
    // 2. STAGEFILE のロード
    // ======================================
    if (!resolved_stagefile.empty()) {
        int handle = VirtualLoadBMPFile(resolved_stagefile);
        if (handle > 0) {
            data.loaded_stagefile = handle;
            std::cout << "[OK] Stagefile loaded: handle=" << handle << std::endl;
        } else {
            std::cout << "[WARN] Failed to load stagefile: "
                      << resolved_stagefile << std::endl;
        }
    }

    // ======================================
    // 3. WAVファイルのロード
    // ======================================
    for (const auto& kv : resolved_wavs) {
        const std::string& wav_id = kv.first;
        const std::string& wav_path = kv.second;

        int handle = VirtualLoadWAVFile(wav_path);
        if (handle > 0) {
            data.loaded_wavs[wav_id] = handle;
            std::cout << "[OK] WAV " << wav_id
                      << " loaded: handle=" << handle << std::endl;
        } else {
            std::cout << "[WARN] Failed to load WAV " << wav_id
                      << ": " << wav_path << std::endl;
        }
    }

    // ======================================
    // 4. BMPファイルのロード
    // ======================================
    for (const auto& kv : resolved_bmps) {
        const std::string& bmp_id = kv.first;
        const std::string& bmp_path = kv.second;

        int handle = VirtualLoadBMPFile(bmp_path);
        if (handle > 0) {
            data.loaded_bmps[bmp_id] = handle;
            std::cout << "[OK] BMP " << bmp_id
                      << " loaded: handle=" << handle << std::endl;
        } else {
            std::cout << "[WARN] Failed to load BMP " << bmp_id
                      << ": " << bmp_path << std::endl;
        }
    }
}
