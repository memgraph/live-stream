import mgp


@mgp.read_proc
def procedure(context: mgp.ProcCtx,
              node: mgp.Nullable[mgp.Vertex],
              ) -> mgp.Record(node=mgp.Vertex):
    return mgp.Record(node=node)
