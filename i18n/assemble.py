#!/usr/bin/env python3
"""Assemble translated string tables into C99 arrays.

Reads all_*.txt files, applies corrections, nullifies KEEP and FMT entries,
and outputs C99 static arrays for each language.
"""

import re
import sys
import os

# ---------------------------------------------------------------------------
# 1.  Parse a translation file
# ---------------------------------------------------------------------------

LINE_RE = re.compile(
    r'^\[(\w+)\]\s*=\s*"((?:[^"\\]|\\.)*)"\s*(?:#\s*KEEP)?\s*$'
)
KEEP_RE = re.compile(r'#\s*KEEP')


def parse_file(path):
    """Return (entries dict, keep set) from a translation file."""
    entries = {}   # key -> value
    keeps = set()
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n\r")
            # skip blank, comment, section header lines
            if not line or line.startswith("#") or line.startswith("##"):
                continue
            # Handle multi-line garbled entries (e.g. value split across lines with ???)
            m = LINE_RE.match(line)
            if not m:
                continue
            key = m.group(1)
            val = m.group(2)
            entries[key] = val
            if KEEP_RE.search(line):
                keeps.add(key)
    return entries, keeps


# ---------------------------------------------------------------------------
# 2.  Format-string keys: always NULL (fallback to English)
# ---------------------------------------------------------------------------

FMT_KEYS = {
    "STR_FMT_PARTICLE_N",
    "STR_FMT_TIME",
    "STR_FMT_ENERGY_ERR",
    "STR_FMT_PITCH",
    "STR_FMT_MU",
    "STR_FMT_BMAG",
    "STR_FMT_ENERGY_MEV",
    "STR_FMT_ENERGY_KEV",
    "STR_FMT_PRAD",
    "STR_FMT_DEDT",
    "STR_FMT_DEDT_MULT",
}

# ---------------------------------------------------------------------------
# 3.  Per-language corrections
# ---------------------------------------------------------------------------

