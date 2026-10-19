from flask import Blueprint, render_template, request, jsonify, redirect, url_for, flash
import database
import c_bridge

review_bp = Blueprint('review', __name__)

@review_bp.route('/review')
def review_queue():
    tab = request.args.get('tab', 'PENDING').upper()
    valid_tabs = {'PENDING', 'INVESTIGATING', 'CLEARED', 'BLOCKED', 'ALL'}
    if tab not in valid_tabs:
        tab = 'PENDING'

    conn = database.get_db_connection()
    cursor = conn.cursor()

    # Get counts for badge headers
    cursor.execute("SELECT review_status, COUNT(*) FROM transactions GROUP BY review_status")
    status_counts = {r[0]: r[1] for r in cursor.fetchall()}

    cursor.execute("SELECT COUNT(*) FROM transactions WHERE is_flagged = 1")
    total_flagged = cursor.fetchone()[0]

    # Query items for current tab
    if tab == 'ALL':
        cursor.execute("SELECT * FROM transactions WHERE is_flagged = 1 ORDER BY risk_score DESC, id DESC")
    else:
        cursor.execute("SELECT * FROM transactions WHERE review_status = ? ORDER BY risk_score DESC, id DESC", (tab,))

    queue_items = [dict(r) for r in cursor.fetchall()]

    # Fetch audit stack from C engine
    audit_stack = c_bridge.run_c_engine('stack')

    conn.close()

    return render_template(
        'review/index.html',
        items=queue_items,
        current_tab=tab,
        counts=status_counts,
        total_flagged=total_flagged,
        audit_stack=audit_stack
    )

@review_bp.route('/review/<int:tx_id>/action', methods=['POST'])
def review_action(tx_id: int):
    new_status = request.form.get('status', '').upper()
    notes = request.form.get('notes', '').strip()

    valid_statuses = {'PENDING', 'INVESTIGATING', 'CLEARED', 'BLOCKED'}
    if new_status not in valid_statuses:
        flash("Invalid review status action.", "danger")
        return redirect(url_for('review.review_queue'))

    # Update in C engine (pushes to LIFO stack)
    res = c_bridge.run_c_engine('update', str(tx_id), new_status, notes)

    if res.get('status') == 'success':
        database.sync_from_c_engine()
        flash(f"Transaction #{tx_id} status updated to {new_status}. Action recorded on LIFO audit stack.", "success")
    else:
        flash(f"Failed to update status: {res.get('message')}", "danger")

    return redirect(url_for('review.review_queue', tab=new_status))

@review_bp.route('/review/undo', methods=['POST'])
def undo_action():
    res = c_bridge.run_c_engine('undo')
    if res.get('status') == 'success':
        database.sync_from_c_engine()
        action = res.get('action', {})
        flash(f"Successfully undone! Reverted transaction #{action.get('transaction_id')} back to {action.get('reverted_to')}.", "info")
    else:
        flash(res.get('message', "Nothing to undo."), "warning")

    return redirect(request.referrer or url_for('review.review_queue'))
