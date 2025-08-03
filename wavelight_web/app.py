from flask import Flask, render_template, jsonify
import json
import math

app = Flask(__name__)

@app.route("/")
def index():
    with open("led_state.json", "r") as f:
        data = json.load(f)

    total = 400
    N_arc = 100  # 100 pour chaque demi-cercle
    N_line = total - 2 * N_arc

    R = 75  # rayon des arcs
    L = 250  # longueur entre les centres des deux arcs

    cx = 300
    cy = 200

    leds = []

    for i in range(total):
        # Calcul de la position des LEDs
        if i < N_arc:
            # Demi-cercle gauche
            theta = math.pi * (i / N_arc - 0.5)
            x = cx - L / 2 - R * math.cos(theta)
            y = cy + R * math.sin(theta)

        elif i < N_arc + N_line // 2:
            # Ligne droite bas → droite
            j = i - N_arc
            x = cx - L / 2 + j * (L / (N_line // 2))
            y = cy + R

        elif i < 2 * N_arc + N_line // 2:
            # Demi-cercle droite
            j = i - (N_arc + N_line // 2)
            theta = math.pi * (j / N_arc - 0.5)
            x = cx + L / 2 - R * math.cos(math.pi - theta)
            y = cy + R * math.sin(-theta)

        else:
            # Ligne droite haut → gauche
            j = i - (2 * N_arc + N_line // 2)
            x = cx + L / 2 - j * (L / (N_line // 2))
            y = cy - R

        # Bouclage pour gérer les trainées qui passent par la fin (ex: 390 à 10)
        color = None
        for t in data["trails"]:
            # Cas où la trainée ne traverse pas la fin
            if t["start"] <= i <= t["end"]:
                color = t
                break
            # Cas où la trainée traverse la fin (start > end)
            elif t["start"] > t["end"]:
                if i >= t["start"] or i <= t["end"]:
                    color = t
                    break
        
        if color:
            r, g, b_val = color["r"], color["g"], color["b"]
        else:
            r, g, b_val = 50, 50, 50

        leds.append({"x": round(x), "y": round(y), "r": r, "g": g, "b": b_val})

    return render_template("index.html", leds=leds)

@app.route("/led_data")
def led_data():
    with open("led_state.json", "r") as f:
        data = json.load(f)

    total = 400
    N_arc = 100
    N_line = total - 2 * N_arc

    R = 75
    L = 250

    cx = 300
    cy = 200

    leds = []

    for i in range(total):
        if i < N_arc:
            theta = math.pi * (i / N_arc - 0.5)
            x = cx - L / 2 - R * math.cos(theta)
            y = cy + R * math.sin(theta)

        elif i < N_arc + N_line // 2:
            j = i - N_arc
            x = cx - L / 2 + j * (L / (N_line // 2))
            y = cy + R

        elif i < 2 * N_arc + N_line // 2:
            j = i - (N_arc + N_line // 2)
            theta = math.pi * (j / N_arc - 0.5)
            x = cx + L / 2 - R * math.cos(math.pi - theta)
            y = cy + R * math.sin(-theta)

        else:
            j = i - (2 * N_arc + N_line // 2)
            x = cx + L / 2 - j * (L / (N_line // 2))
            y = cy - R

        color = next((t for t in data["trails"] if t["start"] <= i <= t["end"]), None)
        if color:
            r, g, b = color["r"], color["g"], color["b"]
        else:
            r, g, b = 50, 50, 50

        leds.append({"r": r, "g": g, "b": b})

    return jsonify({"leds": leds})

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0")