CORRECTIONS = {
    "fr": {
        "STR_FIELD_MODEL": "Modèle de champ",
        "STR_SPECIES_CUSTOM": "Personnalisé",
        "STR_PITCH": "Pitch",
        "STR_RESUME": "Reprendre",
        "STR_SCALED": "Mis à l'échelle",
        "STR_AXES": "Axes",
        "STR_PLOTS": "Graphiques",
        "STR_CAM_FREE": "Libre (V)",
        "STR_LW_AXES": "Axes",
        "STR_COLOR_AXES": "Axes",
        "STR_HELP_TAB_KEYS": "Raccourcis",
        "STR_HELP_FOOTER": "Esc ou F1 pour fermer  |  Flèches ou Tab pour changer d'onglet  |  Défilez pour naviguer",
        "STR_PICKER_HELP": "Cliquez pour sélectionner, double-cliquez ou Ouvrir pour lire  |  Esc pour annuler  |  Entrée pour ouvrir",
        "STR_MODEL_GRAD_B": "Grad-B linéaire",
        "STR_MODEL_QUAD_GRAD": "Grad-B quadratique",
        "STR_MODEL_SINUSOIDAL": "Grad-B sinusoïdal",
        "STR_PARAM_AMPLITUDE": "Amplitude a",
    },
    "de": {
        "STR_SPECIES_CUSTOM": "Benutzerdefiniert",
        "STR_CHARGE": "Ladung",
        "STR_PITCH": "Pitch",
        "STR_AXES": "Achsen",
        "STR_PLOTS": "Diagramme",
        "STR_TOP": "Oben",
        "STR_CAM_FREE": "Frei (V)",
        "STR_LW_AXES": "Achsen",
        "STR_COLOR_AXES": "Achsen",
        "STR_CANCEL": "Abbrechen",
        "STR_ARMED": "BEREIT",
        "STR_HELP_TAB_KEYS": "Tasten",
        "STR_HELP_TAB_ABOUT": "Über",
        "STR_HELP_TAB_INTERFACE": "Oberfläche",
        "STR_HELP_FOOTER": "Esc oder F1 zum Schließen  |  Pfeiltasten oder Tab zum Wechseln  |  Scrollen zum Navigieren",
        "STR_PICKER_HELP": "Klicken zum Auswählen, Doppelklick oder Öffnen zum Abspielen  |  Esc zum Abbrechen  |  Enter zum Öffnen",
        "STR_MODEL_CIRCULAR": "Kreisförmig B (const |B|)",
        "STR_OPEN_RECORDING": "Aufnahme öffnen",
    },
    "es": {
        "STR_SPECIES_CUSTOM": "Personalizado",
        "STR_CHARGE": "Carga",
        "STR_PITCH": "Pitch",
        "STR_DISPLAY": "Visualización",
        "STR_TRAIL": "Estela",
        "STR_SCALED": "Escalado",
        "STR_AXES": "Ejes",
        "STR_PLOTS": "Gráficos",
        "STR_AUTOSCALE": "Autoescala",
        "STR_CAM_FREE": "Libre (V)",
        "STR_DARK_MODE": "Modo oscuro",
        "STR_RIGHT": "Derecha",
        "STR_COLORS": "Colores",
        "STR_LW_AXES": "Ejes",
        "STR_COLOR_AXES": "Ejes",
        "STR_HELP_TAB_KEYS": "Teclas",
        "STR_HELP_FOOTER": "Esc o F1 para cerrar  |  Flechas o Tab para cambiar pestaña  |  Desplazar para navegar",
        "STR_PICKER_HELP": "Clic para seleccionar, doble clic o Abrir para reproducir  |  Esc para cancelar  |  Enter para abrir",
        "STR_MODEL_GRAD_B": "Grad-B lineal",
        "STR_MODEL_QUAD_GRAD": "Grad-B cuadrático",
        "STR_MODEL_SINUSOIDAL": "Grad-B sinusoidal",
        "STR_OPEN": "Abrir",
    },
    "pt": {
        "STR_SPECIES_CUSTOM": "Personalizado",
        "STR_PITCH": "Pitch",
        "STR_AXES": "Eixos",
        "STR_PLOTS": "Gráficos",
        "STR_CAM_FREE": "Livre (V)",
        "STR_LW_AXES": "Eixos",
        "STR_COLOR_AXES": "Eixos",
        "STR_HELP_TAB_KEYS": "Teclas",
        "STR_HELP_FOOTER": "Esc ou F1 para fechar  |  Setas ou Tab para trocar aba  |  Role para navegar",
        "STR_PICKER_HELP": "Clique para selecionar, duplo clique ou Abrir para reproduzir  |  Esc para cancelar  |  Enter para abrir",
    },
    "it": {
        "STR_SPECIES_CUSTOM": "Personalizzato",
        "STR_PITCH": "Pitch",
        "STR_AXES": "Assi",
        "STR_PLOTS": "Grafici",
        "STR_CAM_FREE": "Libero (V)",
        "STR_LW_AXES": "Assi",
        "STR_COLOR_AXES": "Assi",
        "STR_HELP_TAB_KEYS": "Tasti",
        "STR_HELP_FOOTER": "Esc o F1 per chiudere  |  Frecce o Tab per cambiare scheda  |  Scorri per navigare",
        "STR_PICKER_HELP": "Clicca per selezionare, doppio clic o Apri per riprodurre  |  Esc per annullare  |  Invio per aprire",
    },
    "ru": {
        "STR_FIELD_MODEL": "Модель поля",
        "STR_SPECIES_CUSTOM": "Пользовательский",
        "STR_PITCH": "Питч",
        "STR_AXES": "Оси",
        "STR_PLOTS": "Графики",
        "STR_CAM_FREE": "Свободная (V)",
        "STR_LW_AXES": "Оси",
        "STR_COLOR_AXES": "Оси",
        "STR_HELP_TAB_KEYS": "Клавиши",
        "STR_HELP_FOOTER": "Esc или F1 — закрыть  |  Стрелки или Tab — сменить вкладку  |  Прокрутка — навигация",
        "STR_PICKER_HELP": "Щелкните для выбора, двойной щелчок или Открыть для воспроизведения  |  Esc — отмена  |  Enter — открыть",
    },
    "ja": {
        "STR_SPECIES_CUSTOM": "カスタム",
        "STR_PITCH": "ピッチ",
        "STR_HELP_FOOTER": "Esc または F1 で閉じる  |  矢印キーまたは Tab でタブ切替  |  スクロールで移動",
        "STR_PICKER_HELP": "クリックで選択、ダブルクリックまたは「開く」で再生  |  Esc でキャンセル  |  Enter で開く",
    },
    "zh-CN": {
        "STR_SPECIES_CUSTOM": "自定义",
        "STR_PITCH": "螺旋角",
        "STR_HELP_FOOTER": "Esc 或 F1 关闭  |  方向键或 Tab 切换标签  |  滚动浏览",
        "STR_PICKER_HELP": "单击选择，双击或「打开」播放  |  Esc 取消  |  Enter 打开",
    },
    "zh-TW": {
        "STR_SPECIES_CUSTOM": "自訂",
        "STR_PITCH": "螺旋角",
        "STR_HELP_FOOTER": "Esc 或 F1 關閉  |  方向鍵或 Tab 切換標籤  |  捲動瀏覽",
        "STR_PICKER_HELP": "按一下選取，按兩下或「開啟」播放  |  Esc 取消  |  Enter 開啟",
    },
    "ko": {
        "STR_SPECIES_CUSTOM": "사용자 정의",
        "STR_PITCH": "피치",
        "STR_HELP_FOOTER": "Esc 또는 F1로 닫기  |  화살표 또는 Tab으로 탭 전환  |  스크롤로 탐색",
        "STR_PICKER_HELP": "클릭하여 선택, 더블클릭 또는 열기로 재생  |  Esc 취소  |  Enter 열기",
    },
    "eo": {
        "STR_SPECIES_CUSTOM": "Propra",
        "STR_PITCH": "Klinangulo",
        "STR_AXES": "Aksoj",
        "STR_PLOTS": "Grafikoj",
        "STR_CAM_FREE": "Libera (V)",
        "STR_LW_AXES": "Aksoj",
        "STR_COLOR_AXES": "Aksoj",
        "STR_HELP_FOOTER": "Esc aŭ F1 por fermi  |  Sagoklavoj aŭ Tab por ŝanĝi langeton  |  Rulumu por navigi",
        "STR_PICKER_HELP": "Klaku por elekti, duoble klaku aŭ Malfermi por ludi  |  Esc por nuligi  |  Enter por malfermi",
    },
}

