from flask import Blueprint, redirect, url_for, flash, request
import c_bridge
import database

seed_bp = Blueprint('seed', __name__)

@seed_bp.route('/seed/reset', methods=['POST'])
def reset_seed():
    res = c_bridge.run_c_engine('seed')
    if res.get('status') == 'success':
        database.sync_from_c_engine()
        flash(f"System reset: {res.get('count', 22)} sample banking records re-seeded with realistic fraud patterns.", "success")
    else:
        flash(f"Reset failed: {res.get('message')}", "danger")

    return redirect(request.referrer or url_for('dashboard.dashboard_view'))
