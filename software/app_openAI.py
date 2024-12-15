import requests
from flask import Flask, request, jsonify

app = Flask(__name__)

# Put the OpenAPI key here (change this out often if something weird is wrong) 
OPENAI_API_KEY = "sk-proj-7LqoUm2CnnwL-wY5vTC4qveytapIby4E7z2WdVevQyS9kTolglVoB0TqnWzosSnout4ZcXyuIxT3BlbkFJn_e2ZOdRKiButtjxbu6BD1u151OBaPawJS0TU0Zpc0NJ6-kL7P3-DKJR0THpICpUR2>

@app.route('/chat', methods=['POST'])
def chat():
    data = request.json
    if not data or 'command' not in data:
        return jsonify({"response": "Invalid input. 'command' key is required."}), 400
    
    command = data['command']

    # API endpoint and headers
    url = "https://api.openai.com/v1/chat/completions"
    headers = {
        "Authorization": f"Bearer {OPENAI_API_KEY}",
        "Content-Type": "application/json"
    }
    payload = {
        "model": "gpt-3.5-turbo",  # try out gpt-4 too
        "messages": [
            {"role": "system", "content": "You are a helpful assistant."},
            {"role": "user", "content": command}
        ],
        "max_tokens": 150
    }

    try:
        # Make reqest 
        response = requests.post(url, headers=headers, json=payload)
        response.raise_for_status()  # Raise an error for non-2xx responses
        
        # Parse and return the response from ChatGPT
        chatgpt_response = response.json()
        return jsonify({"response": chatgpt_response.get("choices")[0]["message"]["content"]})
    except requests.exceptions.RequestException as e:
        # Log the error and return it in the response
        print(f"Error communicating with ChatGPT: {e}")
        return jsonify({"response": f"Error communicating with ChatGPT: {str(e)}"}), 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
