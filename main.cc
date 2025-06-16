#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>


namespace {


    template<typename T>
    bool contains(std::vector<T> const& list, T const& entry) {
        return std::find(list.begin(), list.end(), entry) != std::end(list);
    }


    [[maybe_unused]]
    inline std::string_view to_string(bool v) { return v ? "true" : "false"; }


    [[maybe_unused]]
    inline bool starts_with(std::string const& s, std::string_view const& what) {
        return s.find(what) != std::string::npos;
    }



    [[maybe_unused]]
    std::string trim(const std::string& str) {
        auto const start = std::find_if_not(str.begin(), str.end(), ::isspace);
        auto const end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
        return (start < end) ? std::string(start, end) : std::string();
    }


    [[maybe_unused]]
    std::vector<std::string> tokenize(std::string const& line, char const seperator = ' ', bool remove_whitespace = true)
    {
        std::istringstream input(line);
        std::vector<std::string> tokens;
        std::string token;

        while (std::getline(input, token, seperator)) {
            if (not token.empty()) {
                if (remove_whitespace)
                    token = trim(token);
                tokens.push_back(token);
            }
        }

        return tokens;
    }


    [[maybe_unused]]
    std::optional<std::pair<std::string, std::string>> split(std::string const& line, char const seperator = '=')
    {
        size_t const path_seperator_pos = line.find(seperator);
        if (path_seperator_pos == std::string::npos)
            return {};
        auto const key = trim(line.substr(0, path_seperator_pos));
        auto const value = trim(line.substr(path_seperator_pos + 1));
        return std::make_pair(key, value);
    }


    template<typename KeyValue>
    void parse_line(std::string const& line, KeyValue& paths, KeyValue& options)
    {
        auto const first_seperator = line.find_first_of("=:");
        if (first_seperator == std::string::npos) // no line of interesst
            return;

        auto const is_path = line[first_seperator] == '=';
        auto const pair = split(line, is_path ? '=' : ':');
        if (not pair.has_value())
            return;

        if (is_path)
            paths.insert(pair.value());
        else
            options.insert(pair.value());
    }


    template<typename KeyValue>
    void merge_private(KeyValue& compiler_options)
    {
        auto iter = compiler_options.begin();
        while (iter != compiler_options.end()) {
            auto const private_pos = iter->first.rfind(".private");
            if (private_pos == std::string::npos) {
                ++iter;
                continue;
            }

            auto const base_key = iter->first.substr(0, private_pos);
            auto non_private= compiler_options.find(base_key);

            if (non_private != compiler_options.end()) {
                non_private->second += " " + iter->second;
                iter = compiler_options.erase(iter);
            }
            ++iter;
        }
    }


    template<typename KeyValue>
    void remove_rpath(KeyValue& compiler_options)
    {
        for (auto& entry : compiler_options) {
            if (not starts_with(entry.first, "Libs"))
                continue;
            auto const& args = entry.second;
            auto const rpath_pos = args.find("rpath");
            if (rpath_pos == std::string::npos)
                continue;

            auto const rpath_start = args.find_last_of(' ', rpath_pos);
            auto const rpath_end = args.find_first_of(' ', rpath_pos);

            auto const pre_rpath = args.substr(0, rpath_start);
            auto const post_rpath = args.substr(rpath_end);

            entry.second = pre_rpath + post_rpath;
        }
    }


    template<typename KeyValue>
    void dump(std::ostream& out, KeyValue const& paths, KeyValue const& compiler)
    {
        // reverse the order, so we get the same output as input was
        std::vector<std::pair<std::string, std::string>> p(paths.begin(), paths.end());
        std::reverse(p.begin(), p.end());

        for (auto const& e : p)
            out << e.first << "=" << e.second << "\n";

        out << "\n";

        std::vector<std::pair<std::string, std::string>> c(compiler.begin(), compiler.end());
        std::reverse(c.begin(), c.end());

        for (auto const& e : c)
            out << e.first << ": " << e.second << "\n";
    }


    template<typename KeyValue>
    void write_to_file(std::string const& filename, KeyValue const& paths, KeyValue const& compiler)
    {
        std::ofstream out(filename);
        if (not out.is_open()) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }
        dump(out, paths, compiler);
    }


    inline bool has_option(std::vector<std::string> const& opts, std::string_view const& opt) {
        auto const pos = opt.find_first_not_of('-');
        std::string const short_opt{ opt.substr(pos -1, 2) };
        std::string const long_opt{ opt };
        return contains(opts, short_opt) or contains(opts, long_opt);
    }

}

int main(int argc, char *argv[])
{
    std::vector<std::string> const args(argv + 1, argv + argc);

    if (args.empty() or has_option(args, "--help")) {
        std::cerr << "Usage: " << argv[0] << " [--inline] [--remove-rpath] [--merge] <pkg-config-file>" << std::endl;
        return 1;
    }

    auto const do_write_source = has_option(args, "--inline");
    auto const do_remove_rpath = has_option(args, "--remove-rpath");
    auto const do_merge_private = has_option(args, "--merge");

    auto const& filename = args.back();

#if 0
    std::cout << "Options:\n";
    std::cout << "  source: " << filename << "\n";
    std::cout << "  target: " << (do_write_source ? filename : "stdout") << "\n";
    std::cout << "  - inline: " << to_string(do_write_source) << "\n";
    std::cout << "  - rpath:  " << to_string(do_remove_rpath) << "\n";
    std::cout << "  - merge:  " << to_string(do_merge_private) << "\n";
    std::cout << "\n";
#endif

    std::ifstream input(filename);
    if (!input.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return 1;
    }

    std::unordered_map<std::string, std::string> pathEntries;
    std::unordered_map<std::string, std::string> compilerEntries;
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty() or line[0] == '#')
            continue;
        parse_line(line, pathEntries, compilerEntries);
    }

    input.close();

    if (do_remove_rpath)
        remove_rpath(compilerEntries);

    if (do_merge_private)
        merge_private(compilerEntries);

    if (do_write_source)
        write_to_file(filename, pathEntries, compilerEntries);
    else
        dump(std::cout, pathEntries, compilerEntries);

    return 0;
}
