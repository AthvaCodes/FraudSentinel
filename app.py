"""Bank Fraud Transaction System - Web Application
Educational Prototype demonstrating Data Structures, Fraud Detection Rules, and Full-Stack Engineering.
"""
import os
from flask import Flask, render_template
import database
import c_bridge

from routes.dashboard_routes import dashboard_bp
from routes.transaction_routes import transactions_bp
from routes.account_routes import accounts_bp
from routes.network_routes import network_bp
from routes.review_routes import review_bp
from routes.report_routes import reports_bp
from routes.rules_routes import rules_bp
from routes.ds_lab_routes import ds_lab_bp
from routes.seed_routes import seed_bp


def create_app() -> Flask:
    app = Flask(__name__)
    app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY', 'bank-fraud-educational-demo-secret-key-2026')

    # Register blueprints
    app.register_blueprint(dashboard_bp)
    app.register_blueprint(transactions_bp)
    app.register_blueprint(accounts_bp)
    app.register_blueprint(network_bp)
    app.register_blueprint(review_bp)
    app.register_blueprint(reports_bp)
    app.register_blueprint(rules_bp)
    app.register_blueprint(ds_lab_bp)
    app.register_blueprint(seed_bp)

    @app.context_processor
    def inject_global_metadata():
        return {
            "app_title": "Bank Fraud Transaction System",
            "prototype_label": "Educational Prototype",
            "currency_symbol": "₹",
            "c_engine_status": "Compiled & Active (GCC 32-bit Native Engine)"
        }

    @app.errorhandler(404)
    def page_not_found(e):
        return render_template('base.html', not_found=True), 404

    @app.errorhandler(500)
    def internal_server_error(e):
        return render_template('base.html', server_error=True), 500

    # Ensure C engine is built and database initialized
    try:
        c_bridge.compile_c_engine()
        database.init_db()
    except Exception as ex:
        print(f"Warning during app init: {ex}")

    return app


app = create_app()

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    print(f"\n==================================================================")
    print(f" * Bank Fraud Detection System (Educational Prototype)")
    print(f" * Core C Engine: Active & Linked")
    print(f" * Starting Web Dashboard at: http://127.0.0.1:{port}")
    print(f"==================================================================\n")
    app.run(host='0.0.0.0', port=port, debug=True)
