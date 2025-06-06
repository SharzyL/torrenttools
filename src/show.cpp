
#include <filesystem>
#include <chrono>
#include <fstream>

#include <date/date.h>
#include <CLI/App.hpp>
#include <dottorrent/metafile.hpp>
#include <bencode/bvalue.hpp>
#include <bencode/encode.hpp>
#include <bencode/events/encode_json_to.hpp>


#include "cli_helpers.hpp"
#include "formatters.hpp"
#include "argument_parsers.hpp"
#include "show.hpp"
#include "escape_binary_fields.hpp"



namespace dt = dottorrent;
namespace fs = std::filesystem;
namespace bc = bencode;
namespace tt = torrenttools;

void configure_show_app(CLI::App* app, show_app_options& options)
{
    auto* announce_subapp = app->add_subcommand("announce-urls",      "Show the announce-urls.");
    auto* collection_subapp = app->add_subcommand("collections",      "Show the collection tags.");
    auto* comment_subapp = app->add_subcommand("comment",             "Show the comment field.");
    auto* created_by_subapp = app->add_subcommand("created-by",       "Show the created-by field.");
    auto* creation_date_subapp = app->add_subcommand("creation-date", "Show the creation-date field.");
    auto* dht_nodes_subapp = app->add_subcommand("dht-nodes",         "Show the dht nodes.");
    auto* file_size_subapp = app->add_subcommand("size",              "Show the total file size.");
    auto* files_subapp = app->add_subcommand("files",                 "Show the files in a metafile.");
    auto* http_seeds_subapp = app->add_subcommand("http-seeds",       "Show the http-seeds.");
    auto* infohash_subapp = app->add_subcommand("infohash",           "Show the the infohash.");
    auto* name_subapp = app->add_subcommand("name",                   "Show the metafile name.");
    auto* piece_count_subapp = app->add_subcommand("piece-count",     "Show the piece count.");
    auto* piece_size_subapp = app->add_subcommand("piece-size",       "Show the piece size.");
    auto* private_subapp = app->add_subcommand("private",             "Show the private flag.");
    auto* protocol_subapp = app->add_subcommand("protocol",           "Show the protocol.");
    auto* query_subapp = app->add_subcommand("query",                 "Show data referenced by a bencode pointer.");
    auto* similar_torrents_subapp = app->add_subcommand("similar",    "Show the similar torrent infohashes.");
    auto* source_subapp = app->add_subcommand("source",               "Show the source field.");
    auto* web_seeds_subapp = app->add_subcommand("web-seeds",         "Show the web-seeds.");
    auto* checksums_subapp = app->add_subcommand("checksums",         "Show the per-file checksums.");

    std::array apps = {
            announce_subapp,
            comment_subapp,
            created_by_subapp,
            creation_date_subapp,
            file_size_subapp,
            files_subapp,
            infohash_subapp,
            name_subapp,
            piece_count_subapp,
            piece_size_subapp,
            private_subapp,
            protocol_subapp,
            query_subapp,
            source_subapp,
            web_seeds_subapp,
            http_seeds_subapp,
            dht_nodes_subapp,
            similar_torrents_subapp,
            collection_subapp,
            checksums_subapp
    };

    for (auto* app: apps) {
        configure_show_common(app, options);
    }

    configure_show_announce_subapp(       announce_subapp,         options);
    configure_show_comment_subapp(        comment_subapp,          options);
    configure_show_created_by_subapp(     created_by_subapp,       options);
    configure_show_creation_date_subapp(  creation_date_subapp,    options);
    configure_show_file_size_subapp(      file_size_subapp,        options);
    configure_show_files_subapp(          files_subapp,            options);
    configure_show_infohash_subapp(       infohash_subapp,         options);
    configure_show_name_subapp(           name_subapp,             options);
    configure_show_piece_count_subapp(    piece_count_subapp,      options);
    configure_show_piece_size_subapp(     piece_size_subapp,       options);
    configure_show_private_subapp(        private_subapp,          options);
    configure_show_protocol_subapp(       protocol_subapp,         options);
    configure_show_query_subapp(          query_subapp,            options);
    configure_show_source_subapp(         source_subapp,           options);
    configure_web_seeds_subapp(           web_seeds_subapp,        options);
    configure_http_seeds_subapp(          http_seeds_subapp,       options);
    configure_dht_nodes_subapp(           dht_nodes_subapp,        options);
    configure_similar_torrents_subapp(    similar_torrents_subapp, options);
    configure_collection_subapp(          collection_subapp,       options);
    configure_checksum_subapp(            checksums_subapp,       options);
}

