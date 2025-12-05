#pragma once

#include <string>
#include "Data.h"

// =======================================
// BMS パーサクラス
// 役割：BMSファイルを解析し BMSData に格納する
// =======================================
class BMSParser
{
public:
    // ---------------------------------------
    // BMSファイルを解析
    // filepath : BMSファイルのパス
    // out_data : 解析結果の格納先
    //
    // 成功: true
    // 失敗: false（ファイルが開けない等）
    // ---------------------------------------
    static bool Parse(const std::string& filepath, BMSData& out_data);
};

void ResolveResourcePaths(BMSData& data, const std::string& bms_filepath);

std::string GetBMSDirectory(const std::string& bms_filepath);
