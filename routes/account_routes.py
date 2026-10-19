from flask import Blueprint, render_template, request, jsonify, abort
import database
import c_bridge

accounts_bp = Blueprint('accounts', __name__)

@accounts_bp.route('/accounts')
def list_accounts():
    search = request.args.get('search', '').strip()
    risk_filter = request.args.get('risk', '').strip().upper()

    conn = database.get_db_connection()
    cursor = conn.cursor()

    conditions = []
    params = []
    if search:
        conditions.append("account_name LIKE ?")
        params.append(f"%{search}%")
    if risk_filter and risk_filter != "ALL":
        conditions.append("risk_level = ?")
        params.append(risk_filter)

    where = " WHERE " + " AND ".join(conditions) if conditions else ""
    query = f"""
    SELECT * FROM accounts
    {where}
    ORDER BY total_sent + total_received DESC, transaction_count DESC
    """
    cursor.execute(query, params)
    accounts = [dict(r) for r in cursor.fetchall()]
    conn.close()

    return render_template(
        'accounts/index.html',
        accounts=accounts,
        search=search,
        risk_filter=risk_filter
    )

@accounts_bp.route('/accounts/<account_name>')
def account_detail(account_name: str):
    conn = database.get_db_connection()
    cursor = conn.cursor()

    cursor.execute("SELECT * FROM accounts WHERE account_name = ?", (account_name,))
    account = cursor.fetchone()

    if not account:
        conn.close()
        abort(404, description="Account not found")

    account = dict(account)

    # Get outgoing transactions
    cursor.execute("""
    SELECT * FROM transactions WHERE sender = ? ORDER BY id DESC
    """, (account_name,))
    outgoing = [dict(r) for r in cursor.fetchall()]

    # Get incoming transactions
    cursor.execute("""
    SELECT * FROM transactions WHERE receiver = ? ORDER BY id DESC
    """, (account_name,))
    incoming = [dict(r) for r in cursor.fetchall()]

    # Counterparties
    counterparties = {}
    for tx in outgoing:
        target = tx['receiver']
        if target not in counterparties:
            counterparties[target] = {'sent_to': 0.0, 'recv_from': 0.0, 'count': 0}
        counterparties[target]['sent_to'] += tx['amount']
        counterparties[target]['count'] += 1

    for tx in incoming:
        src = tx['sender']
        if src not in counterparties:
            counterparties[src] = {'sent_to': 0.0, 'recv_from': 0.0, 'count': 0}
        counterparties[src]['recv_from'] += tx['amount']
        counterparties[src]['count'] += 1

    conn.close()

    return render_template(
        'accounts/detail.html',
        account=account,
        outgoing=outgoing,
        incoming=incoming,
        counterparties=counterparties
    )
