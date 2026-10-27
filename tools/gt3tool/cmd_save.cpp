// gt3tool GT-mode save commands: move progress between a GT3 Revisited native
// save (<user data>/saves/*.sav) and the original game's PS2 memory-card save
// (docs/dev/GT_MODE.md "Saves").

#include "tool.h"

#include "game/gt_save.h"

#include <cstdio>

namespace gt3r::tool {

namespace {

game::GtData::Reader vol_reader(Disc& disc) {
    Volume* vol = &disc.volume();
    return [vol](const std::string& p) -> std::optional<Bytes> {
        const Volume::Entry* e = vol->find(p);
        if (!e) return std::nullopt;
        return vol->read(*e);
    };
}

std::shared_ptr<game::GtData> load_gt_data(Disc& disc) {
    auto d = std::make_shared<game::GtData>();
    if (!d->load(vol_reader(disc))) throw FormatError("GT mode data: " + d->error());
    return d;
}

void summary(const char* what, const game::GameState& s) {
    std::printf("%s: day %u, %lld Cr, %zu cars, %zu race results\n", what, s.day, (long long)s.credits,
                s.garage.size(), s.race_best.size());
}

} // namespace

GT3TOOL_COMMAND(save_import, "save-import", "<ps2 save> <out.sav>",
                "convert an original GT3 save (data file, GT3 Save Editor .raw, save folder, PCSX2 folder card, .psu, .ps2 card) to a GT3 Revisited save",
                2, [](Disc& disc, const Args& a) {
                    auto data = load_gt_data(disc);
                    game::GameState s = game::import_ps2_save_path(a[0], *data);
                    std::string err;
                    if (!game::write_save(a[1], s, &err)) throw FormatError(err);
                    summary(a[1].c_str(), s);
                    return 0;
                });

GT3TOOL_COMMAND(save_export, "save-export", "<save.sav> <out.psu | out.ps2 | out.raw | out folder>",
                "write a GT3 Revisited save as an original GT3 memory-card save (.raw: the bare save GT3 Save Editor reads)",
                2,
                [](Disc& disc, const Args& a) {
                    auto data = load_gt_data(disc);
                    std::string err;
                    std::optional<game::GameState> s = game::read_save(a[0], &err);
                    if (!s) throw FormatError(err);
                    game::export_ps2_save_path(*s, data, vol_reader(disc), a[1]);
                    summary(a[1].c_str(), *s);
                    return 0;
                });

} // namespace gt3r::tool
