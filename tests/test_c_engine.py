"""Automated tests for compiled C backend engine."""
import pytest
import os
import sys

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, BASE_DIR)
import c_bridge


def test_c_engine_compilation_and_init():
    assert c_bridge.compile_c_engine() is True
    res = c_bridge.run_c_engine("init")
    assert res.get("status") == "success"
    assert res.get("count", 0) > 0


def test_c_engine_statistics():
    stats = c_bridge.run_c_engine("stats")
    assert isinstance(stats, dict)
    assert stats.get("total_transactions", 0) > 0
    assert stats.get("total_amount", 0.0) > 0.0
    assert "flagged_count" in stats
    assert "high_risk_count" in stats


def test_c_engine_large_transaction_rule():
    # Adding an amount >= ₹10,000 should flag as Large Transaction (+2 points)
    res = c_bridge.run_c_engine("add", "TestSenderRule1", "TestReceiverRule1", "15000.0", "Transfer", "Rule1 Test")
    assert res.get("status") == "success"
    tx = res.get("transaction", {})
    assert tx.get("amount") == 15000.0
    assert tx.get("is_flagged") is True
    assert tx.get("risk_score") >= 2
    assert "Large transaction" in tx.get("flag_reason", "")


def test_c_engine_high_activity_rule():
    # Adding multiple transactions from same sender should trigger high activity rule
    sender = "HighActivityTester"
    for i in range(6):
        c_bridge.run_c_engine("add", sender, f"Recipient_{i}", "500.0", "Transfer", f"Payment {i}")

    # Check that later transactions from this sender have high activity flag
    txs = c_bridge.run_c_engine("list")
    sender_txs = [t for t in txs if t.get("sender") == sender]
    assert len(sender_txs) >= 6
    # 5th or 6th transaction should have triggered high activity rule (+2 points)
    assert any(t.get("risk_score") >= 2 for t in sender_txs)


def test_c_engine_graph_and_cycle_detection():
    # Graph data export
    graph = c_bridge.run_c_engine("graph")
    assert "nodes" in graph
    assert "edges" in graph
    assert len(graph["nodes"]) > 0

    # Cycle detection for smurfing rings
    cycles = c_bridge.run_c_engine("graph_cycles")
    assert isinstance(cycles, list)
    assert len(cycles) > 0
    # Seeded data contains Mule Account A -> B -> C -> A
    cycle_paths = [c.get("path") for c in cycles]
    assert any("Mule Account A" in p for p in cycle_paths)


def test_c_engine_data_structures():
    # Linked list
    ll = c_bridge.run_c_engine("linked_list")
    assert "total_nodes" in ll
    assert "nodes" in ll
    assert ll["total_nodes"] > 0

    # Queue
    q = c_bridge.run_c_engine("queue")
    assert "size" in q
    assert "items" in q

    # BST
    bst = c_bridge.run_c_engine("bst")
    assert "total_nodes" in bst
    assert "root" in bst

    # BST Range Query
    range_res = c_bridge.run_c_engine("bst_range", "1000", "50000")
    assert isinstance(range_res, list)

    # Hash Map
    hm = c_bridge.run_c_engine("hash_map")
    assert "num_buckets" in hm
    assert hm["num_buckets"] == 16
    assert "buckets" in hm

    # Bubble sort trace
    bs = c_bridge.run_c_engine("bubble_sort")
    assert "total_comparisons" in bs
    assert "steps" in bs

    # Binary search trace
    bsearch = c_bridge.run_c_engine("binary_search", "1500.0")
    assert "steps" in bsearch
    assert "total_comparisons" in bsearch