void configure_show_common(CLI::App* subapp, show_app_options& options)
{
    CLI::callback_t metafile_parser = [&](const CLI::results_t& v) -> bool {
        options.metafile = metafile_target_transformer(v);
        return true;
    };

    subapp->add_option("target", metafile_parser, "Target bittorrent metafile.")
        ->type_name("<path>")
        ->required();
}

void configure_show_announce_subapp(CLI::App* announce_subapp, show_app_options& options)
{
    announce_subapp->parse_complete_callback([&](){ options.subcommand = "announce-urls"; });

    announce_subapp->add_flag("--flat", options.announce_flatten,
            "Flatten announce tiers.\n"
            "This results in the output containing one announce per line.");
}


void configure_show_protocol_subapp(CLI::App* protocol_app, show_app_options& options)
{
    protocol_app->parse_complete_callback([&](){ options.subcommand = "protocol"; });
}


void configure_show_infohash_subapp(CLI::App* infohash_app, show_app_options& options)
{
    infohash_app->parse_complete_callback([&](){ options.subcommand = "infohash"; });

    CLI::callback_t protocol_parser = [&](const CLI::results_t& v) -> bool {
        options.infohash_protocol = protocol_transformer(v);
        return true;
    };

    infohash_app->add_option("-v,--protocol", protocol_parser,
        "Show only the infohash of the specified protocol for hybrid metafiles. [Default: hybrid]"
        "This option is only used for hybrid metafiles.\n"
        "When hybrid is specified, hybrid torrents will print both \n"
        "the v1 and v2 infohash on seperate lines.\n"
        "When 1, or 2 is given only the corresponding infohash will be returned.\n"
        "Options are: 1, 2 or hybrid.");

    infohash_app->add_flag("-t,--truncate", options.infohash_truncate,
            "Truncate v2 infohash to 20 bytes");
}

void configure_show_piece_count_subapp(CLI::App* piece_size_app, show_app_options& options)
{
    piece_size_app->parse_complete_callback([&](){ options.subcommand = "piece-count"; });
}

void configure_show_piece_size_subapp(CLI::App* piece_size_app, show_app_options& options)
{
    piece_size_app->parse_complete_callback([&](){ options.subcommand = "piece-size"; });

    piece_size_app->add_flag("-H,--human-readable", options.piece_size_human_readable,
            "Output size in a human readable format: eg. 1 MiB.");
}


void configure_show_created_by_subapp(CLI::App* created_by_subapp, show_app_options& options)
{
    created_by_subapp->parse_complete_callback([&](){ options.subcommand = "created-by"; });
}

void configure_show_creation_date_subapp(CLI::App* creation_date_subapp, show_app_options& options)
{
    creation_date_subapp->parse_complete_callback([&](){ options.subcommand = "creation-date"; });

    creation_date_subapp->add_flag("--iso", options.creation_date_iso_format,
            "Output size in an ISO 8601 datetime format instead of POSIX time.");
}

void configure_show_private_subapp(CLI::App* private_subapp, show_app_options& options)
{
    private_subapp->parse_complete_callback([&](){ options.subcommand = "private"; });
}


void configure_show_name_subapp(CLI::App* name_subapp, show_app_options& options)
{
    name_subapp->parse_complete_callback([&](){ options.subcommand = "name"; });

}

void configure_show_comment_subapp(CLI::App* creation_date_subapp, show_app_options& options)
{
    creation_date_subapp->parse_complete_callback([&](){ options.subcommand = "comment"; });
}

