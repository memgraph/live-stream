#include <iostream>
#include <numeric>
#include <vector>

#include "mg_procedure.h"
#include "on_scope_exit.hpp"

static void procedure(const struct mgp_list *args,
                      const struct mgp_graph *graph, struct mgp_result *result,
                      struct mgp_memory *memory) {
  std::vector<uint64_t> local2mg_map;
  std::unordered_map<uint64_t, uint64_t> mg2local_map;
  // For each node count of out going edges.
  std::vector<uint64_t> out_count;

  // Build the mapping between Memgraph vertex ids and local vertex ids.
  // At the same time it is also possible to count how many outgoing edges
  // each node has.
  {
    auto *mapping_vertices_it = mgp_graph_iter_vertices(graph, memory);
    if (mapping_vertices_it == nullptr) {
      mgp_result_set_error_msg(result, "Not enough memory!");
      return;
    }
    utils::OnScopeExit mapping_vertices_it_deleter([&mapping_vertices_it] {
      if (mapping_vertices_it != nullptr) {
        mgp_vertices_iterator_destroy(mapping_vertices_it);
      }
    });
    for (const auto *vertex = mgp_vertices_iterator_get(mapping_vertices_it);
         vertex; vertex = mgp_vertices_iterator_next(mapping_vertices_it)) {
      uint64_t mg_vertex_id = mgp_vertex_get_id(vertex).as_int;
      local2mg_map.emplace_back(mg_vertex_id);
      mg2local_map[mg_vertex_id] = local2mg_map.size() - 1;
      auto *out_edges_it = mgp_vertex_iter_out_edges(vertex, memory);
      utils::OnScopeExit out_edges_iterator_deleter([&out_edges_it] {
        if (out_edges_it != nullptr) {
          mgp_edges_iterator_destroy(out_edges_it);
        }
      });
      uint64_t out_edges_counter = 0;
      for (const auto *edge = mgp_edges_iterator_get(out_edges_it); edge;
           edge = mgp_edges_iterator_next(out_edges_it)) {
        out_edges_counter++;
      }
      out_count.emplace_back(out_edges_counter);
    }
  }

  // An additional vertex iteration is required to construct incoming vertices
  // for each vertex.
  // From the previous pass we know the exact number of vertices so it's
  // it's possible to reserve the memory upfront which is more efficient then
  // resising on the fly.
  // For each node a list of adjecent incoming nodes.
  std::vector<std::vector<uint64_t>> in_vertices(local2mg_map.size(),
                                                 std::vector<uint64_t>(0));
  {
    auto *in_vertices_it = mgp_graph_iter_vertices(graph, memory);
    if (in_vertices_it == nullptr) {
      mgp_result_set_error_msg(result, "Not enough memory!");
      return;
    }
    utils::OnScopeExit in_vertices_it_deleter([&in_vertices_it] {
      if (in_vertices_it != nullptr) {
        mgp_vertices_iterator_destroy(in_vertices_it);
      }
    });
    for (const auto *vertex = mgp_vertices_iterator_get(in_vertices_it); vertex;
         vertex = mgp_vertices_iterator_next(in_vertices_it)) {
      uint64_t mg_vertex_id = mgp_vertex_get_id(vertex).as_int;
      uint64_t local_vertex_id = mg2local_map[mg_vertex_id];
      auto *in_edges_it = mgp_vertex_iter_in_edges(vertex, memory);
      utils::OnScopeExit in_edges_iterator_deleter([&in_edges_it] {
        if (in_edges_it != nullptr) {
          mgp_edges_iterator_destroy(in_edges_it);
        }
      });
      for (const auto *edge = mgp_edges_iterator_get(in_edges_it); edge;
           edge = mgp_edges_iterator_next(in_edges_it)) {
        uint64_t from_mg_vertex_id =
            mgp_vertex_get_id(mgp_edge_get_from(edge)).as_int;
        uint64_t from_local_vertex_id = mg2local_map[from_mg_vertex_id];
        in_vertices[local_vertex_id].emplace_back(from_local_vertex_id);
      }
    }
  }

  uint64_t vertices_count = local2mg_map.size();
  uint64_t max_iterations = 100;
  double damping_factor = 0.85;
  double stop_epsilon = 0.000001;
  std::vector<double> rank(vertices_count, 1.0 / vertices_count);
  std::vector<double> rank_next(vertices_count);
  size_t iter = 0;
  bool continue_iterate = true;
  if (max_iterations == 0) continue_iterate = false;
  while (continue_iterate) {
    continue_iterate = false;
    for (size_t node_id = 0; node_id < vertices_count; node_id++) {
      rank_next[node_id] = 0.0;
      const auto &in_neighbours = in_vertices[node_id];
      for (const auto in_neighbour : in_neighbours) {
        rank_next[node_id] += rank[in_neighbour] / out_count[in_neighbour];
      }
      rank_next[node_id] *= damping_factor;
      rank_next[node_id] += (1.0 - damping_factor) / vertices_count;
      if (std::abs(rank[node_id] - rank_next[node_id]) > stop_epsilon)
        continue_iterate = true;
    }
    rank.swap(rank_next);
    iter++;
    if (iter == max_iterations) continue_iterate = false;
  }
  double sum = std::accumulate(rank.begin(), rank.end(), 0.0);
  for (size_t node_id = 0; node_id < vertices_count; node_id++)
    rank[node_id] /= sum;

  for (uint64_t index = 0; index < local2mg_map.size(); ++index) {
    struct mgp_value *local_id_value = mgp_value_make_int(index, memory);
    if (local_id_value == nullptr) {
      mgp_result_set_error_msg(result, "Unable to make local vertex id");
    }
    utils::OnScopeExit local_id_value_deleter([&local_id_value] {
      if (local_id_value != nullptr) {
        mgp_value_destroy(local_id_value);
      }
    });

    struct mgp_value *mg_id_value =
        mgp_value_make_int(local2mg_map[index], memory);
    if (mg_id_value == nullptr) {
      mgp_result_set_error_msg(result, "Unable to make memgraph vertex id");
    }
    utils::OnScopeExit mg_id_value_deleter([&mg_id_value] {
      if (mg_id_value != nullptr) {
        mgp_value_destroy(mg_id_value);
      }
    });

    struct mgp_value *out_count_value =
        mgp_value_make_int(out_count[index], memory);
    if (out_count_value == nullptr) {
      mgp_result_set_error_msg(result, "Unable to make out count value");
    }
    utils::OnScopeExit out_count_value_deleter([&out_count_value] {
      if (out_count_value != nullptr) {
        mgp_value_destroy(out_count_value);
      }
    });

    struct mgp_value *in_count_value =
        mgp_value_make_int(in_vertices[index].size(), memory);
    if (in_count_value == nullptr) {
      mgp_result_set_error_msg(result, "Unable to make in count value");
    }
    utils::OnScopeExit in_count_value_deleter([&in_count_value] {
      if (in_count_value != nullptr) {
        mgp_value_destroy(in_count_value);
      }
    });

    struct mgp_value *rank_value = mgp_value_make_double(rank[index], memory);
    if (rank_value == nullptr) {
      mgp_result_set_error_msg(result, "Unable to make in rank value");
    }
    utils::OnScopeExit rank_value_value_deleter([&rank_value] {
      if (rank_value != nullptr) {
        mgp_value_destroy(rank_value);
      }
    });

    struct mgp_result_record *record = mgp_result_new_record(result);
    if (record == nullptr) {
      mgp_result_set_error_msg(result, "Unable to make a new record");
    }

    int mg_id_inserted = mgp_result_record_insert(record, "mg_id", mg_id_value);
    if (!mg_id_inserted) {
      mgp_result_set_error_msg(result, "Unable to insert mg id");
    }

    int local_id_inserted =
        mgp_result_record_insert(record, "local_id", local_id_value);
    if (!local_id_inserted) {
      mgp_result_set_error_msg(result, "Unable to insert local id");
    }

    int in_count_inserted =
        mgp_result_record_insert(record, "in_count", in_count_value);
    if (!in_count_inserted) {
      mgp_result_set_error_msg(result, "Unable to insert in count value");
    }

    int out_count_inserted =
        mgp_result_record_insert(record, "out_count", out_count_value);
    if (!out_count_inserted) {
      mgp_result_set_error_msg(result, "Unable to insert out count value");
    }

    int rank_value_inserted =
        mgp_result_record_insert(record, "rank", rank_value);
    if (!rank_value_inserted) {
      mgp_result_set_error_msg(result, "Unable to insert rank value");
    }
  }
}

extern "C" int mgp_init_module(struct mgp_module *module,
                               struct mgp_memory *memory) {
  struct mgp_proc *proc =
      mgp_module_add_read_procedure(module, "run", procedure);
  if (!proc) return 1;
  if (!mgp_proc_add_result(proc, "mg_id", mgp_type_int())) return 1;
  if (!mgp_proc_add_result(proc, "local_id", mgp_type_int())) return 1;
  if (!mgp_proc_add_result(proc, "in_count", mgp_type_int())) return 1;
  if (!mgp_proc_add_result(proc, "out_count", mgp_type_int())) return 1;
  if (!mgp_proc_add_result(proc, "rank", mgp_type_float())) return 1;
  return 0;
}

extern "C" int mgp_shutdown_module() { return 0; }
