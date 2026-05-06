import matplotlib.pyplot as plt
from matplotlib.patches import Patch


tasks = [
    ("Analiza i Planowanie", 0, 2, "IT"),

    # Usunięto: Zakres i specyfikacja

    ("Projekt architektury", 2, 5, "Programiści"),
    ("Model danych i UML", 3, 6, "Programiści"),
    ("Backend (logika + API)", 4, 8, "Programiści"),
    ("Frontend (UI/UX)", 5, 9, "Graficy"),
    ("Fizyka gry", 5, 8, "Programiści"),

    # Testy skrócone do marca
    ("Testy jednostkowe", 2, 12, "Testerzy"),
    ("Testy integracyjne", 2, 12, "Testerzy"),
    ("Testy systemowe", 2, 12, "Testerzy"),

    # Wersja 1.0 przesunięta na ostatni kafelek, czyli grudzień
    ("Wersja 1.0 (Personal)", 11, 12, "Marketing"),

    # Usunięto: Wersja 1.5 i Wersja 2.0

    ("Konfiguracja środowiska", 8, 10, "Wdrożeniowcy"),

    # Usunięto: Szkolenie użytkowników

    ("Rozruch systemu", 10, 12, "Wdrożeniowcy"),

    # Serwis zaczyna się tam, gdzie Frontend i Fizyka gry, czyli w czerwcu
    ("Serwis i utrzymanie", 5, 12, "IT"),
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

months = [
    "Sty", "Lut", "Mar", "Kwi", "Maj", "Cze",
    "Lip", "Sie", "Wrz", "Paź", "Lis", "Gru"
]

ax.set_xticks(range(12))
ax.set_xticklabels(months)

ax.set_xlim(0, 12)

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

ax.legend(handles=legend_elements, loc="lower center", ncol=3)

plt.tight_layout()
plt.show()