import requests
from flask import Flask, request, jsonify, render_template
from flask_sqlalchemy import SQLAlchemy
from flask_socketio import SocketIO, emit
from datetime import datetime

app = Flask(__name__)

# OpenAI API key 
OPENAI_API_KEY = "sk-proj-7LqoUm2CnnwL-wY5vTC4qveytapIby4E7z2WdVevQyS9kTolglVoB0TqnWzosSnout4ZcXyuIxT3BlbkFJn_e2ZOdRKiButtjxbu6BD1u151OBaPawJS0TU0Zpc0NJ6-kL7P3-DKJR0>"

# SQLite database configuration - we're using a simple SQLite file to store chat logs and button presses
app.config["SQLALCHEMY_DATABASE_URI"] = "sqlite:///chat_logs.db"
app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False
db = SQLAlchemy(app)

# Initialize Flask-SocketIO for WebSocket connections
# simplify against more complex flask_socketIO library expected inputs
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="eventlet", logger=True, engineio_logger=True)
socketio.run(app, host="0.0.0.0", port=5000, debug=True)

# Database model to store chat logs for persistant storage :D
class ChatLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)  # Primary key
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)  # Time the message was logged
    user_input = db.Column(db.Text, nullable=False)  # What the user said
    chatgpt_response = db.Column(db.Text, nullable=False)  # ChatGPT's reply

# Database model to store button presses ("happy" or "angry" button clicks)
class ButtonPress(db.Model):
    id = db.Column(db.Integer, primary_key=True)  # Unique ID for each press
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)  # When the button was pressed
    button = db.Column(db.String(50), nullable=False)  # Which button was pressed

# Initialize the database - create tables if they don't already exist
with app.app_context():
    db.create_all()

# Root endpoint - Displays a history of chats and button presses (HTML-based UI)
@app.route("/", methods=["GET"])
def homepage():
    try:
        logs = ChatLog.query.order_by(ChatLog.timestamp.desc()).all()  # Retrieve chat logs
        presses = ButtonPress.query.order_by(ButtonPress.timestamp.asc()).all()  
        return render_template("history.html", logs=logs, presses=presses)  # Render the data to a webpage, template/history.html setup 
    except Exception as e:
        print(f"Error retrieving chat history or button presses: {e}")
        return jsonify({"error": "No data to retrieve"}), 500  

# Endpoint to log button presses (e.g., "happy" or "angry")
@app.route("/log_button", methods=["POST"])
def log_button():
    data = request.json  # Parse the JSON payload
    if not data or "button" not in data:
        return jsonify({"error": "Invalid input. 'button' key is required."}), 400  # error logging, make sure that it actually exists 

    button = data["button"]
    if button not in ["happy", "angry"]:  
        return jsonify({"error": "Invalid button value. Must be 'happy' or 'angry'."}), 400

    try:
        new_press = ButtonPress(button=button)  # new database entry
        db.session.add(new_press)  
        db.session.commit()  

        # "Emit" WebSocket event for real-time updates
        socketio.emit("update_chart", {"button": button}, broadcast=True)
        return jsonify({"message": "Button press logged."}), 201
    except Exception as e:
        print(f"Error logging button press: {e}")  
        return jsonify({"error": f"Error logging button press: {str(e)}"}), 500

# Endpoint - fetch button press data as JSON
@app.route("/button_stats", methods=["GET"])
def button_stats():
    try:
        presses = ButtonPress.query.order_by(ButtonPress.timestamp.asc()).all()  
        stats = [
            {
                "timestamp": press.timestamp.strftime("%Y-%m-%d %H:%M:%S"),
                "button": press.button,
            }
            for press in presses
        ]
        return jsonify(stats), 200  # Return stats in JSON format
    except Exception as e:
        print(f"Error retrieving button stats: {e}")
        return jsonify({"error": f"Error retrieving button stats: {str(e)}"}), 500

# Endpoint - send a user's prompt to ChatGPT and save the result in the database
@app.route("/chat", methods=["POST"])
def chat():
    data = request.json
    if not data or "command" not in data:  # Validate input
        return jsonify({"response": "Invalid input. 'command' key is required."}), 400

    command = data["command"]

    url = "https://api.openai.com/v1/chat/completions"  # ChatGPT API URL!!!
    headers = {
        "Authorization": f"Bearer {OPENAI_API_KEY}",  # Pass API key in the header
        "Content-Type": "application/json"
    }

    payload = {
        "model": "gpt-3.5-turbo",
        "messages": [
            {"role": "system", "content": "You are a helpful assistant."},
            {"role": "user", "content": command}
        ],
        "max_tokens": 150  # Limit the response length
    }

    try:
        response = requests.post(url, headers=headers, json=payload)  
        response.raise_for_status()

        chatgpt_response = response.json()
        gpt_reply = chatgpt_response.get("choices")[0]["message"]["content"]  

        # Save chat log to the database
        new_log = ChatLog(user_input=command, chatgpt_response=gpt_reply)
        db.session.add(new_log)
        db.session.commit()

        return jsonify({"response": gpt_reply})
    except requests.exceptions.RequestException as e:
        print(f"Error communicating with ChatGPT: {e}")
        return jsonify({"response": f"Error communicating with ChatGPT: {str(e)}"}), 500
    except Exception as e:
        print(f"Database save error: {e}")
        return jsonify({"response": "Database error"}), 500

# WebSocket event handler, client connection
@socketio.on("connect")
def handle_connect():
    print(f"Client connected: {request.sid}")  

@socketio.on("update_chart")
def handle_button_press(data):
    print(f"Received button press: {data}")  
    socketio.emit("update_chart", data, broadcast=True)  

@socketio.on("disconnect")
def handle_disconnect():
    print(f"Client disconnected: {request.sid}")  # Log when a client disconnects

@socketio.on("button_press")
def handle_button_press(data):
    print(f"Button press received: {data}")
    emit("update_chart", data, broadcast=True)  # Notify all clients of the new data

@socketio.on_error_default
def default_error_handler(e):
    print(f"WebSocket error: {e}")  

# Main entry point for the application
if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5000, debug=True)  # Start the server with WebSocket support
