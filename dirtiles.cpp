#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <sqlite3.h>

#include "jsonpull/jsonpull.h"
#include "mbtiles.hpp"
#include "dirtiles.hpp"
#include "errors.hpp"
#include "write_json.hpp"

namespace fs = std::filesystem;

std::string dir_read_tile(std::string base, struct zxy tile) {
    fs::path tilePath = fs::path(base) / tile.path();
    std::ifstream pbfFile(tilePath, std::ios::in | std::ios::binary);
	std::ostringstream contents;
	contents << pbfFile.rdbuf();
	pbfFile.close();

    return contents.str();
}

void dir_write_tile(const char *outdir, int z, int tx, int ty, std::string const &pbf) {

    // Create <outdir>/<z>/<tx>
    fs::path targetDir = fs::path(outdir) / std::to_string(z) / std::to_string(tx);
    fs::create_directories(targetDir);

    fs::path tile_path = targetDir / (std::to_string(ty) + ".pbf");
    if (fs::exists(tile_path)) {
        std::cerr << tile_path << ": file exists" << std::endl;
        std::exit(EXIT_EXISTS);
    }

    std::ofstream out(tile_path, std::ios::binary);

    if (!out) {
        std::cerr << tile_path << ": Failed to open." << std::endl;
        std::exit(EXIT_WRITE);
    }

    out.write(pbf.data(), static_cast<std::streamsize>(pbf.size()));

    if (!out) {
        std::cerr << tile_path << ": Failed to write." << std::endl;
        std::exit(EXIT_WRITE);
    }

    out.close();

    if (!out) {
        std::cerr << tile_path << ": Failed to close." << std::endl;
        std::exit(EXIT_CLOSE);
    }
}

static bool numeric(const std::string &s) {

    std::string_view sv{s};

    return !sv.empty() &&
           std::all_of(sv.begin(), sv.end(),
                       [](unsigned char c) {
                           return std::isdigit(c);
                       });

}

static bool pbfname(const std::string &s) {
    fs::path p{s};

    auto stem = p.stem().string();

    if (stem.empty() ||
        !std::all_of(stem.begin(), stem.end(), [](unsigned char c) { return std::isdigit(c); })) {
        return false;
    }
    auto ext = p.extension();

   return ext == ".pbf" || ext == ".m*t";
}

void check_dir(const char *dir, char **argv, bool force, bool forcetable) {

    fs::create_directories(dir);

    const fs::path meta = fs::path(dir) / "metadata.json";

    std::error_code ec;
    if (force) {
        fs::remove(meta, ec);  // OK if it doesn't exist
    } else {
        if (fs::exists(meta)) {
            std::cerr << argv[0]
                      << ": Tileset \"" << dir
                      << "\" already exists. You can use --force if you want to delete the old tileset." << std::endl;

            std::cerr << argv[0]
                      << ": " << meta
                      << ": file exists" << std::endl;

            if (!forcetable) {
                std::exit(EXIT_EXISTS);
            }
        }
    }

    if (forcetable) {
        // Don't clear existing tiles
        return;
    }

    const auto tiles = enumerate_dirtiles(dir, INT_MIN, INT_MAX);

    for (const auto &tile : tiles) {
        const fs::path fn = fs::path(dir) / tile.path();

        if (force) {
            if (!fs::remove(fn, ec)) {
                std::cerr << "Failed to remove " << fn << std::endl;
                std::exit(EXIT_UNLINK);
            }
        } else {
            std::cerr << fn << ": file exists" << std::endl;
            std::exit(EXIT_EXISTS);
        }
    }
}

std::vector<zxy> enumerate_dirtiles(const char *fname, int minzoom, int maxzoom)
{
    std::vector<zxy> tiles;
    std::error_code ec;

    fs::directory_iterator z_iter(fname, ec);

    if (ec) {
        std::cerr << fname << ": " << ec.message() << std::endl;
        std::exit(EXIT_OPEN);
    }

    for (const auto &z_entry : z_iter) {
        const auto z_name = z_entry.path().filename().string();

        if (!numeric(z_name)) {
            continue;
        }

        const int tz = std::stoi(z_name);

        if (tz < minzoom || tz > maxzoom) {
            continue;
        }

        fs::directory_iterator x_iter(z_entry.path(), ec);

        if (ec) {
            std::cerr << z_entry.path() << ": "
                      << ec.message() << std::endl;
            std::exit(EXIT_OPEN);
        }

        for (const auto &x_entry : x_iter) {

            const auto x_name = x_entry.path().filename().string();

            if (!numeric(x_name)) {
                continue;
            }

            const int tx = std::stoi(x_name);

            fs::directory_iterator y_iter(x_entry.path(), ec);

            if (ec) {
                std::cerr << x_entry.path() << ": "
                          << ec.message() << std::endl;
                std::exit(EXIT_OPEN);
            }

            for (const auto &y_entry : y_iter) {

                const auto y_name = y_entry.path().filename().string();

                if (!pbfname(y_name)) {
                    continue;
                }

                const int ty = std::stoi(y_entry.path().stem().string());

                zxy tile(tz, tx, ty);

                if (y_entry.path().extension() == ".mvt") {
                    tile.extension = ".mvt";
                }

                tiles.push_back(std::move(tile));
            }
        }
    }

    std::stable_sort(tiles.begin(), tiles.end());
    return tiles;
}

#include <filesystem>
#include <system_error>

