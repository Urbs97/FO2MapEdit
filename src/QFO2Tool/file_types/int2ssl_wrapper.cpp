#include "int2ssl_wrapper.h"

#include "FalloutScript.h"
#include "main.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// Re-define the globals that were originally in int2ssl/main.cpp.
// These are referenced via extern declarations throughout the int2ssl library.
bool g_bDump = false;
int g_nFalloutVersion = 2;
std::string g_strIndentFill("\t");
bool g_bIgnoreWrongNumOfArgs = false;
bool g_bInsOmittedArgsBackward = false;
bool g_bStopOnError = false;
std::ifstream g_ifstream;
std::ofstream g_ofstream;
std::string g_inputFileName;
std::string g_outputFileName;
bool useOldShortCircuit = false;

bool decompile_int_to_ssl(const char* int_file_path, std::string& out_ssl) {
    out_ssl.clear();

    // Reset globals to defaults
    g_bDump = false;
    g_nFalloutVersion = 2;
    g_strIndentFill = "\t";
    g_bIgnoreWrongNumOfArgs = false;
    g_bInsOmittedArgsBackward = false;
    g_bStopOnError = false;
    useOldShortCircuit = false;
    g_inputFileName = int_file_path;

    // Generate a temp file path for the decompiled output
    std::filesystem::path tmp_path = std::filesystem::temp_directory_path() / "int2ssl_out.ssl";
    g_outputFileName = tmp_path.string();

    // Open input
    g_ifstream.open(g_inputFileName.c_str(), std::fstream::in | std::fstream::binary);
    if (!g_ifstream.is_open()) {
        std::filesystem::remove(tmp_path);
        out_ssl = "Error: unable to open input file: " + g_inputFileName;
        return false;
    }

    // Open output
    g_ofstream.open(g_outputFileName.c_str(), std::fstream::out | std::fstream::trunc);
    if (!g_ofstream.is_open()) {
        g_ifstream.close();
        std::filesystem::remove(tmp_path);
        out_ssl = "Error: unable to open output file: " + g_outputFileName;
        return false;
    }

    bool success = false;
    try {
        CFalloutScript Script;
        Script.Serialize();
        Script.InitDefinitions();
        Script.ProcessCode();
        Script.StoreSource();
        success = true;
    } catch (const std::exception& e) {
        out_ssl = std::string("Decompilation error: ") + e.what();
    } catch (...) { out_ssl = "Decompilation error: unknown exception"; }

    g_ifstream.close();
    g_ofstream.close();

    if (success) {
        // Read the temp file into the output string
        std::ifstream result_file(tmp_path);
        if (result_file.is_open()) {
            std::ostringstream ss;
            ss << result_file.rdbuf();
            out_ssl = ss.str();
            result_file.close();
        } else {
            out_ssl = "Error: could not read decompiled output";
            success = false;
        }
    }

    std::filesystem::remove(tmp_path);
    return success;
}
