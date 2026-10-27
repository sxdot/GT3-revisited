// The platform file dialogs for the menus' save page (menu::flow::FileDialogs): pick a
// save to import / load as a guest garage (native .sav or any PS2 save container), or
// where to export a save for the PS2 (.psu / .ps2 card image). SDL's dialogs are
// asynchronous and may answer on another thread; the answer waits in a mailbox the page
// polls. On Android they are the system's document picker (content:// URIs).
//
// Development: GT3R_FILE_DIALOG=<path> answers every dialog with that path at once
// (scripted screenshots, no window), "-" with a cancel.

#include "app/save_dialogs.h"

#include "app/app.h"
#include "menu/flow/save_page.h"

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_stdinc.h>

#include <cstdlib>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace gt3r {

namespace {

std::mutex g_mutex;
std::optional<std::string> g_answer;

void SDLCALL on_dialog(void*, const char* const* files, int) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_answer = files && files[0] ? std::string(files[0]) : std::string(); // null = error, empty = cancelled
}

const SDL_DialogFileFilter kOpenFilters[] = {
    {"Saves (GT3 Revisited, PS2)", "sav;psu;ps2;mcd;raw"},
    {"All files", "*"},
};
const SDL_DialogFileFilter kExportFilters[] = {
    {"PS2 save (.psu)", "psu"},
    {"PS2 memory card image (.ps2)", "ps2"},
};

} // namespace

void install_save_dialogs(App& app) {
    menu::flow::FileDialogs d;
    SDL_Window* window = app.window();
    d.show = [window](menu::flow::FileDialogs::Kind kind, const std::string& name) {
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_answer.reset();
        }
        if (const char* env = std::getenv("GT3R_FILE_DIALOG"); env && *env) {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_answer = std::string(env) == "-" ? std::string() : std::string(env);
            return;
        }
        if (kind == menu::flow::FileDialogs::Kind::OpenSave) {
            SDL_ShowOpenFileDialog(on_dialog, nullptr, window, kOpenFilters, SDL_arraysize(kOpenFilters), nullptr, false);
        } else {
            const std::string file = (name.empty() ? std::string("GT3") : name) + ".psu";
            SDL_ShowSaveFileDialog(on_dialog, nullptr, window, kExportFilters, SDL_arraysize(kExportFilters), file.c_str());
        }
    };
    d.poll = []() -> std::optional<std::string> {
        std::lock_guard<std::mutex> lock(g_mutex);
        return std::exchange(g_answer, std::nullopt);
    };
    menu::flow::set_file_dialogs(std::move(d));
}

} // namespace gt3r
