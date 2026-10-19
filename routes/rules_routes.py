from flask import Blueprint, render_template, request, redirect, url_for, flash, jsonify
import c_bridge
import database

rules_bp = Blueprint('rules', __name__)

@rules_bp.route('/rules')
def rules_config_view():
    config = c_bridge.run_c_engine('config_get')
    if not isinstance(config, dict):
        config = {
            'large_amount_threshold': 10000.0,
            'high_activity_threshold': 5,
            'large_amount_weight': 2,
            'high_activity_weight': 2,
            'cycle_weight': 3,
            'high_risk_threshold': 4,
            'medium_risk_threshold': 2
        }
    return render_template('rules/index.html', config=config)

@rules_bp.route('/rules/update', methods=['POST'])
def update_rules():
    try:
        lg_amt = float(request.form.get('large_amount_threshold', 10000.0))
        hi_act = int(request.form.get('high_activity_threshold', 5))
        lg_wt = int(request.form.get('large_amount_weight', 2))
        hi_wt = int(request.form.get('high_activity_weight', 2))
        cycle_wt = int(request.form.get('cycle_weight', 3))
        hi_cut = int(request.form.get('high_risk_threshold', 4))
        med_cut = int(request.form.get('medium_risk_threshold', 2))

        # Call C engine to update config and re-evaluate all transactions in memory & file
        res = c_bridge.run_c_engine(
            'config_set',
            str(lg_amt), str(hi_act), str(lg_wt),
            str(hi_wt), str(cycle_wt), str(hi_cut), str(med_cut)
        )

        if res.get('status') == 'success':
            database.sync_from_c_engine()
            flash("Fraud rules updated and all transactions successfully re-evaluated!", "success")
        else:
            flash(f"Failed to update rules: {res.get('message')}", "danger")

    except ValueError as e:
        flash(f"Invalid input values: {str(e)}", "danger")

    return redirect(url_for('rules.rules_config_view'))
