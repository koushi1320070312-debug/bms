#include <iostream>
#include <iomanip>

#include "Parser.h"
#include "Data.h"

int main()
{
    // -----------------------------
    // 解析対象の BMS ファイルパス
    // -----------------------------
    std::string filepath = "test.bms";  // 必要に応じて変更してください

    // -----------------------------
    // データ格納用インスタンス
    // -----------------------------
    BMSData data;

    // -----------------------------
    // BMS 解析実行
    // -----------------------------
    if (!BMSParser::Parse(filepath, data))
    {
        std::cerr << "[ERROR] BMS parsing failed." << std::endl;
        return 1;
    }

    // -----------------------------
    // ヘッダ情報のログ出力
    // -----------------------------
    std::cout << "===== BMS HEADER =====" << std::endl;
    std::cout << "TITLE       : " << data.title << std::endl;
    std::cout << "INITIAL BPM : " << data.initial_bpm << std::endl;
    std::cout << std::endl;

    // -----------------------------
    // ノーツ一覧のログ出力
    // -----------------------------
    std::cout << "===== NOTES =====" << std::endl;

    for (const auto& note : data.notes)
    {
        std::cout
            << "[NOTE] time="
            << std::fixed << std::setprecision(2)
            << note.time_ms << "ms"
            << " | measure=" << note.measure
            << " | ch=" << note.channel
            << " | wav=" << note.wav_id
            << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Total Notes: " << data.notes.size() << std::endl;

    return 0;
}
