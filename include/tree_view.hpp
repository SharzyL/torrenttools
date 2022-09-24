#pragma once

#include <string>
#include <string_view>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <concepts>

#include <fmt/format.h>
#include <fmt/color.h>

#include <cliprogress/functional/progress_bar.hpp>
#include <termcontrol/detail/format.hpp>
#include <termcontrol/detail/color.hpp>
#include <termcontrol/detail/display_width.hpp>

#include <dottorrent/metafile.hpp>
#include <dottorrent/storage_verifier.hpp>

#include "natural_sort.hpp"
#include "formatters.hpp"
#include "ls_colors.hpp"


namespace fs = std::filesystem;
namespace tc = termcontrol;
namespace dt = dottorrent;
namespace tt = torrenttools;
namespace rng = std::ranges;
namespace clp = cliprogress;


using namespace std::string_view_literals;


// TODO: Support user defined color in file tree via LS_COLORS on linux.

struct tree_options
{
    bool show_file_size = true;
    bool show_directory_size = true;
    bool use_color = true;
    bool list_padding_files = false;
    std::size_t max_entry_size = -1;
};


inline bool ellipsize(std::string& line, std::size_t max_size = 100)
{
    auto wline = termcontrol::utf8_decode(line);
    bool needs_ellipsis = wline.size() > max_size;

    if (needs_ellipsis) {
        std::fill(wline.end()-3, wline.end(), '.');
    }
    line = termcontrol::utf8_encode(wline);
    return needs_ellipsis;
}


struct filetree_index
{
    struct node {
        std::size_t first;                      // index of first file in dir
        std::size_t last;                       // index of one past last file in dir
        std::size_t total_file_size;            // total file_size of files between first and last
        bool padding_dir = false;
    };

    struct directory_content_entry
    {
        fs::path path;
        const dottorrent::file_entry* entry;
    };


    explicit filetree_index(const dottorrent::file_storage& storage, bool list_padding_files = false)
            : storage_(storage)
            , list_padding_files_(list_padding_files)
    {
        create_sorted_file_indices(storage_);
        create_directory_map(storage_);

        if (!list_padding_files_) {
            remove_padding_file_only_directories();
        }
    }

    auto get_directory_size(const fs::path& dir) -> std::size_t
    {
        Ensures(directories_.contains(dir));
        return directories_.at(dir).total_file_size;
    }

    std::vector<directory_content_entry> list_directory_content(const fs::path& root)
    {
        std::vector<directory_content_entry> out {};

        const auto& storage = storage_.get();
        const auto& node = directories_.at(root);

        for (auto i = node.first; i < node.last; ) {
            Ensures(node.last != std::size_t(-1));
            const auto &f = storage[indices_[i]];
            fs::path relative_path = f.path().lexically_relative(root);

            // path has subdirectories: list the directory but do not list the files inside the subdirectory
            if (relative_path.has_parent_path()) {
                auto dir_path = *relative_path.begin();

                const auto& node_ref = directories_.at(root / dir_path);

                // do not add an entry for paddinf ifle only directories
                if (!node_ref.padding_dir) {
                    out.push_back({.path = dir_path, .entry = nullptr});
                }
                i = node_ref.last;
            }
            else {
                if (!list_padding_files_ && f.is_padding_file()) {
                    ++i;
                }
                else {
                    out.push_back({.path = relative_path.filename(), .entry = &f});
                    ++ i;
                }
            }
        }
        return out;
    }

private:
    void create_sorted_file_indices(const dt::file_storage& storage)
    {
        indices_.resize(storage.file_count());
        std::iota(indices_.begin(), indices_.end(), 0);

        const auto cmp = [&](std::size_t lhs, std::size_t rhs) {

            Expects(lhs < storage.size());
            Expects(rhs < storage.size());
            return storage[lhs] < storage[rhs];
        };
        std::sort(indices_.begin(), indices_.end(), cmp);
    }

    void create_directory_map(const dt::file_storage& storage)
    {
        fs::path current_root_dir {};

        // root node is the default constructed path.
        directories_.try_emplace(fs::path{},
                                 node{0, indices_.size(), storage.total_file_size()});

        for (std::size_t meta_index = 0; meta_index < indices_.size(); ++meta_index) {
            auto index = indices_[meta_index];
            Ensures(index < storage.size());
            const auto& f = storage[index];
            const auto& path = f.path();

            auto part_count = std::distance(path.begin(), path.end());
            auto it = path.begin();
            fs::path partial_path {};

            for (std::size_t i = 0; i < part_count - 1; ++i, ++it) {
                partial_path /= *it;
                auto node_it = directories_.find(partial_path);

                // directory is not already present
                if (node_it == directories_.end()) {
                    directories_.try_emplace(
                            partial_path,
                            node{meta_index, meta_index+1, f.file_size()});
                    current_root_dir = *it;
                }
                else {
                    node_it->second.total_file_size += f.file_size();
                    node_it->second.last += 1;
                }
            }
        }
    }

    void remove_padding_file_only_directories()
    {
        const dottorrent::file_storage& storage = storage_;

        for (auto& [path, node] : directories_) {
            bool contains_all_padding_files = dt::is_padding_directory(storage, path);
            if (contains_all_padding_files) {
                node.padding_dir = true;
            }
        }
    }

    std::reference_wrapper<const dottorrent::file_storage> storage_;
    std::vector<std::size_t> indices_ {};
    std::map<fs::path, node> directories_ {};
    bool list_padding_files_;
};


class tree_printer
{
    static constexpr auto node = "├── "sv;
    static constexpr auto end_node = "└── "sv;
    static constexpr auto sub = "│   "sv;
    static constexpr auto sub_last = "    "sv;

    static constexpr auto directory_color = fg(tc::terminal_color::blue) /*| fmt::emphasis::bold*/;
    static constexpr auto file_color = tc::text_style{};

    using directory_size_map = std::unordered_map<std::string, std::size_t>;
    using entry = dottorrent::file_entry;

    struct stack_frame
    {
        fs::path path;
        std::vector<filetree_index::directory_content_entry> content;
        std::size_t index;
    };

public:
    explicit tree_printer(const dottorrent::metafile& m, std::string_view prefix = ""sv, tree_options options = {});

    const std::vector<std::pair<std::string, const dt::file_entry*>> entries() const noexcept;

    [[nodiscard]]
    std::string result() const;

    void walk(const fs::path& root = "");

    bool print_entry(const fs::path& path, const dt::file_entry* entry_ptr, std::string_view node, const fs::path& root);

private:
    std::vector<std::pair<std::string, const dt::file_entry*>> output_;
    const dottorrent::metafile& metafile_;
    std::string prefix_;
    filetree_index index_;
    tree_options options_;
    ls_colors ls_colors_;
};


std::string format_file_tree(const dottorrent::metafile& m,
                             std::string_view prefix = ""sv,
                             const tree_options& options = {});


std::string format_verify_file_tree(
        const dottorrent::metafile& m,
        const dottorrent::storage_verifier& verifier,
        std::string_view prefix = ""sv,
        const tree_options& options = {});


std::string format_file_stats(const dottorrent::metafile& m,
                              std::string_view prefix = "",
                              bool include_pad_files = false);


std::string format_announce_tree(const dottorrent::announce_url_list& e, std::string_view line_format = "");