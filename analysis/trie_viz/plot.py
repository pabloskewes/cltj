"""Draw a metatrie as a tree using networkx + matplotlib."""

from pathlib import Path

import matplotlib.pyplot as plt
import networkx as nx
import pandas as pd


def _build_graph(df: pd.DataFrame) -> nx.DiGraph:
    G = nx.DiGraph()
    for idx, row in df.iterrows():
        key = int(row["key"])
        depth = int(row["depth"])
        n_children = int(row["n_children"])
        is_leaf = bool(row.get("is_leaf", False))
        label = "root" if row["parent"] == -1 else str(key)
        G.add_node(idx, label=label, depth=depth, n_children=n_children, is_leaf=is_leaf)
        if row["parent"] != -1:
            G.add_edge(int(row["parent"]), idx)
    return G


def _hierarchy_pos(
    G: nx.DiGraph,
    root: int,
    width: float = 1.0,
    y_gap: float = 1.0,
) -> dict[int, tuple[float, float]]:
    """Compute positions for a tree layout (root at top)."""
    pos: dict[int, tuple[float, float]] = {}

    def _place(node, left, right, depth):
        children = list(G.successors(node))
        pos[node] = ((left + right) / 2.0, -depth * y_gap)
        if not children:
            return
        slot = (right - left) / len(children)
        for i, child in enumerate(children):
            _place(child, left + i * slot, left + (i + 1) * slot, depth + 1)

    _place(root, 0.0, width, 0)
    return pos


def plot_trie(
    df: pd.DataFrame,
    title: str = "",
    savepath: str | Path | None = None,
    figsize: tuple[float, float] | None = None,
) -> plt.Figure:
    """Draw a single trie as a top-down tree."""
    G = _build_graph(df)
    root = int(df.index[df["parent"] == -1][0])
    n_nodes = len(G)

    if figsize is None:
        figsize = (max(6, n_nodes * 0.6), max(4, df["depth"].max() * 2 + 1))

    pos = _hierarchy_pos(G, root, width=n_nodes * 0.8)
    labels = nx.get_node_attributes(G, "label")
    is_leaf = nx.get_node_attributes(G, "is_leaf")

    internal_nodes = [n for n, leaf in is_leaf.items() if not leaf]
    leaf_nodes = [n for n, leaf in is_leaf.items() if leaf]

    fig, ax = plt.subplots(figsize=figsize)
    nx.draw_networkx_edges(G, pos, ax=ax, edge_color="#888", arrows=False)
    nx.draw_networkx_nodes(G, pos, nodelist=internal_nodes, ax=ax,
                           node_color="#a8d8ea", edgecolors="#333", node_size=600)
    nx.draw_networkx_nodes(G, pos, nodelist=leaf_nodes, ax=ax,
                           node_color="#d4edda", edgecolors="#555",
                           node_size=400, node_shape="s")
    nx.draw_networkx_labels(G, pos, labels=labels, ax=ax, font_size=8)
    if title:
        ax.set_title(title)
    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig


def plot_all_tries(
    trie_dumps: dict[str, pd.DataFrame],
    savedir: str | Path | None = None,
) -> list[plt.Figure]:
    """Plot each trie and optionally save to *savedir*."""
    if savedir:
        savedir = Path(savedir)
        savedir.mkdir(parents=True, exist_ok=True)
    figs = []
    for name, df in trie_dumps.items():
        sp = (savedir / f"{name}.png") if savedir else None
        fig = plot_trie(df, title=f"Trie {name}", savepath=sp)
        figs.append(fig)
    return figs