void configure_show_source_subapp(CLI::App* source_subapp, show_app_options& options)
{
    source_subapp->parse_complete_callback([&](){ options.subcommand = "source"; });
}

void configure_show_query_subapp(CLI::App* query_subapp, show_app_options& options)
{
    query_subapp->parse_complete_callback([&](){ options.subcommand = "query"; });

    query_subapp->add_option("query", options.query,
             "Retrieve a field referenced by a bpointer in the target field.")
             ->type_name("<query>");

    query_subapp->add_flag("-b,--show-binary", options.query_show_binary,
            "Show binary fields as hexadecimal strings. "
            "If not enabled binary fields will be replaced by a string descibing the content");
}


void configure_show_file_size_subapp(CLI::App* file_size_subapp, show_app_options& options)
{
    file_size_subapp->parse_complete_callback([&](){ options.subcommand = "size"; });

    file_size_subapp->add_flag("-H,--human-readable", options.file_size_human_readable,
            "Output size in a human readable format: eg. 1 MiB.");
}


void configure_show_files_subapp(CLI::App* files_subapp, show_app_options& options)
{
    files_subapp->parse_complete_callback([&](){ options.subcommand = "files"; });

    files_subapp->add_flag("--show-padding-files", options.show_padding_files,
               "Show padding files in the file tree.")
       ->default_val(false);

    files_subapp->add_option("--prefix", options.files_prefix,
               "Custom prefix to prepend to the files.");
}


void configure_web_seeds_subapp(CLI::App* web_seeds_subapp, show_app_options& options)
{
    web_seeds_subapp->parse_complete_callback([&](){ options.subcommand = "web-seeds"; });
}

void configure_http_seeds_subapp(CLI::App* http_seeds_subapp, show_app_options& options)
{
    http_seeds_subapp->parse_complete_callback([&](){ options.subcommand = "http-seeds"; });
}

void configure_dht_nodes_subapp(CLI::App* dht_nodes_subapp, show_app_options& options)
{
    dht_nodes_subapp->parse_complete_callback([&](){ options.subcommand = "http-seeds"; });
}

void configure_similar_torrents_subapp(CLI::App* similar_torrents_subapp, show_app_options& options)
{
    similar_torrents_subapp->parse_complete_callback([&](){ options.subcommand = "similar"; });
}

void configure_collection_subapp(CLI::App* similar_torrents_subapp, show_app_options& options)
{
    similar_torrents_subapp->parse_complete_callback([&](){ options.subcommand = "collection"; });
}

void configure_checksum_subapp(CLI::App* checksum_subapp, show_app_options& options)
{
    checksum_subapp->parse_complete_callback([&](){ options.subcommand = "checksum"; });

    CLI::callback_t checksum_parser =[&](const CLI::results_t& v) -> bool {
        auto r = checksum_transformer(v);
        options.checksum_algorithm = std::move(*r.begin());
        return true;
    };

    checksum_subapp->add_option("--checksum", checksum_parser,
                    "The checksums algorithm to show checksums for." )
            ->type_name("<algorithm>")
            ->expected(0, 1);
}



void run_show_app(CLI::App* show_app, const main_app_options& main_options, const show_app_options& options)
{
    using run_show_app_function_type = void (*)(const main_app_options&, const show_app_options&);

    static std::unordered_map<std::string_view, run_show_app_function_type> dispatch_table {
            {"announce-urls",   &run_show_announce_subapp},
            {"comment",         &run_show_comment_subapp},
            {"created-by",      &run_show_created_by_subapp},
            {"creation-date",   &run_show_creation_date_subapp},
            {"dht-nodes",       &run_show_dht_nodes_subapp},
            {"files",           &run_show_files_subapp},
            {"http-seeds",      &run_show_http_seeds_subapp},
            {"infohash",        &run_show_infohash_subapp},
            {"name",            &run_show_name_subapp},
            {"piece-count",     &run_show_piece_count_subapp},
            {"piece-size",      &run_show_piece_size_subapp},
            {"private",         &run_show_private_subapp},
            {"protocol",        &run_show_protocol_subapp},
            {"query",           &run_show_query_subapp},
            {"similar",         &run_show_similar_torrents_subapp},
            {"size",            &run_show_file_size_subapp},
            {"source",          &run_show_source_subapp},
            {"web-seeds",       &run_show_web_seeds_subapp},
            {"collection",      &run_show_collection_subapp},
            {"checksum",        &run_show_checksum_subapp},
    };

    if (!options.subcommand.empty()) {
        std::invoke(dispatch_table[options.subcommand], main_options, options);
    }
    else if (show_app->count_all() == 1) {
        std::cout << show_app->help();
    }
}


