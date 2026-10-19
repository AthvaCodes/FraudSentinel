from flask import Blueprint, render_template, jsonify
import database
import c_bridge

dashboard_bp = Blueprint('dashboard', __name__)

@dashboard_bp.route('/')
@dashboard_bp.route('/dashboard')
def dashboard_view():
    stats = c_bridge.run_c_engine('stats')
    if not isinstance(stats, dict) or 'total_transactions' not in stats:
        stats = {
            'total_transactions': 0, 'total_amount': 0.0, 'average_amount': 0.0,
            'largest_amount': 0.0, 'smallest_amount': 0.0, 'flagged_count': 0,
            'high_risk_count': 0, 'medium_risk_count': 0, 'low_risk_count': 0,
            'pending_reviews': 0, 'cleared_reviews': 0, 'blocked_reviews': 0
        }

    # Fetch recent transactions
    conn = database.get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM transactions ORDER BY id DESC LIMIT 6")
    recent_transactions = [dict(r) for r in cursor.fetchall()]

    # High activity accounts
    cursor.execute("""
    SELECT sender AS account, COUNT(*) AS tx_count, SUM(amount) AS total_out,
           MAX(risk_score) AS max_risk
    FROM transactions
    GROUP BY sender
    HAVING COUNT(*) >= 3
    ORDER BY COUNT(*) DESC, total_out DESC
    LIMIT 5
    """)
    high_activity_accounts = [dict(r) for r in cursor.fetchall()]

    # Suspicious flagged transactions needing review
    cursor.execute("""
    SELECT * FROM transactions
    WHERE is_flagged = 1 OR risk_level = 'HIGH'
    ORDER BY risk_score DESC, amount DESC
    LIMIT 5
    """)
    flagged_alerts = [dict(r) for r in cursor.fetchall()]

    # Unique accounts count
    cursor.execute("SELECT COUNT(*) FROM accounts")
    account_count = cursor.fetchone()[0]

    conn.close()

    return render_template(
        'dashboard.html',
        stats=stats,
        recent_transactions=recent_transactions,
        high_activity_accounts=high_activity_accounts,
        flagged_alerts=flagged_alerts,
        account_count=account_count
    )

@dashboard_bp.route('/api/stats')
def api_stats():
    stats = c_bridge.run_c_engine('stats')
    return jsonify(stats)
