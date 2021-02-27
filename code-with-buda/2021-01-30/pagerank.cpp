#include <iostream>
#include <vector>

#include "mg_procedure.h"
#include "on_scope_exit.hpp"

static void procedure(const struct mgp_list *args,
                      const struct mgp_graph *graph, struct mgp_result *result,
                      struct mgp_memory *memory) {
  auto *vertices_it = mgp_graph_iter_vertices(graph, memory);
  if (vertices_it == nullptr) {
    mgp_result_set_error_msg(result, "Not enough memory!");
    return;
  }
  utils::OnScopeExit vertices_it_deleter([&vertices_it] {
    if (vertices_it != nullptr) {
      mgp_vertices_iterator_destroy(vertices_it);
    }
  });

  std::vector<uint64_t> mapping;
  std::vector<uint64_t> out_count;
  for (const auto *vertex = mgp_vertices_iterator_get(vertices_it); vertex;
       vertex = mgp_vertices_iterator_next(vertices_it)) {
    mapping.emplace_back(mgp_vertex_get_id(vertex).as_int);
    auto *out_edges_it = mgp_vertex_iter_out_edges(vertex, memory);
    utils::OnScopeExit out_edges_iterator_deleter([&out_edges_it] {
      if (out_edges_it != nullptr) {
        mgp_edges_iterator_destroy(out_edges_it);
      }
    });
    int64_t out_edges_counter = 0;
    for (const auto *edge = mgp_edges_iterator_get(out_edges_it); edge;
         edge = mgp_edges_iterator_next(out_edges_it)) {
      out_edges_counter++;
    }
    out_count.emplace_back(out_edges_counter);
  }

  for (uint64_t index = 0; index < mapping.size(); ++index) {
    struct mgp_value *id_value = mgp_value_make_int(mapping[index], memory);
    if (id_value == nullptr) {
      mgp_result_set_error_msg(result, "Unable to make vertex id");
    }
    utils::OnScopeExit id_value_deleter([&id_value] {
      if (id_value != nullptr) {
        mgp_value_destroy(id_value);
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

    struct mgp_result_record *record = mgp_result_new_record(result);
    if (record == nullptr) {
      mgp_result_set_error_msg(result, "Unable to make a new record");
    }
    int id_inserted = mgp_result_record_insert(record, "id", id_value);
    if (!id_inserted) {
      mgp_result_set_error_msg(result, "Unable to insert id");
    }
    int out_count_inserted =
        mgp_result_record_insert(record, "out_count", out_count_value);
    if (!out_count_inserted) {
      mgp_result_set_error_msg(result, "Unable to insert out count value");
    }
  }

  return;
}

extern "C" int mgp_init_module(struct mgp_module *module,
                               struct mgp_memory *memory) {
  struct mgp_proc *proc =
      mgp_module_add_read_procedure(module, "run", procedure);
  if (!proc) return 1;
  if (!mgp_proc_add_result(proc, "id", mgp_type_int())) return 1;
  if (!mgp_proc_add_result(proc, "out_count", mgp_type_int())) return 1;
  return 0;
}

extern "C" int mgp_shutdown_module() {
  return 0;
}