# ---------------------------------------------------------------------------
# 4.  Language files
# ---------------------------------------------------------------------------

LANGUAGES = [
    ("fr",    "all_fr.txt"),
    ("de",    "all_de.txt"),
    ("es",    "all_es.txt"),
    ("pt",    "all_pt.txt"),
    ("it",    "all_it.txt"),
    ("ru",    "all_ru.txt"),
    ("ja",    "all_ja.txt"),
    ("zh_CN", "all_zh-CN.txt"),
    ("zh_TW", "all_zh-TW.txt"),
    ("ko",    "all_ko.txt"),
    ("eo",    "all_eo.txt"),
]

# ---------------------------------------------------------------------------
# 5.  Escape a string for C
# ---------------------------------------------------------------------------

def c_escape(s):
    """Escape backslashes and double-quotes for a C string literal."""
    s = s.replace("\\", "\\\\")
    s = s.replace('"', '\\"')
    return s


# ---------------------------------------------------------------------------
# 6.  Main
# ---------------------------------------------------------------------------

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    out_lines = []
    out_lines.append("/* Auto-generated by assemble.py — do not edit by hand. */")
    out_lines.append("")

    for lang_id, filename in LANGUAGES:
        path = os.path.join(script_dir, filename)
        entries, keeps = parse_file(path)

        # Determine the corrections key (use hyphen form for zh variants)
        corr_key = lang_id.replace("_", "-")
        corrections = CORRECTIONS.get(corr_key, {})

        # Apply corrections
        for k, v in corrections.items():
            entries[k] = v

        # Build the output entries: skip KEEP, skip FMT_*, skip NULL
        output_entries = []
        for key, val in entries.items():
            # FMT keys -> NULL (skip)
            if key in FMT_KEYS:
                continue
            # KEEP keys -> NULL (skip)
            if key in keeps:
                continue
            output_entries.append((key, val))

        # Emit the array
        out_lines.append(
            f"static const char *g_strings_{lang_id}[STR__COUNT] = {{"
        )
        for key, val in output_entries:
            out_lines.append(f'    [{key}] = "{c_escape(val)}",')
        out_lines.append("};")
        out_lines.append("")

    print("\n".join(out_lines))


if __name__ == "__main__":
    main()
