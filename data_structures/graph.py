"""Adjacency List Graph for Account Money Flows and Fraud Ring Detection.

Role in Banking System:
- Models financial relationships between accounts where nodes are bank accounts
  and directed edges represent money transfers.
- Critical for discovering complex money laundering topologies:
  1. Mule networks (high in-degree fan-out or fan-in)
  2. Circular routing / smurfing cycles (A -> B -> C -> A)
  3. Shortest path analysis between high-risk blacklisted entities and legitimate accounts.
- Tradeoff vs SQLite: Graph algorithms (cycle detection, BFS/DFS) are highly performant
  in-memory, whereas multi-hop graph queries in relational SQL require expensive recursive CTEs.
"""
from typing import Dict, List, Optional, Set, Tuple, Any
from collections import deque


class GraphEdge:
    """Represents a directed money transfer edge from sender to receiver."""
    def __init__(self, sender: str, receiver: str, amount: float, transaction_id: Optional[int] = None, timestamp: Optional[str] = None):
        self.sender = sender
        self.receiver = receiver
        self.amount = float(amount)
        self.transaction_id = transaction_id
        self.timestamp = timestamp

    def to_dict(self) -> Dict[str, Any]:
        return {
            "sender": self.sender,
            "receiver": self.receiver,
            "amount": self.amount,
            "transaction_id": self.transaction_id,
            "timestamp": self.timestamp
        }


class GraphNode:
    """Represents an account node in the transfer network."""
    def __init__(self, account_id: str, label: Optional[str] = None):
        self.account_id = account_id
        self.label = label or account_id
        self.total_sent: float = 0.0
        self.total_received: float = 0.0
        self.risk_score: int = 0
        self.is_flagged: bool = False

    def to_dict(self) -> Dict[str, Any]:
        return {
            "account_id": self.account_id,
            "label": self.label,
            "total_sent": self.total_sent,
            "total_received": self.total_received,
            "risk_score": self.risk_score,
            "is_flagged": self.is_flagged
        }


