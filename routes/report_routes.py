from flask import Blueprint, render_template
import database
import c_bridge

reports_bp = Blueprint('reports', __name__)

@reports_bp.route('/reports')
def fraud_report():
    stats = c_bridge.run_c_engine('stats')
    config = c_bridge.run_c_engine('config_get')

    conn = database.get_db_connection()
    cursor = conn.cursor()

    # Rule 1: Large Transactions (>= threshold)
    large_amt_thresh = config.get('large_amount_threshold', 10000.0) if isinstance(config, dict) else 10000.0
    cursor.execute("""
    SELECT * FROM transactions WHERE amount >= ? ORDER BY amount DESC
    """, (large_amt_thresh,))
    large_transactions = [dict(r) for r in cursor.fetchall()]

    # Rule 2: High Activity Accounts (>= threshold)
    high_act_thresh = config.get('high_activity_threshold', 5) if isinstance(config, dict) else 5
    cursor.execute("""
    SELECT sender AS account, COUNT(*) AS tx_count, SUM(amount) AS total_amount,
           AVG(amount) AS avg_amount, MAX(risk_score) AS max_risk
    FROM transactions
    GROUP BY sender
    HAVING COUNT(*) >= ?
    ORDER BY COUNT(*) DESC
    """, (high_act_thresh,))
    high_activity_accounts = [dict(r) for r in cursor.fetchall()]

    # All Flagged Transactions
    cursor.execute("""
    SELECT * FROM transactions WHERE is_flagged = 1 ORDER BY risk_score DESC, amount DESC
    """)
    flagged_transactions = [dict(r) for r in cursor.fetchall()]

    # High Risk transactions
    cursor.execute("""
    SELECT * FROM transactions WHERE risk_level = 'HIGH' ORDER BY amount DESC
    """)
    high_risk_transactions = [dict(r) for r in cursor.fetchall()]

    # Categories breakdown
    cursor.execute("""
    SELECT category, COUNT(*) as count, SUM(amount) as total_vol,
           SUM(CASE WHEN is_flagged = 1 THEN 1 ELSE 0 END) as flagged_count
    FROM transactions
    GROUP BY category
    ORDER BY total_vol DESC
    """)
    category_breakdown = [dict(r) for r in cursor.fetchall()]

    conn.close()

    # Detected cycles from C engine
    cycles = c_bridge.run_c_engine('graph_cycles')

    return render_template(
        'reports/index.html',
        stats=stats,
        config=config,
        large_transactions=large_transactions,
        high_activity_accounts=high_activity_accounts,
        flagged_transactions=flagged_transactions,
        high_risk_transactions=high_risk_transactions,
        category_breakdown=category_breakdown,
        cycles=cycles if isinstance(cycles, list) else []
    )
