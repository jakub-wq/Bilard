import matplotlib.pyplot as plt
from matplotlib.patches import Patch


tasks = [
    ("Analiza i Planowanie", 0, 2, "IT"),
    ("Zakres i specyfikacja", 1, 3, "IT"),
    ("Projekt architektury", 2, 5, "Programiści"),
    ("Model danych i UML", 3, 6, "Programiści"),
    ("Backend (logika + API)", 4, 8, "Programiści"),
    ("Frontend (UI/UX)", 5, 9, "Graficy"),
    ("Fizyka gry", 5, 8, "Programiści"),

    # TESTERZY — TERAZ CAŁY ROK
    ("Testy jednostkowe", 0, 12, "Testerzy"),
    ("Testy integracyjne", 0, 12, "Testerzy"),
    ("Testy systemowe", 0, 12, "Testerzy"),

    ("Wersja 1.0 (Personal)", 8, 9, "Marketing"),
    ("Wersja 1.5 (Professional)", 9, 10, "Marketing"),
    ("Wersja 2.0 (Enterprise)", 10, 11, "Marketing"),

    ("Konfiguracja środowiska", 8, 10, "Wdrożeniowcy"),
    ("Szkolenie użytkowników", 9, 11, "Wdrożeniowcy"),
    ("Rozruch systemu", 10, 12, "Wdrożeniowcy"),

    ("Serwis i utrzymanie", 11, 12, "IT"),
]

colors = {
    "Programiści": "green",
    "Graficy": "purple",
    "Testerzy": "orange",
    "Marketing": "blue",
    "Wdrożeniowcy": "red",
    "IT": "gray"
}

fig, ax = plt.subplots(figsize=(12, 8))

for i, (task, start, end, team) in enumerate(tasks):
    ax.barh(i, end - start, left=start, color=colors[team])

ax.set_yticks(range(len(tasks)))
ax.set_yticklabels([t[0] for t in tasks])

months = ["Sty", "Lut", "Mar", "Kwi", "Maj", "Cze",
          "Lip", "Sie", "Wrz", "Paź", "Lis", "Gru"]

ax.set_xticks(range(12))
ax.set_xticklabels(months)

ax.set_title("Harmonogram projektu Bilard 3D (rok)")
ax.invert_yaxis()
ax.grid(True)

legend_elements = [
    Patch(facecolor=colors["Programiści"], label="Programiści"),
    Patch(facecolor=colors["Graficy"], label="Graficy"),
    Patch(facecolor=colors["Testerzy"], label="Testerzy"),
    Patch(facecolor=colors["Marketing"], label="Marketing"),
    Patch(facecolor=colors["Wdrożeniowcy"], label="Wdrożeniowcy"),
    Patch(facecolor=colors["IT"], label="IT"),
]

ax.legend(handles=legend_elements, loc='lower center', ncol=3)

plt.tight_layout()
plt.show()