class TransactionGraph:
    """Directed graph using adjacency lists for account transactions."""
    def __init__(self):
        self.nodes: Dict[str, GraphNode] = {}
        self.adjacency: Dict[str, List[GraphEdge]] = {}
        self.in_edges: Dict[str, List[GraphEdge]] = {}

    def add_node(self, account_id: str, label: Optional[str] = None) -> GraphNode:
        """Adds an account node if not present."""
        if account_id not in self.nodes:
            node = GraphNode(account_id, label)
            self.nodes[account_id] = node
            self.adjacency[account_id] = []
            self.in_edges[account_id] = []
        return self.nodes[account_id]

    def add_edge(self, sender: str, receiver: str, amount: float, transaction_id: Optional[int] = None, timestamp: Optional[str] = None) -> GraphEdge:
        """Adds a directed transaction edge from sender to receiver."""
        self.add_node(sender)
        self.add_node(receiver)

        edge = GraphEdge(sender, receiver, amount, transaction_id, timestamp)
        self.adjacency[sender].append(edge)
        self.in_edges[receiver].append(edge)

        # Update node aggregates
        self.nodes[sender].total_sent += float(amount)
        self.nodes[receiver].total_received += float(amount)
        return edge

    def out_degree(self, account_id: str) -> int:
        return len(self.adjacency.get(account_id, []))

    def in_degree(self, account_id: str) -> int:
        return len(self.in_edges.get(account_id, []))

    def get_counterparties(self, account_id: str) -> Dict[str, List[str]]:
        """Returns lists of outgoing and incoming counterparties for an account."""
        outgoing = list({edge.receiver for edge in self.adjacency.get(account_id, [])})
        incoming = list({edge.sender for edge in self.in_edges.get(account_id, [])})
        return {"outgoing": outgoing, "incoming": incoming}

    def bfs(self, start_account: str) -> List[Dict[str, Any]]:
        """Breadth-First Search traversal starting from an account.
        Returns visited order with discovery distance (hops) and parent.
        """
        if start_account not in self.nodes:
            return []

        visited: Set[str] = {start_account}
        queue = deque([(start_account, 0, None)])
        steps: List[Dict[str, Any]] = []

        while queue:
            current, distance, parent = queue.popleft()
            steps.append({
                "account": current,
                "hops": distance,
                "parent": parent,
                "out_degree": self.out_degree(current)
            })

            for edge in self.adjacency.get(current, []):
                neighbor = edge.receiver
                if neighbor not in visited:
                    visited.add(neighbor)
                    queue.append((neighbor, distance + 1, current))

        return steps

    def dfs(self, start_account: str) -> List[Dict[str, Any]]:
        """Depth-First Search traversal starting from an account."""
        if start_account not in self.nodes:
            return []

        visited: Set[str] = set()
        steps: List[Dict[str, Any]] = []

        def _dfs_util(current: str, depth: int, parent: Optional[str]):
            visited.add(current)
            steps.append({
                "account": current,
                "depth": depth,
                "parent": parent
            })
            for edge in self.adjacency.get(current, []):
                neighbor = edge.receiver
                if neighbor not in visited:
                    _dfs_util(neighbor, depth + 1, current)

        _dfs_util(start_account, 0, None)
        return steps

    def find_shortest_path(self, start_account: str, end_account: str) -> Optional[Dict[str, Any]]:
        """Finds the shortest transfer path (fewest hops) between two accounts using BFS."""
        if start_account not in self.nodes or end_account not in self.nodes:
            return None
        if start_account == end_account:
            return {"path": [start_account], "edges": [], "total_amount": 0.0, "hops": 0}

        queue = deque([(start_account, [start_account], [])])
        visited: Set[str] = {start_account}

        while queue:
            current, path, edge_list = queue.popleft()

            if current == end_account:
                total_amount = sum(e["amount"] for e in edge_list)
                return {
                    "path": path,
                    "edges": edge_list,
                    "total_amount": total_amount,
                    "hops": len(path) - 1
                }

            for edge in self.adjacency.get(current, []):
                neighbor = edge.receiver
                if neighbor not in visited:
                    visited.add(neighbor)
                    queue.append((neighbor, path + [neighbor], edge_list + [edge.to_dict()]))

        return None

    def detect_cycles(self, max_length: int = 6) -> List[Dict[str, Any]]:
        """Detects circular fund routing (e.g., A -> B -> C -> A) characteristic of smurfing / money laundering.
        Uses depth-limited DFS back-edge detection.
        """
        cycles: List[Dict[str, Any]] = []
        seen_cycles: Set[Tuple[str, ...]] = set()

        def _normalize_cycle(path: List[str]) -> Tuple[str, ...]:
            # Canonical cycle representation for deduplication
            cycle_nodes = path[:-1]
            min_idx = cycle_nodes.index(min(cycle_nodes))
            return tuple(cycle_nodes[min_idx:] + cycle_nodes[:min_idx])

        def _find_cycles_from(start_node: str, current_node: str, path: List[str], path_edges: List[GraphEdge]):
            if len(path) > max_length:
                return

            for edge in self.adjacency.get(current_node, []):
                nxt = edge.receiver
                if nxt == start_node and len(path) >= 2:
                    full_path = path + [nxt]
                    norm = _normalize_cycle(full_path)
                    if norm not in seen_cycles:
                        seen_cycles.add(norm)
                        cycle_edges = path_edges + [edge]
                        total_cycle_volume = sum(e.amount for e in cycle_edges)
                        cycles.append({
                            "path": full_path,
                            "length": len(path),
                            "total_amount": total_cycle_volume,
                            "edges": [e.to_dict() for e in cycle_edges]
                        })
                elif nxt not in path and len(path) < max_length:
                    _find_cycles_from(start_node, nxt, path + [nxt], path_edges + [edge])

        for node_id in self.nodes.keys():
            _find_cycles_from(node_id, node_id, [node_id], [])

        return cycles

    def to_network_data(self) -> Dict[str, Any]:
        """Exports graph data formatted for interactive canvas/SVG rendering."""
        node_list = []
        for account_id, node in self.nodes.items():
            in_deg = self.in_degree(account_id)
            out_deg = self.out_degree(account_id)
            node_list.append({
                "id": account_id,
                "label": node.label,
                "in_degree": in_deg,
                "out_degree": out_deg,
                "total_sent": node.total_sent,
                "total_received": node.total_received,
                "risk_score": node.risk_score,
                "is_flagged": node.is_flagged
            })

        edge_list = []
        for sender, edges in self.adjacency.items():
            for edge in edges:
                edge_list.append({
                    "source": edge.sender,
                    "target": edge.receiver,
                    "amount": edge.amount,
                    "transaction_id": edge.transaction_id,
                    "timestamp": edge.timestamp
                })

        return {
            "nodes": node_list,
            "edges": edge_list,
            "summary": {
                "total_accounts": len(self.nodes),
                "total_transactions": len(edge_list)
            }
        }
