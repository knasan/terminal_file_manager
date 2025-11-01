#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <map>

// Stellen Sie sicher, dass Ihr uicontrol.hpp hier liegt oder der Pfad angepasst ist.
#include "uicontrol.hpp" 

using namespace ftxui;

// Dummy-Daten und Statusvariablen
std::vector<std::string> left_files = {"..", "Folder A/", "File 1.txt", "File 2.log"};
std::vector<std::string> right_files = {"..", "Folder B/", "Image.jpg", "Doc.pdf"};
std::string left_path = "/home/user/data";
std::string right_path = "/mnt/backup";

int left_selected = 0;
int right_selected = 0;
int top_menu_selected = 0;
std::string current_status = "Bereit. Tab zum Wechseln des Panels.";

// Größe des linken Panels (für ResizableSplit)
int left_panel_size = 50; 

// ----------------------------------------------------------------------
// Hauptprogramm
// ----------------------------------------------------------------------
int main() {
    auto screen = ScreenInteractive::TerminalOutput();

    // -- HILFSFUNKTION --
    auto get_action_id_by_index = [&](int index) -> ActionID {
        auto it = ActionMap.begin();
        // Stellen Sie sicher, dass die Reihenfolge der Menü-Einträge in uicontrol.hpp
        // und die ActionMap dieselbe ist, damit dies funktioniert.
        std::advance(it, index); 
        return (it != ActionMap.end()) ? it->first : ActionID::Quit;
    };

    // 1. TOP SECTION: Das Haupt-Aktionsmenü
    std::vector<std::string> menu_entries = getMenuEntries();
    Component top_menu = Menu(&menu_entries, &top_menu_selected, MenuOption::Horizontal());

    // Lokale Aktion für das Top-Menü (Enter-Taste)
    top_menu = top_menu | CatchEvent([&](Event event) {
        if (event == Event::Return) {
            ActionID action_id = get_action_id_by_index(top_menu_selected);
            if (action_id == ActionID::Quit) {
                screen.Exit();
            } else {
                current_status = "Menü-Aktion: " + ActionMap.at(action_id).menu_text + " ausgeführt.";
            }
            return true; 
        }
        return false;
    });

    // 2. MIDDLE SECTION: Die beiden Dateibrowser-Panels (Split View)
    auto left_menu = Menu(&left_files, &left_selected);
    auto right_menu = Menu(&right_files, &right_selected);

    auto left_panel = Renderer(left_menu, [&] {
        return vbox({
            text(left_path) | bold | color(Color::Green),
            separator(),
            left_menu->Render() | vscroll_indicator | frame
        }) | border;
    });

    auto right_panel = Renderer(right_menu, [&] {
        return vbox({
            text(right_path) | bold | color(Color::Green),
            separator(),
            right_menu->Render() | vscroll_indicator | frame
        }) | border;
    });

    // ResizableSplitLeft für die horizontale Teilung. 
    // Handhabt die Trennlinie und den Tab-Wechsel (wie Container::Horizontal)
    // und zusätzlich die Größenanpassung per Maus.
    
    Component file_split_view = ResizableSplitLeft(
        left_panel,    // Hauptpanel (wird von der Größe beeinflusst)
        right_panel,   // Backpanel (nimmt den Rest ein)
        &left_panel_size // Pointer zur Speicherung der Größe
    );


    // 3. COMPLETE LAYOUT: Stacking der Komponenten (Menü, Split View, Status)
    Component document = Container::Vertical({
        top_menu,
        Renderer([] { return separator(); }), // 👈 Korrektur: Element in Renderer verpackt
        file_split_view | flex, // Nimmt den vertikalen Platz ein
        Renderer([&] {       // Statusleiste am unteren Rand
            return text("STATUS: " + current_status) | color(Color::GrayLight) | hcenter;
        })
    });

    // 4. GLOBALER EVENT-HANDLER
    auto global_handler = CatchEvent(document, [&](Event event) {
        // ... (Ihre globale Tastenkürzel-Logik bleibt hier, unverändert und korrekt)
        if (event.is_character()) {
            char key_pressed = event.character()[0];
            for (const auto &pair : ActionMap) {
                if (key_pressed == pair.second.shortcut) {
                    if (pair.first == ActionID::Quit) {
                        screen.Exit();
                    } else {
                        current_status = "Globaler Shortcut: '" + std::string(1, key_pressed) +
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

    std::cout << "Programm beendet. Letzter Status: " << current_status << std::endl;
    return 0;
}