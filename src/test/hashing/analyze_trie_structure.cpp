#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <index/cltj_index_spo_lite.hpp>
#include <trie/cltj_compact_trie.hpp>
#include <util/csv_util.hpp>
#include <util/logger.hpp>

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

    LOG_INFO("  Starting correct trie traversal...");

    // Level 0: Root
    LOG_INFO("  Processing Level 0 (Root)...");
    cltj::compact_trie::size_type root_children = trie.children(0);
    std::vector<std::string> root_row = {"0", std::to_string(root_children), "0", "root"};
    csv_writer.add_row(root_row);

    total_children += root_children;
    max_children = std::max(max_children, root_children);
    if (root_children > 0)
        nodes_with_children++;
    nodes_processed++;

    LOG_INFO("    Root has " << root_children << " children");

    // Level 1: Children of root (process all)
    if (root_children > 0) {
        LOG_INFO("  Processing Level 1 (Children of root)...");
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
                LOG_INFO("    Processed " << nodes_processed << " nodes");
            }
        }

        LOG_INFO("    Processed " << level1_limit << " level 1 nodes");

        // Level 2: Children of level 1 (process all)
        if (level1_limit > 0) {
            LOG_INFO("  Processing Level 2 (Children of level 1)...");
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

            LOG_INFO("    Processed level 2 nodes");
        }
    }

    csv_writer.flush();  // Ensure all data is written

    LOG_INFO("  Total nodes processed: " << nodes_processed);
    LOG_INFO("Trie " << trie_id << " statistics:");
    LOG_INFO("  Total nodes: " << nodes_processed);
    LOG_INFO("  Total children: " << total_children);
    LOG_INFO("  Max children per node: " << max_children);
    LOG_INFO("  Nodes with children: " << nodes_with_children);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << ((double)total_children / nodes_processed);
    LOG_INFO("  Average children per node: " << oss.str());
}

/**
 * Analyze a single trie using correct navigation
 */
void analyze_single_trie(
    cltj::cltj_index_spo_lite<cltj::compact_trie>& index, int trie_id, const std::string& output_dir
) {
    LOG_INFO("Processing trie " << trie_id << "...");

    const auto* trie = index.get_trie(trie_id);
    if (!trie) {
        LOG_WARN("  Warning: Trie " << trie_id << " is null, skipping...");
        return;
    }

    LOG_INFO("  Trie " << trie_id << " sequence size: " << trie->seq.size());
    analyze_trie_structure_correct(*trie, trie_id, output_dir);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        LOG_ERROR("Usage: " << argv[0] << " <index_file> <output_directory> <trie_id>");
        LOG_ERROR("Example: " << argv[0] << " data/index.cltj data/trie_analysis/ 0");
        LOG_ERROR("Note: Process only one trie at a time to avoid memory issues");
        return 1;
    }

    std::string index_file = argv[1];
    std::string output_dir = argv[2];
    int trie_id = std::stoi(argv[3]);

    std::string mkdir_cmd = "mkdir -p " + output_dir;
    system(mkdir_cmd.c_str());

    try {
        LOG_INFO("Loading index from " << index_file << "...");
        cltj::cltj_index_spo_lite<cltj::compact_trie> index;
        sdsl::load_from_file(index, index_file);

        LOG_INFO("Index loaded successfully. Size: " << sdsl::size_in_bytes(index) << " bytes");

        analyze_single_trie(index, trie_id, output_dir);

        LOG_INFO("Analysis complete! Check " << output_dir << " for CSV files.");

    } catch (const std::exception& e) {
        LOG_ERROR("Error: " << e.what());
        return 1;
    }

    return 0;
}