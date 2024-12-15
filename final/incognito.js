const axios = require('axios');

// OpenAI API endpoint and key
const url = "https://api.openai.com/v1/chat/completions";
const apiKey = "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";

// Interact  with chatGPT, specify "persona", clarify authorizations
async function askChatGPT(prompt) {
    const data = {
        model: "gpt-3.5-turbo",
        messages: [
            { role: "system", content: "You are a friendly and comforting assistant with high moral standards." },
            { role: "user", content: prompt }
        ],
        max_tokens: 100
    };

    try {
        const response = await axios.post(url, data, {
            headers: {
                "Authorization": `Bearer ${apiKey}`,
                "Content-Type": "application/json"
            }
        });

        // Show the response
        console.log("ChatGPT Response:", response.data.choices[0].message.content);
    } catch (error) {
        console.error("Error:", error.response ? error.response.data : error.message);
    }
}

// confess here :D 
const prompt = process.argv.slice(2).join(" ") || "I have a deep dark secret. 5 years ago, I published my cousin's credit card information on Reddit because she said I looked tired. Should I apologize?";
askChatGPT(prompt);