void run_show_protocol_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);

    auto protocol = m.storage().protocol();
    switch(protocol) {
    case dt::protocol::v1:     { std::cout << 1 << std::endl; return; }
    case dt::protocol::v2:     { std::cout << 2 << std::endl; return; }
    case dt::protocol::hybrid: { std::cout << "hybrid" << std::endl; return; }
    case dt::protocol::none:   { std::cout << "incomplete metafile" << std::endl; return; }
    }
}

void run_show_announce_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);

    if (options.announce_flatten) {
        for (const auto& announce : m.trackers()) {
            std::cout << announce.url << std::endl;
        }
    }
    else {
        const auto& announces = m.trackers();
        for (std::size_t idx = 0; idx < announces.tier_count(); ++idx ) {
            const auto& [begin, end] = announces.get_tier(idx);

            auto it = begin;
            std::cout << it->url;
            ++it;
            for ( ; it != end; ++it) {
                std::cout << " " << it->url;
            }
            std::cout << '\n';
        }
     }
}

void run_show_piece_count_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    std::cout << m.piece_count() << std::endl;
}

void run_show_piece_size_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    if (options.piece_size_human_readable) {
        std::cout << tt::format_size(m.piece_size()) << std::endl;
    } else {
        std::cout << m.piece_size() << std::endl;
    }
}

void run_show_infohash_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    const auto& s = m.storage();

    auto print_v1 = [&m]() {
        std::cout << dt::info_hash_v1(m).hex_string() << std::endl;
    };

    auto print_v2 = [&m]() {
        std::cout << dt::info_hash_v2(m).hex_string() << std::endl;
    };

    auto print_v2_truncated = [&m]() {
        std::cout << dt::truncate_v2_hash(dt::info_hash_v2(m)).hex_string() << std::endl;
    };

    switch (s.protocol())
    {
    case dt::protocol::v1: {
        print_v1();
        return;
    }
    case dt::protocol::v2: {
        if (options.infohash_truncate) {
            print_v2_truncated();
        } else {
            print_v2();
        }
        return;
    }
    case dt::protocol::hybrid: {
        if (options.infohash_protocol == dt::protocol::hybrid) {
            print_v1();
            if (options.infohash_truncate) {
                print_v2_truncated();
            } else {
                print_v2();
            }
        }
        else if (options.infohash_protocol == dt::protocol::v1) {
            print_v1();
        }
        else if (options.infohash_protocol == dt::protocol::v2) {
            if (options.infohash_truncate) {
                print_v2_truncated();
            } else {
                print_v2();
            }
        } else {
            throw std::invalid_argument("Unrecognised protocol version");
        }
        return;
    }
    case dottorrent::protocol::none:
        throw std::invalid_argument("BitTorrent metafile is incomplete");
    }
}

void run_show_created_by_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    std::cout << m.created_by() << std::endl;
}

void run_show_creation_date_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    if (! options.creation_date_iso_format) {
        std::cout << m.creation_date().count() << std::endl;
    }
    else {
        auto tp = std::chrono::system_clock::time_point(m.creation_date());
        auto daypoint = date::floor<date::days>(tp);
        auto ymd = date::year_month_day(daypoint);   // calendar date
        auto tod = date::make_time(date::floor<std::chrono::seconds>(tp - daypoint)); // Yields time_of_day type
        std::cout << ymd << 'T' << tod << 'Z' << std::endl;
    }
}

void run_show_private_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    std::cout << int(m.is_private()) << std::endl;
}

void run_show_name_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    std::cout << m.name() << std::endl;
}