void dir_erase_zoom(const char *fname, int zoom) {
    namespace fs = std::filesystem;

    std::error_code ec;

    const fs::path zpath = fs::path(fname) / std::to_string(zoom);

    if (!fs::exists(zpath, ec)) {
        return;
    }

    fs::directory_iterator x_iter(zpath, ec);

    if (ec) {
        std::cerr << zpath << ": " << ec.message() << std::endl;
        std::exit(EXIT_OPEN);
    }

    for (const auto& x_entry : x_iter) {

        const auto x_name = x_entry.path().filename().string();

        if (!numeric(x_name)) {
            continue;
        }

        fs::directory_iterator y_iter(x_entry.path(), ec);

        if (ec) {
            std::cerr << x_entry.path() << ": " << ec.message() << std::endl;
            std::exit(EXIT_OPEN);
        }

        for (const auto& y_entry : y_iter) {

            const auto filename =
                y_entry.path().filename().string();

            if (!pbfname(filename)) {
                continue;
            }

            if (!fs::remove(y_entry.path(), ec)) {
                std::cerr << y_entry.path() << ": " << ec.message() << std::endl;
                std::exit(EXIT_UNLINK);
            }
        }
    }
}

sqlite3 *dirmeta2tmp(const char *fname) {
    sqlite3 *db = nullptr;
    char *err = nullptr;

    if (sqlite3_open("", &db) != SQLITE_OK) {
        std::cerr << "Temporary db: " << sqlite3_errmsg(db) << std::endl;
        std::exit(EXIT_SQLITE);
    }

    if (sqlite3_exec(db, "CREATE TABLE metadata (name text, value text);", nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "Create metadata table: " << err << std::endl;
        std::exit(EXIT_SQLITE);
    }

    const fs::path metadata_path = fs::path(fname) / "metadata.json";

    auto file =
        std::unique_ptr<FILE, decltype(&fclose)>(
            fopen(metadata_path.string().c_str(), "r"),
            fclose);

    if (!file) {
        std::perror(metadata_path.string().c_str());
        return db;
    }

    json_pull *jp = json_begin_file(file.get());

    json_object *o = json_read_tree(jp);

    if (o == nullptr) {
        std::cerr << metadata_path << ": metadata parsing error: " << jp->error << std::endl;
        std::exit(EXIT_JSON);
    }

    if (o->type != JSON_HASH) {
        std::cerr << metadata_path << ": bad metadata format\n";
        std::exit(EXIT_JSON);
    }

    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, "INSERT INTO metadata (name, value) VALUES (?, ?)", -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare metadata insert: " << sqlite3_errmsg(db) << std::endl;
        std::exit(EXIT_SQLITE);
    }

    for (size_t i = 0; i < o->value.object.length; ++i) {
        auto *key = o->value.object.keys[i];
        auto *value = o->value.object.values[i];

        if (key->type != JSON_STRING ||
            value->type != JSON_STRING) {
            std::cerr << metadata_path << ": non-string in metadata\n";
            continue;
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        sqlite3_bind_text(stmt, 1, key->value.string.string, -1, SQLITE_TRANSIENT);

        sqlite3_bind_text(stmt, 2, value->value.string.string, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "set " << key->value.string.string << " in metadata: " << sqlite3_errmsg(db) << std::endl;
        }
    }

    sqlite3_finalize(stmt);

    json_end(jp);

    return db;
}

static void out(json_writer &state, std::string k, std::string v) {
	state.json_comma_newline();
	state.json_write_string(k);
	state.json_write_string(v);
}

void dir_write_metadata(const char *outdir, const metadata &m) {
    const fs::path metadata_path = fs::path(outdir) / "metadata.json";

    // Leave existing metadata in place with --allow-existing
    if (fs::exists(metadata_path)) {
        return;
    }

    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(
        fopen(metadata_path.string().c_str(), "w"),
        fclose);

    if (!fp) {
        perror(metadata_path.string().c_str());
        exit(EXIT_OPEN);
    }

    json_writer state(fp.get());

    state.json_write_hash();
    state.json_write_newline();

    out(state, "name", m.name);
    out(state, "description", m.description);
    out(state, "version", std::to_string(m.version));
    out(state, "minzoom", std::to_string(m.minzoom));
    out(state, "maxzoom", std::to_string(m.maxzoom));
    out(state, "center", std::to_string(m.center_lon) + "," + std::to_string(m.center_lat) + "," + std::to_string(m.center_z));
    out(state, "bounds", std::to_string(m.minlon) + "," + std::to_string(m.minlat) + "," + std::to_string(m.maxlon) + "," + std::to_string(m.maxlat));
    out(state, "antimeridian_adjusted_bounds", std::to_string(m.minlon2) + "," + std::to_string(m.minlat2) + "," + std::to_string(m.maxlon2) + "," + std::to_string(m.maxlat2));
    out(state, "type", m.type);
    if (m.attribution.size() > 0) {
        out(state, "attribution", m.attribution);
    }
    if (m.strategies_json.size() > 0) {
        out(state, "strategies", m.strategies_json);
    }
    if (m.decisions_json.size() > 0) {
        out(state, "tippecanoe_decisions", m.decisions_json);
    }
    out(state, "format", m.format);
    out(state, "generator", m.generator);
    out(state, "generator_options", m.generator_options);

    if (!m.vector_layers_json.empty() || !m.tilestats_json.empty()) {
        std::string json = "{";

        if (!m.vector_layers_json.empty()) {
            json += "\"vector_layers\":" + m.vector_layers_json;

            if (!m.tilestats_json.empty()) {
                json += ",\"tilestats\":" + m.tilestats_json;
            }
        } else {
            if (!m.tilestats_json.empty()) {
                json += "\"tilestats\":" + m.tilestats_json;
            }
        }

        json += "}";

        out(state, "json", json);
    }

    state.json_write_newline();
    state.json_end_hash();
    state.json_write_newline();
}
