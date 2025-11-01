#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <map>

#include "fileprocessor.hpp" // scan directory ....
#include "uicontrol.hpp"

using namespace ftxui;

std::vector<std::string> left_files = {};
std::vector<std::string> right_files = {};
std::string current_dir = std::filesystem::current_path();

std::string left_panel_path = current_dir;
std::string right_panel_path = "right panel path";

// Die Hilfsfunktion, die die Umwandlung vornimmt
std::vector<std::string>
fileInfoToMenuStrings(const std::vector<FileInfo> &infos) {
  std::vector<std::string> strings;
  for (const auto &info : infos) {
    // Nutzen Sie die getDisplayName-Methode, die nur den Dateinamen liefert!
    strings.push_back(info.getDisplayName());
  }
  return strings;
}

// std::vector<std::string> left_panel_files = {};
// std::vector<std::string> right_panel_files = {};

int left_selected = 0;
int right_selected = 0;
int top_menu_selected = 0;
std::string current_status = "Bereit. Tab zum Wechseln des Panels.";

// Größe des linken Panels (für ResizableSplit)
int left_panel_size = 100;

// ----------------------------------------------------------------------
// Hauptprogramm
// ----------------------------------------------------------------------

int main() {
  FileProcessor fp(current_dir);

  // 1. Logik-Teil: Daten von der Logik-Klasse holen
  std::vector<FileInfo> left_file_infos = fp.scanDirectory();

  // 2. UI-Teil: Daten für FTXUI konvertieren
  std::vector<std::string> left_panel_files =
      fileInfoToMenuStrings(left_file_infos);

  std::vector<std::string> right_panel_files = {};

  /*for (const auto &entry : std::filesystem::directory_iterator(left_path)) {
    left_files.push_back(entry.path());
  }*/

  auto screen = ScreenInteractive::TerminalOutput();

  // -- HILFSFUNKTION --
  auto get_action_id_by_index = [&](int index) -> ActionID {
    auto it = ActionMap.begin();
    std::advance(it, index);
    return (it != ActionMap.end()) ? it->first : ActionID::Quit;
  };

  // 1. TOP SECTION: Das Haupt-Aktionsmenü
  std::vector<std::string> menu_entries = getMenuEntries();

  Component top_menu =
      Menu(&menu_entries, &top_menu_selected, MenuOption::Horizontal());

  // Lokale Aktion für das Top-Menü (Enter-Taste)
  top_menu = top_menu | CatchEvent([&](Event event) {
               if (event == Event::Return) {
                 ActionID action_id = get_action_id_by_index(top_menu_selected);
                 if (action_id == ActionID::Quit) {
                   screen.Exit();
                 } else {
                   current_status =
                       "Menü-Aktion: " + ActionMap.at(action_id).menu_text +
                       " ausgeführt.";
                 }
                 return true;
               }

               return false;
             });

  // 2. MIDDLE SECTION: Die beiden Dateibrowser-Panels (Split View)
  auto left_menu = Menu(&left_panel_files, &left_selected);
  auto right_menu = Menu(&right_panel_files, &right_selected);

  // ********** HIER GEHÖRT DIE INTERAKTIVE LOGIK HIN **********
  left_menu =
      left_menu | CatchEvent([&](Event event) {
        // Reagiere nur auf die Enter-Taste
        if (event == Event::Return && !left_file_infos.empty()) {

          // 1. Die aktuell ausgewählte Datei/Ordner abrufen
          const FileInfo &selected_info = left_file_infos[left_selected];

          if (selected_info.is_directory) {

            // AKTION: VERZEICHNISWECHSEL

            // 💡 Aktualisiere den Pfad der UI-Anzeige
            left_panel_path = selected_info.path.string();

            // 💡 Aktualisiere den FileProcessor mit dem neuen Pfad und scanne
            // neu Da fp mit dem aktuellen Pfad initialisiert wurde, müssen wir
            // ein neues FileProcessor-Objekt erstellen, da der Pfad in der
            // fp-Klasse konstant ist (m_path). (Später können Sie fp so
            // refaktorisieren, dass der Pfad änderbar ist.)
            FileProcessor new_fp(left_panel_path);

            // 💡 NEUE DATEN vom FileProcessor abrufen
            left_file_infos = new_fp.scanDirectory();

            // UI-Listen aktualisieren (Der String-Vektor muss neu befüllt
            // werden)
            left_panel_files = fileInfoToMenuStrings(left_file_infos);

            // Auswahl zurücksetzen
            left_selected = 0;
            current_status = "Verzeichnis gewechselt: " + left_panel_path;

            return true; // Event konsumiert

          } else {
            // AKTION: DATEI MARKIEREN (für rechtes Panel)

            // Logik zum Hinzufügen der Datei zu right_file_infos und
            // Aktualisierung
            current_status = "Datei zur Verarbeitung vorgemerkt: " +
                             selected_info.getDisplayName();
            return true;
          }
        }
        return false;
      });
  // ************************************************************

  // ... (ab hier folgen Renderer für left_panel und right_panel) ...

  auto left_panel = Renderer(left_menu, [&] {
    return vbox({text(left_panel_path) | bold | color(Color::Green),
                 separator(),
                 left_menu->Render() | vscroll_indicator | frame}) |
           border;
  });

  auto right_panel = Renderer(right_menu, [&] {
    return vbox({text(right_panel_path) | bold | color(Color::Green),
                 separator(),
                 right_menu->Render() | vscroll_indicator | frame}) |
           border;
  });

  // ResizableSplitLeft für die horizontale Teilung.
  // Handhabt die Trennlinie und den Tab-Wechsel (wie Container::Horizontal)
  // und zusätzlich die Größenanpassung per Maus.

  Component file_split_view = ResizableSplitLeft(
      left_panel,      // Hauptpanel (wird von der Größe beeinflusst)
      right_panel,     // Backpanel (nimmt den Rest ein)
      &left_panel_size // Pointer zur Speicherung der Größe
  );

  // 3. COMPLETE LAYOUT: Stacking der Komponenten (Menü, Split View, Status)
  Component document = Container::Vertical(
      {top_menu, Renderer([] { return separator(); }),

       file_split_view | flex, // Nimmt den vertikalen Platz ein
       Renderer([&] {          // Statusleiste am unteren Rand
         return text("STATUS: " + current_status) | color(Color::GrayLight) |
                hcenter;
       })

      });

  // 4. GLOBALER EVENT-HANDLER
  auto global_handler = CatchEvent(document, [&](Event event) {
    if (event.is_character()) {
      char key_pressed = event.character()[0];
      for (const auto &pair : ActionMap) {
        if (key_pressed == pair.second.shortcut) {
          if (pair.first == ActionID::Quit) {
            screen.Exit();
          } else {
            current_status = "Globaler Shortcut: '" +
                             std::string(1, key_pressed) +

                             "' -> " + pair.second.menu_text;
          }
          return true;
        }
      }
    }

    return false;
  });

  // Die Event-Schleife starten
  screen.Loop(global_handler);
  std::cout << "Programm beendet. Letzter Status: " << current_status
            << std::endl;

  return 0;
}