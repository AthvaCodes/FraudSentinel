"""SQLite Database persistence and synchronization layer.

Maintains SQLite as the durable source of truth while synchronizing
with the high-performance in-memory C engine data structures.
"""
import sqlite3
import os
from typing import List, Dict, Any, Optional
import c_bridge

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "data")
DB_PATH = os.path.join(DATA_DIR, "bank_fraud.db")


def get_db_connection() -> sqlite3.Connection:
    """Returns a connection to the SQLite database with row factory enabled."""
    os.makedirs(DATA_DIR, exist_ok=True)
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    """Initializes tables in SQLite database."""
    conn = get_db_connection()
    cursor = conn.cursor()

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS transactions (
        id INTEGER PRIMARY KEY,
        sender TEXT NOT NULL,
        receiver TEXT NOT NULL,
        amount REAL NOT NULL,
        timestamp TEXT NOT NULL,
        category TEXT DEFAULT 'Transfer',
        risk_score INTEGER DEFAULT 0,
        risk_level TEXT DEFAULT 'LOW',
        is_flagged INTEGER DEFAULT 0,
        flag_reason TEXT,
        review_status TEXT DEFAULT 'PENDING',
        analyst_notes TEXT
    )
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS accounts (
        account_name TEXT PRIMARY KEY,
        balance REAL DEFAULT 100000.0,
        total_sent REAL DEFAULT 0.0,
        total_received REAL DEFAULT 0.0,
        transaction_count INTEGER DEFAULT 0,
        flagged_count INTEGER DEFAULT 0,
        risk_level TEXT DEFAULT 'LOW',
        created_at TEXT DEFAULT CURRENT_TIMESTAMP
    )
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS audit_log (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        action_type TEXT NOT NULL,
        transaction_id INTEGER,
        old_status TEXT,
        new_status TEXT,
        details TEXT,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP
    )
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS rules_config (
        id INTEGER PRIMARY KEY,
        large_amount_threshold REAL DEFAULT 10000.0,
        high_activity_threshold INTEGER DEFAULT 5,
        large_amount_weight INTEGER DEFAULT 2,
        high_activity_weight INTEGER DEFAULT 2,
        cycle_weight INTEGER DEFAULT 3,
        high_risk_threshold INTEGER DEFAULT 4,
        medium_risk_threshold INTEGER DEFAULT 2
    )
    """)

    # Seed initial default rules config if empty
    cursor.execute("SELECT COUNT(*) FROM rules_config")
    if cursor.fetchone()[0] == 0:
        cursor.execute("""
        INSERT INTO rules_config (id, large_amount_threshold, high_activity_threshold,
                                  large_amount_weight, high_activity_weight, cycle_weight,
                                  high_risk_threshold, medium_risk_threshold)
        VALUES (1, 10000.0, 5, 2, 2, 3, 4, 2)
        """)

    conn.commit()
    conn.close()

    # Synchronize database with C engine's seeded or existing data
    sync_from_c_engine()


def sync_from_c_engine():
    """Fetches full transaction list from the C engine and updates SQLite."""
    tx_list = c_bridge.run_c_engine("list")
    if not isinstance(tx_list, list):
        return

    conn = get_db_connection()
    cursor = conn.cursor()

    for tx in tx_list:
        cursor.execute("""
        INSERT INTO transactions (id, sender, receiver, amount, timestamp, category,
                                  risk_score, risk_level, is_flagged, flag_reason,
                                  review_status, analyst_notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            sender=excluded.sender,
            receiver=excluded.receiver,
            amount=excluded.amount,
            timestamp=excluded.timestamp,
            category=excluded.category,
            risk_score=excluded.risk_score,
            risk_level=excluded.risk_level,
            is_flagged=excluded.is_flagged,
            flag_reason=excluded.flag_reason,
            review_status=excluded.review_status,
            analyst_notes=excluded.analyst_notes
        """, (
            tx.get("id"),
            tx.get("sender"),
            tx.get("receiver"),
            float(tx.get("amount", 0.0)),
            tx.get("timestamp"),
            tx.get("category", "Transfer"),
            int(tx.get("risk_score", 0)),
            tx.get("risk_level", "LOW"),
            1 if tx.get("is_flagged") else 0,
            tx.get("flag_reason", ""),
            tx.get("review_status", "PENDING"),
            tx.get("analyst_notes", "")
        ))

    # Update account aggregates
    cursor.execute("DELETE FROM accounts")
    cursor.execute("""
    WITH senders AS (
        SELECT sender AS account, SUM(amount) AS sent, COUNT(*) AS count_sent,
               SUM(CASE WHEN is_flagged = 1 THEN 1 ELSE 0 END) AS flagged_sent,
               MAX(risk_score) AS max_risk
        FROM transactions GROUP BY sender
    ),
    receivers AS (
        SELECT receiver AS account, SUM(amount) AS received, COUNT(*) AS count_received,
               SUM(CASE WHEN is_flagged = 1 THEN 1 ELSE 0 END) AS flagged_recv
        FROM transactions GROUP BY receiver
    ),
    all_accs AS (
        SELECT account FROM senders UNION SELECT account FROM receivers
    )
    INSERT INTO accounts (account_name, balance, total_sent, total_received, transaction_count, flagged_count, risk_level)
    SELECT
        a.account,
        (100000.0 + COALESCE(r.received, 0.0) - COALESCE(s.sent, 0.0)) AS balance,
        COALESCE(s.sent, 0.0) AS total_sent,
        COALESCE(r.received, 0.0) AS total_received,
        (COALESCE(s.count_sent, 0) + COALESCE(r.count_received, 0)) AS transaction_count,
        (COALESCE(s.flagged_sent, 0) + COALESCE(r.flagged_recv, 0)) AS flagged_count,
        CASE
            WHEN COALESCE(s.max_risk, 0) >= 4 OR COALESCE(s.flagged_sent, 0) >= 2 THEN 'HIGH'
            WHEN COALESCE(s.max_risk, 0) >= 2 OR COALESCE(s.flagged_sent, 0) >= 1 THEN 'MEDIUM'
            ELSE 'LOW'
        END AS risk_level
    FROM all_accs a
    LEFT JOIN senders s ON a.account = s.account
    LEFT JOIN receivers r ON a.account = r.account
    """)

    conn.commit()
    conn.close()


def query_transactions(search: str = "", risk_filter: str = "", status_filter: str = "",
                       min_amount: Optional[float] = None, max_amount: Optional[float] = None,
                       sort_by: str = "id", sort_order: str = "desc",
                       page: int = 1, page_size: int = 10) -> Dict[str, Any]:
    """Queries transactions from SQLite with full search, filter, sort, and pagination."""
    conn = get_db_connection()
    cursor = conn.cursor()

    conditions = []
    params = []

    if search:
        s = f"%{search.strip()}%"
        conditions.append("(sender LIKE ? OR receiver LIKE ? OR category LIKE ? OR CAST(id AS TEXT) LIKE ?)")
        params.extend([s, s, s, s])

    if risk_filter and risk_filter.upper() != "ALL":
        conditions.append("risk_level = ?")
        params.append(risk_filter.upper())

    if status_filter and status_filter.upper() != "ALL":
        conditions.append("review_status = ?")
        params.append(status_filter.upper())

    if min_amount is not None:
        conditions.append("amount >= ?")
        params.append(min_amount)

    if max_amount is not None:
        conditions.append("amount <= ?")
        params.append(max_amount)

    where_clause = " WHERE " + " AND ".join(conditions) if conditions else ""

    # Validate sort column
    valid_cols = {"id", "amount", "timestamp", "risk_score", "sender", "receiver", "review_status"}
    col = sort_by if sort_by in valid_cols else "id"
    order = "ASC" if sort_order.lower() == "asc" else "DESC"

    # Count total matching rows
    count_query = f"SELECT COUNT(*) FROM transactions {where_clause}"
    cursor.execute(count_query, params)
    total_count = cursor.fetchone()[0]

    # Paginated query
    offset = (page - 1) * page_size
    query = f"""
    SELECT * FROM transactions
    {where_clause}
    ORDER BY {col} {order}
    LIMIT ? OFFSET ?
    """
    cursor.execute(query, params + [page_size, offset])
    rows = [dict(row) for row in cursor.fetchall()]

    conn.close()

    total_pages = max(1, (total_count + page_size - 1) // page_size)

    return {
        "items": rows,
        "total": total_count,
        "page": page,
        "page_size": page_size,
        "total_pages": total_pages
    }
