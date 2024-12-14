import requests
from flask import Flask, request, jsonify, render_template
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime

app = Flask(__name__)

# OpenAI API Key
OPENAI_API_KEY = "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"

# Configure SQLite database
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///chat_logs.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# Define database model for chat logs
class ChatLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    user_input = db.Column(db.Text, nullable=False)
    chatgpt_response = db.Column(db.Text, nullable=False)

# Initialize the database
with app.app_context():
    db.create_all()

# Root endpoint to display chat history
@app.route('/', methods=['GET'])
def homepage():
    try:
        logs = ChatLog.query.order_by(ChatLog.timestamp.desc()).all()
        if not logs:
            print("No logs found in the database.")
        return render_template('history.html', logs=logs)
    except Exception as e:
        print(f"Error retrieving chat history: {e}")
        return jsonify({"error": "Unable to retrieve chat history."}), 500

# Endpoint to send a prompt to ChatGPT and store the result
@app.route('/chat', methods=['POST'])
def chat():
    data = request.json
    print(f"Received request data: {data}")
    if not data or 'command' not in data:
        return jsonify({"response": "Invalid input. 'command' key is required."}), 400

    command = data['command']
    print(f"Command received: {command}")

    # API endpoint and headers
    url = "https://api.openai.com/v1/chat/completions"
    headers = {
        "Authorization": f"Bearer {OPENAI_API_KEY}",
        "Content-Type": "application/json"
    }
    payload = {
        "model": "gpt-3.5-turbo",
        "messages": [
            {"role": "system", "content": "You are a helpful assistant."},
            {"role": "user", "content": command}
        ],
        "max_tokens": 150
    }

    try:
        # Make request to ChatGPT
        response = requests.post(url, headers=headers, json=payload)
        response.raise_for_status()

        # Parse and return the response from ChatGPT
        chatgpt_response = response.json()
        gpt_reply = chatgpt_response.get("choices")[0]["message"]["content"]
        print(f"GPT response: {gpt_reply}")

        # Save the chat log to the database
        new_log = ChatLog(user_input=command, chatgpt_response=gpt_reply)
        db.session.add(new_log)
        db.session.commit()
        print(f"Log saved: {new_log}")

        return jsonify({"response": gpt_reply})
    except requests.exceptions.RequestException as e:
        print(f"Error communicating with ChatGPT: {e}")
        return jsonify({"response": f"Error communicating with ChatGPT: {str(e)}"}), 500
    except Exception as e:
        print(f"Database save error: {e}")
        return jsonify({"response": "Database error"}), 500

# Endpoint to retrieve chat history as JSON
@app.route('/history', methods=['GET'])
def history():
    try:
        logs = ChatLog.query.order_by(ChatLog.timestamp.desc()).all()
        history = [
            {
                "timestamp": log.timestamp.strftime('%Y-%m-%d %H:%M:%S'),
                "user_input": log.user_input,
                "chatgpt_response": log.chatgpt_response
            }
            for log in logs
        ]
        return jsonify(history)
    except Exception as e:
        print(f"Error retrieving history: {e}")
        return jsonify({"error": "Unable to retrieve history."}), 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)

