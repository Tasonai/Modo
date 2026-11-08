#pragma once

#include "types.h"
#include <windows.h>

namespace ModoCLI {

// Checks whether command-line arguments specify a CLI subcommand
// (e.g. style, restore, folder, batch, showcase, export, theme, version, help).
// If so, connects to the parent console, runs the command headlessly,
// and returns true with the process exit code.
// If not (e.g. no arguments or GUI setup/update flags), returns false so the GUI window launches normally.
bool TryDispatchCLI(int argc, wchar_t* argv[], int& outExitCode, const CLSID* pngClsid);

} // namespace ModoCLI