void run_show_comment_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    std::cout << m.comment() << std::endl;
}

void run_show_source_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    std::cout << m.source() << std::endl;
}

void run_show_file_size_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    if (options.file_size_human_readable) {
        std::cout << tt::format_size(m.total_regular_file_size()) << std::endl;
    } else {
        std::cout << m.total_regular_file_size() << std::endl;
    }
}


void run_show_query_subapp(const main_app_options& main_options, const show_app_options& options)
{
    std::ifstream ifs (options.metafile);
    std::string data(std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{});

    auto bv = bencode::decode_value(data);
    if (!options.query_show_binary) {
        bv = escape_binary_metafile_fields(bv);
    }
    else {
        bv = escape_binary_metafile_fields_hex(bv);
    }
    try {
        auto pointer = bc::bpointer(options.query);
        auto result = bv.at(pointer);
        bc::events::encode_json_to formatter(std::cout);
        bc::connect(formatter, result);
        std::cout << std::endl;
    }
    catch (const bc::bpointer_error& err) {
        auto help_message = fmt::format("Invalid query: {}", err.what());
        throw std::invalid_argument(help_message);
    }
    catch (const bc::out_of_range& err) {
        throw std::invalid_argument("No data matching query");
    }
}


void run_show_files_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);

    for (const auto& f: m.storage()) {
        if (!options.show_padding_files && f.is_padding_file())
            continue;

        if (!options.files_prefix.empty()) {
            std::cout << (options.files_prefix.lexically_normal() / f.path()).string() << '\n';
        } else {
            std::cout << f.path().string() << '\n';
        }
    }
}

void run_show_web_seeds_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    for (const auto& url : m.web_seeds()) {
        std::cout << url << '\n';
    }
}

void run_show_http_seeds_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    for (const auto& url : m.http_seeds()) {
        std::cout << url << '\n';
    }
}

void run_show_dht_nodes_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    for (const auto& node : m.dht_nodes()) {
        std::cout << std::string(node) << '\n';
    }
}

void run_show_similar_torrents_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    for (const auto& s : m.similar_torrents()) {
        if (s.version() == dt::protocol::v1) {
            std::cout << s.v1().hex_string() << '\n';
        }
        else if (s.version() == dt::protocol::v2) {
            std::cout << s.v2().hex_string() << '\n';
        }
        // This branch is never used since we parse hybrid similar-infohashes as seperate v1 and v2 hashes.
        else if (s.version() == dt::protocol::hybrid) {
            std::cout << s.v1().hex_string() << " | " << s.v2().hex_string() << '\n';
        }
    }
}

void run_show_collection_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);
    for (const auto& c : m.collections()) {
        std::cout << c << '\n';
    }
}

void run_show_checksum_subapp(const main_app_options& main_options, const show_app_options& options)
{
    auto m = dt::load_metafile(options.metafile);\
    const auto& storage = m.storage();

    // check the available checksum functions
    std::set<dt::hash_function> available_checksums;

    for (const auto& f: storage) {
        for (const auto& [key, value] : f.checksums()) {
            available_checksums.emplace(value->algorithm());
        }
    }

    std::vector<dt::hash_function> out;
    rng::copy(available_checksums, std::back_inserter(out));
    rng::sort(out);

    if (!options.checksum_algorithm) {
        for (const auto& algo : out) {
            std::cout << dt::to_string(algo) << '\n';
        }

        return;
    }

    // first check if the metafile contains checksums of this algorithm
    if (!available_checksums.contains(*options.checksum_algorithm)) {
        auto s = fmt::format( "no {} checksums contained in metafile",
                             dt::to_string(*options.checksum_algorithm));
        throw std::invalid_argument(s);
    }

    // otherwise we print all files and their checksums in <algorithm_name>sum format
    auto checksum_key = std::string(dt::to_string(*options.checksum_algorithm));
    for (const auto& file : m.storage()) {
        auto checksum = file.checksums().at(checksum_key)->value();
        auto line = fmt::format("{} *{}", dt::to_hexadecimal_string(checksum), file.path().string());
        std::cout << line << std::endl;
    }
}

