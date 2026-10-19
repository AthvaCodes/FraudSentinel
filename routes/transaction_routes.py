from flask import Blueprint, render_template, request, jsonify, redirect, url_for, flash, Response
import csv
import io
import database
import c_bridge

transactions_bp = Blueprint('transactions', __name__)

@transactions_bp.route('/transactions')
def list_transactions():
    search = request.args.get('search', '')
    risk_filter = request.args.get('risk', '')
    status_filter = request.args.get('status', '')
    min_amount_str = request.args.get('min_amount', '')
    max_amount_str = request.args.get('max_amount', '')
    sort_by = request.args.get('sort_by', 'id')
    sort_order = request.args.get('sort_order', 'desc')
    page = int(request.args.get('page', 1))
    page_size = int(request.args.get('page_size', 10))

    min_amount = float(min_amount_str) if min_amount_str else None
    max_amount = float(max_amount_str) if max_amount_str else None

    result = database.query_transactions(
        search=search,
        risk_filter=risk_filter,
        status_filter=status_filter,
        min_amount=min_amount,
        max_amount=max_amount,
        sort_by=sort_by,
        sort_order=sort_order,
        page=page,
        page_size=page_size
    )

    return render_template(
        'transactions/index.html',
        transactions=result['items'],
        total=result['total'],
        page=result['page'],
        page_size=result['page_size'],
        total_pages=result['total_pages'],
        search=search,
        risk_filter=risk_filter,
        status_filter=status_filter,
        min_amount=min_amount_str,
        max_amount=max_amount_str,
        sort_by=sort_by,
        sort_order=sort_order
    )

@transactions_bp.route('/transactions/new', methods=['POST'])
def create_transaction():
    sender = request.form.get('sender', '').strip()
    receiver = request.form.get('receiver', '').strip()
    amount_str = request.form.get('amount', '0').strip()
    category = request.form.get('category', 'Transfer').strip() or 'Transfer'
    notes = request.form.get('notes', '').strip()

    # Validation
    if not sender or not receiver:
        flash("Sender and receiver accounts are required.", "danger")
        return redirect(url_for('transactions.list_transactions'))

    if sender.lower() == receiver.lower():
        flash("Sender and receiver cannot be the same account.", "danger")
        return redirect(url_for('transactions.list_transactions'))

    try:
        amount = float(amount_str)
        if amount <= 0:
            flash("Transaction amount must be strictly greater than zero.", "danger")
            return redirect(url_for('transactions.list_transactions'))
    except ValueError:
        flash("Invalid transaction amount.", "danger")
        return redirect(url_for('transactions.list_transactions'))

    # Call C engine to execute transaction, evaluate fraud score, and push to linked list if flagged
    res = c_bridge.run_c_engine('add', sender, receiver, str(amount), category, notes)

    if res.get('status') == 'success':
        database.sync_from_c_engine()
        tx = res.get('transaction', {})
        if tx.get('is_flagged'):
            flash(f"Transaction #{tx.get('id')} processed! WARNING: Flagged as {tx.get('risk_level')} risk ({tx.get('flag_reason')}).", "warning")
        else:
            flash(f"Transaction #{tx.get('id')} of ₹{amount:,.2f} completed successfully.", "success")
    else:
        flash(f"Failed to process transaction: {res.get('message', 'Unknown error')}", "danger")

    return redirect(url_for('transactions.list_transactions'))

@transactions_bp.route('/transactions/<int:tx_id>')
def get_transaction(tx_id: int):
    conn = database.get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM transactions WHERE id = ?", (tx_id,))
    row = cursor.fetchone()
    conn.close()

    if not row:
        return jsonify({"status": "error", "message": "Transaction not found"}), 404
    return jsonify(dict(row))

@transactions_bp.route('/transactions/<int:tx_id>/edit', methods=['POST'])
def edit_transaction(tx_id: int):
    category = request.form.get('category', '').strip()
    status = request.form.get('review_status', '').strip()
    notes = request.form.get('analyst_notes', '').strip()

    res = c_bridge.run_c_engine('update', str(tx_id), status, notes)
    if res.get('status') == 'success':
        database.sync_from_c_engine()
        flash(f"Transaction #{tx_id} updated successfully.", "success")
    else:
        flash(f"Update failed: {res.get('message', 'Error')}", "danger")

    return redirect(request.referrer or url_for('transactions.list_transactions'))

@transactions_bp.route('/transactions/<int:tx_id>/delete', methods=['POST'])
def delete_transaction(tx_id: int):
    res = c_bridge.run_c_engine('delete', str(tx_id))
    if res.get('status') == 'success':
        database.sync_from_c_engine()
        flash(f"Transaction #{tx_id} deleted successfully.", "success")
    else:
        flash(f"Failed to delete transaction: {res.get('message')}", "danger")

    return redirect(request.referrer or url_for('transactions.list_transactions'))

@transactions_bp.route('/transactions/export/csv')
def export_csv():
    """Streams full transaction ledger as formatted CSV."""
    conn = database.get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM transactions ORDER BY id ASC")
    rows = cursor.fetchall()
    conn.close()

    output = io.StringIO()
    writer = csv.writer(output)
    writer.writerow([
        "Transaction ID", "Sender", "Receiver", "Amount (INR)",
        "Timestamp", "Category", "Risk Score", "Risk Level",
        "Flagged", "Flag Reason", "Review Status", "Analyst Notes"
    ])

    for r in rows:
        writer.writerow([
            r['id'], r['sender'], r['receiver'], f"{r['amount']:.2f}",
            r['timestamp'], r['category'], r['risk_score'], r['risk_level'],
            "YES" if r['is_flagged'] else "NO", r['flag_reason'] or "",
            r['review_status'], r['analyst_notes'] or ""
        ])

    output.seek(0)
    return Response(
        output.getvalue(),
        mimetype="text/csv",
        headers={"Content-Disposition": "attachment;filename=bank_transactions_ledger.csv"}
    )
