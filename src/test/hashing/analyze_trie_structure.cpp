#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <index/cltj_index_spo_lite.hpp>
#include <trie/cltj_compact_trie.hpp>
#include <util/csv_util.hpp>

/**
 * Analyze the structure of a single trie using correct LOUDS navigation
 */
void analyze_trie_structure_correct(
    const cltj::compact_trie& trie, int trie_id, const std::string& output_dir
) {
    std::string children_file = output_dir + "/trie_" + std::to_string(trie_id) + "_children.csv";
    std::vector<std::string> header = {"node_id", "children_count", "depth", "level_type"};
    util::CSVWriter csv_writer(children_file, header, 10000);

    cltj::compact_trie::size_type nodes_processed = 0;
    cltj::compact_trie::size_type total_children = 0;
    cltj::compact_trie::size_type max_children = 0;
    cltj::compact_trie::size_type nodes_with_children = 0;

    std::cout << "  Starting correct trie traversal..." << std::endl;

    // Level 0: Root
    std::cout << "  Processing Level 0 (Root)..." << std::endl;
    cltj::compact_trie::size_type root_children = trie.children(0);
    std::vector<std::string> root_row = {"0", std::to_string(root_children), "0", "root"};
    csv_writer.add_row(root_row);

    total_children += root_children;
    max_children = std::max(max_children, root_children);
    if (root_children > 0)
        nodes_with_children++;
    nodes_processed++;

    std::cout << "    Root has " << root_children << " children" << std::endl;

    // Level 1: Children of root (process all)
    if (root_children > 0) {
        std::cout << "  Processing Level 1 (Children of root)..." << std::endl;
        cltj::compact_trie::size_type first_child_pos = trie.first_child(0);
        cltj::compact_trie::size_type level1_limit = root_children;

        for (cltj::compact_trie::size_type i = 0; i < level1_limit; i++) {
            cltj::compact_trie::size_type pos = first_child_pos + i;
            cltj::compact_trie::size_type node_id = trie.nodeselect(pos, 1);  // gap = 1 for root
            cltj::compact_trie::size_type num_children = trie.children(node_id);

            std::vector<std::string> row = {
                std::to_string(node_id), std::to_string(num_children), "1", "level1"
            };
            csv_writer.add_row(row);

            total_children += num_children;
            max_children = std::max(max_children, num_children);
            if (num_children > 0)
                nodes_with_children++;
            nodes_processed++;

            if (nodes_processed % 10000 == 0) {
                std::cout << "    Processed " << nodes_processed << " nodes" << std::endl;
            }
        }

        std::cout << "    Processed " << level1_limit << " level 1 nodes" << std::endl;

        // Level 2: Children of level 1 (process all)
        if (level1_limit > 0) {
            std::cout << "  Processing Level 2 (Children of level 1)..." << std::endl;
            cltj::compact_trie::size_type level2_limit = level1_limit;

            for (cltj::compact_trie::size_type i = 0; i < level2_limit; i++) {
                cltj::compact_trie::size_type pos = first_child_pos + i;
                cltj::compact_trie::size_type level1_node = trie.nodeselect(pos, 1);
                cltj::compact_trie::size_type level1_children = trie.children(level1_node);

                if (level1_children > 0) {
                    cltj::compact_trie::size_type level1_first_child_pos = trie.first_child(level1_node);

                    for (cltj::compact_trie::size_type j = 0; j < level1_children; j++) {
                        cltj::compact_trie::size_type level2_pos = level1_first_child_pos + j;
                        cltj::compact_trie::size_type level2_node =
                            trie.nodeselect(level2_pos, 0);  // gap = 0 for no root
                        cltj::compact_trie::size_type num_children = trie.children(level2_node);

                        std::vector<std::string> row = {
                            std::to_string(level2_node), std::to_string(num_children), "2", "level2"
                        };
                        csv_writer.add_row(row);

                        total_children += num_children;
                        max_children = std::max(max_children, num_children);
                        if (num_children > 0)
                            nodes_with_children++;
                        nodes_processed++;
                    }
                }
            }

            std::cout << "    Processed level 2 nodes" << std::endl;
        }
    }

    csv_writer.flush();  // Ensure all data is written

    std::cout << "  Total nodes processed: " << nodes_processed << std::endl;
    std::cout << "Trie " << trie_id << " statistics:" << std::endl;
    std::cout << "  Total nodes: " << nodes_processed << std::endl;
    std::cout << "  Total children: " << total_children << std::endl;
    std::cout << "  Max children per node: " << max_children << std::endl;
    std::cout << "  Nodes with children: " << nodes_with_children << std::endl;
    std::cout << "  Average children per node: " << (double)total_children / nodes_processed << std::endl;
}

/**
 * Analyze a single trie using correct navigation
 */
void analyze_single_trie(
    cltj::cltj_index_spo_lite<cltj::compact_trie>& index, int trie_id, const std::string& output_dir
) {
    std::cout << "Processing trie " << trie_id << "..." << std::endl;

    const auto* trie = index.get_trie(trie_id);
    if (!trie) {
        std::cout << "  Warning: Trie " << trie_id << " is null, skipping..." << std::endl;
        return;
    }

    std::cout << "  Trie " << trie_id << " sequence size: " << trie->seq.size() << std::endl;
    analyze_trie_structure_correct(*trie, trie_id, output_dir);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <index_file> <output_directory> <trie_id>" << std::endl;
        std::cerr << "Example: " << argv[0] << " data/index.cltj data/trie_analysis/ 0" << std::endl;
        std::cerr << "Note: Process only one trie at a time to avoid memory issues" << std::endl;
        return 1;
    }

    std::string index_file = argv[1];
    std::string output_dir = argv[2];
    int trie_id = std::stoi(argv[3]);

    std::string mkdir_cmd = "mkdir -p " + output_dir;
    system(mkdir_cmd.c_str());

    try {
        std::cout << "Loading index from " << index_file << "..." << std::endl;
        cltj::cltj_index_spo_lite<cltj::compact_trie> index;
        sdsl::load_from_file(index, index_file);

        std::cout << "Index loaded successfully. Size: " << sdsl::size_in_bytes(index) << " bytes"
                  << std::endl;

        analyze_single_trie(index, trie_id, output_dir);

        std::cout << "Analysis complete! Check " << output_dir << " for CSV files." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}